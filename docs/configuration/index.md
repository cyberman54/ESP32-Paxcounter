# Configuration

## Sensors and Peripherals

You can add up to 3 user defined sensors. Insert your sensor's payload scheme in [`sensor.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/sensor.cpp). More examples and a detailed description can be found in the [sensor documentation](custom-sensors.md).

### Supported Peripherals

* Bosch BMP180 / BME280 / BME680
* SDS011
* RTC DS3231
* generic serial NMEA GPS
* I2C Lopy GPS

For these peripherals no additional code is needed. To activate configure them in the board's hal file before building the code.

See [`generic.h`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/hal/generic.h) for all options and for proper configuration of BME280/BME680.

=== "BME/ BMP Configuration"
	```c linenums="37" title="src/hal/generic.h"
	--8<-- "src/hal/generic.h:37:49"
	```
=== "SDS011 Configuration"
	```c linenums="51" title="src/hal/generic.h"
	--8<-- "src/hal/generic.h:51:56"
	```
=== "Custom Sensors Configuration"
	```c linenums="57" title="src/hal/generic.h"
	--8<-- "src/hal/generic.h:57:60"
	```

=== "Complete `generic.h`"
	```c linenums="1" title="src/hal/generic.h"
	--8<-- "src/hal/generic.h"
	```

Output of user sensor data can be switched by user remote control command `0x14` sent to Port 2.

