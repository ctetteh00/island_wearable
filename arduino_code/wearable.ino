//WEARABLE CODE

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <esp_now.h>
#include <esp_wifi.h> // Required for esp_wifi_get_mac
#include <vector>       // Include for std::vector
#include <algorithm>    // Include for std::remove_if

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "DukeOpen"
// #define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

// Insert Firebase project API Key
#define API_KEY "AIzaSyBCGRSbRu-nXQZbRS1puLU_bRv-U4lj_m8"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "island-wearable-default-rtdb.firebaseio.com"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

const char* tempSPath = "/skintemp";
const char* tempAPath = "/ambtemp";
const char* humPath = "/humidity";
const char* parentPath = "sensor_data";

FirebaseJson json;

unsigned long sendDataPrevMillis = 0;
const unsigned long sensorTimeout = 10000; // 10 seconds
bool signupOK = false;

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

// skin_temp
const int BUFFER_SIZE = 6;
int current_temp = 0;
float temp[BUFFER_SIZE];
int count = 0;
float min_temp;
//TEMP SETUP Variables
const int TempPin = 18; // Temp sensor WHITE WIRE connected to A1
float celsiusExpCal = 0;
float celsiusPolyCal = 0;
float fahrenheit = 0;
float celsiusAvg = 0;
float Vout = 0;

bool isSameMac(const uint8_t *addr1, const uint8_t *addr2) {
  return memcmp(addr1, addr2, 6) == 0;
}

// Callback function for received data
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  Serial.println("ESP-NOW data received!");
  const uint8_t *peer_addr = recv_info->src_addr;

  if (len == sizeof(sensor_data)) { // Check for the correct struct size
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

    // If it's a new MAC address, add it to the list
    if (!found) {
      received_data_t newSensor;
      memcpy(newSensor.macAddr, peer_addr, 6);
      newSensor.skin_temperature = getSkinTemp();
      newSensor.ambient_temperature = receivedTempData.ambient_temperature;
      newSensor.humidity = receivedTempData.humidity;
      newSensor.rssi = rssi;
      newSensor.lastSeen = millis();
      nearbySensors.push_back(newSensor);
      Serial.print("Found new sensor with MAC: ");
      for (int i = 0; i < 6; i++) Serial.printf("%02X:", peer_addr[i]);
      Serial.println();
    }
    Serial.printf("Received temperature: %.2f from ",
    receivedTempData.ambient_temperature);
    for (int i = 0; i < 6; i++) Serial.printf("%02X:", peer_addr[i]);
    Serial.printf(" RSSI: %d\n", rssi);
  } else {
    Serial.println("Received ESP-NOW data length mismatch!");
  }
}

// Function to find the nearest sensor based on RSSI
received_data_t* findNearestSensor() {
  if (nearbySensors.empty()) {
    return nullptr;
  }
  received_data_t* nearest = &nearbySensors[0];
  for (size_t i = 1; i < nearbySensors.size(); ++i) {
    if (nearbySensors[i].rssi > nearest->rssi) {
      nearest = &nearbySensors[i];
    }
  }
  return nearest;
}

void setup() {
  Serial.begin(115200);
  //skin_temp
  analogReadResolution(10);
  //TEMP
  pinMode(TempPin, INPUT);

  WiFi.mode(WIFI_STA);
  Serial.println("Wearable MAC Address: " + WiFi.macAddress());

  Serial.print("Setting WiFi channel to 11...");
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.println("Done.");
  Serial.print("Current WiFi channel (after setting): ");
  Serial.println(WiFi.channel());

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESP-NOW initialized successfully");
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("ESP-NOW receive callback registered.");
  } else {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  WiFi.begin(WIFI_SSID, "", 1); // Explicitly specify the channel
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Current WiFi channel (after connection): ");
  Serial.println(WiFi.channel());

/* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signup ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

float getSkinTemp() {
// measure temperature in Celsius
  Vout = (((float)analogRead(TempPin) * 3.3) / 4095.0) * 1000; //sensor read out of temperature in mV
  // celsiusTemp=(Vout-400)/19.5 
  celsiusExpCal = 7.3975 * exp(0.0014 * Vout);
  celsiusPolyCal = 0.00005 * Vout * Vout + 0.0037 * Vout + 6.09;
  celsiusAvg = ((celsiusExpCal + celsiusPolyCal) / 2)+12;
  Serial.println(celsiusAvg);

  return celsiusAvg;
}

void loop() {
  // Serial.printf("Firebase Error: %s\n", fbdo.errorReason().c_str());
  Serial.println("Wearable MAC Address: " + WiFi.macAddress());
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  //Serial.println(WiFi.channel());
  // Periodically remove sensors not heard from in a while
  nearbySensors.erase(std::remove_if(nearbySensors.begin(), nearbySensors.end(),
                                     [](const received_data_t& s) 
                                     { return (millis() - s.lastSeen > 
                                     sensorTimeout); }),
                    nearbySensors.end());

  received_data_t* nearestSensorPtr = findNearestSensor();

  if (signupOK && nearestSensorPtr != nullptr && (millis()
   - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    Serial.println("Sending data to Firebase from the nearest sensor...");
    Serial.print("Nearest Sensor MAC: ");
    for (int i = 0; i < 6; i++) Serial.printf("%02X:", 
    nearestSensorPtr->macAddr[i]);
    Serial.println();
    Serial.print("Skin Temperature: ");
    Serial.println(nearestSensorPtr->skin_temperature);
    Serial.print("Ambient Temperature: ");
    Serial.println(nearestSensorPtr->ambient_temperature);
    Serial.print("Humidity: ");
    Serial.println(nearestSensorPtr->humidity);
    Serial.printf("Free heap: %d bytes\n", 
    heap_caps_get_free_size(MALLOC_CAP_8BIT));

    json.set(tempSPath, String(nearestSensorPtr->skin_temperature, 2));
    json.set(tempAPath, String(nearestSensorPtr->ambient_temperature, 2));
    json.set(humPath, String(nearestSensorPtr->humidity, 2));

    Serial.printf("Set json to path: %s ... %s\n", parentPath, 
    Firebase.RTDB.setJSON(&fbdo, parentPath, &json) ? "ok" : fbdo.errorReason().c_str());

    
  }
  delay(2000);
}