# Custom sensors

You can add up to 3 user defined sensors. Insert your sensor's payload scheme in [`sensor.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/sensor.cpp).
The following exampls show how to add a custom temperature and humidty sensor.

1. Add variables or needed libraries
2. Add sensor specific code to `sensor_init` in [`sensor.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/sensor.cpp)
3. Add sensor specific code to `sensor_read` function in [`sensor.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/sensor.cpp)
4. Add Payload functions to [`payload.h`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/include/payload.h) and [`payload.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/payload.cpp) (Optional)
5. Use payload functions in [`sensor.cpp`](https://github.com/cyberman54/ESP32-Paxcounter/blob/master/src/sensor.cpp) to send sensor data

## Example 1: Custom Temperature and Humidity Sensor *GY-21*

To use a custom sensor you first have to enable the Sensor which you want to use. For this you have to edit or add `HAS_SENSOR_1` in either the `paxcounter.conf` or the `hal` file of your board.
=== "Activate Sensor 1"

    ```c linenums="132" title="src/paxcounter_orig.conf"
    #define HAS_SENSOR_1 1
    ```
=== "Activate Sensor 2"

    ```c linenums="132" title="src/paxcounter_orig.conf"
    #define HAS_SENSOR_2 1
    ```
=== "Activate Sensor 3"

    ```c linenums="132" title="src/paxcounter_orig.conf"
    #define HAS_SENSOR_3 1
    ```

You might also add a constant for your custom sensor in the `paxcounter.conf` file. This is optional but can be used to identify the sensor type.

```c linenums="133" title="src/paxcounter_orig.conf"
#define HAS_GY21 1 // (1)
```

1. See usage of this in the example below

### 1. Add variables or needed libraries
If you want to use any libary for you custom sensor you have to add it to the `platformio.ini` file.

In this example we use the [`HTU2xD_SHT2x_Si70xx`](https://github.com/enjoyneering/HTU2xD_SHT2x_Si70xx.git) for the GY-21 sensor.

<!-- FIXME comments did not work for ini file type-->
```yaml linenums="127" title="platformio.ini"
[env:usb] # (1)
upload_protocol = esptool
lib_deps = # (2)
    ${common.lib_deps_all}
    https://github.com/enjoyneering/HTU2xD_SHT2x_Si70xx.git

```

1. Selected the env you want to use. In this example we use the `usb` env.
2. Add the libary to the `lib_deps` section.

Add the import of libary in the `sensor.cpp` file.

```c linenums="5" title="sensor.cpp"
#if (HAS_GY21) // (1)
#include <HTU2xD_SHT2x_Si70xx.h>
HTU2xD_SHT2x_SI70xx ht2x(HTU2xD_SENSOR, HUMD_12BIT_TEMP_14BIT); // sensor type, resolution
double temperature, humidity;
#endif // HAS_GY21
```

1. Define `HAS_GY21` either in hal file of your board or in `paxcounter.conf` file.

### 2. Add sensor specific code to `sensor_init` function


```c linenums="1" title="src/sensor.cpp"
#if (HAS_GY21) // (1)
if (ht2x.begin() != true) // reset sensor, set heater off, set resolution, check power
                          // (sensor doesn't operate correctly if VDD < +2.25v)
{
    ESP_LOGE(TAG, "HTU2xD/SHT2x not connected, fail or VDD < +2.25v");
} else {
    ESP_LOGE(TAG, "HTU2xD/SHT2x/GY21 found");
}
#endif // HAS_GY21
```

1. Define `HAS_GY21` either in hal file of your board or in `paxcounter.conf` file.


### 3. Add sensor specific code to sensor_read function

In this case we choose that our custom sensor is Sensor 3. This means the data will be sent on `SENSOR3PORT` which is by default `12`. You can change this in the `paxcounter.conf` file.

```c linenums="78" title="src/sensor.cpp"
  case 3:
    #if (HAS_GY21)
    ESP_LOGE(TAG, "Reading Sensor 3, GY21"); // (1)
    temperature =
        ht2x.readTemperature(); // accuracy +-0.3C in range 0C..60C at  14-bit
    delay(100);
    humidity =
        ht2x.readHumidity(); // accuracy +-2% in range 20%..80%/25C at 12-bit
    ESP_LOGE(TAG, "GY21: Temperature: %f", temperature); // (2)
    ESP_LOGE(TAG, "GY21: Humidity: %f", humidity); // (3)
    #endif // HAS_GY21
    break;
```

1. These logs are only for debugging. You can remove them if you want.
2. These logs are only for debugging. You can remove them if you want.
3. These logs are only for debugging. You can remove them if you want.

### 4. Payload functions for a custom sensor

If you have added your custom sensor code as described before you can also add custom payload function if you need others than the provided ones. For this you have to change two files. First you have to add your payload function to the `payload.h` file.

===  "Example for a custom temperature / humidity payload function"
    ```c linenums="57" title="src/payload.h"
    void addTempHum(float temperature, float humidity);
    ```

Then you have to add your payload function to the `payload.cpp` file. You can provide functions for all payload formates (see [Payload Formats](../payloadformat.md)) or just add it for the one you are using.

Example for a custom temperature / humidity payload function

=== "Plain payload format"

    ```c linenums="128" title="src/payload.cpp"
    void PayloadConvert::addTempHum(float temperature, float humidity) {
    int16_t temperature = (int16_t)(temperature); // float -> int
    uint16_t humidity = (uint16_t)(humidity);     // float -> int
    buffer[cursor++] = highByte(temperature);
    buffer[cursor++] = lowByte(temperature);
    buffer[cursor++] = highByte(humidity);
    buffer[cursor++] = lowByte(humidity);
    }
    ```

=== "Packed payload format"

    ```c linenums="230" title="src/payload.cpp"
    void PayloadConvert::addTempHum(float temperature, float humidity) {
    writeFloat(temperature);
    writeFloat(humidity);
    }
    ```

=== "Cayenne payload format"

    ```c linenums="488" title="src/payload.cpp"
    void PayloadConvert::addTempHum(float temperature, float humidity) {
    // data value conversions to meet cayenne data type definition
    // 0.1°C per bit => -3276,7 .. +3276,7 °C
    int16_t temp = temperature * 10;
    // 0.5% per bit => 0 .. 128 %C
    uint16_t hum = humidity * 2;
    #if (PAYLOAD_ENCODER == 3)
    buffer[cursor++] = LPP_TEMPERATURE_CHANNEL;
    #endif
    buffer[cursor++] = LPP_TEMPERATURE; // 2 bytes 0.1 °C Signed MSB
    buffer[cursor++] = highByte(temperature);
    buffer[cursor++] = lowByte(temperature);
    #if (PAYLOAD_ENCODER == 3)
    buffer[cursor++] = LPP_HUMIDITY_CHANNEL;
    #endif
    buffer[cursor++] = LPP_HUMIDITY; // 1 byte 0.5 % Unsigned
    buffer[cursor++] = humidity;
    }
    ```


### 5. Sending the data

After you have added your custom sensor code and payload function you can send the data. For this you have to add the following code to the `sensor.cpp` file.

```c linenums="78" title="src/sensor.cpp" hl_lines="11"
  case 3:
    #if (HAS_GY21)
    ESP_LOGE(TAG, "Reading Sensor 3, GY21");
    temperature =
        ht2x.readTemperature(); // accuracy +-0.3C in range 0C..60C at  14-bit
    delay(100);
    humidity =
        ht2x.readHumidity(); // accuracy +-2% in range 20%..80%/25C at 12-bit
    ESP_LOGE(TAG, "GY21: Temperature: %f", temperature);
    ESP_LOGE(TAG, "GY21: Humidity: %f", humidity);
    payload.addTempHum(temperature, humidity); // (1)
    #endif // HAS_GY21
    break;
```

1. Add your custom payload function here.


Now you can build and upload the code to your ESP. Do not forget to erase the flash before uploading since you probably changed the `paxcounter.conf` file.
