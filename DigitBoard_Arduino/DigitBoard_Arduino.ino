/**
* DigitBoard Arduino Code
*
* This is the code which is run by the Arduino Nano 33 BLE Sense while
* being attached on the pen
* 
* Required Libraries:
* - TensorFlow Lite for Microcontrollers    (Running TinyML models)
* - Arduino_LSM9DS1                         (Tracking motion)
* - Arduino_APDS9960                        (Measuring illumination)
*
* Functionality:
* - The code utilizes two tinyML models - one for digit recognition
*   and the other for gesture recognition
* - After establishing serial connection, the IMU sensor continuously
*   monitors the motion of the pen which is then fed to one of the two
*   tinyML models (based on the switch)
* - The analog output of the piezo-electric cell system is also used
*   as a condition for digit recognition to happen
* - Lighting conditions are also monitored continuously to provide
*   Dim Lighting Assistance
*
**/

//==============================================================================
// Includes
//==============================================================================

// LSM9DS1
#include <Arduino_LSM9DS1.h>

// Tensorflow Lite for Microcontrollers
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

// APDS9960
#include <Arduino_APDS9960.h>



//==============================================================================
// TinyML Models Parameter Initialization
//==============================================================================

// Digit Recognizer (only 0, 1 & 2)
#include "digit012_recognizer_model.h"
#define DIGITS_MOTION_THRESHOLD     0.093
#define DIGITS_CAPTURE_DELAY        500
#define DIGITS_NUM_SAMPLES          200
const char *DIGITS[] = {"0", "1", "2"};

// Gesture Recognizer
#include "gesture_recognizer_model.h"
#define GESTURES_MOTION_THRESHOLD   0.09
#define GESTURES_CAPTURE_DELAY      1000
#define GESTURES_NUM_SAMPLES        20
const char *GESTURES[] = {"r", "l", "b", "s", "u", "d"};



//==============================================================================
// TinyML Models Capture variables
//==============================================================================

// Digit Recognizer
#define NUM_DIGITS (sizeof(DIGITS) / sizeof(DIGITS[0]))
bool DIGITS_isCapturing = false;
int DIGITS_numSamplesRead = 0;

#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))
bool GESTURES_isCapturing = false;
int GESTURES_numSamplesRead = 0;



//==============================================================================
// TensorFlow variables
//==============================================================================

tflite::MicroErrorReporter tflErrorReporter;
tflite::AllOpsResolver tflOpsResolver;

// Digit Recognizer
const tflite::Model* DIGITS_tflModel = nullptr;
tflite::MicroInterpreter* DIGITS_tflInterpreter = nullptr;
TfLiteTensor* DIGITS_tflInputTensor = nullptr;
TfLiteTensor* DIGITS_tflOutputTensor = nullptr;
constexpr int DIGITS_tensorArenaSize = 8 * 1024;
byte DIGITS_tensorArena[DIGITS_tensorArenaSize];

const tflite::Model* GESTURES_tflModel = nullptr;
tflite::MicroInterpreter* GESTURES_tflInterpreter = nullptr;
TfLiteTensor* GESTURES_tflInputTensor = nullptr;
TfLiteTensor* GESTURES_tflOutputTensor = nullptr;
constexpr int GESTURES_tensorArenaSize = 8 * 1024;
byte GESTURES_tensorArena[GESTURES_tensorArenaSize];



//==============================================================================
// Piezo-electric System Variables
//==============================================================================

// Global variables post-calibration
uint8_t piezo_avg_val, piezo_min_val, piezo_max_val;
uint8_t piezo_upper_thresh, piezo_lower_thresh;

// Pressed / Not-pressed status macros & variables
#define STATUS_PRESSED                  1
#define STATUS_NOTPRESSED               0
uint8_t piezo_pressed_status;

