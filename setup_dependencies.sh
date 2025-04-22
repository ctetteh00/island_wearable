#!/bin/bash

# Ensure Arduino CLI is installed
if ! command -v arduino-cli &> /dev/null
then
    echo "❌ arduino-cli could not be found. Please install it from https://arduino.github.io/arduino-cli"
    exit
fi

echo "✅ Arduino CLI is installed."

# Set up Arduino CLI config if not already present
arduino-cli config init

# Add ESP32 board support
echo "🔄 Installing ESP32 board platform..."
arduino-cli core update-index
arduino-cli core install esp32:esp32

# Create the libraries folder if not already present
mkdir -p ~/Arduino/libraries

# Install required libraries
echo "📦 Installing required libraries..."

arduino-cli lib install "Firebase ESP Client for ESP8266 and ESP32"       # Firebase_ESP_Client.h
arduino-cli lib install "DHT sensor library"        # DHT.h
arduino-cli lib install "Adafruit Unified Sensor"   # Required by DHT
# These are part of the STL and ESP32 Arduino core:
# vector, algorithm, WiFi.h, esp_now.h, esp_wifi.h, Arduino.h

echo "✅ All libraries and platform dependencies are installed."
