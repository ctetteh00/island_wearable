// Stationary Node (Sender)
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h> // Required for esp_wifi_get_mac
#include "DHT.h"

// DHT Setup

#define DHTPIN D12          // DHT data pin connected to digital pin 2

#define DHTTYPE DHT22     // or DHT11

DHT dht(DHTPIN, DHTTYPE);

// Define a structure to hold the sensor data
typedef struct struct_sensor_data {
  uint8_t macAddr[6];
  float skin_temperature;
  float ambient_temperature;
  float humidity;
} sensor_data;

sensor_data myData;

// MAC address of the Wearable Node (Receiver)
uint8_t receiverMacAddress[] = {0xEC, 0xDA, 0x3B, 0x61, 0x93, 0xA4}; // EC:DA:3B:61:93:A4

// Callback function that will be executed when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status to: ");
  for (int i = 0; i < 6; i++) Serial.printf("%02X:", mac_addr[i]);
  Serial.print(" Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  dht.begin();
  Serial.begin(115200);
  Serial.println("Stationary ESP32 (Sender)");

  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  Serial.print("Setting WiFi channel to 1...");
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.println("Done.");
  Serial.print("Current WiFi channel: ");
  Serial.println(WiFi.channel());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register for Send CB to get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Get the ESP32's MAC address
  esp_wifi_get_mac(WIFI_IF_STA, myData.macAddr);
  Serial.print("Stationary Node MAC Address: ");
  Serial.println(WiFi.macAddress()); // Print in human-readable format
}

void loop() {
  Serial.print("Stationary Node MAC Address (Loop): ");
  Serial.println(WiFi.macAddress()); // Print in human-readable format

  // Simulate reading temperature
  myData.skin_temperature; // 25.0 + random(-5, 5) / 2.0; // Simulate temperature between 22.5 and 27.5
  myData.ambient_temperature = dht.readTemperature(true);//
  myData.humidity = dht.readHumidity();

  // Configure peer information
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6); // Use the correct receiver MAC address
  peerInfo.channel = 1; // Same channel as receiving device
  peerInfo.encrypt = false;

  Serial.print("Attempting to send to MAC: ");
  for (int i = 0; i < 6; i++) Serial.printf("%02X:", peerInfo.peer_addr[i]);
  Serial.println();
  Serial.printf("Peer Channel: %d, Encryption: %d\n", peerInfo.channel, peerInfo.encrypt);

  // Add peer (if not already added)
  if (!esp_now_is_peer_exist(peerInfo.peer_addr)) {
    esp_err_t addPeerResult = esp_now_add_peer(&peerInfo);
    if (addPeerResult != ESP_OK) {
      Serial.print("Error adding peer: ");
      Serial.println(esp_err_to_name(addPeerResult));
      delay(2000); // Wait a bit before trying again
      return;
    } else {
      Serial.println("Peer added successfully");
    }
  }

  esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t *) &myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.print("Sent Temperature: ");
    Serial.println(myData.skin_temperature);
    Serial.println(myData.ambient_temperature);
    Serial.println(myData.humidity);
  } else {
    Serial.print("Error sending data: ");
    Serial.println(esp_err_to_name(result));
  }

  delay(10000); // Broadcast every 10 seconds
}