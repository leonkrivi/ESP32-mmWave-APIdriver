# Requirements
**project specific**
- ESP32 controller
- MR24HPC1 mmWave sensor + wires to connect it to the ESP32
- MQTT broker (I used Mosquitto)

**other**
- ESP-IDF (I recommend VS Code extension: https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)
- Wi-Fi connection
- Git

# Startup instructions

### Clone GitHub repository
Change into the directory where you want the GitHub repository to be cloned.

`cd path_to_directory`

Run: `git clone https://github.com/LK110/ESP32-mmWave-APIdriver`

***

### Prepare valid credentials
Create a .env file and place it in the project root directory `ESP32-mmWave-APIdriver/`.

The file should contain the Wi-Fi SSID and Wi-Fi password for the network that you want your ESP32 to connect to, as well as the MQTT broker IP address.

The credentials should be in this format and order:
```
WIFI_SSID=your_wifi_ssid
WIFI_PASSWORD=your_wifi_password
MQTT_BROKER_IP=your_mqtt_broker_ip
```
**Note:** for demo use, I am using my home WLAN and am running the MQTT broker on my computer which is connected to that WLAN.
That means that MQTT_BROKER_IP is set to my WLAN IP address. If you want to mimic my setup, you should set MQTT_BROKER_IP to be
your WLAN IP, and run the broker from a computer connected to the same WLAN.

***

### Connect MR24HPC1 mmWave sensor to ESP32 board
Connect the sensor to the ESP32 controller as follows:
| MR24HPC1 | ESP32 |
|--|--|
| RX | GPIO17 |
| TX | GPIO16 |
| 5V | V5 |
| GND | GND |
> Manufacturer's information about the MR24HPC1 sensor:
>
> product overview: https://files.seeedstudio.com/wiki/mmWave-radar/24GHz_mmWave_Sensor-Human_Static_Presence_Module_Lite_Datasheet.pdf
>
> user manual: https://files.seeedstudio.com/wiki/mmWave-radar/24GHz_mmWave_Sensor-Human_Static_Presence_Module_Lite_Datasheet.pdf

**Note:** This project is not designed exclusively for the MR24HPC1 sensor. The code is separated into components, and you can write your own code
for a different sensor as a separate component and plug it into the existing code by changing a few lines in `main/app_main.c`.

***

### Run MQTT broker on your machine
If you want to use the Mosquitto MQTT broker like me, you will have to configure it so that it listens on the required interface on your computer.
Simplest configuration for local use:

```bash
listener 1883 0.0.0.0   # listens on port 1883 on all interfaces
allow_anonymous true    # enables connection without authentication
```
**Note:** The port should be set to 1883 in the Mosquitto configuration file because that is the value the project source code is expecting.
If you want to change it, go to `/components/mqtt_client_app/mqtt_app.c` and change the port value from `:1883` to match your Mosquitto configuration file.

```C
esp_mqtt_client_config_t cfg = {
        .broker.address.uri = "mqtt://"MQTT_BROKER_IP":1883",
};
```

> For more info on Mosquitto configuration you can look up: https://mosquitto.org/man/mosquitto-conf-5.html

Start Mosquitto with:

```bash
mosquitto -c path_to_mosquitto_config_file
```

> For more info on start up options visit: https://mosquitto.org/man/mosquitto-8.html

***

### Compile and run the driver
Connect your ESP32 to the computer with a USB data/power cable.

Position yourself in the root ESP-IDF project directory `ESP32-mmWave-APIdriver/`.

Make sure your ESP-IDF environment is configured for your setup.

> Here are the instructions for ESP-IDF as a VS Code extension: https://github.com/espressif/vscode-esp-idf-extension/blob/master/README.md
>
> Here are the instructions for ESP-IDF as a CLI: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#build-your-first-project

From here on, instructions assume you are using the ESP‑IDF VS Code extension.

Click on `Open ESP-IDF Terminal` in the status bar. This will bring up a terminal with all of the environment variables automatically loaded.

In the opened terminal run:
```bash
idf.py build flash monitor
```

This command will build the code, flash it to the ESP32, and monitor its output.

If automatic serial port detection fails you can specify the port manually. Replace `<port>` with your device path, for example on Linux:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

or on macOS:

```bash
idf.py -p /dev/tty.usbserial-0001 flash monitor
```