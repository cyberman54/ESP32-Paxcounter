# ESP32-Paxcounter
![logo](docs/assets/paxcounter_logo_white.png)

**Wifi & Bluetooth driven, LoRaWAN enabled, battery powered mini Paxcounter built on cheap ESP32 LoRa IoT boards**

[Tutorial (in german language): heise.de](https://www.heise.de/select/make/2019/1/1551099236518668)

[![CodeFactor](https://www.codefactor.io/repository/github/cyberman54/esp32-paxcounter/badge)](https://www.codefactor.io/repository/github/cyberman54/esp32-paxcounter)
[![PlatformIO CI](https://github.com/cyberman54/ESP32-Paxcounter/actions/workflows/build.yml/badge.svg?event=push)](https://github.com/cyberman54/ESP32-Paxcounter/actions/workflows/build.yml)

---

**Documentation**: <a href="https://cyberman54.github.io/ESP32-Paxcounter" target="_blank">https://cyberman54.github.io/ESP32-Paxcounter</a>

**Source Code**: <a href="https://github.com/cyberman54/ESP32-Paxcounter" target="_blank">https://github.com/cyberman54/ESP32-Paxcounter</a>

---

<img src="docs/img/Paxcounter-title.jpg">
<img src="docs/img/Paxcounter-ttgo.jpg">
<img src="docs/img/Paxcounter-lolin.gif">
<img src="docs/img/Paxcounter-Screen.png">
<img src="docs/img/TTGO-case.jpg">
<img src="docs/img/TTGO-curves.jpg">
<img src="docs/img/Paxcounter-LEDmatrix.jpg">
<img src="docs/img/Paxcounter-Clock2.png">
<img src="docs/img/Paxcounter-ttgo-twristband.jpg">

# Use case

Paxcounter is an [ESP32](https://www.espressif.com/en/products/socs/esp32) MCU based device for metering passenger flows in realtime. It counts how many mobile devices are around. This gives an estimation how many people are around. Paxcounter detects Wifi and Bluetooth signals in the air, focusing on mobile devices by evaluating their MAC adresses.

Intention of this project is to do this without intrusion in privacy: You don't need to track people owned devices, if you just want to count them. Therefore, Paxcounter does not persistenly store MAC adresses and does no kind of fingerprinting the scanned devices.

Data can either be stored on a local SD-card, transferred to cloud using LoRaWAN network (e.g. TheThingsNetwork or Helium) or MQTT over TCP/IP, or transmitted to a local host using serial (SPI) interface.

You can build this project battery powered using ESP32 deep sleep mode and reach long uptimes with a single 18650 Li-Ion cell.

# License

Copyright 2018-2022 Oliver Brandmueller <ob@sysadm.in>

Copyright 2018-2022 Klaus Wilting <verkehrsrot@arcor.de>

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
- [Caspar Armster](https://www.dasdigidings.de/) for the The Things Stack V3 payload converter
- [terrillmoore](https://github.com/mcci-catena) for maintaining the LMIC for arduino LoRaWAN stack
- [sbamueller](https://github.com/sbamueller) for writing the tutorial in Make Magazine
- [Stefan](https://github.com/nerdyscout) for paxcounter opensensebox integration
- [August Quint](https://github.com/AugustQu) for adding SD card data logger and SDS011 support
- [t-huyeng](https://github.com/t-huyeng) for adding a CI workflow and rework documentation
