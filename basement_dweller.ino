#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <SPI.h>
#include "SparkFunBME280.h"
#include <BluetoothSerial.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Arduino.h"

const char* ssid = "umad";
const char* password = "usickcunt001";
const char* serverName = "http://192.168.1.196:5000/api/sample/";

const int left_button = 32;     
const int right_button = 33;     
const int red_button = 35;     
const int ledPin = 25;

#define BUTTON_PIN_BITMASK 0x800000000

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60 * 5       /* Time ESP32 will go to sleep (in seconds) */
#define CLOCK_TWEAK 0.05 * uS_TO_S_FACTOR
#define max_samples 290
#define dweller_id 2
#define BME_ADDRESS 0x76

typedef struct {
    int humid;
    float temp;
    float pres;
} measurement;

RTC_DATA_ATTR int boot_counter = 0;
RTC_DATA_ATTR measurement samples[max_samples]; 

BME280 bme;

void bmeForceRead() {
    uint8_t value = bme.readRegister(BME280_CTRL_MEAS_REG);
    value = (value & 0xFC) + 0x01;
    bme.writeRegister(BME280_CTRL_MEAS_REG, value);
    delay(10);
}

int measure() {
    bmeForceRead();
    int humid = bme.readFloatHumidity();
    float temp = bme.readTempC();
    float pres = bme.readFloatPressure();
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
    Wire.begin();

    bme.setI2CAddress(BME_ADDRESS);
    
    if (bme.beginI2C() == false) //Begin communication over I2C
    {
        //Serial.println("The sensor did not respond. Please check wiring.");
        while(1); //Freeze
    }

    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    pinMode(ledPin, OUTPUT);

    if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        //Serial.println("ESP_SLEEP_WAKEUP_TIMER");
        measure();
        if(boot_counter >= max_samples) {
            call_home();
        }
    } else if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        //Serial.println("ESP_SLEEP_WAKEUP_EXT1!");
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
    //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ALL_LOW);
    esp_sleep_enable_timer_wakeup((TIME_TO_SLEEP * uS_TO_S_FACTOR) - (CLOCK_TWEAK));
    esp_deep_sleep_start();
}

void loop() {
    1+1;
}
