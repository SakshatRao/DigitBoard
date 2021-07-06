void calibrate_piezo(void);

uint8_t piezo_avg_val, piezo_min_val, piezo_max_val;
uint8_t piezo_upper_thresh, piezo_lower_thresh;

uint8_t piezo_timer_start;

#define STATUS_PRESSED                  1
#define STATUS_NOTPRESSED               0
uint8_t piezo_pressed_status;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A0, INPUT);

  Serial.begin(9600);

  piezo_pressed_status = STATUS_NOTPRESSED;

  calibrate_piezo();
}

void calibrate_piezo(void)
{
  uint8_t piezo_calibration_iters = 20;
  int piezo_sum_val = 0;
  piezo_min_val = 255;
  piezo_max_val = 0;
  uint8_t piezo_val;
  for(int i = 0; i < piezo_calibration_iters; i++)
  {
    piezo_val = analogRead(A0);
    piezo_sum_val += piezo_val;
    if(piezo_val > piezo_max_val)
      piezo_max_val = piezo_val;
    if(piezo_val < piezo_min_val)
      piezo_min_val = piezo_val;
    delay(50);
  }
  piezo_avg_val = piezo_sum_val / piezo_calibration_iters;

  piezo_upper_thresh = piezo_avg_val + 2 * (piezo_max_val - piezo_avg_val);
  piezo_lower_thresh = piezo_avg_val - 2 * (piezo_avg_val - piezo_min_val);
}

void loop() {
  uint8_t piezo_val = analogRead(A0);
  if(piezo_pressed_status == STATUS_NOTPRESSED)
  {
    if(piezo_val >= piezo_upper_thresh)
    {
      piezo_pressed_status = STATUS_PRESSED;
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
  else
  {
    piezo_timer_start += 1;
    if(piezo_timer_start == 40)
    {
      piezo_pressed_status = STATUS_NOTPRESSED;
      piezo_timer_start = 0;
      digitalWrite(LED_BUILTIN, LOW);
      calibrate_piezo();
    }
  }
  delay(50);
}
