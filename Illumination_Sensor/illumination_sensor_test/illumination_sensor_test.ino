#include <Arduino_APDS9960.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!APDS.begin()) {
    Serial.println("Error initializing APDS9960 sensor.");
  }
}

void loop() {
  // check if a color reading is available
  while (! APDS.colorAvailable()) {
    delay(5);
  }
  int r, g, b, a;

  // read the color
  APDS.readColor(r, g, b, a);

  // print the values
    Serial.println(a);
    Serial.println();

  // wait a bit before reading again
  delay(1000);
}
