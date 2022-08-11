#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "umad";
const char* password = "usickcunt001";
const char* serverName = "http://192.168.1.196:5000/api/sample/";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Adafruit_BME280 bme; // I2C
char post_buff[256];
int dweller_id = 1;

void setup() {
    int status = bme.begin(0x76);
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    timeClient.begin();
}

void loop() {
    timeClient.update();
    if ((millis() - lastTime) > timerDelay) {
        if(WiFi.status()== WL_CONNECTED){
            WiFiClient client;
            HTTPClient http;
            int temp = bme.readTemperature();
            int pressure = (bme.readPressure() / 100.0F);
            int humid = bme.readHumidity();
            int timestamp = timeClient.getEpochTime();
            memset(post_buff, 0, 256);
            sprintf(post_buff, "{\"dweller\":\"%d\",\"temp\":\"%d\",\"humid\":\"%d\",\"timestamp\":\"%d\"}", dweller_id, temp, humid, timestamp);
            http.begin(client, serverName);
            http.addHeader("Content-Type", "text/plain");
            int httpResponseCode = http.POST(post_buff);     
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            http.end();
        } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
