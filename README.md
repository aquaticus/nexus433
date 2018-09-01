![logo](pics/logo.png)

# Introduction
This program can read data from cheap 433 MHz wireless temperature and humidity sensors
and integrate with home automation systems using MQTT broker.
It was designed to work with popular ARM boards like Raspberry Pi, Orange Pi and many others.
Home Assistant MQTT sensor discovery is available.

To make it working you need only one external component &ndash; 433 MHz receiver.
It have to be connected to one of the I/O pin, the program can decode directly
data from sensors without any third party additional components.
It supports Nexus sensor protocol which is implemented in many cheap sensors.

## Features

* **Works on popular ARM Linux boards**
* **Only 1 EUR external part required**
* **Supports 5 EUR cheap Temperature and Humidity sensors**
* **Tracks sensor connection quality**
* **Reports sensor online and offline status**
* **Seamless integration with Home Assistant**
* **Easy integration with MQTT home automation systems**
* **Many configuration options**

## Applications

* **Home automation**
* **Temperature monitoring**
* **433 MHz to MQTT gateway**
* **433 MHz sensor diagnostics**

### License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.


# Hardware

## Supported boards

Any board that can run **Linux** with block device gpiod I/O driver can be used.
That means any board with modern version of Linux is supported. Specifically:
* **Orange Pi** running Armbian version 5.x. All boards.
* **Raspberry Pi** running Raspbian. All boards.
* Any board running **Armbian 5.x**.

## 433 MHz receiver

Any of cheap and popular receivers can be used. Make sure it is superheterodyne and supports
ASK/OOK modulation. The cost of the receiver is about 1 EUR when ordering directly from China.

![433 MHz receiver](pics/433mhz_receiver.png)

## Sensors

There are many vendors that produce temperature sensors compatible with Nexus protocol.
Here are a few that are confirmed to use Nexus protocol:
1. **Digoo DG-R8H** &ndash; Temperature and Humidity sensor. Easily available on the internet. Cost 5 EUR.
2. **Sencor SWS 21TS** &ndash; temperature only.

![433 MHz Sensors](pics/433mhz_sensors.jpg)

# Nexus protocol

Device sends every minute or so (Sencor every 58 seconds, Digoo every 80 seconds) 12 data frames.

Each data frame consists of 36 bits. There is no any checksum.

The meaning of bits:

| Bits    | 8  | 1       | 1 | 2       | 12          | 4    | 8        |
|---------|----|---------|---|---------|-------------|------|----------|
| Meaning | ID | Battery | 0 | Channel | Temperature | 1111 | Humidity |

1. *ID* &ndash; unique ID; Sencor generates new ID when battery is changed, Digoo keeps the same ID all the time.
2. *Battery* &ndash; low battery indicator; 1 &ndash; battery ok, 0 &ndash; low battery
3. *Channel* &ndash; channel number, 0 &ndash; first channel, 1 &ndash; second and so on.
4. *Temperature* &ndash; signed temperature in Celsius degrees.
5. *Humidity* &ndash; humidity

Every bit starts with 500 µs high pulse. The length of the following low
state decides if this is `1` (2000 µs) or `0` (1000 µs).
There is 4000 µs long low state period before every 36 bits.

![Nexus protocol timing](pics/nexus_protocol.png)

# How it works

