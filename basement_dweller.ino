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

const char* ssid = "umad";
const char* password = "usickcunt001";
const char* serverName = "http://192.168.1.196:5000/api/sample/";

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60       /* Time ESP32 will go to sleep (in seconds) */
#define CLOCK_TWEAK 0.05 * uS_TO_S_FACTOR
#define max_samples 290
#define dweller_id 1

typedef struct {
    int humid;
    float temp;
    float pres;
} measurement;

RTC_DATA_ATTR int boot_counter = 0;
RTC_DATA_ATTR measurement samples[max_samples]; 

int measure() {
    Adafruit_BME280 bme;
    int status = bme.begin(0x76);

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
    setCpuFrequencyMhz(10);
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
    btStop();
}

void enableWiFi(){
    setCpuFrequencyMhz(80);
    WiFi.disconnect(false);  // Reconnect the network
    WiFi.mode(WIFI_STA);    // Switch WiFi off
 
    Serial2.println("START WIFI");
    WiFi.begin(ssid, password);
 
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial2.print(".");
    }
 
    Serial2.println("");
    Serial2.println("WiFi connected");
    Serial2.println("IP address: ");
    Serial2.println(WiFi.localIP());
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

    //Serial.print("First timestamp: ");
    //Serial.println(first_stamp);
    int counter = 0;
    
    while(counter < max_samples) {
        memset(post_buff, 0, 256);
        sprintf(post_buff, "{\"dweller\":\"%d\",\"temp\":\"%f\",\"humid\":\"%d\",\"timestamp\":\"%d\",\"pressure\":\"%f\"}", dweller_id, samples[counter].temp, samples[counter].humid, first_stamp + (TIME_TO_SLEEP * counter),samples[counter].pres);
        //Serial.printf(post_buff);
        http.begin(client, serverName);
        http.addHeader("Content-Type", "text/plain");
        int httpResponseCode = http.POST(post_buff);     
        http.end();
        //Serial.print("HTTP Response code: ");
        //Serial.println(httpResponseCode);
        counter++;
    }  
    disableWiFi();
    boot_counter = 0;
    return 1;
}

void setup() {
    setCpuFrequencyMhz(10);
    measure();
    if(boot_counter >= max_samples) {
        call_home();
    }
    //Serial.println("Going to sleep");
    esp_sleep_enable_timer_wakeup((TIME_TO_SLEEP * uS_TO_S_FACTOR) - (CLOCK_TWEAK));
    esp_deep_sleep_start();
}

void loop() {
    1+1;
}
