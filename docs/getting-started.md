# Preparing

## Install Platformio

Install <A HREF="https://platformio.org/">PlatformIO IDE for embedded development</A> to build this project. Platformio integrates with your favorite IDE, choose e.g. [Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide), Atom, Eclipse etc.

Compile time configuration is spread across several files. Before compiling the code, edit or create the following files:

## platformio.ini
Edit `platformio_orig.ini` (for ESP32 CPU based boards) *or* `platformio_orig_s3.ini` (for ESP32-S3 CPU based boards) and select desired board in section **board**. To add a new board, create an appropriate hardware abstraction layer file in hal subdirectory, and add a pointer to this file in section **board**. Copy or rename to `platformio.ini` in the root directory of the project. Now start Platformio.
### Selecting a board

```ini linenums="6" title="Uncomment your board"
--8<-- "platformio_orig.ini:6:36"
```

=== "Copy"
    ``` bash
    cp platformio_orig.ini platformio.ini
    ```
=== "Rename"
    ``` bash
    mv platformio_orig.ini platformio.ini
    ```

??? info "platformio_orig_s3.ini"
    === "Copy"
        ``` bash
        cp platformio_orig_s3.ini platformio.ini
        ```
    === "Rename"
        ``` bash
        mv platformio_orig_s3.ini platformio.ini
        ```


!!! info

    Platformio is looking for `platformio.ini` in the root directory and won't start if it does not find this file!



## paxcounter.conf
Edit `src/paxcounter_orig.conf` and tailor settings in this file according to your needs and use case. Please take care of the duty cycle regulations of the LoRaWAN network you're going to use. Copy or rename to `src/paxcounter.conf`.

=== "Copy"
    ``` bash
    cp src/paxcounter_orig.conf src/paxcounter.conf
    ```
=== "Rename"
    ``` bash
    mv src/paxcounter_orig.conf src/paxcounter.conf
    ```




If your device has a **real time clock** it can be updated by either LoRaWAN network or GPS time, according to settings *TIME_SYNC_INTERVAL* and *TIME_SYNC_LORAWAN* in `paxcounter.conf`.

```c linenums="85" title="paxcounter.conf"
--8<-- "src/paxcounter_orig.conf:85:85"
```

## src/lmic_config.h
Edit `src/lmic_config.h` and tailor settings in this file according to your country and device hardware. Please take care of national regulations when selecting the frequency band for LoRaWAN.

```c linenums="9" title="national regulations in src/lmic_config.h "
--8<-- "src/lmic_config.h:9:18"
```

## src/loraconf.h
Create file `src/loraconf.h` using the template [src/loraconf_sample.h](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/loraconf_sample.h) and adjust settings to use your personal values. To join the network and activate your paxcounter, you must configure either OTAA or ABP join method. You should use OTAA, whenever possible. To understand the differences of the two methods, [this article](https://www.thethingsnetwork.org/docs/devices/registration.html) may be useful.

=== "Copy"
    ``` bash
    cp src/loraconf_sample.h src/loraconf.h
    ```
=== "Rename"
    ``` bash
    mv src/loraconf_sample.h src/loraconf.h
    ```


To configure OTAA, leave `#define LORA_ABP` deactivated (commented). To use ABP, activate (uncomment) `#define LORA_ABP` in the file `src/loraconf.h`.
The file `src/loraconf_sample.h` contains more information about the values to provide.

=== "Activate OTAA (Default), Deactivate ABP"
    ``` c linenums="18" title="src/loraconf.h"
    --8<-- "src/loraconf_sample.h:18:18"
    ```
=== "Deactivate OTAA, Activate ABP"
    ``` c linenums="18" title="src/loraconf.h"
    #define LORA_ABP
    ```


## src/ota.conf
Create file `src/ota.conf` using the template [src/ota_sample.conf](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/ota_sample.conf) and enter your WIFI network & key. These settings are used for downloading updates via WiFi, either from a remote https server, or locally via WebUI. If you want to use a remote server, you need a <A HREF="https://github.com/paxexpress/docs">PAX.express repository</A>. Enter your PAX.express credentials in ota.conf. If you don't need wireless firmware updates just rename ota.sample.conf to ota.conf.

=== "Copy"
    ``` bash
    cp src/ota_sample.conf src/ota.conf
    ```
=== "Rename"
    ``` bash
    mv src/ota_sample.conf src/ota.conf
    ```
# Building

Use <A HREF="https://platformio.org/">PlatformIO</A> with your preferred IDE for development and building this code. Make sure you have latest PlatformIO version.

# Uploading

!!! warning

     1. After editing `paxcounter.conf`, use "clean" button before "build" in PlatformIO!
     2. Clear NVRAM of the board to delete previous stored runtime settings (`pio run -t erase`)

- **by cable, via USB/UART interface:**
To upload the code via cable to your ESP32 board this needs to be switched from run to bootloader mode. Boards with USB bridge like Heltec and TTGO usually have an onboard logic which allows soft switching by the upload tool. In PlatformIO this happenes automatically.<p>
The LoPy/LoPy4/FiPy board needs to be set manually. See these
<A HREF="https://www.thethingsnetwork.org/labs/story/program-your-lopy-from-the-arduino-ide-using-lmic">instructions</A> how to do it. Don't forget to press on board reset button after switching between run and bootloader mode.<p>
The original Pycom firmware is not needed, so there is no need to update it before flashing Paxcounter. Just flash the compiled paxcounter binary (.elf file) on your LoPy/LoPy4/FiPy. If you later want to go back to the Pycom firmware, download the firmware from Pycom and flash it over.

- **over the air (OTA), download via WiFi:**
After the ESP32 board is initially flashed and has joined a LoRaWAN network, the firmware can update itself by OTA. This process is kicked off by sending a remote control command (see below) via LoRaWAN to the board. The board then tries to connect via WiFi to a cloud service (<A HREF="https://github.com/paxexpress">PAX.express</A>), checks for update, and if available downloads the binary and reboots with it. If something goes wrong during this process, the board reboots back to the current version. Prerequisites for OTA are: 1. You own a PAX.express repository, 2. you pushed the update binary to your PAX.express repository, 3. internet access via encrypted (WPA2) WiFi is present at the board's site, 4. WiFi credentials were set in ota.conf and initially flashed to the board. Step 2 runs automated, just enter the credentials in ota.conf and set `upload_protocol = custom` in platformio.ini. Then press build and lean back watching platformio doing build and upload.

- **over the air (OTA), upload via WiFi:**
If option *BOOTMENU* is defined in `paxcounter.conf`, the ESP32 board will try to connect to a known WiFi access point each time cold starting (after a power cycle or a reset), using the WiFi credentials given in `ota.conf`. Once connected to the WiFi it will fire up a simple webserver, providing a bootstrap menu waiting for a user interaction (pressing "START" button in menu). This process will time out after *BOOTDELAY* seconds, ensuring booting the device to runmode. Once a user interaction in bootstrap menu was detected, the timeout will be extended to *BOOTTIMEOUT* seconds. During this time a firmware upload can be performed manually by user, e.g. using a smartphone in tethering mode providing the firmware upload file.
