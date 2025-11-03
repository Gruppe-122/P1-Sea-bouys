#include "accel.h"

struct AccelData
{ // samler x y og z under en varibel = AccelData
    float x;
    float y;
    float z;
};

static Adafruit_ADXL345_Unified accelM;
static bool done = false;
static float xtest[100];
static float ytest[100];
static float ztest[100];
static float gennemsnitX, gennemsnitY, gennemsnitZ = 0;
static float sumX, sumY, sumZ = 0;

AccelData readAcceleration()
{
    sensors_event_t event;   // opretter en struct (fra library adafruit sensor)
    accelM.getEvent(&event); // fylder event værdier

    AccelData data; // opretter variabel af AccelData og indsætter værdier fra structen event
    data.x = event.acceleration.x;
    data.y = event.acceleration.y;
    data.z = event.acceleration.z;

    return data;
}

int accelSetup()
{
    accelM = Adafruit_ADXL345_Unified(12345);

    Wire.begin(7, 6); // fortæller hvilke pins der bruges, 7 = SDA og 6 = SCL

    if (!accelM.begin())
    {
        Serial.println("ADXL345 ikke fundet!");
        while (1)
            ;
    }

    accelM.setRange(ADXL345_RANGE_16_G); // sætter range til +-16 g

    return 1;
}

int calibrate()
{
    if (done == false)
    {
        Serial.println("KALIBRERING starter om 5 sekunder:");
        Serial.println("PLACER VERTIKALT FLADT");
        for (int x = 5; x > 0; x--)
        {
            Serial.println(x);
            delay(1000);
        }
        for (int i = 0; i < sizeof(xtest) / sizeof(xtest[0]); i++)
        { // hver gang "i", skal vi readAcceleration og gemme i et array i struct maling
            AccelData maling = readAcceleration();
            xtest[i] = maling.x;
            ytest[i] = maling.y;
            ztest[i] = maling.z;
            sumX += xtest[i];
            sumY += ytest[i];
            sumZ += ztest[i];
            delay(50);

            done = true;
        }

        return 1;
    }
}

int accelerometer()
{
    gennemsnitX = sumX / (sizeof(xtest) / sizeof(xtest[0]));
    gennemsnitY = sumY / (sizeof(ytest) / sizeof(ytest[0]));
    gennemsnitZ = sumZ / (sizeof(ztest) / sizeof(ztest[0]));

    Serial.println("KALIBRERING DONE:");

    Serial.print("X i m/s2 -> sum ");
    Serial.print(sumX);
    Serial.print(" gennemsnit ->");
    Serial.println(gennemsnitX);

    Serial.print("Y i m/s2 -> sum ");
    Serial.print(sumY);
    Serial.print(" gennemsnit ->");
    Serial.println(gennemsnitY);

    Serial.print("Z i m/s2 -> sum ");
    Serial.print(sumZ);
    Serial.print(" gennemsnit ->");
    Serial.println(gennemsnitZ);

    delay(3000);

    AccelData a = readAcceleration();

    float xG = (a.x - gennemsnitX);
    float yG = (a.y - gennemsnitY);
    float zG = (a.z - gennemsnitZ);

    float samletPavirkning = sqrt(xG * xG + yG * yG + zG * zG) / tyngdeAcc; // vektorlængden, så roden ax,ay,az i anden, delt med 9.81 for G

    Serial.print("min:");
    Serial.print(-16);
    Serial.print("\tmax:");
    Serial.print(16);
    Serial.print(" ");
    Serial.print("X:");
    Serial.print(xG);
    Serial.print(" Y:");
    Serial.print(yG);
    Serial.print(" Z:");
    Serial.print(zG);
    Serial.print(" Total:");
    Serial.println(samletPavirkning);

    if (samletPavirkning > 4)
    {
        Serial.print("PORT OF AALBORG BESKED");
        return 0;
    }

    delay(50);
    return 1;
}