// Piezo-electric cell calibration function
void calibrate_piezo(void)
{
  // How many times to fetch piezo-electric cell output
  uint8_t piezo_calibration_iters = 20;
  
  // Defining variables to find average, min & max values
  int piezo_sum_val = 0;
  piezo_min_val = 255;
  piezo_max_val = 0;
  
  // Fetching piezo-electric cell output
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
  
  } // for(int i = 0; i < piezo_calibration_iters; i++)
  
  // Determining lower & upper thresholds using min, avg & max values
  piezo_avg_val = piezo_sum_val / piezo_calibration_iters;
  piezo_upper_thresh = piezo_avg_val + 2 * (piezo_max_val - piezo_avg_val);
  piezo_lower_thresh = piezo_avg_val - 2 * (piezo_avg_val - piezo_min_val);

} // void calibrate_piezo(void)

//==============================================================================
// Other Variables
//==============================================================================

// Illumincation variables
int r, g, b, a;

// Digital pin mapping
#define LED_ILLUM                       D6
#define BUTTON_MODE                     D5
#define LED_MOTION                      LED_BUILTIN


















//==============================================================================
// Setup
// - Initializating pins, Serial, variables & sensors
// - Calibrating piezo-electric system
// - Setting up TinyML models
//==============================================================================

void setup()
{

  // Initializating pins, Serial, variables & sensors

  pinMode(LED_MOTION, OUTPUT);
  pinMode(LED_ILLUM, OUTPUT);
  pinMode(BUTTON_MODE, INPUT);
  pinMode(A0, INPUT);

  Serial.begin(115200);

  if (!IMU.begin())
    while (1);
  if (!APDS.begin())
    while(1);
  


  // Calibrating piezo-electric system

  piezo_pressed_status = STATUS_NOTPRESSED;
  calibrate_piezo();



  // Setting up TinyML models

  DIGITS_tflModel = tflite::GetModel(digit012_recognizer_model_weights);
  if (DIGITS_tflModel->version() != TFLITE_SCHEMA_VERSION)
    while (1);
  DIGITS_tflInterpreter = new tflite::MicroInterpreter(DIGITS_tflModel, tflOpsResolver, DIGITS_tensorArena, DIGITS_tensorArenaSize, &tflErrorReporter);
  DIGITS_tflInterpreter->AllocateTensors();
  DIGITS_tflInputTensor = DIGITS_tflInterpreter->input(0);
  DIGITS_tflOutputTensor = DIGITS_tflInterpreter->output(0);

  GESTURES_tflModel = tflite::GetModel(gesture_recognizer_model_weights);
  if (GESTURES_tflModel->version() != TFLITE_SCHEMA_VERSION)
    while (1);
  GESTURES_tflInterpreter = new tflite::MicroInterpreter(GESTURES_tflModel, tflOpsResolver, GESTURES_tensorArena, GESTURES_tensorArenaSize, &tflErrorReporter);
  GESTURES_tflInterpreter->AllocateTensors();
  GESTURES_tflInputTensor = GESTURES_tflInterpreter->input(0);
  GESTURES_tflOutputTensor = GESTURES_tflInterpreter->output(0);

} // void setup()










//==============================================================================
// Loop
// - Establishing serial connection
// - Determining mode of operation (digit / gesture recognition)
// - Wait for motion above the threshold setting
// - Capture data for inference
// - If mode is digit recognition, check for piezo-electric output crossing threshold
// - Run TinyML model & transmit winner
// - Check for dark lighting & switch on LED
//==============================================================================

