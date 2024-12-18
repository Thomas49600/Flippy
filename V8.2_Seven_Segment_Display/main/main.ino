#include "wifi.h"
#include "counter.h"


void setup () {
  Serial.begin(115200); // start serial communication
  delay(10);

  setupWifi();
  delay(1000);
  setupCounter();
  setupCounterObjs();
}


void loop() {
  wifiChecks();
}
