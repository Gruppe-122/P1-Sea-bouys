#include "accel.h"
#include "driver/rtc_io.h"

#define ADXL345_ADDRESS 0x53

struct AccelData
{ // samler x y og z under en varibel = AccelData
    float x;
    float y;
    float z;
};

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
    readRegister(ADXL345_ADDRESS, 0x30); // ifølge datasheet, når man læser int_source, så clearer den alle interrupts
}

static bool done = false;
static float xtest[100];
static float ytest[100];
static float ztest[100];
static float gennemsnitX, gennemsnitY, gennemsnitZ = 0;
static float sumX, sumY, sumZ = 0;

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

int accelSetup()
{
    pinMode(5, INPUT);
    Wire.begin(7, 6); //SDA og SCL

    writeRegister(ADXL345_ADDRESS, 0x2D, 0x1C); // tænder målings mode og autosleep
    delay(10);

    writeRegister(ADXL345_ADDRESS, 0x31, 0x01); // range 4G

    writeRegister(ADXL345_ADDRESS, 0x24, 43);   //(43 for 2.69G - 24 for 1.5G) treshhold

    writeRegister(ADXL345_ADDRESS, 0x27, 0xF0); // aktivere måling på hhv. x, y og z

    writeRegister(ADXL345_ADDRESS, 0x2F, 0x00); // alle bits sat til 0, for at aktivere på INT1, modsat for INT2

    writeRegister(ADXL345_ADDRESS, 0x2E, 0x10); // aktivere interrupt

    writeRegister(ADXL345_ADDRESS, 0x2C, 0x0D); // i low-power mode sender vi data med 400hz, for at spare mest muligt 0x18-> 12.5hz 0x1B->100hz 0x0D -> 400hz

    return 1;
}

int calibrate()
{
    if (done == false)
    {
        // Serial.println("KALIBRERING starter om 5 sekunder:");
        // Serial.println("PLACER VERTIKALT FLADT");
        // for (int x = 5; x > 0; x--)
        // {
        //     Serial.println(x);
        //     delay(1000);
        // }
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
        done = true;

        gennemsnitX = sumX / (sizeof(xtest) / sizeof(xtest[0]));
        gennemsnitY = sumY / (sizeof(ytest) / sizeof(ytest[0]));
        gennemsnitZ = sumZ / (sizeof(ztest) / sizeof(ztest[0]));

        Serial.println("KALIBRERING DONE");

        // Serial.print("X i m/s2 -> sum ");
        // Serial.print(sumX);
        // Serial.print(" gennemsnit ->");
        // Serial.println(gennemsnitX);

        // Serial.print("Y i m/s2 -> sum ");
        // Serial.print(sumY);
        // Serial.print(" gennemsnit ->");
        // Serial.println(gennemsnitY);

        // Serial.print("Z i m/s2 -> sum ");
        // Serial.print(sumZ);
        // Serial.print(" gennemsnit ->");
        // Serial.println(gennemsnitZ);

        // delay(3000);

        return 1;
    }
}

int accelerometer()
{
    int intState = digitalRead(5);
    AccelData accel = readAccel();

    resetINT1();

    float xG = (accel.x - gennemsnitX);
    float yG = (accel.y - gennemsnitY);
    float zG = (accel.z - gennemsnitZ);

    if (intState == HIGH)
    {
        Serial.println("Aktivitet over 2.69G registreret");
        resetINT1();
        return 1;
    }
    else
    {
        Serial.println("Ingen aktivitet");
        // Serial.print("min:");
        // Serial.print(-16);
        // Serial.print("\tmax:");
        // Serial.print(16);
        // Serial.print(" ");
        // Serial.print("X:");
        // Serial.print(xG);
        // Serial.print(" Y:");
        // Serial.print(yG);
        // Serial.print(" Z:");
        // Serial.println(zG);
        return 0;
    }
}