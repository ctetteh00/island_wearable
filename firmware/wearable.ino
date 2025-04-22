// ----------------------------------------
// Heat Risk Monitoring - Wearable Node
// ESP32 Wearable Node receives ESP-NOW data and uploads to Firebase
// ----------------------------------------

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <vector>
#include <algorithm>
#include "secrets.h"

// Firebase Helper Includes
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

// Firebase data paths
const char* tempSPath = "/skintemp";
const char* tempAPath = "/ambtemp";
const char* humPath   = "/humidity";
const char* parentPath = "sensor_data";

// Timing
unsigned long sendDataPrevMillis = 0;
const unsigned long sensorTimeout = 10000;
bool signupOK = false;

// -------------------------------
// ESP-NOW Data Structure
// -------------------------------
typedef struct struct_sensor_data {
  uint8_t macAddr[6];
  float skin_temperature;
  float ambient_temperature;
  float humidity;
} sensor_data;

typedef struct received_data_t {
  uint8_t macAddr[6];
  float skin_temperature;
  float ambient_temperature;
  float humidity;
  int8_t rssi;
  unsigned long lastSeen;
} received_data_t;

std::vector<received_data_t> nearbySensors;

// -------------------------------
// Temperature Sensor Setup
// -------------------------------
const int TempPin = 18; // Analog pin connected to TMP236 white wire
const int BUFFER_SIZE = 6;
float temp[BUFFER_SIZE];
int count = 0;
float Vout = 0;

// -------------------------------
// Utility Function: Check MAC Equality
// -------------------------------
bool isSameMac(const uint8_t *addr1, const uint8_t *addr2) {
  return memcmp(addr1, addr2, 6) == 0;
}

// -------------------------------
// ESP-NOW Receive Callback
// -------------------------------
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  Serial.println("ESP-NOW data received!");
  const uint8_t *peer_addr = recv_info->src_addr;

  if (len == sizeof(sensor_data)) {
    sensor_data receivedTempData;
    memcpy(&receivedTempData, incomingData, sizeof(receivedTempData));
    int8_t rssi = recv_info->rx_ctrl->rssi;

    bool found = false;
    for (auto &sensor : nearbySensors) {
      if (isSameMac(sensor.macAddr, peer_addr)) {
        sensor.skin_temperature = getSkinTemp();
        sensor.ambient_temperature = receivedTempData.ambient_temperature;
        sensor.humidity = receivedTempData.humidity;
        sensor.rssi = rssi;
        sensor.lastSeen = millis();
        found = true;
        break;
      }
    }

    if (!found) {
      received_data_t newSensor;
      memcpy(newSensor.macAddr, peer_addr, 6);
      newSensor.skin_temperature = getSkinTemp();
      newSensor.ambient_temperature = receivedTempData.ambient_temperature;
      newSensor.humidity = receivedTempData.humidity;
      newSensor.rssi = rssi;
      newSensor.lastSeen = millis();
      nearbySensors.push_back(newSensor);
      Serial.print("New sensor discovered with MAC: ");
      for (int i = 0; i < 6; i++) Serial.printf("%02X:", peer_addr[i]);
      Serial.println();
    }

    Serial.printf("Ambient Temp: %.2f°F, RSSI: %d\n", receivedTempData.ambient_temperature, rssi);
  } else {
    Serial.println("ESP-NOW data length mismatch!");
  }
}

// -------------------------------
// Utility Function: Find Nearest Sensor
// -------------------------------
received_data_t* findNearestSensor() {
  if (nearbySensors.empty()) return nullptr;
  received_data_t* nearest = &nearbySensors[0];
  for (size_t i = 1; i < nearbySensors.size(); ++i) {
    if (nearbySensors[i].rssi > nearest->rssi) {
      nearest = &nearbySensors[i];
    }
  }
  return nearest;
}

// -------------------------------
// Setup Function
// -------------------------------
void setup() {
  Serial.begin(115200);
  analogReadResolution(10);
  pinMode(TempPin, INPUT);

  WiFi.mode(WIFI_STA);
  Serial.println("Wearable MAC Address: " + WiFi.macAddress());

  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESP-NOW initialized");
    esp_now_register_recv_cb(OnDataRecv);
  } else {
    Serial.println("ESP-NOW init failed");
    return;
  }

  // Wi-Fi for Firebase (use empty password for DukeOpen)
  WiFi.begin(WIFI_SSID, "", 1);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("Connected to WiFi");

  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase sign-up failed: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// -------------------------------
// Measure Skin Temperature
// -------------------------------
float getSkinTemp() {
  Vout = (((float)analogRead(TempPin) * 3.3) / 4095.0) * 1000; // Convert to mV
  float celsiusExpCal = 7.3975 * exp(0.0014 * Vout);
  float celsiusPolyCal = 0.00005 * Vout * Vout + 0.0037 * Vout + 6.09;
  float celsiusAvg = ((celsiusExpCal + celsiusPolyCal) / 2) + 12;
  Serial.printf("Calculated Skin Temp: %.2f °C\n", celsiusAvg);
  return celsiusAvg;
}

// -------------------------------
// Main Loop
// -------------------------------
void loop() {
  // Remove sensors not heard from in a while
  nearbySensors.erase(std::remove_if(nearbySensors.begin(), nearbySensors.end(),
    [](const received_data_t& s) { return (millis() - s.lastSeen > sensorTimeout); }),
    nearbySensors.end());

  received_data_t* nearestSensorPtr = findNearestSensor();

  if (signupOK && nearestSensorPtr != nullptr && (millis() - sendDataPrevMillis > 15000)) {
    sendDataPrevMillis = millis();

    Serial.println("Uploading to Firebase from nearest sensor:");
    for (int i = 0; i < 6; i++) Serial.printf("%02X:", nearestSensorPtr->macAddr[i]);
    Serial.println();
    Serial.printf("Skin Temp: %.2f °C\n", nearestSensorPtr->skin_temperature);
    Serial.printf("Ambient Temp: %.2f °F\n", nearestSensorPtr->ambient_temperature);
    Serial.printf("Humidity: %.2f %%\n", nearestSensorPtr->humidity);

    // Prepare Firebase JSON payload
    json.set(tempSPath, String(nearestSensorPtr->skin_temperature, 2));
    json.set(tempAPath, String(nearestSensorPtr->ambient_temperature, 2));
    json.set(humPath, String(nearestSensorPtr->humidity, 2));

    // Upload to Firebase
    if (Firebase.RTDB.setJSON(&fbdo, parentPath, &json)) {
      Serial.println("Firebase upload successful");
    } else {
      Serial.printf("Firebase error: %s\n", fbdo.errorReason().c_str());
    }
  }

  delay(2000); // Small delay between checks
}
