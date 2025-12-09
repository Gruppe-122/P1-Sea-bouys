#include "accel.h"
#include "driver/rtc_io.h"

#define ADXL345_ADDRESS 0x53

static float xtest[100];
static float ytest[100];
static float ztest[100];
static float gennemsnitX = 0, gennemsnitY = 0, gennemsnitZ = 0;
static float sumX = 0, sumY = 0, sumZ = 0;
volatile bool activityDetected = false;

static void writeRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t value)
{ // funktionen gør at vi kan ændre registrene på ADXL345
    Wire.beginTransmission(deviceAddress);
    Wire.write(registerAddress);
    Wire.write(value);
    Wire.endTransmission();
}

static byte readRegister(uint8_t deviceAddress, uint8_t registerAddress)
{ // funktionen læser hvad der står på de gældene registre, så der kan tjekkes om det der står er rigtig
    Wire.beginTransmission(deviceAddress);
    Wire.write(registerAddress);
    Wire.endTransmission(false);
    Wire.requestFrom(deviceAddress, (uint8_t)1);
    return Wire.read();
}

void resetINT1()
{
    accelLog.logln("reset the interrupt pin 5", "INFO", true);
    readRegister(ADXL345_ADDRESS, 0x30); // ifølge datasheet, når man læser int_source, så clearer den alle interrupts
}

int accelSetup()
{
    accelLog.logln("setup accelometer", "INFO", true);
    Wire.begin(6, 7); // SDA og SCL

    accelLog.logln("tænder målings mode", "INFO", true);
    writeRegister(ADXL345_ADDRESS, 0x2D, 0x08);
    delay(10);

    accelLog.logln("sets range 4G", "INFO", true);
    writeRegister(ADXL345_ADDRESS, 0x31, 0x01); // range 4G

    accelLog.logln("sets (43 for 2.69G - 0x18 for 1.5G) treshhold", "INFO", true);
    writeRegister(ADXL345_ADDRESS, 0x24, 0x18); //(0x18 for 1.5G) treshhold

    accelLog.logln("aktivere måling på hhv. x, y og z", "INFO", true);
    writeRegister(ADXL345_ADDRESS, 0x27, 0xF0); // aktivere måling på hhv. x, y og z

    writeRegister(ADXL345_ADDRESS, 0x2E, 0x00);

    accelLog.logln("alle bits sat til 0, for at aktivere på INT1, modsat for INT2", "INFO", true);
    writeRegister(ADXL345_ADDRESS, 0x2F, 0x00); // alle bits sat til 0, for at aktivere på INT1, modsat for INT2

    writeRegister(ADXL345_ADDRESS, 0x38, 0x00);

    accelLog.logln("aktivere interrupt", "INFO", true);
    writeRegister(ADXL345_ADDRESS, 0x2E, 0x10); // aktivere interrupt

    accelLog.logln("0x1B->100hz 0x0D -> 400hz", "INFO", true);
    writeRegister(ADXL345_ADDRESS, 0x2C, 0x0B); // 0x1B->100hz 0x0D -> 400hz

    pinMode(5, INPUT_PULLUP);
    delay(10);

    resetINT1();

    return 1;
}

AccelData readAccel()
{
    int16_t raw_x = (readRegister(ADXL345_ADDRESS, 0x33) << 8 | readRegister(ADXL345_ADDRESS, 0x32));
    int16_t raw_y = (readRegister(ADXL345_ADDRESS, 0x35) << 8 | readRegister(ADXL345_ADDRESS, 0x34));
    int16_t raw_z = (readRegister(ADXL345_ADDRESS, 0x37) << 8 | readRegister(ADXL345_ADDRESS, 0x36));

    AccelData data;
    data.x = raw_x * 0.0078;
    data.y = raw_y * 0.0078; // 7.8mg / LBS for at få reelle målinger
    data.z = raw_z * 0.0078;

    return data;
}

int calibrate()
{
    accelLog.log("KALIBRERING starter om 5 sekunder:", "INFO", true);
    accelLog.log("PLACER VERTIKALT FLADT", "INFO", true);

    for (int i = 0; i < sizeof(xtest) / sizeof(xtest[0]); i++)
    { // hver gang "i", skal vi readAcceleration og gemme i et array i struct
        AccelData accel = readAccel();
        xtest[i] = accel.x;
        ytest[i] = accel.y;
        ztest[i] = accel.z;
        sumX += xtest[i];
        sumY += ytest[i];
        sumZ += ztest[i];
        delay(50);
    }

    gennemsnitX = sumX / (sizeof(xtest) / sizeof(xtest[0]));
    gennemsnitY = sumY / (sizeof(ytest) / sizeof(ytest[0]));
    gennemsnitZ = sumZ / (sizeof(ztest) / sizeof(ztest[0]));
    accelLog.logln("KALIBRERING DONE", "INFO", true);

    accelLog.log("X i m/s2 -> sum ", "DEBUG", true);
    accelLog.logln(sumX, "DEBUG", false);
    accelLog.log(" gennemsnit ->", "DEBUG", true);
    accelLog.logln(gennemsnitX, "DEBUG", false);

    accelLog.log("Y i m/s2 -> sum ", "DEBUG", true);
    accelLog.logln(sumY, "DEBUG", false);
    accelLog.log(" gennemsnit ->", "DEBUG", true);
    accelLog.logln(gennemsnitY + 1, "DEBUG", false);

    accelLog.log("Z i m/s2 -> sum ", "DEBUG", true);
    accelLog.logln(sumZ, "DEBUG", false);
    accelLog.log(" gennemsnit ->", "DEBUG", true);
    accelLog.logln(gennemsnitZ, "DEBUG", false);

    return 1;
}

bool accelerometer()
{
    int intState = digitalRead(5);

    AccelData accel = readAccel();
    float xG = (accel.x - gennemsnitX);
    float yG = (accel.y - gennemsnitY);
    float zG = (accel.z - gennemsnitZ);

    accelLog.log("min:", "DEBUG", true);
    accelLog.logln(-16, "DEBUG", false);

    accelLog.log("max:", "DEBUG", true);
    accelLog.logln(16, "DEBUG", false);

    // X
    accelLog.log("X:", "DEBUG", true);
    accelLog.logln(xG, "DEBUG", false);

    // Y
    accelLog.log("Y:", "DEBUG", true);
    accelLog.logln(yG, "DEBUG", false);

    // Z
    accelLog.log("Z:", "DEBUG", true);
    accelLog.logln(zG, "DEBUG", false);

    if (intState == HIGH)
    {
        accelLog.logln("!!AKTIVITET REGISTRERET!!", "INFO", true);
        resetINT1();
        return true;
    }

    // byte intSource = readRegister(ADXL345_ADDRESS, 0x30);
    // Serial.print("INT_SOURCE register: 0x");
    // Serial.println(intSource, HEX);
    // if (intSource & 0x10)
    // {
    //     Serial.println("!!AKTIVITET AKTIVITET!!");
    //     resetINT1();
    //     return true;
    // }
    // else
    // {
    //     Serial.println("INGEN AKTIVITET");
    //     Serial.print("Actual interrupt: 0x");
    //     Serial.println(intSource, HEX);

    //     resetINT1();
    // }
    return false;
}
