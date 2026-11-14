# ECSE3038 Test 2 — Bathroom Fan IoT System

## Project Description

This project is a smart bathroom fan system developed for ECSE3038 Test 2.

It uses an ESP32 microcontroller simulated in Wokwi, a DHT22 humidity sensor, and an LED that represents the bathroom extractor fan.

The ESP32 sends humidity readings to a FastAPI backend hosted on an Azure virtual machine. The API stores fan information, humidity readings, and settings in MongoDB Atlas.

MQTT is used to send fan settings from the API to the ESP32 and to publish the fan's current state.

## Hardware and Simulation

- ESP32 DevKit
- DHT22 humidity sensor connected to GPIO 15
- Fan LED connected to GPIO 2
- 220Ω resistor
- Wokwi simulator

## Fan Operating Modes

The fan supports three operating modes:

- **ON:** The fan stays on regardless of humidity.
- **OFF:** The fan stays off regardless of humidity.
- **AUTO:** The fan turns on when humidity is greater than `max_humidity` and turns off otherwise.

The fan starts in the OFF state until valid settings are received through MQTT.

## Technologies Used

- ESP32 and Arduino
- PlatformIO
- Wokwi
- Python and FastAPI
- MongoDB Atlas
- MQTT
- Azure Virtual Machine
- GitHub

## Project Structure

```text
ECSE3038-Test-2/
├── api/
│   ├── app.py
│   └── requirements.txt
├── board/
│   ├── include/
│   ├── src/
│   │   └── main.cpp
│   ├── diagram.json
│   ├── platformio.ini
│   └── wokwi.toml
├── .gitignore
└── README.md
```

## Running the API

1. Open a terminal in the `api` directory.
2. Create and activate a Python virtual environment.
3. Install dependencies:

   ```bash
   pip install -r requirements.txt
   ```

4. Create an `.env` file containing:

   ```env
   MONGODB_URI=your_mongodb_connection_string
   MQTT_NAME=skyethomas
   ```

5. Start the FastAPI server:

   ```bash
   uvicorn app:app --host 127.0.0.1 --port 8000
   ```

6. Open `/docs` on the API server to test the endpoints.

## Running the ESP32 Board

1. Open the `board` folder in VS Code.
2. Install the PlatformIO and Wokwi extensions.
3. Create `board/include/secrets.h` with the registered fan's API key:

   ```cpp
   #pragma once
   #define DEVICE_KEY "your_fan_api_key"
   ```

4. Build the project using PlatformIO.
5. Start the Wokwi simulation.
6. View humidity readings and MQTT activity in the Serial Monitor.

## API Endpoints

| Method | Endpoint | Description |
|---|---|---|
| POST | `/fans` | Register a fan and generate an API key |
| GET | `/fans` | List registered fans |
| GET | `/fans/{mac}` | Retrieve a fan |
| POST | `/readings` | Submit humidity readings |
| GET | `/readings` | Retrieve stored readings |
| PUT | `/fans/{mac}/settings` | Update fan settings |
| DELETE | `/fans/{mac}` | Delete a registered fan |

## MQTT Topics

The project uses the following MQTT topics:

- `ecse3038/skyethomas/fan/settings` — receives fan mode and humidity threshold
- `ecse3038/skyethomas/fan/state` — publishes the fan's ON/OFF state
- `ecse3038/skyethomas/status` — publishes the ESP32 connection status

MQTT broker: `135.232.203.144:1883`

## Testing

The system was tested using Wokwi, FastAPI Swagger UI, and the ECSE3038 MQTT webpage.

The tests confirmed that humidity readings are accepted by the API with HTTP 201, MQTT settings are received by the ESP32, and the LED responds to ON, OFF, and AUTO modes.

## Security

The `.env` and `board/include/secrets.h` files contain credentials and are excluded from GitHub using `.gitignore`. Virtual environments and PlatformIO build files are also excluded.