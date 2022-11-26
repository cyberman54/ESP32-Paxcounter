
# Payload format

You can select different payload formats in [`paxcounter.conf`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/paxcounter_orig.conf):

- ***Plain*** uses big endian format and generates json fields, e.g. useful for TTN console

- ***Packed*** uses little endian format and generates json fields

- [***CayenneLPP***](https://mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload-reference-implementation) generates MyDevices Cayenne readable fields


```c linenums="20" title="src/paxcounter_orig.conf"
--8<-- "src/paxcounter_orig.conf:20:20"
```


!!! danger "Decrepated information from the things network v2"

	If you're using [TheThingsNetwork](https://www.thethingsnetwork.org/) (TTN) you may want to use a payload converter. Go to TTN Console - Application - Payload Formats and paste the code example below in tabs Decoder and Converter. This way your MQTT application can parse the fields `pax`, `ble` and `wifi`.

	To add your device to myDevices Cayenne platform select "Cayenne-LPP" from Lora device list and use the CayenneLPP payload encoder.

	To track a paxcounter device with on board GPS and at the same time contribute to TTN coverage mapping, you simply activate the [TTNmapper integration](https://www.thethingsnetwork.org/docs/applications/ttnmapper/) in TTN Console. Both formats *plain* and *packed* generate the fields `latitude`, `longitude` and `hdop` required by ttnmapper. Important: set TTN mapper port filter to '4' (paxcounter GPS Port).


Hereafter described is the default *plain* format, which uses MSB bit numbering. Under /TTN in this repository you find some ready-to-go decoders which you may copy to your TTN console:

[**plain_decoder.js**](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/TTN/plain_decoder.js) |
[**plain_converter.js**](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/TTN/plain_converter.js) |
[**packed_decoder.js**](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/TTN/packed_decoder.js) |
[**packed_converter.js**](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/TTN/packed_converter.js)

**Port #1:** Paxcount data

	byte 1-2:		Number of unique devices, seen on Wifi [00 00 if Wifi scan disabled]
	byte 3-4:		Number of unique devices, seen on Bluetooth [ommited if BT scan disabled]

**Port #2:** Device status query result

  	byte 1-2:		Battery or USB Voltage [mV], 0 if no battery probe
	byte 3-10:		Uptime [seconds]
	byte 11: 		CPU temperature [°C]
	bytes 12-15:	Free RAM [bytes]
	byte 16:		Last CPU core 0 reset reason
	bytes 17-20:	Number of restarts since last power cycle

**Port #3:** Device configuration query result

	byte 1:			Lora DR (0..15, see rcommand 0x05) [default 5]
	byte 2:			Lora TXpower (2..15) [default 15]
	byte 3:			Lora ADR (1=on, 0=off) [default 1]
	byte 4:			Screensaver status (1=on, 0=off) [default 0]
	byte 5:			Display status (1=on, 0=off) [default 0]
	byte 6:			Counter mode (0=cyclic unconfirmed, 1=cumulative, 2=cyclic confirmed) [default 0]
	bytes 7-8:		RSSI limiter threshold value (negative, MSB) [default 0]
	byte 9:			Scan and send cycle in seconds/2 (0..255) [default 120]
	byte 10:		Wifi channel hopping interval in seconds/100 (0..255), 0 means no hopping [default 50]
	byte 11:		Bluetooth channel switch interval in seconds/100 (0..255) [default 10]
	byte 12:		Bluetooth scanner status (1=on, 0=0ff) [default 1]
	byte 13:		Wifi antenna switch (0=internal, 1=external) [default 0]
	bytes 14-15:	Sleep cycle in seconds/10 (MSB) [default 0]
	byte 16:		Payloadmask (Bitmask, 0..255, see rcommand 0x14)
	byte 17:		0 (reserved)
	bytes 18-28:	Software version (ASCII format, terminating with zero)


**Port #4:** GPS data (only if device has fature GPS, and GPS data is enabled and GPS has a fix)

	bytes 1-4:		Latitude
	bytes 5-8:		Longitude
	byte 9:			Number of satellites
	bytes 10-11:	HDOP
	bytes 12-13:	Altitude [meter]

**Port #5:** Button pressed alarm

	byte 1:			static value 0x01

**Port #6:** (unused)

**Port #7:** Environmental sensor data (only if device has feature BME)

	bytes 1-2:	Temperature [°C]
	bytes 3-4:	Pressure [hPa]
	bytes 5-6:	Humidity [%]
	bytes 7-8:	Indoor air quality index (0..500), see below

	Indoor air quality classification:
	0-50		good
	51-100		average
	101-150 	little bad
	151-200 	bad
	201-300 	worse
	301-500 	very bad

**Port #8:** Battery voltage data (only if device has feature BATT)

  	bytes 1-2:	Battery or USB Voltage [mV], 0 if no battery probe

**Port #9:** Time/Date

  	bytes 1-4:	board's local time/date in UNIX epoch (number of seconds that have elapsed since January 1, 1970 (midnight UTC/GMT), not counting leap seconds)

**Ports #10, #11, #12:** User sensor data

	Format is specified by user in function `sensor_read(uint8_t sensor)`, see `src/sensor.cpp`.
