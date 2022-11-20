# Display
<!-- TODO update image -->
![Display Image](img/Paxcounter-Screen.png)

If you're using a device with OLED display, or if you add such one to the I2C bus, the device shows live data on the display. You can flip display pages showing

- recent count of pax
- histogram
- GPS data
- BME sensor data
- time of day
- blank page

by pressing the button of the device.

# LED blink pattern

**Mono color LED:**

- Single Flash (50ms): seen a new Wifi or BLE device
- Quick blink (20ms on each 1/5 second): joining LoRaWAN network in progress or pending
- Small blink (10ms on each 1/2 second): LoRaWAN data transmit in progress or pending
- Long blink (200ms on each 2 seconds): LoRaWAN stack error

**RGB LED:**

- Green: seen a new Wifi device
- Magenta: seen a new BLE device
- Yellow: joining LoRaWAN network in progress or pending
- Pink: LORAWAN MAC transmit in progress
- Blue: LoRaWAN data transmit in progress or pending
- Red: LoRaWAN stack errors