uint8_t digit = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if(Serial)
  {
    while(Serial.available())
      if(Serial.read() - 48 == 2)
        break;

    digitalWrite(LED_BUILTIN, HIGH);
    
    while(Serial)
    {
      digit = (digit + 1) % 10;
      Serial.println(digit);
      delay((int)random(1, 9) * 1000);
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
}
