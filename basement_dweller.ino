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
const char* serverName = "http://192.168.1.196:5000/api/bulksample/";

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60 * 10       /* Time ESP32 will go to sleep (in seconds) */
#define CLOCK_TWEAK 0.05 * uS_TO_S_FACTOR
#define max_samples 290
#define BME_ADDRESS 0x76
#define WIFI_TIMEOUT 10000

#define dweller_id 3
IPAddress staticIP(192, 168, 1, 230);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

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
    WiFi.mode(WIFI_STA);    // Switch WiFi on
    WiFi.begin(ssid, password);
 
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
        millis() - startAttemptTime < WIFI_TIMEOUT){
    delay(10);
  }
}

int call_home() {
    unsigned int start = millis();
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP);

    char *post_buff = (char*)malloc((128 * max_samples) * sizeof(char));
    if (post_buff == NULL)
    {
        1+1;
        //Serial.println("Failed to malloc post_buff!");
    }

    memset(post_buff, 0,128 * max_samples);

    enableWiFi();
    timeClient.begin();  
    
    while(!timeClient.update()) {
        timeClient.forceUpdate();
    }

    WiFiClient client;
    HTTPClient http;
    int timestamp = timeClient.getEpochTime();
    int first_stamp = timestamp - (TIME_TO_SLEEP * max_samples);
    int counter = 0;
    int buf_idx = 0;
    int c_tmp = 0;

    c_tmp = sprintf(post_buff+buf_idx, "[");
    buf_idx += c_tmp;    

    while(counter < max_samples -1) {
        c_tmp = sprintf(post_buff+buf_idx, "{\"dweller\":\"%d\",\"temp\":\"%f\",\"humid\":\"%d\",\"timestamp\":\"%d\",\"pressure\":\"%f\"},", dweller_id, samples[counter].temp, samples[counter].humid, first_stamp + (TIME_TO_SLEEP * counter),samples[counter].pres);
        buf_idx += c_tmp;    
        counter++;
    }

    sprintf(post_buff+buf_idx, "{\"dweller\":\"%d\",\"temp\":\"%f\",\"humid\":\"%d\",\"timestamp\":\"%d\",\"pressure\":\"%f\"}]", dweller_id, samples[counter].temp, samples[counter].humid, first_stamp + (TIME_TO_SLEEP * counter),samples[counter].pres);
    counter++;
    
    http.begin(client, serverName);
    http.addHeader("Content-Type", "text/plain");
    int httpResponseCode = http.POST(post_buff);   
    http.end();

    disableWiFi();
    boot_counter = 0;

    free(post_buff); 
    return 1;
}

void setup() {
    setCpuFrequencyMhz(80);

    Wire.begin();
    bme.setI2CAddress(BME_ADDRESS);
    
    if (bme.beginI2C() == false) //Begin communication over I2C
    {
        delay(0.1);
    }

    measure();
    if(boot_counter >= max_samples) {
        call_home();
    }

    esp_sleep_enable_timer_wakeup((TIME_TO_SLEEP * uS_TO_S_FACTOR) - (CLOCK_TWEAK));
    esp_deep_sleep_start();
}

void loop() {
    1+1;
}
