# ESP32 mmWave API driver (MR24HPC)

ESP32 project that reads an MR24HPC1 mmWave sensor over UART and publishes the sensor state to an MQTT broker.

This README is written to be easy to follow (tested on macOS + VS Code ESP-IDF extension) and matches the current project code.

## What you need

### Hardware

- ESP32 development board
- MR24HPC1 mmWave sensor
- Jumper wires
- USB data cable

### Software

- VS Code + ESP-IDF extension: https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension
- Git
- Wi-Fi network
- MQTT broker (Mosquitto recommended)

## Quick start (end-to-end)

1. Clone the repository

```bash
cd path_to_directory
git clone https://github.com/leonkrivi/ESP32-mmWave-APIdriver
cd ESP32-mmWave-APIdriver
```

2. Create your `.env`

This project reads credentials from a `.env` file in the project root during CMake configure.

```bash
cp .env.example .env
```

Edit `.env`:

```env
WIFI_SSID=your_wifi_ssid
WIFI_PASSWORD=your_wifi_password
MQTT_BROKER_IP=your_mqtt_broker_ip
```

If you change `.env` later, run:

```bash
idf.py reconfigure
```

3. Wire the MR24HPC1 sensor

The UART pins are fixed in the code:

- ESP32 TX = GPIO17
- ESP32 RX = GPIO16

Connect UART as a cross-over (TX -> RX, RX -> TX):

| MR24HPC1 | ESP32       |
| -------- | ----------- |
| RX       | GPIO17 (TX) |
| TX       | GPIO16 (RX) |
| 5V       | 5V          |
| GND      | GND         |

Sensor docs:

- Datasheet: https://files.seeedstudio.com/wiki/mmWave-radar/24GHz_mmWave_Sensor-Human_Static_Presence_Module_Lite_Datasheet.pdf
- User manual: https://files.seeedstudio.com/wiki/mmWave-radar/MR24HPC1_User_Manual-V2.0.pdf

4. Start your MQTT broker

This firmware expects MQTT on port 1883.
The app waits for MQTT to connect before it starts publishing sensor data.

Minimal Mosquitto config:

```conf
listener 1883 0.0.0.0
allow_anonymous true
```

Start Mosquitto:

```bash
mosquitto -c path_to_mosquitto_config_file
```

5. Build, flash and monitor

In VS Code, click `Open ESP-IDF Terminal` in the status bar, then run:

```bash
idf.py build flash monitor
```

If the serial port is not detected automatically on macOS:

```bash
ls /dev/cu.*
idf.py -p /dev/cu.usbserial-0001 flash monitor
```

## Troubleshooting

- Build fails with missing credentials: ensure `.env` exists and contains `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_BROKER_IP`, then run `idf.py reconfigure`.
- No sensor data: double-check TX/RX are crossed and the sensor is powered.
- MQTT never connects: verify `MQTT_BROKER_IP` is reachable from the ESP32 on your Wi-Fi network and that the broker listens on port 1883.

## Notes

- The firmware waits for Wi-Fi, then waits for MQTT, then starts publishing.
- Default sensor query interval is 5000 ms.
- The codebase is component-based, so swapping the sensor component is straightforward if needed.
