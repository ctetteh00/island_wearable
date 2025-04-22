// ----------------------------------------
// Heat Risk Monitoring - Stationary Node
// ESP32 Stationary Node sends ambient temp & humidity to Wearable Node via ESP-NOW
// ----------------------------------------

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>  // Required to retrieve MAC address
#include "DHT.h"       // DHT Sensor Library

// --- DHT Sensor Configuration ---
#define DHTPIN D12           // DHT22 data pin
#define DHTTYPE DHT22        // Define sensor type
DHT dht(DHTPIN, DHTTYPE);    // Initialize DHT sensor

// --- Data Structure to Send ---
typedef struct struct_sensor_data {
  uint8_t macAddr[6];             // Sender MAC Address
  float skin_temperature;         // Placeholder for skin temp (not used in this node)
  float ambient_temperature;      // Ambient temperature from DHT
  float humidity;                 // Relative humidity from DHT
} sensor_data;

sensor_data myData;               // Instance of data struct

// MAC address of the Wearable Node (Receiver)
uint8_t receiverMacAddress[] = {0xEC, 0xDA, 0x3B, 0x61, 0x93, 0xA4};

// --- Callback on Send Completion ---
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status to: ");
  for (int i = 0; i < 6; i++) Serial.printf("%02X:", mac_addr[i]);
  Serial.print(" Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  dht.begin();  // Start DHT sensor
  Serial.println("Stationary ESP32 (Sender)");

  // --- WiFi Setup for ESP-NOW ---
  WiFi.mode(WIFI_STA);  // Set ESP32 to station mode
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // Lock channel to 1
  Serial.print("Current WiFi channel: ");
  Serial.println(WiFi.channel());

  // --- ESP-NOW Initialization ---
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback to check send status
  esp_now_register_send_cb(OnDataSent);

  // Get this node's MAC address and save to packet
  esp_wifi_get_mac(WIFI_IF_STA, myData.macAddr);
  Serial.print("Stationary Node MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // Read data from DHT sensor
  myData.skin_temperature = 0.0f;  // Not applicable for this device
  myData.ambient_temperature = dht.readTemperature(true);  // true = Fahrenheit
  myData.humidity = dht.readHumidity();

  // --- Peer Configuration ---
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);  // Set receiver MAC
  peerInfo.channel = 1;       // Must match both sender & receiver
  peerInfo.encrypt = false;   // Encryption not used in this version

  // Add peer if not already known
  if (!esp_now_is_peer_exist(peerInfo.peer_addr)) {
    esp_err_t addPeerResult = esp_now_add_peer(&peerInfo);
    if (addPeerResult != ESP_OK) {
      Serial.print("Failed to add peer: ");
      Serial.println(esp_err_to_name(addPeerResult));
      delay(2000);
      return;
    } else {
      Serial.println("Peer added successfully");
    }
  }

  // --- Send Data Packet ---
  esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t *) &myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Data Sent:");
    Serial.printf("Ambient Temp: %.2f °F\n", myData.ambient_temperature);
    Serial.printf("Humidity: %.2f %%\n", myData.humidity);
  } else {
    Serial.print("Error sending data: ");
    Serial.println(esp_err_to_name(result));
  }

  delay(10000);  // Send every 10 seconds
}
