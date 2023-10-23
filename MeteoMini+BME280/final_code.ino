/* Laskakit Meteo_Mini Meteostanice
 * Pro server TMEP
 * Čte Teplotu, tlak, vlhkost a odesílá na server TMEP.cz
 * 
 * Nastavení provádíme v souboru config.h
 * 
 * Vývoj HW:
 * Email:podpora@laskakit.cz
 * Web:laskakit.cz
 * Board: ESP32-C3 Dev Module

 * SW připravil:
 * Jakub Krejčí
 * Email: krejci@czlan.cz
 */

// připojení knihoven
#include "config.h"            // nahraje nastavení

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>      // přidá knihovnu pro sensor BME280
#include <ESP32AnalogRead.h>      // přidá knihovnu ESP32AnalogRead by madhephaestus https://github.com/madhephaestus/ESP32AnalogRead 
#include <WiFiManager.h>          // přidá knihovnu WiFi manager by tzapu https://github.com/tzapu/WiFiManager


#define BME280_ADDRESS (0x77)     // (I2C adresa sensoru BME280
#define SLEEP_SEC 10*60    // Interval měření z config.h (2 sekundy odečteny - doba měření)
#define ADC_PIN 0                 // pin I2C
#define deviderRatio 1.7693877551 // Parametr pro měření napětí baterie

// Vytvoření instance
Adafruit_BME280 bme;
ESP32AnalogRead adc;

//vytvoření proměnných pro hodnoty
float temperature;
float pressure;
float humidity;
float bat_voltage;

void postData(){

  if(WiFi.status()== WL_CONNECTED) {
    HTTPClient http;
      
    //GUID (teplota), nasleduje hodnota teploty, pro vlhkost "humV", pro napeti baterie "v"
    String serverPath = serverName + "" + GUID + "=" + temperature + "&humV=" + humidity + "&pressV=" + pressure + "&v=" + bat_voltage; 
      
    // zacatek http spojeni
    http.begin(serverPath.c_str());
      
    // http GET funkce- odesílá data
    int httpResponseCode = http.GET();
      
    if (httpResponseCode>0) {
      Serial.print("HTTP odpoved: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.print("Error kod: ");
      Serial.println(httpResponseCode);
    }
    //ukončení http spojení
    http.end();
  } else 
      Serial.println("Wi-Fi odpojeno");

}

void GoToSleep(){
    delay(1);
  // ESP usne - šetří baterii
  Serial.println("ESP in sleep mode");
  esp_sleep_enable_timer_wakeup(SLEEP_SEC * 1000000);
  esp_deep_sleep_start();
}

// pripojeni k WiFi
void WiFiConnection(){
    // Probudit WiFi
  WiFi.mode( WIFI_STA);
  

 //WiFiManager, připojuje nás k wifi
    WiFiManager wm;    
    wm.setConfigPortalTimeout(180);   // nastaví čas spuštěného portálu na 3 minuty, poté usne
    bool res;
    res = wm.autoConnect("LaskaKit AutoConnectAP","laskakit"); // spustí zaheslovaný Ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
        //wm.erase(); //vymaže nastavení wifi
    } 
    else {
        //pokud se wifi připojí   
        Serial.println("Wi-Fi connected successfully");
    }
}

// Přečíst data z BME280
void readBME(){
  temperature = bme.readTemperature();
  humidity    = bme.readHumidity();
  pressure    = bme.readPressure() / 100.0F;  
  
  Serial.print("Temp: "); Serial.print(temperature); Serial.println("°C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println("% RH");
  Serial.print("Pressure: "); Serial.print(pressure); Serial.println("hPa");
}


// Měření napětí baterie
void readBat(){
  bat_voltage = adc.readVoltage()*deviderRatio;
  Serial.print("Battery voltage " + String(bat_voltage) + "V");

}

void setup() {
  //po probuzení vypne wifi
  WiFi.mode( WIFI_OFF );
  delay( 1 );
  
  
  adc.attach(ADC_PIN);  

  Serial.begin(115200);
  while(!Serial) {} 

  // initilizace BME280
  Wire.begin(19,18); // SDA SCL
  if (! bme.begin(BME280_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  Serial.println("-- Weather Station Scenario --");
  Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
  Serial.println("filter off");
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // teplota
                  Adafruit_BME280::SAMPLING_X1, // tlak
                  Adafruit_BME280::SAMPLING_X1, // vlhkost
                  Adafruit_BME280::FILTER_OFF   );
  delay(10);

  readBME();
  readBat();     
  
  // Pripojeni k WiFi 
  WiFiConnection();
  postData();

  WiFi.disconnect(true);
  GoToSleep();
}

void loop(){
  // Nepotřebujeme loop
}