#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BluetoothSerial.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Arduino.h"

//Add BME280 power save

const char* ssid = "umad";
const char* password = "usickcunt001";
const char* serverName = "http://192.168.1.196:5000/api/sample/";

const int left_button = 32;     
const int right_button = 33;     
const int red_button = 35;     
const int ledPin = 25;

#define BUTTON_PIN_BITMASK 0x800000000

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  2       /* Time ESP32 will go to sleep (in seconds) */
#define CLOCK_TWEAK 0.05 * uS_TO_S_FACTOR
#define max_samples 290
#define dweller_id 3
#define BME_ADDRESS 0x76

typedef struct {
    int humid;
    float temp;
    float pres;
} measurement;

RTC_DATA_ATTR int boot_counter = 0;
RTC_DATA_ATTR measurement samples[max_samples]; 

void BME280_Sleep(int device_address) {
  const uint8_t CTRL_MEAS_REG = 0xF4;
  Wire.beginTransmission(device_address);
  Wire.requestFrom(device_address, 1);
  uint8_t value = Wire.read();
  value = (value & 0xFC) + 0x00;         // Clear bits 1 and 0
  Wire.write((uint8_t)CTRL_MEAS_REG);    // Select Control Measurement Register
  Wire.write((uint8_t)value);            // Send 'XXXXXX00' for Sleep mode
  Wire.endTransmission();
}

void bmeForceRead(device_address) {
  const uint8_t CTRL_MEAS_REG = 0xF4;
  Wire.beginTransmission(device_address);
  Wire.requestFrom(device_address, 1);
  uint8_t value = Wire.read();
  value = (value & 0xFC) + 0x01;         // Clear bits 1 and 0
  Wire.write((uint8_t)CTRL_MEAS_REG);    // Select Control Measurement Register
  Wire.write((uint8_t)value);            // Send 'XXXXXX00' for Sleep mode
  Wire.endTransmission();
  delay(10);
}

int measure() {
    bmeForceRead(BME_ADDRESS)
    Adafruit_BME280 bme;
    int status = bme.begin(BME_ADDRESS);

    int humid = bme.readHumidity();
    float temp = bme.readTemperature();
    float pres = bme.readPressure() / 100.0F;

    samples[boot_counter].temp = temp;
    samples[boot_counter].humid = humid;
    samples[boot_counter].pres = pres;
    boot_counter++;
    return 1;
}

void disableWiFi(){
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
    btStop();
}

void enableWiFi(){
    WiFi.disconnect(false);  // Reconnect the network
    WiFi.mode(WIFI_STA);    // Switch WiFi off
    WiFi.begin(ssid, password);
 
    while (WiFi.status() != WL_CONNECTED) {
        1+1;
    }
}

int one_shot() {
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP);
    char post_buff[256];
    enableWiFi();
    timeClient.begin();  
    timeClient.update();  
    WiFiClient client;
    HTTPClient http;
    int timestamp = timeClient.getEpochTime();
    bmeForceRead(BME_ADDRESS)
    Adafruit_BME280 bme;
    int status = bme.begin(BME_ADDRESS);
    int humid = bme.readHumidity();
    float temp = bme.readTemperature();
    float pres = bme.readPressure() / 100.0F;
    memset(post_buff, 0, 256);
    sprintf(post_buff, "{\"dweller\":\"%d\",\"temp\":\"%f\",\"humid\":\"%d\",\"timestamp\":\"%d\",\"pressure\":\"%f\"}", dweller_id, temp, humid, timestamp, pres);
    http.begin(client, serverName);
    http.addHeader("Content-Type", "text/plain");
    int httpResponseCode = http.POST(post_buff);
    http.end();
    disableWiFi();
    BME280_Sleep(BME_ADDRESS);  
    return 1;  
}

int call_home() {
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP);
    char post_buff[256];

    enableWiFi();
    timeClient.begin();  
    timeClient.update();  
    WiFiClient client;
    HTTPClient http;
    int timestamp = timeClient.getEpochTime();
    int first_stamp = timestamp - (TIME_TO_SLEEP * max_samples);
    int counter = 0;
    
    while(counter < max_samples) {
        memset(post_buff, 0, 256);
        sprintf(post_buff, "{\"dweller\":\"%d\",\"temp\":\"%f\",\"humid\":\"%d\",\"timestamp\":\"%d\",\"pressure\":\"%f\"}", dweller_id, samples[counter].temp, samples[counter].humid, first_stamp + (TIME_TO_SLEEP * counter),samples[counter].pres);
        http.begin(client, serverName);
        http.addHeader("Content-Type", "text/plain");
        int httpResponseCode = http.POST(post_buff);     
        http.end();
        counter++;
    }  
    disableWiFi();
    boot_counter = 0;
    return 1;
}

void setup() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    pinMode(ledPin, OUTPUT);

    if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        measure();
        if(boot_counter >= max_samples) {
            call_home();
        }
    } else if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        digitalWrite(ledPin, HIGH);
        delay(200);
        one_shot();
        digitalWrite(ledPin, LOW);
    } else {
        digitalWrite(ledPin, HIGH);
        delay(50);
        digitalWrite(ledPin, LOW);
        delay(50);
        digitalWrite(ledPin, HIGH);
        delay(50);
        digitalWrite(ledPin, LOW);
        delay(50);
        digitalWrite(ledPin, HIGH);
        delay(50);
        digitalWrite(ledPin, LOW);
    }
    esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ALL_LOW);
    esp_sleep_enable_timer_wakeup((TIME_TO_SLEEP * uS_TO_S_FACTOR) - (CLOCK_TWEAK));
    esp_deep_sleep_start();
}

void loop() {
    1+1;
}