The program is written in C++. It supports new block device GPIO subsystem. It uses [libgpiod](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/)
C library for I/O operations. For MQTT communication Mosquitto client [libmosquittpp](https://mosquitto.org/man/libmosquitto-3.html) is linked.
INI file parsing with the help of https://github.com/jtilly/inih.

One input pin is used to read data from 433 MHz receiver. Any pin can be used for this operation.
There is *no* requirement pin supports events.

The state of the pin is probed to detect valid frames. When 36 bits are read the data is converted to
JSON and pushed to MQTT broker. In addition program tracks when new transmitter is detected and when
transmitter become silent. This information is also pushed to MQTT.

New transmitters can be automatically detected by popular home automation system [Home Assistant](https://www.home-assistant.io/).

## Standard operations

Nexus protocol does not offer any way to check if the frame was received properly. The software accepts data from
sensor only when minimum 2 valid frames were received. Valid frame is when temperature and humidity values are reasonable
and fixed 4 bits are set to 1.

All sensor data is send as one JSON package in topic `nexus433/sensor/XXXX/state`, when XXXX is unique
transmitter ID made of ID (first byte) and channel number (second byte) in hex format.
For example `ae01` means transmitter with id `0xAE` and channel 2 (channels are 0 based).

In addition when transmitter first sends data program sends `online` on topic `nexus433/sensor/XXXX/connection`.
If there is no data from sensor for 90 seconds, program sends `offline` to the same topic.

When program starts and stops it sends `online` or `offline` to global connection topic: `nexus433/connection`.

For newly detected transmitters specially formatted JSON data is send to `homeassistant` topic to make
Home Assistant automatically discover sensor. For one transmitter 4 sensors are created: temperature,
humidity, battery and quality.

Battery value is either 100 (normal) or 0 (low). These values are compatible with Home Assistant battery class.
They can be changed in configuration file.

Quality is percentage of received frames. 100% is when all 12 frames are received.

## Configuration

### Command line options

|Short|Long&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|Description|
|-----|----|-----------|
| | `--verbose`| Enable verbose mode. More information is printed on the screen.|
| | `--daemon` | Run in daemon mode. Program works in background without a console. When verbose mode is on, it will prints messages.|
|`-g` | `--config` | Path to configuration file. Configuration options from file got higher priority than command line options. Default config file is `/etc/nexus433.ini`.|
|`-c` | `--chip` | GPIO chip device name, by default `/dev/gpiochip0`.|
| `-n` | `--pin` | Pin number to use from specified GPIO chip, default 1. This is not physical pin number but number assigned by block device GPIO. See discussion below for more info.|
| `-a` | `--address` | MQTT broker host, default `127.0.0.1`.|
| `-p` | `--port` | MQTT port number, default `1883`.|
| `-h` | `--help` | Displays help message.|

### Port number and GPIO chip name

Pin numbers are assigned by block GPIO device driver. To see available lines you can use `gpioinfo` utility from libgpiod library.

#### Raspberry Pi

Only one GPIO device is available `/dev/gpiochip0`.
Pin numbers are the same as GPIO line number, for example physical pin 11 is `GPIO17`. Pin number for that line is `17`.

#### Orange Pi

Boards based on Allwinner H3 chip got 2 I/O devices:
1. `/dev/gpiochip0` with 223 I/O lines
2. `/dev/gpiochip1` with 32 I/O lines

Port PA lines are assigned to 0&ndash;7, PBB to 8&ndash;15 an so on.
PL port is the first assigned to chip number 2.

### Configuration file

Thanks to configuration file you can alter the way program behaves. The file is typical INI file
divided into section. Every section got its own configuration keys.

By default program tries to open `/etc/nexus433` file. This location can be changed using `-g/--config`
command line option.

Strings must be entered without `"` characters. Boolean values accepted: `true`/`false`, `yes`/`no`, `1`/`0`.
Comments must begin with `;`.

#### `[transmitter]`

|Key|Type|Default|Description|
|---|----|-------|-----------|
|`silent_timeout`|number|90|When no data is received during this time in seconds, transmitters is treated as offline.|
|`minimum_frames`|number|2|Minimum number of data frames to accept transmission as valid.|_
|`battery_normal`|string|100|String sent in JSON MQTT message when battery level is normal.|
|`battery_low`|string|0|String sent in JSON MQTT message when battery level is low.|
|`discovery`|bool|false|Enable or disable Home Assistant MQTT sensor discovery.|
|`discovery_prefix`|string|homeassistant| Home Assistant MQTT discovery topic.|

#### `[receiver]`

|Key|Type|Default|Description|
|---|----|-------|-----------|
|`chip`|string|/dev/gpiochip0|GPIO device|
|`pin`|number|1|I/O pin number|
|`resolution_us`|number|1|decoder resolution in microseconds. Lower is better but higher system load.|
|`tolerance_us`|number|300|± tolerance of pulse length in microseconds.|
|`internal_led`|string|   | LED device name from `/sys/class/leds` used to indicate new readings; disabled by default.  |

#### `[mqtt]`

|Key|Type|Default|Description|
|---|----|-------|-----------|
|`host`|string|127.0.0.1|MQTT host name.|
|`port`|string|1883|MQTT port number.|
|`user`|string||MQTT user name.|
|`password`|string||MQTT password.|
|`cafile`|string||Path to a file containing the PEM encoded trusted CA certificate files.|
|`capath`|string||Path to a directory containing the PEM encoded trusted CA certificate files.|
|`certfile`|string||Path to a file containing the PEM encoded certificate file.|
|`keyfile`|string||Path to a file containing the PEM encoded private key. If encrypted, password must be passed in `keypass`.|
|`keypass`|string||Password to encrypted `keyfile`.|

##### SSL/TLS support

To enable SSL/TLS encrypted connection to MQTT broker specify CA certificate and optionally client certificate.

For CA certificate, one of two options must be set: `capath` or `cafile`.
For `capath` to work correctly, the certificates files must have `.pem` as the extension and you must run `openssl rehash <path to capath>` each time you add or remove a certificate.

If `capath` or `cafile` is specified without any other SSL/TLS option the client will verify Mosquitto server but the server will be unable to verify the client.
The connection will be encrypted.

To specify client certificate set `certfile`, `keyfile` and optionally `keypass`.

>:information_source: Please note that the traffic to MQTT broker would be encrypted but temperate and humidity data from sensors is available for everyone as radio transmission is not encrypted in any way.

#### `[ignore]`

List of transmitter IDs (2 bytes, ID and channel number) to be ignored. Data from ignored
transmitters in not sent to MQTT.

Key must be 2 byte ID and the value is true if transmitter is ignored, otherwise false.
For example to ignore transmitter 0xAE on channel 1 add:
`ae00=true`.

#### `[substitute]`

This section allows to change original ID of transmitter to another number. This may be useful
when the sensors is broken or the battery was changed and you do not want to reconfigure existing infrastructure.

Key must be ID of the transmitted and the value new ID.
For example, to change 0xAE channel 1 to 0x78 channel 2 use: `ae00=7801`.

#### `[temperature]`

Name of the temperature sensor reported to Home Assistant discovery mechanism for specified transmitter.
Key must be 2 bytes ID, value is the name, for example:
`ae00=Kitchen Temperature`
If not specified default value of `433MHz Sensor Id:XX channel Y Temperature` will be reported.

#### `[humidity]`

Name of the humidity sensor reported to Home Assistant discovery mechanism for specified transmitter.
Key must be 2 bytes ID, value is the name, for example:
`ae00=Kitchen Humidity`
If not specified default value of `433MHz Sensor Id:XX channel Y Humidity` will be reported.

#### `[battery]`

Name of the battery sensor reported to Home Assistant discovery mechanism for specified transmitter.
Key must be 2 bytes ID, value is the name, for example:
`ae00=Kitchen Sensor Battery`
If not specified default value of `433MHz Sensor Id:XX channel Y Battery` will be reported.

#### `[quality]`

Name of the quality sensor reported to Home Assistant discovery mechanism for specified transmitter.
Key must be 2 bytes ID, value is the name, for example:
`ae00=Kitchen Sensor Connection Quality`
If not specified default value of `433MHz Sensor Id:XX channel Y Quality` will be reported.

# Installation

Install mosquitto C++ library
```bash
sudo apt install -y libmosquittopp-dev
```

Install libgpiod C library
```bash
sudo apt install -y autoconf
sudo apt install pkg-config
sudo apt install -y libtool
sudo apt install -y autoconf-archive
git clone git://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git
cd libgpiod
./autogen.sh --enable-tools=yes
sudo make
sudo make install
sudo ldconfig
```

Clone git repository:

```bash
git clone https://github.com/aquaticus/nexus433
cd nexus433
make
sudo make install
```

# Quickstart

433 MHz receiver got typcally 3 pins: VIN, GND and DATA.
For Raspberry Pi and boards with compatible connector like Orange Pi connect VIN to pin #2 (5V), GND to pin #6 (GND)
and data to pin #11	(GPIO17).

|433 MHz receiver|Raspberry Pi|Orange Pi|
|----------------|------------|---------|
|`VIN`|`2` (+5V)|`2` (+5V)|
|`GND`|`6` (GND)|`6` (GND)|
|`DATA` | `11` (GPIO17)  | `11` (PA1) |


>:information_source: If you have no free 5V pin you can use 3.3V pin. 433 MHz receivers work with 3.3V as well.

![Raspberry Pi with 433MHz receiver](pics/rpi.jpg)

## MQTT Broker
Make sure you got any MQTT broker up and running. If not please install
[Mosquitto](https://mosquitto.org/). The examples below assume that MQTT broker is installed on the same machine. If not use `-a` command line option to specify MQTT address and `-p` for port number if different than default 1883.

## First run

Get your sensor close to the receiver and remove battery.

Run program in verbose mode and specify pin number.

|Raspberry Pi|Orange Pi|
|---|---|
|`sudo nexus433 --verbose -n 17`|`sudo nexus433 --verbose -n 1`|

Insert battery &ndash; that way the sensor sends immediately data, you do not need to wait
(remember sensor sends data about once per minute).
You should see new readings on the screen:
```
Loading configuration from /etc/nexus433.ini
Reading data from 433MHz receiver on /dev/gpiochip0 pin 17.
Decoder resolution: 1 µs; tolerance: 300 µs
Connected to MQTT broker.
New transmitter 0x5c channel 2
0x5c910ff38 Id:0x5c Channel:2 Temperature: 27.1°C Humidity: 56% Battery:1 Frames:12 (100%)
```
The last line is actual data from sensor. What is interesting that all 12 data frames were received.
That means reception is very good.
The next step is to monitor MQTT data. Install mosquitto client:
```bash
sudo apt install mosquitto-clients
```
and use `mosquitto_sub` to subscribe to interesting topics.
```bash
mosquitto_sub -v -t "nexus433/#"
```
Of course add `-h` and `-P` options if needed to specify MQTT host and port.
You should see:
```
nexus433/sensor/5c01/state { "temperature": 27.3, "humidity": 56, "battery": "100", "quality": 100 }
```
Now lets see what other data is sent to MQTT broker.
Stop both `nexus433` and `mosquitto_sub` by pressing `Control-C`.

Run `mosquitto-sub` first and subscribe to additional topic:
```bash
mosquitto_sub -v -t "nexus433/#" -t "homeassistant/#"
```
Now run:

|Raspberry Pi|Orange Pi|
|------------|---------|
|`sudo nexus433 --verbose -n 17`|`sudo nexus433 --verbose -n 1`|

and wait for the reading (you can use battery trick to limit waiting time).
When you receive first data packets check `mosquitto_sub` output. This time it logged more lines:
```
nexus433/connection offline
nexus433/connection online
nexus433/sensor/5c01/connection online
nexus433/sensor/5c01/state { "temperature": 27.4, "humidity": 56, "battery": "100", "quality": 100 }
```
Note that there are 2 connection topics. When program starts it publishes `online` message on `nexus433/connection`. When
stops for some reason or connection to MQTT is lost `offline`. Because `offline` is published with *retain* flag
you get this message every time subscribed to this topic. Thanks to this feature when the program is down any client
receives this information.

When new sensor is detected `online` is published on sensor specific topic.
The format is `nexus433/sensor/XXXX/connection`, where `XXXX` is 2 bytes sensor ID. When a sensor does not send
any data during 90 seconds (this can be configured) on the very same topic `offline` is published.

## LED

It is possible to use one of the built-in LEDs to indicate new packets. When configured, every time  valid data is received LED will be on for 500 ms.

To check available LEDs use: `ls /sys/class/leds`. Depending on your device number and names may differ.
Here is the result for Orange Pi PC board:
```
orangepi:green:pwr   
orangepi:red:status  
```

To use red LED modify nexus433.ini configuration file:
```ini
[receiver]
internal_led=orangepi:red:status
```

# Start as a service

`make install` automatically install service configuration files.
To start the service:
```bash
sudo service nexus433 start
```
To stop
```bash
sudo service nexus433 stop
```
To view service status:
```bash
sudo service nexus433 status
```
To run service every time system starts:
```bash
sudo systemctl enable nexus433
```

# Home Assistant integration

[Home Assistant](https://www.home-assistant.io/) is a popular open source home automation system.
It can automatically discover and configure sensors.

To make this feature working you must first enable MQTT support and enable discovery feature.
Make sure you add the following lines to your `configuration.yaml`:
```yaml
mqtt:
  discovery: true
```
For more information read this article: https://www.home-assistant.io/docs/mqtt/discovery/

You must also enable discovery support in `nexus433.ini` file, by default it is disabled. Add:
```ini
[transmitter]
discovery=yes
```

When enabled, for every new transmitter detected an extra JSON data is published on `homeassistant` topic.
One transmitter will create 4 new sensors: temperature, humidity, battery and quality.
Sample JSON data will look like this:
```json
{"name": "433MHz Sensor Id:78 channel 1 Temperature", "device_class": "temperature", "state_topic": "nexus433/sensor/7800/state", "availability_topic": "nexus433/connection", "unit_of_measurement": "°C", "value_template": "{{ value_json.temperature }}", "expire_after": 90}
```
You should see new sensors in Home Assistant interface. Now you must create a group or view to show them.
Note that device `name` is automatically generated. You can change the name by adding appropriate
section to configuration file. Remember to change the name *before* sensor is discovered.

There is no easy way to remove MQTT discovered sensors from Home Assistant. Once added they will be shown on the list of sensors forever.
If you like to experiment only with sensors, consider adding sensors manually as described in the next chapter.

## Adding sensors manually to Home Assistant

When you add sensor manually, you can modify it or remove any time you like.
Automatically added sensors cannot be modified nor removed.

To add a new sensor, you must perform exactly the same steps as adding any other MQTT sensor. See https://www.home-assistant.io/components/sensor.mqtt/ for more information.

To create a new humidity sensor add the following lines to your `configuration.yaml` (it is assumed the sensor id is `5c01`):
```yaml
- platform: mqtt
  state_topic: "nexus433/sensor/5c01/state"
  name: "Bedroom Humidity"
  expire_after: 90
  unit_of_measurement: '%'
  device_class: humidity
  availability_topic: "nexus433/connection"
  value_template: "{{ value_json.humidity }}"
```
Alternatively you could change `availability_topic` to `nexus433/sensor/5c01/connection`
and remove `expiry_after`.

Temperature, battery and quality sensors can be added in the same way. Just remember to change
`device_class` and `unit_of_measurement` and `value_template`.
Note that there is no special device class for quality, so you can use something like this:
```yaml
- platform: mqtt
  state_topic: "nexus433/sensor/5c01/state"
  name: "Bedroom Sensor Connection Quality"
  expire_after: 90
  unit_of_measurement: '%'
  icon: mdi:wifi
  availability_topic: "nexus433/connection"
  value_template: "{{ value_json.quality }}"
```

# Reception problems

If you got problems receiving proper data from sensors you can do one the following:

1. Check if data is properly received from close range, if not check batter or test with other sensor.
2. Move antenna to different position.
3. Reduce `resolution_us` parameter to 0 and increase `tolerance_us`.
4. Use longer antenna. Just attach a piece of wire. The length of the wire is related to wave length, you can use: 69cm, 34cm, 17cm, 8cm, 4cm wires.
5. If you got more than one sensor make sure they transmit data in different time. If two sensors collide remove battery and insert again.
