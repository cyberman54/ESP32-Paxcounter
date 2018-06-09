# ESP32-Paxcounter
**Wifi & Bluetooth driven, LoRaWAN enabled, battery powered mini Paxcounter built on cheap ESP32 LoRa IoT boards**

--> see development branch of this repository for latest alpha version <--

<img src="img/Paxcounter-title.jpg">

# Use case

Paxcounter is a proof-of-concept device for metering passenger flows in realtime. It counts how many mobile devices are around. This gives an estimation how many people are around. Paxcounter detects Wifi and Bluetooth signals in the air, focusing on mobile devices by filtering vendor OUIs in the MAC adress.

Intention of this project is to do this without intrusion in privacy: You don't need to track people owned devices, if you just want to count them. Therefore, Paxcounter does not persistenly store MAC adresses and does no kind of fingerprinting the scanned devices.

Bonus of this project is that metered data is transferred via a LoRaWAN network, not via usual GSM/LTE or Wifi uplink. 

You can build this project battery powered and reach a full day uptime with a single 18650 Li-Ion cell.

This can all be done with a single small and cheap ESP32 board for less than $20.

# Hardware

Supported ESP32 based LoRa IoT boards:
- Heltec LoRa-32 {1}
- TTGOv1 {1}
- TTGOv2 {1}{4}
- TTGOv2.1 {1}{5}
- TTGO T-Beam {4}{5}
- Pycom LoPy {2}
- Pycom LoPy4 {2}
- Pycom FiPy {2}
- LoLin32 with [LoraNode32 shield](https://github.com/hallard/LoLin32-Lora) {2}{3}
- LoLin32 Lite with [LoraNode32-Lite shield](https://github.com/hallard/LoLin32-Lite-Lora) {2}{3}

{1} on board OLED Display supported; {2} on board RGB LED supported; {3} on board Hardware unique DEVEUI supported; {4} special wiring needed, see instructions in file /hal/<board>.h; {5} battery voltage monitoring supported

Target platform must be selected in [platformio.ini](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/platformio.ini).<br>
Hardware dependent settings (pinout etc.) are stored in board files in /hal directory.<br>

<b>3D printable cases</b> can be found (and, if wanted so, ordered) on Thingiverse, see 
<A HREF="https://www.thingiverse.com/thing:2670713">Heltec</A> and <A HREF="https://www.thingiverse.com/thing:2811127">TTGOv2</A>, for example.<br>

<b>Power consumption</b> was metered at around 1000mW, depending on board (i.e. has display or not) and user settings in paxcounter.conf. If you are limited on battery, you may want to save around 30% power by disabling bluetooth (commenting out line *#define BLECOUNTER* in paxcounter.conf).

# Preparing

Before compiling the code,

- **edit paxcounter.conf** and taylor settings in this file according to your needs and use case. Please take care of the duty cycle regulations of the LoRaWAN network you're going to use.

- **create file loraconf.h in your local /src directory** using the template [loraconf.sample.h](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/loraconf.sample.h) and populate it with your personal APPEUI und APPKEY for the LoRaWAN network. If you're using popular <A HREF="https://thethingsnetwork.org">TheThingsNetwork</A> you can copy&paste the keys from TTN console or output of ttnctl.

To join the network only method OTAA is supported, not ABP. The DEVEUI for OTAA will be derived from the device's MAC adress during device startup and is shown as well on the device's display (if it has one) as on the serial console for copying it to your LoRaWAN network server settings.

If your device has a fixed DEVEUI enter this in your local loraconf.h file. During compile time this DEVEUI will be grabbed from loraconf.h and inserted in the code.

If your device has silicon **Unique ID** which is stored in serial EEPROM Microchip 24AA02E64 you don't need to change anything. The Unique ID will be read during startup and DEVEUI will be generated from it, overriding settings in loraconf.h.

# Building

Use <A HREF="https://platformio.org/">PlatformIO</A> with your preferred IDE for development and building this code. Make sure you have latest PlatformIO version, at least 
<A HREF="https://community.platformio.org/t/platformio-core-3-5-3/4238?utm_source=platformio&utm_medium=piohome">v3.5.3</A>.

# Uploading

To upload the code to your ESP32 board this needs to be switched from run to bootloader mode. Boards with USB bridge like Heltec and TTGO usually have an onboard logic which allows soft switching by the upload tool. In PlatformIO this happenes automatically.<p>
The LoPy/LoPy4/FiPy board needs to be set manually. See these 
<A HREF="https://www.thethingsnetwork.org/labs/story/program-your-lopy-from-the-arduino-ide-using-lmic">instructions</A> how to do it. Don't forget to press on board reset button after switching between run and bootloader mode.<p>
The original Pycom firmware is not needed, so there is no need to update it before flashing Paxcounter. Just flash the compiled paxcounter binary (.elf file) on your LoPy/LoPy4/FiPy. If you later want to go back to the Pycom firmware, download the firmware from Pycom and flash it over.

# Legal note

**Depending on your country's laws it may be illegal to sniff wireless networks for MAC addresses. Please check and respect your country's laws before using this code!**

(e.g. US citizens may want to check [Section 18 U.S. Code § 2511](https://www.law.cornell.edu/uscode/text/18/2511) and [discussion](https://github.com/schollz/howmanypeoplearearound/issues/4) on this)

(e.g. UK citizens may want to check [Data Protection Act 1998](https://ico.org.uk/media/1560691/wi-fi-location-analytics-guidance.pdf) and [GDPR 2018](https://ico.org.uk/for-organisations/guide-to-the-general-data-protection-regulation-gdpr/key-definitions/))

(e.g. Citizens in the the Netherlands may want to read [this article](https://www.ivir.nl/publicaties/download/PrivacyInformatie_2016_6.pdf) and [this article](https://autoriteitpersoonsgegevens.nl/nl/nieuws/europese-privacytoezichthouders-publiceren-opinie-eprivacyverordening)) 

Note: If you use this software you do this at your own risk. That means that you alone - not the authors of this software - are responsible for the legal compliance of an application using this or build from this software and/or usage of a device created using this software. You should take special care and get prior legal advice if you plan metering passengers in public areas and/or publish data drawn from doing so.

# Privacy disclosure

Paxcounter generates identifiers for sniffed MAC adresses and collects them temporary in the device's RAM for a configurable scan cycle time (default 240 seconds). After each scan cycle the collected identifiers are cleared. Identifiers are generated by salting and hashing MAC adresses. The random salt value changes after each scan cycle. Identifiers and MAC adresses are never transferred to the LoRaWAN network. No persistent storing of MAC adresses, identifiers or timestamps and no other kind of analytics than counting are implemented in this code. Wireless networks are not touched by this code, but MAC adresses from wireless devices as well within as not within wireless networks, regardless if encrypted or unencrypted, are sniffed and processed by this code. If the bluetooth option in the code is enabled, bluetooth MACs are scanned and processed by the included BLE stack, then hashed and counted by this code.

# LED

Legend for mono color on board LED:

- Single Flash (50ms): seen a new Wifi or BLE device
- Quick blink (20ms on each 1/5 second): joining LoRaWAN network in progress or pending
- Small blink (10ms on each 1/2 second): LoRaWAN data transmit in progress or pending
- Long blink (200ms on each 2 seconds): LoRaWAN stack error

Legend for RGB LED (LoPy/LoPy4/FiPy/Lolin32 only):

- Green each blink: seen a new Wifi device
- Magenta each blink: seen a new BLE device
- Yellow quick blink: joining LoRaWAN network in progress or pending
- Blue blink: LoRaWAN data transmit in progress or pending
- Red long blink: LoRaWAN stack error

# Payload

LoRaWAN Port #1: Counter data

	byte 1:			WiFi counter, MSB
	byte 2:			WiFi counter, LSB
	byte 3:			BLE counter, MSB
	byte 4:			BLE counter, LSB

LoRaWAN Port #2: Remote commands

	see remote command set

LoRaWAN Port #3: GPS data

	bytes 1-4:			Latitude
	bytes 4-8:			Longitude
	bytes 9-10:			Satellites
	bytes 11-12:		HDOP
	bytes 13-14:		Altitude

If you're using [TheThingsNetwork](https://www.thethingsnetwork.org/) you may want to use a payload converter. Go to TTN Console - Application - Payload Formats and paste the code example below in tabs Decoder and Converter. Make sure that your application parses the fields `pax`, `ble` and `wifi`.

Decoder:

```javascript
function Decoder(bytes, port) {
  // decode counter messages
  var decoded = {};

  if (port === 1) {
    decoded.wifi = (bytes[0] << 8) | bytes[1];
    decoded.ble = (bytes[2] << 8) | bytes[3];
  }

  // decode GPS messages
  if (port === 3) {
    decoded.latitude = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
    decoded.longitude = (bytes[7] << 24) | (bytes[6] << 16) | (bytes[5] << 8) | bytes[4];
    decoded.satellites = (bytes[9] << 8) | bytes[8];
    decoded.hdop = (bytes[11] << 8) | bytes[10];
    decoded.altitude = (bytes[13] << 8) | bytes[12];
  }

  return decoded;
}
```

Converter:

```javascript
function Converter(decoded, port) {
  var converted = decoded;
  // sum up ble + wifi counters
  if (port === 1) {
    converted.pax = converted.ble + converted.wifi;
  }
  // convert some GPS values
  if (port === 3) {
    converted.latitude = converted.latitude / 100000;
	converted.longitude = converted.longitude / 100000;
	converted.hdop = converted.hdop / 100;
  return converted;
}
```

# Remote control

The device listenes for remote control commands on LoRaWAN Port 2.
Each command is followed by exactly one parameter.
For "set" commands, multiple command/parameter pairs can be concatenated and sent in one downlink, all commands are executed. For "get" commands, only one command/parameter pair per downlink is processed.

Note: all settings are stored in NVRAM and will be reloaded when device starts. To reset device to factory settings press button (if device has one), or send remote command 09 02 09 00 unconfirmed(!) once.

0x01 set scan RSSI limit

	1 ... 255 used for wifi and bluetooth scan radius (greater values increase scan radius, values 50...110 make sense)
	0 = RSSI limiter disabled [default]

0x02 set counter mode

	0 = cyclic unconfirmed, mac counter reset after each wifi scan cycle, data is sent only once [default]
	1 = cumulative counter, mac counter is never reset
	2 = cyclic confirmed, like 0 but data is resent until confirmation by network received
  
0x03 set GPS on/off (NOT YET IMPLEMENTED)

	0 = GPS off [default]
	1 = GPS on, GPS data set (if present) is added to payload

0x04 set display on/off

	0 = display off
	1 = display on [default]

0x05 set LoRa spread factor

	7 ... 12 [default: 9]

0x06 set LoRa TXpower

	2 ... 15 [default: 15]
	
0x07 set LoRa Adaptive Data Rate mode

	0 = ADR off
	1 = ADR on [default]

	Note: set ADR to off, if device is moving, set to on, if not.
	If ADR is set to on, SF value is shown inverted on display.

0x08 do nothing

	useful to clear pending commands from LoRaWAN server quere, or to check RSSI on device

0x09 reset functions

	0 = restart device
	1 = reset MAC counter to zero
	2 = reset device to factory settings

0x0A set LoRaWAN payload send cycle

	0 ... 255 payload send cycle in seconds/2
	e.g. 120 -> payload is transmitted each 240 seconds [default]

0x0B set Wifi channel switch interval timer

	0 ... 255 duration for scanning a wifi channel in seconds/100
	e.g. 50 -> each channel is scanned for 500 milliseconds [default]

0x0C set Bluetooth channel switch interval timer

	0 ... 255 duration for scanning a bluetooth advertising channel in seconds/100
	e.g. 8 -> each channel is scanned for 80 milliseconds [default]

0x0D (NOT YET IMPLEMENTED) set BLE and WIFI vendorfilter mode

	0 = disabled (use to count devices, not people)
	1 = enabled [default]

0x0E set Bluetooth scanner

	0 = disabled
	1 = enabled [default]

0x0F set WIFI antenna switch (works on LoPy/LoPy4/FiPy only)

	0 = internal antenna [default]
	1 = external antenna

0x10 set RGB led luminosity (works on LoPy/LoPy4/FiPy and LoRaNode32 shield only)

	0 ... 100 percentage of luminosity (100% = full light)
	e.g. 50 -> 50% of luminosity [default]

0x80 get device configuration

device answers with it's current configuration. The configuration is a C structure declared in file [globals.h](src/globals.h#L32-L50) with the following definition:

	byte 1:			Lora SF (7..12)
	byte 2:			Lora TXpower (2..15)
	byte 3:			Lora ADR (1=on, 0=off)
	byte 4:			Screensaver status (1=on, 0=off)
	byte 5:			Display status (1=on, 0=off)
	byte 6:			Counter mode (0=cyclic unconfirmed, 1=cumulative, 2=cyclic confirmed)
	bytes 7-8:		RSSI limiter threshold value (negative)
	byte 9:			Lora Payload send cycle in seconds/2 (0..255)
	byte 10:		Wifi channel switch interval in seconds/100 (0..255)
	byte 11:		Bluetooth channel switch interval in seconds/100 (0..255)
	byte 12:		Bluetooth scanner status (1=on, 0=0ff)
	byte 13:		Wifi antenna switch (0=internal, 1=external)
	byte 14:		Vendorfilter mode (0=disabled, 1=enabled)
	byte 15:		RGB LED luminosity (0..100 %)
	byte 16:		GPS status (1=on, 0=off)
	bytes 17-27:		Software version (ASCII format, terminating with zero)

0x81 get device uptime

	bytes 1-8:		uptime in seconds (little endian format)

0x82 get device cpu temperature

	bytes 1-4:		chip temperature in degrees celsius (little endian format)

0x83 get device battery voltage

	bytes 1-2:		battery voltage in millivolt, 0 if unreadable (little endian format)

0x84 get device GPS status (NOT YET IMPLEMENTED)

	bytes 1-4:		latitude
	bytes 5-8:		longitude
	byte 9:			number of satellites
	byte 10:		HDOP
	bytes 11-12:	altidute [meter]

# License

Copyright  2018 Oliver Brandmueller <ob@sysadm.in>

Copyright  2018 Klaus Wilting <verkehrsrot@arcor.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

NOTICE: 
Parts of the source files in this repository are made available under different licenses,
see file <A HREF="https://github.com/cyberman54/ESP32-Paxcounter/blob/master/LICENSE">LICENSE.txt</A> in this repository. Refer to each individual source file for more details.

# Credits

Thanks to 
- [Oliver Brandmüller](https://github.com/spmrider) for idea and initial setup of this project
- [Charles Hallard](https://github.com/hallard) for major code contributions to this project
- [robbi5](https://github.com/robbi5) for the payload converter