Output of sensor and peripheral data is internally switched by a bitmask register. Default mask can be tailored by editing *cfg.payloadmask* initialization value in [`configmanager.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/configmanager.cpp) following this scheme:

| Bit | Sensordata    | Default |
| --- | ------------- | ------- |
| 0   | Paxcounter    | on      |
| 1   | unused        | off     |
| 2   | BME280/680    | on      |
| 3   | GPS*          | on      |
| 4   | User sensor 1 | on      |
| 5   | User sensor 2 | on      |
| 6   | User sensor 3 | on      |
| 7   | Batterylevel  | off     |

\*) GPS data can also be combined with paxcounter payload on port 1, `#define GPSPORT 1` in paxcounter.conf to enable

```c linenums="102" title="src/paxcounter_orig.conf"
--8<-- "src/paxcounter_orig.conf:102:102"
```


## Power saving mode

Paxcounter supports a battery friendly power saving mode. In this mode the device enters deep sleep, after all data is polled from all sensors and the dataset is completeley sent through all user configured channels (LORAWAN / SPI / MQTT / SD-Card). Set `#define SLEEPCYCLE` in paxcounter.conf to enable power saving mode and to specify the duration of a sleep cycle.

```c linenums="20" title="src/paxcounter_orig.conf"
--8<-- "src/paxcounter_orig.conf:20:20"
```


 Power consumption in deep sleep mode depends on your hardware, i.e. if on board peripherals can be switched off or set to a chip specific sleep mode either by MCU or by power management unit (PMU) as found on TTGO T-BEAM v1.0/V1.1. See [`power.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/power.cpp) for power management, and [`reset.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/reset.cpp) for sleep and wakeup logic.



## Time sync

Paxcounter can keep a time-of-day synced with external or on board time sources. Set `#define TIME_SYNC_INTERVAL` in `paxcounter.conf` to enable time sync.

```c linenums="88" title="src/paxcounter_orig.conf"
--8<-- "src/paxcounter_orig.conf:88:88"
```

Supported external time sources are GPS, LORAWAN network time and LORAWAN application timeserver time. Supported on board time sources are the RTC of ESP32 and a DS3231 RTC chip, both are kept sycned as fallback time sources. Time accuracy depends on board's time base which generates the pulse per second. Supported are GPS PPS, SQW output of RTC, and internal ESP32 hardware timer. Time base is selected by #defines in the board's hal file, see example in [`generic.h`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/hal/generic.h).

```c linenums="87" title="src/hal/generic.h"
--8<-- "src/hal/generic.h:87:96"
```


!!! tip

	If your LORAWAN network does not support network time, you can run a Node-Red timeserver application using the enclosed [**Timeserver code**](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/Node-RED/Timeserver.json). Configure the MQTT nodes in Node-Red for the LORAWAN application used by your paxocunter device. Time can also be set without precision liability, by simple remote command, see section remote control.

## Wall clock controller

Paxcounter can be used to sync a wall clock which has a DCF77 or IF482 time telegram input. Set `#define HAS_IF482` or `#define HAS_DCF77` in board's hal file to setup clock controller. Use case of this function is to integrate paxcounter and clock. Accurary of the synthetic DCF77 signal depends on accuracy of on board's time base, see above.

## Mobile PaxCounter using <A HREF="https://opensensemap.org/">openSenseMap</A>

This describes how to set up a mobile PaxCounter:<br> Follow all steps so far for preparing the device, selecting the packed payload format. In `paxcounter.conf` set `PAYLOAD_OPENSENSEBOX` to `1`.

```c linenums="60" title="src/paxcounter_orig.conf"
--8<-- "src/paxcounter_orig.conf:60:60"
```

 Register a new sensebox on [https://opensensemap.org/](https://opensensemap.org). In the sensor configuration select "TheThingsNetwork" and set decoding profile to "LoRa serialization". Enter your TTN Application and Device ID. Setup decoding option using:

```json
 [{"decoder":"latLng"},{"decoder":"uint16",sensor_id":"yoursensorid"}]
```

## SD-card

Data can be stored on SD-card if the board provides an SD card interface, either with SPI or MMC mode. To enable this feature, specify interface mode and hardware pins in board's hal file (`src/hal/<board.h\>`):

```c
    #define HAS_SDCARD 1     // SD-card interface, using SPI mode
	//OR
	#define HAS_SDCARD 2     // SD-card interface, using MMC mode

    // Pins for SPI interface
    #define SDCARD_CS   (13) // fill in the correct numbers for your board
    #define SDCARD_MOSI (15)
    #define SDCARD_MISO (2)
    #define SDCARD_SCLK (14)
```

This is an example of a board with MMC SD-card interface: [https://www.aliexpress.com/item/32915894264.html](https://www.aliexpress.com/item/32915894264.html). For this board use file [`src/hal/ttgov21new.h`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/hal/ttgov21new.h) and add the lines given above.

Another approach would be this tiny board: [https://www.aliexpress.com/item/32424558182.html](https://www.aliexpress.com/item/32424558182.html) (needs 5V).
In this case you choose the correct file for your ESP32-board in the src/hal-directory and add the lines given above. Edit the pin numbers given in the example, according to your wiring.

Data is written on SD-card to a single file. After 3 write operations the data is flushed to the disk to minimize flash write cycles. Thus, up to the last 3 records of data will get lost when the Paxcounter looses power during operation.

Format of the resulting file is CSV, thus easy import in LibreOffice, Excel, InfluxDB, etc. Each record contains timestamp (in ISO8601 format), paxcount (wifi and ble) and battery voltage (optional). Voltage is logged if the device has a battery voltage sensor (to be configured in board hal file).

File contents example:
```csv
	timestamp,wifi,ble[,voltage]
	2022-01-30T21:12:41Z,11,25[,4100]
	2022-01-30T21:14:24Z,10,21[,4070]
	2022-01-30T21:16:08Z,12,26[,4102]
	2022-01-30T21:17:52Z,11,26[,4076]
```
If you want to change this, modify `src/sdcard.cpp` and `include/sdcard.h`.

Additionally, it's possible to redirect system console output to a plain text file on SD card. This can be useful for debugging headless devices in the field. In `paxcounter.conf` set `SDLOGGING` to `1`.

```c linenums="16" title="src/paxcounter_orig.conf"
--8<-- "src/paxcounter_orig.conf:16:16"
```