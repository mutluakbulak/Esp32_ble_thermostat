#define BLYNK_TEMPLATE_ID "TMPL6VLTWhLZC"
#define BLYNK_TEMPLATE_NAME "Nodemcu Led 2"
#define BLYNK_AUTH_TOKEN "pZoDt-JIkRYvg4WhiGE5q7hdYR-woGHS"




#include <Arduino.h>
    /////////////////////////////////////////////////////////////////
   //     ESP32 & Xiaomi Bluetooth  sensor   Oct. 2020  v1.01     //
  //       Get the latest version of the code here:              //
 //           http://educ8s.tv/esp32-xiaomi-hack                //
/////////////////////////////////////////////////////////////////


// IMPORTANT
// You need to install this library as well else it won't work
// https://github.com/fguiet/ESP32_BLE_Arduino

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define led LED_BUILTIN

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_system.h"
#include <sstream>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BlynkTimer timer;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Aaa";
char pass[] = "aaaaaaaa";



#define SCAN_TIME  10 // seconds

TaskHandle_t blynkLoop;

BLEScan *pBLEScan;

void IRAM_ATTR resetModule(){
    ets_printf("reboot\n");
    esp_restart();
}

float  current_humidity = -100;
float  current_batt_mv = -100;
float  current_batt_level = -100;
float current_temperature = -100;

void initBluetooth();
String convertFloatToString(char c);
float CelciusToFahrenheit(float f);
void createBlynkTask();

// This function will be called every time V1 changed
// in Blynk app writes values to the Virtual Pin V1
BLYNK_WRITE(V3)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  Serial.printf("Pin Vlue is %d ", pinValue);
  // process received value
}

// This function sends Temperature, Humidty, and Batt Level.
// In the app, Widget's reading frequency should be set to PUSH. This means
// that you define how often to send data to Blynk App.
void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V4, current_batt_level);
  Blynk.virtualWrite(V0, current_temperature);
  Blynk.virtualWrite(V2, current_humidity);
  Blynk.virtualWrite(V1, current_batt_mv);

}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.haveName() && advertisedDevice.haveServiceData() && !advertisedDevice.getName().compare("Mutluuu")) {

            int serviceDataCount = advertisedDevice.getServiceDataCount();
            std::string strServiceData = advertisedDevice.getServiceData(0);

            uint8_t cServiceData[30];
            char charServiceData[30];

            strServiceData.copy((char *)cServiceData, strServiceData.length(), 0);

            Serial.printf("\n\nAdvertised Device: %s\n", advertisedDevice.toString().c_str());
            for (int i=0;i<strServiceData.length();i++) {
                sprintf(&charServiceData[i*2], "%02x", cServiceData[i]);
            }

            std::stringstream ss;
            ss << charServiceData;
            
            Serial.print("Payload:");
            Serial.println(ss.str().c_str());

            unsigned long value, value2;
            char charValue[5] = {0,};

            // Get temperature
            sprintf(charValue, "%02X%02X", cServiceData[7], cServiceData[6]);
            value = strtol(charValue, 0, 16);
            current_temperature = (float)value/100;
            Serial.printf("TEMPERATURE_EVENT: %s, %d\n", charValue, value);
            Serial.printf("Current Temperature is: %f°C\n", current_temperature);

            //Get humidity
            sprintf(charValue, "%02X%02X", cServiceData[9], cServiceData[8]);
            value = strtol(charValue, 0, 16);
            current_humidity = (float)value/100;
            Serial.printf("Humidity Event: %s, %d\n", charValue, value);
            Serial.printf("Current Humidty is: %f\n", current_humidity);

            //Get Battery voltage
            sprintf(charValue, "%02X%02X", cServiceData[11], cServiceData[10]);
            value = strtol(charValue, 0, 16);
            current_batt_mv = (float)value;
            Serial.printf("Battery Voltage Event: %s, %d\n", charValue, value);
            Serial.printf("Battery Voltage is: %f\n", current_batt_mv);

            //Get Battery level
            sprintf(charValue, "%02X", cServiceData[12]);
            value = strtol(charValue, 0, 16);
            current_batt_level = (float)value;
            Serial.printf("Battery Level Event: %s, %d\n", charValue, value);
            Serial.printf("Battery Level is: %f\n", current_batt_level);
        }
    }
};

void setup() {
  

  Serial.begin(115200);
  Serial.println("ESP32 XIAOMI DISPLAY");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  initBluetooth();
  // Setup a function to be called every second
  timer.setInterval(1000L, myTimerEvent);
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
  createBlynkTask();
}

void loop() {
    Serial.printf("Start BLE scan for %d seconds...\n", SCAN_TIME);
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
    int count = foundDevices.getCount();
    printf("Found device count : %d\n", count);

    delay(100);
}



void initBluetooth()
{
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(0x50);
    pBLEScan->setWindow(0x30);
}

//TBlynk task: keep up to date the blynk
void blynkTaskCode( void * pvParameters ){
  Serial.print("blynkTaskNode running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    Blynk.run();
    timer.run(); // Initiates BlynkTimer
    vTaskDelay(10);
  } 
}
 //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
 void createBlynkTask(){
  xTaskCreatePinnedToCore(
                    blynkTaskCode,   /* Task function. */
                    "blynkLoop",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &blynkLoop,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); }