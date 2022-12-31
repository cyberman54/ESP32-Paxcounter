# Hardware

### Supported ESP32 based boards:

*With LoRa radio data transfer*:

- **LilyGo: [Paxcounter-Board*](https://www.aliexpress.com/item/32915894264.html?spm=a2g0o.productlist.0.0.3d656325QrcfQc&algo_pvid=4a150199-63e7-4d21-bdb1-b48164537744&algo_exp_id=4a150199-63e7-4d21-bdb1-b48164537744-2&pdp_ext_f=%7B%22sku_id%22%3A%2212000023374441919%22%7D)**
- TTGO: T1*, T2*, T3*, T-Beam, T-Fox
- Heltec: LoRa-32 v1 and v2
- Pycom: LoPy, LoPy4, FiPy
- Radioshuttle.de: [ECO Power Board](https://www.radioshuttle.de/esp32-eco-power/esp32-eco-power-board/)
- WeMos: LoLin32 + [LoraNode32 shield](https://github.com/hallard/LoLin32-Lora),
LoLin32lite + [LoraNode32-Lite shield](https://github.com/hallard/LoLin32-Lite-Lora)
- Adafruit ESP32 Feather + LoRa Wing + OLED Wing, #IoT Octopus32 (Octopus + ESP32 Feather)
- M5Stack: [Basic Core IoT*](https://m5stack.com/collections/m5-core/products/basic-core-iot-development-kit) + [Lora Module RA-01H](https://m5stack.com/collections/m5-module/products/lora-module-868mhz), [Fire IoT*](https://m5stack.com/collections/m5-core/products/fire-iot-development-kit)

*Without LoRa*:

- LilyGo: [T-Dongle S3*](https://github.com/Xinyuan-LilyGO/T-Dongle-S3)
- Pyom: WiPy
- WeMos: LoLin32, LoLin32 Lite, WeMos D32, [Wemos32 Oled](https://www.instructables.com/id/ESP32-With-Integrated-OLED-WEMOSLolin-Getting-Star/)
- Crowdsupply: [TinyPICO](https://www.crowdsupply.com/unexpected-maker/tinypico)
- TTGO: [T-Display](https://www.aliexpress.com/item/33048962331.html)
- TTGO: [T-Wristband](https://www.aliexpress.com/item/4000527495064.html)
- Generic ESP32

*) supports microSD/TF-card for local logging of paxcounter data

Depending on board hardware following features are supported:

- LoRaWAN communication, supporting various payload formats (see enclosed .js converters)
- MQTT communication via TCP/IP and Ethernet interface (note: payload transmitted over MQTT will be base64 encoded)
- SPI serial communication to a local host
- [LED](display-led.md) (shows power & status)
- [OLED Display](display-led.md) (shows detailed status)
- RGB LED (shows colorized status)
- Button (short press: flip display page / long press: send alarm message)
- Battery voltage monitoring (analog read / AXP192 / IP5306)
- GPS (Generic serial NMEA, or Quectel L76 I2C)
- Environmental sensors (Bosch BMP180/BME280/BME680 I2C; SDS011 serial)
- Real Time Clock (Maxim DS3231 I2C)
- IF482 (serial) and DCF77 (gpio) time telegram generator
- Switch external power / battery
- LED Matrix display (similar to [this 64x16 model](https://www.instructables.com/id/64x16-RED-LED-Marquee/), can be ordered on [Aliexpress](https://www.aliexpress.com/item/P3-75-dot-matrix-led-module-3-75mm-high-clear-top1-for-text-display-304-60mm/32616683948.html))
- SD-card (see section SD-card here) for logging pax data

Target platform must be selected in `platformio.ini`.<br>
Hardware dependent settings (pinout etc.) are stored in board files in [/hal](https://github.com/cyberman54/ESP32-Paxcounter/tree/master/src/hal) directory. If you want to use a ESP32 board which is not yet supported, use hal file generic.h and tailor pin mappings to your needs. Pull requests for new boards welcome.<br>

### 3D printed cases
Some 3D printable cases can be found (and, if wanted so, ordered) on Thingiverse, see

- <A HREF="https://www.thingiverse.com/thing:2670713">Heltec</A>
- <A HREF="https://www.thingiverse.com/thing:2811127">TTGOv2</A>
- <A HREF="https://www.thingiverse.com/thing:3005574">TTGOv2.1</A>
- <A HREF="https://www.thingiverse.com/thing:3385109">TTGO</A>
- <A HREF="https://www.thingiverse.com/thing:3041339">T-BEAM</A>
- <A HREF="https://www.thingiverse.com/thing:3203177">T-BEAM parts</A>


### Power consumption

<b>Power consumption</b> was metered at around 450 - 1000mW, depending on board and user settings in `paxcounter.conf`.

By default, bluetooth sniffing is not started. If you enable bluetooth be aware that this goes on expense of wifi sniffing results, because then wifi and bt stack must share the 2,4 GHz RF ressources of ESP32. If you need to sniff wifi and bt in parallel and need best possible results, use two boards - one for wifi only and one for bt only - and add counted results.

=== "Deactivate BLE sniffing (Default)"

    ``` c linenums="29" title="paxcounter.conf"
    #define BLECOUNTER 0
    ```
=== "Activate BLE sniffing"

    ``` c linenums="29" title="paxcounter.conf"
    #define BLECOUNTER 1
    ```