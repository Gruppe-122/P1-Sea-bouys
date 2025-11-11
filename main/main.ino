#include "src/accel.h"
#include "src/amp.h"
#include "src/gps.h"
#include "esp_sleep.h"

void setup()
{
  delay(1000);
  Serial.begin(115200);
  accelSetup();
  calibrate();

  if (accelerometer() == 1)
  {
    Serial.println("sending msg to port");
    delay(10000);
  }

  esp_sleep_enable_ext0_wakeup((gpio_num_t)5, 1);
  esp_deep_sleep_start();
}
void loop() {

};
