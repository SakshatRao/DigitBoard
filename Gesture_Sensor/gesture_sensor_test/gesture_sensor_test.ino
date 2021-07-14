#include <Arduino_APDS9960.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  if (!APDS.begin())
  {
    Serial.println("Error initializing APDS9960 sensor!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (APDS.gestureAvailable())
  {
    int gesture = APDS.readGesture();
    switch (gesture)
    {
      case GESTURE_UP:
        Serial.println("Detected UP gesture");
        break;
      case GESTURE_DOWN:
        Serial.println("Detected DOWN gesture");
        break;
      case GESTURE_LEFT:
        Serial.println("Detected LEFT gesture");
        break;
      case GESTURE_RIGHT:
        Serial.println("Detected RIGHT gesture");
        break;
      default:
        break;
    }
  }
}