void loop()
{
  // Establishing serial connection
  if(Serial)
  {
    while(Serial.available())
      if(Serial.read() - 48 == 2)
        break;

    while(Serial)
    {
      uint8_t mode = digitalRead(BUTTON_MODE);
      
      // Determining mode of operation (digit / gesture recognition)
      if(mode == 1)
      {
        float aX, aY, aZ, gX, gY, gZ;
      
        // Wait for motion above the threshold setting
        if(!DIGITS_isCapturing)
        {
          if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
          {
            IMU.readAcceleration(aX, aY, aZ);
            IMU.readGyroscope(gX, gY, gZ);
      
            float average = fabs(aX / 4.0) + fabs(aY / 4.0) + fabs(aZ / 4.0) + fabs(gX / 2000.0) + fabs(gY / 2000.0) + fabs(gZ / 2000.0);
            average /= 6.;
      
            if (average >= DIGITS_MOTION_THRESHOLD)
            {
              DIGITS_isCapturing = true;
              DIGITS_numSamplesRead = 0;
              break;
            
            } // if (average >= DIGITS_MOTION_THRESHOLD)
          
          } // if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
        
        } // if(!DIGITS_isCapturing)
      
        if(DIGITS_isCapturing)
        {
          if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
          {
            // Capture data for inference
            IMU.readAcceleration(aX, aY, aZ);
            IMU.readGyroscope(gX, gY, gZ);
      
            DIGITS_tflInputTensor->data.f[DIGITS_numSamplesRead * 6 + 0] = aX / 4.0;
            DIGITS_tflInputTensor->data.f[DIGITS_numSamplesRead * 6 + 1] = aY / 4.0;
            DIGITS_tflInputTensor->data.f[DIGITS_numSamplesRead * 6 + 2] = aZ / 4.0;
            DIGITS_tflInputTensor->data.f[DIGITS_numSamplesRead * 6 + 3] = gX / 2000.0;
            DIGITS_tflInputTensor->data.f[DIGITS_numSamplesRead * 6 + 4] = gY / 2000.0;
            DIGITS_tflInputTensor->data.f[DIGITS_numSamplesRead * 6 + 5] = gZ / 2000.0;
      
            DIGITS_numSamplesRead++;
      
            // If mode is digit recognition, check for piezo-electric output crossing threshold
            if(piezo_pressed_status == STATUS_NOTPRESSED)
            {
              uint8_t piezo_val = analogRead(A0);
              if(piezo_val >= piezo_upper_thresh)
              {
                piezo_pressed_status = STATUS_PRESSED;
                digitalWrite(LED_MOTION, HIGH);
              
              } // if(piezo_val >= piezo_upper_thresh)
            
            } // if(piezo_pressed_status == STATUS_NOTPRESSED)
      
            if (DIGITS_numSamplesRead == DIGITS_NUM_SAMPLES)
            {
              DIGITS_isCapturing = false;
      
              if(piezo_pressed_status == STATUS_PRESSED)
              {
                // Run TinyML model & transmit winner
                TfLiteStatus DIGITS_invokeStatus = DIGITS_tflInterpreter->Invoke();
                if (DIGITS_invokeStatus != kTfLiteOk)
                  while (1);
        
                int maxIndex = 0;
                float maxValue = 0;
                for (int i = 0; i < NUM_DIGITS; i++)
                {
                  float _value = DIGITS_tflOutputTensor->data.f[i];
                  if(_value > maxValue)
                  {
                    maxValue = _value;
                    maxIndex = i;
                  
                  } // if(_value > maxValue)
                
                } // for (int i = 0; i < NUM_DIGITS; i++)
                
                if(maxValue >= 0.5)
                  Serial.println(DIGITS[maxIndex]);
      
                digitalWrite(LED_MOTION, LOW);
                delay(DIGITS_CAPTURE_DELAY);
                calibrate_piezo();
      
                piezo_pressed_status = STATUS_NOTPRESSED;
              
              } // if(piezo_pressed_status == STATUS_PRESSED)
            
            } // if (DIGITS_numSamplesRead == DIGITS_NUM_SAMPLES)
          
          } // if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
        
        } // if(DIGITS_isCapturing)
        
        // Check for dark lighting & switch on LED
        if(APDS.colorAvailable())
        {
          APDS.readColor(r, g, b, a);
          if(a < 5)
            digitalWrite(LED_ILLUM, HIGH);
          else
            digitalWrite(LED_ILLUM, LOW);
        
        } // if(APDS.colorAvailable())
      
      } // if(mode == 1)
      else
      { 
        float aX, aY, aZ, gX, gY, gZ;
      
        // Wait for motion above the threshold setting
        if(!GESTURES_isCapturing)
        {
          if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
          {
            // Capture data for inference
            IMU.readAcceleration(aX, aY, aZ);
            IMU.readGyroscope(gX, gY, gZ);
      
            float average = fabs(aX / 4.0) + fabs(aY / 4.0) + fabs(aZ / 4.0) + fabs(gX / 2000.0) + fabs(gY / 2000.0) + fabs(gZ / 2000.0);
            average /= 6.;
      
            if (average >= GESTURES_MOTION_THRESHOLD)
            {
              digitalWrite(LED_MOTION, HIGH);
              GESTURES_isCapturing = true;
              GESTURES_numSamplesRead = 0;
              break;
            
            } // if (average >= GESTURES_MOTION_THRESHOLD)
          
          } // if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
        
        } // if(!GESTURES_isCapturing)
      
        if(GESTURES_isCapturing)
        {
          if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
          {
            IMU.readAcceleration(aX, aY, aZ);
            IMU.readGyroscope(gX, gY, gZ);
      
            GESTURES_tflInputTensor->data.f[GESTURES_numSamplesRead * 6 + 0] = aX / 4.0;
            GESTURES_tflInputTensor->data.f[GESTURES_numSamplesRead * 6 + 1] = aY / 4.0;
            GESTURES_tflInputTensor->data.f[GESTURES_numSamplesRead * 6 + 2] = aZ / 4.0;
            GESTURES_tflInputTensor->data.f[GESTURES_numSamplesRead * 6 + 3] = gX / 2000.0;
            GESTURES_tflInputTensor->data.f[GESTURES_numSamplesRead * 6 + 4] = gY / 2000.0;
            GESTURES_tflInputTensor->data.f[GESTURES_numSamplesRead * 6 + 5] = gZ / 2000.0;
      
            GESTURES_numSamplesRead++;
      
            if (GESTURES_numSamplesRead == GESTURES_NUM_SAMPLES)
            {  
              GESTURES_isCapturing = false;
      
              // Run TinyML model & transmit winner
              TfLiteStatus GESTURES_invokeStatus = GESTURES_tflInterpreter->Invoke();
              if (GESTURES_invokeStatus != kTfLiteOk)
                while (1);
      
              // Loop through the output tensor values from the model
              int maxIndex = 0;
              float maxValue = 0;
              for (int i = 0; i < NUM_GESTURES; i++)
              {
                float _value = GESTURES_tflOutputTensor->data.f[i];
                if(_value > maxValue)
                {
                  maxValue = _value;
                  maxIndex = i;
                
                } // if(_value > maxValue)
              
              } // for (int i = 0; i < NUM_GESTURES; i++)
              
              Serial.println(GESTURES[maxIndex]);
              
              digitalWrite(LED_MOTION, LOW);
              delay(GESTURES_CAPTURE_DELAY);
            
            } // if (GESTURES_numSamplesRead == GESTURES_NUM_SAMPLES)
          
          } // if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
        
        } // if(GESTURES_isCapturing)
        
        // Check for dark lighting & switch on LED
        if(APDS.colorAvailable())
        {
          APDS.readColor(r, g, b, a);
          if(a < 5)
            digitalWrite(LED_ILLUM, HIGH);
          else
            digitalWrite(LED_ILLUM, LOW);
        
        } // if(APDS.colorAvailable())
      
      } // else
    
    } // while(Serial)
  
  } // if(Serial)

  // Check for dark lighting & switch on LED
  if(APDS.colorAvailable())
  {
    APDS.readColor(r, g, b, a);
    if(a < 5)
      digitalWrite(LED_ILLUM, HIGH);
    else
      digitalWrite(LED_ILLUM, LOW);
  
  } // if(APDS.colorAvailable())

} // void loop()
