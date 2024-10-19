#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <EEPROM.h>

#define version 0.1

// Voltage divider for battery measurement
#define DIVIDER_RATIO 1.7693877551

// PWR PIN for uSup 
#define PWR_PIN 3

// ADC IN (battery voltage)
#define ADC_IN  0

// Create instance of WiFiManager
WiFiManager wm;

// Custom parameters
char serverAddress[40] = "";
int sleepTime = 10; // Default sleep time in minutes

WiFiManagerParameter custom_serverAddress("server", "Server Address (max 40 chars)", serverAddress, 40);
WiFiManagerParameter custom_sleepTime("sleepTime", "Sleep Time (max 200 minutes)", String(sleepTime).c_str(), 6);

Adafruit_BME280 bme; // Create BME280 instance

void saveConfigCallback() {
  Serial.println("Saving config...");
  
  // Update server address and sleep time
  strcpy(serverAddress, custom_serverAddress.getValue());
  sleepTime = atoi(custom_sleepTime.getValue());

  // Save to EEPROM
  EEPROM.put(0, serverAddress);
  EEPROM.put(40, sleepTime);
  EEPROM.commit();
}

void setup() 
{
  EEPROM.begin(512);
  pinMode(PWR_PIN, OUTPUT);    
  digitalWrite(PWR_PIN, HIGH); // Turn power ON uSUP connector
  delay(100);            

  Serial.begin(115200);
  delay(100);

  Serial.println("-------------------");
  Serial.println("Laskakit Meteo Mini");
  Serial.println("BME280 sensor");
  Serial.print("version: "); Serial.println(version);
  Serial.println("-------------------");

  // Read stored configuration
  EEPROM.get(0, serverAddress);
  EEPROM.get(40, sleepTime);

  Serial.println("Stored Server Address: " + String(serverAddress));
  Serial.println("Stored Sleep Time: " + String(sleepTime));

  // Set custom parameters
  wm.addParameter(&custom_serverAddress);
  wm.addParameter(&custom_sleepTime);

  // Set save config callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Connect to WiFi
  if (!wm.autoConnect("Laskakit Meteo Mini config")) 
  {
    Serial.println("Failed to connect and hit timeout");
    esp_sleep_enable_timer_wakeup(sleepTime * 60 * 1000000);
    esp_deep_sleep_start();
  }

  Serial.println("Connected to WiFi");

  // I2C
  Wire.begin(19, 18); //19 - SDA, 18 - SCL

  // Initialize BME280
  if (!bme.begin(0x77))
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    return;
  }
  Serial.println("BME280 found");
}

void loop() 
{
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  float battery_voltage = (analogReadMilliVolts(ADC_IN) * DIVIDER_RATIO / 1000);

  /* Print to Serial Monitor */
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("\tHumidity: ");
  Serial.print(humidity);
  Serial.print("\tPressure: ");
  Serial.println(pressure);

  if (WiFi.status() == WL_CONNECTED) 
  {
    HTTPClient http;
    String url = "http://" + String(serverAddress) + "/?temp=" + String(temperature) + "&humV=" + String(humidity) + "&pressV=" + String(pressure) + "&voltage=" + String(battery_voltage);

    http.begin(url);
    Serial.println(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) 
    {
      Serial.println("HTTP Response code: " + String(httpResponseCode));
    } 
    else 
    {
      Serial.println("Error in HTTP request");
    }
    http.end();
  } 
  else 
  {
    Serial.println("WiFi not connected");
  }

  Serial.println("Meteo Mini is going to sleep...");
  
  digitalWrite(PWR_PIN, LOW); // Turn power OFF uSUP connector
  Serial.flush();
  delay(100);

  // Sleep for the configured time
  esp_sleep_enable_timer_wakeup(sleepTime * 60 * 1000000);
  esp_deep_sleep_start();
}
