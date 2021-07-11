#include <ArduinoBLE.h>
BLEService DBService("1011");
BLEUnsignedIntCharacteristic DBDigitChar("936b6a25-e503-4f7c-9349-bcc76c22b8c3", BLERead | BLENotify);
BLEUnsignedIntCharacteristic DBCmdChar("a1e8f5b1-696b-4e4c-87c6-69dfe0b0093b", BLERead | BLEWrite);

uint8_t digit = 0;

#define NONE_CODE                                 ((uint8_t)1)
#define DIGIT_ACK_CODE                            ((uint8_t)2)

void setup()
{
  Serial.begin(9600);
  
  pinMode(LED_BUILTIN, OUTPUT);
  if (!BLE.begin()) 
  {
    Serial.println("starting BLE failed!");
    while (1);
  }
  
  BLE.setLocalName("DigitBoardBLE");
  BLE.setAdvertisedService(DBService);
  DBService.addCharacteristic(DBDigitChar);
  DBService.addCharacteristic(DBCmdChar);
  BLE.addService(DBService);

  DBDigitChar.setValue(0);
  DBCmdChar.setValue(0);
  
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop()
{
  BLEDevice central = BLE.central();
  
  if (central) 
  {
    Serial.println("Connected!");
    digitalWrite(LED_BUILTIN, HIGH);

    byte rec_value = 0;
    while(central.connected())
    {
      DBCmdChar.readValue(rec_value);
      Serial.println(rec_value);
      if(rec_value == DIGIT_ACK_CODE)
      {
        DBCmdChar.writeValue(NONE_CODE);
        break;
      }
      delay(100);
    }

    Serial.println("Initiating communication...");
    delay(10000);
    
    while(central.connected())
    {
      digit = (digit + 1) % 10;
      Serial.println(digit);
      DBDigitChar.writeValue(digit);
      delay((int)random(1, 9) * 1000);
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
}
