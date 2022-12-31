# Integration LoRaWAN

## "The Things Stack Community Edition" aka "The Things Stack V3"

To use the ESP32-Paxcounter in The Things Stack Community Edition you need an account to reach the console. Go to:

- [The Things Stack Community Edition Console](https://console.cloud.thethings.network/)
- choose your region and go to applications
- create an application by clicking "**+ Add application**" and give it a id, name, etc.
- create a device by clicking "**+ Add end device**"
- Select the end device:  choose the Brand "**Open Source Community Projects**" and the Model "**ESP32-Paxcounter**", leave Hardware Version to "**Unknown**" and select your **Firmware Version** and **Profile (Region)**
- Enter registration data: choose the **frequency plan** (for EU choose the recommended), set the **AppEUI** (Fill with zeros), set the **DeviceEUI** (generate), set the **AppKey** (generate), choose a **device ID** and hit "Register end device"
- got to Applications -> "your App ID" -> Payload formatters -> Uplink, choose "**Repository**" and hit "Save changes"

The "Repository" payload decoder uses the packed format, explained below. If you want to use MyDevices from Cayenne you should use the Cayenne payload decoder instead.

## TTN Mapper

If you want your devices to be feeding the [TTN Mapper](https://ttnmapper.org/), just follow this manual: [https://docs.ttnmapper.org/integration/tts-integration-v3.html](https://docs.ttnmapper.org/integration/tts-integration-v3.html) - different than indicated in the manual you can leave the payload decoder to "Repository" for the ESP32-Paxcounter and you are fine.

## ChirpStack

!!! todo