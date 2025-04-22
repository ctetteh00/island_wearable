# 🌡️ Heat Risk Monitoring System

A dual-device system for real-time, personalized heat stress monitoring. Combines ESP32-based wearable and stationary nodes with a Firebase-connected dashboard to calculate the Modified Physiological Strain Index (mPSI), Heat Index, and a user-specific risk score.

## 🚀 Project Structure

- **Stationary Device**: Measures ambient temperature and humidity, broadcasts data via ESP-NOW.
- **Wearable Device**: Receives strongest signal, calculates skin temperature, uploads all data to Firebase.
- **Web Dashboard**: Real-time interface for monitoring risk levels and metrics.

---

## Repository Structure
/island_wearable/
├── firmware/
│   ├── wearable.ino         # Wearable ESP32 firmware
│   ├── stationary.ino       # Stationary ESP32 firmware
│   └── secrets.h            # Credentials (use from .env)
├── public/
│   ├── index.html           # Web UI
│   ├── app.js               # Logic & Firebase JS SDK
├── docs/
│   ├── 462_island_wearable_2.zip
    ├── wearable.kicad_pcb
    ├── wearable.kicad_pro
    ├── wearable.kicad_sch 
    ├── wearable_circuit_schematic.png
    ├── wearable_pcb_schematic.png
│   └── stationary_schematic.png
├── .env                     # Environment variables
├── setup_dependencies.sh # Library Dependencies for Arduino
├── README.md

## 📦 Hardware Components

### Wearable Device

| Component           | Part Number            | Description                          |
|--------------------|------------------------|--------------------------------------|
| Microcontroller     | Arduino Nano ESP32     | Wi-Fi + ESP-NOW                      |
| Temperature Sensor  | TMP236                 | Skin temperature measurement         |
| Charging Module     | Adafruit PowerBoost 1000C | LiPo charging circuit             |
| Battery             | LP103454               | 3.7V 1000mAh LiPo                    |
| Strap & Enclosure   | Chest band + PLA case  | Wearability and housing              |

### Stationary Device

| Component           | Part Number            | Description                          |
|--------------------|------------------------|--------------------------------------|
| Microcontroller     | Arduino Nano ESP32     | ESP-NOW broadcaster                  |
| Sensor              | DHT22                  | Temp + Humidity measurement          |
| Power               | 9V Battery + Switch    | Portable outdoor deployment          |
| Case                | Acrylic Enclosure      | Ventilated weather housing           |

---

## 🧠 Features

- ESP-NOW peer-to-peer communication
- RSSI-based device proximity selection
- mPSI calculation with exponential calibration
- Firebase-hosted real-time database
- Personalized scoring with comorbidities, sleep, and medications
- Dynamic UI with recommendations and color-coded feedback

---

## 🌐 Firebase Setup Instructions

1. **Create a Firebase Project**
   - Go to [Firebase Console](https://console.firebase.google.com/)
   - Click **Add Project** → Give it a name → Continue
   - Disable Google Analytics (optional)

2. **Add Realtime Database**
   - Navigate to `Build > Realtime Database`
   - Create a new database in test mode (can secure later)

3. **Set Up Web App**
   - Click **</>** to register a new Web App in Firebase
   - Save the API Key and Database URL (add to `.env`)

4. **Enable Authentication (Optional)**
   - Go to `Authentication > Sign-in method`
   - Enable "Email/Password" or other methods

---

## 🔒 Environment Configuration

Add a `.env` file in the root of your repo:

```env
# Firebase Credentials
FIREBASE_API_KEY=your_api_key_here
FIREBASE_DATABASE_URL=https://your-database-name.firebaseio.com

# WiFi Access
WIFI_SSID="YourNetwork"
WIFI_PASSWORD="YourPassword"

# Optional
WIFI_CHANNEL=1
