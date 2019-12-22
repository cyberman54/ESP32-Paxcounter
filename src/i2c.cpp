// Basic config
#include "globals.h"
#include "i2c.h"

// Local logging tag
static const char TAG[] = __FILE__;

void i2c_init(void) {
#ifdef HAS_DISPLAY
  Wire.begin(MY_OLED_SDA, MY_OLED_SCL, 400000);
#else
  Wire.begin(SDA, SCL, 400000);
#endif
}

void i2c_deinit(void) {
  Wire.~TwoWire(); // shutdown/power off I2C hardware
#ifdef HAS_DISPLAY
  // to save power, because Wire.end() enables pullups
  pinMode(MY_OLED_SDA, INPUT);
  pinMode(MY_OLED_SCL, INPUT);
#else
  pinMode(SDA, INPUT);
  pinMode(SCL, INPUT);
#endif
}

int i2c_scan(void) {

  int i2c_ret, addr;
  int devices = 0;

  ESP_LOGI(TAG, "Starting I2C bus scan...");

  // block i2c bus access
  if (I2C_MUTEX_LOCK()) {

    // Scan at 100KHz low speed
    Wire.setClock(100000);

    for (addr = 8; addr <= 119; addr++) {

      Wire.beginTransmission(addr);
      Wire.write(addr);
      i2c_ret = Wire.endTransmission();

      if (i2c_ret == 0) {
        devices++;

        switch (addr) {

        case SSD1306_PRIMARY_ADDRESS:
        case SSD1306_SECONDARY_ADDRESS:
          ESP_LOGI(TAG, "0x%X: SSD1306 Display controller", addr);
          break;

        case BME_PRIMARY_ADDRESS:
        case BME_SECONDARY_ADDRESS:
          ESP_LOGI(TAG, "0x%X: Bosch BME MEMS", addr);
          break;

        case AXP192_PRIMARY_ADDRESS:
          ESP_LOGI(TAG, "0x%X: AXP192 power management", addr);
          break;

        case QUECTEL_GPS_PRIMARY_ADDRESS:
          ESP_LOGI(TAG, "0x%X: Quectel GPS", addr);
          break;

        case MCP_24AA02E64_PRIMARY_ADDRESS:
          ESP_LOGI(TAG, "0x%X: 24AA02E64 serial EEPROM", addr);
          break;

        default:
          ESP_LOGI(TAG, "0x%X: Unknown device", addr);
          break;
        }
      } // switch
    }   // for loop

    ESP_LOGI(TAG, "I2C scan done, %u devices found.", devices);

    // Set back to 400KHz
    Wire.setClock(400000);

    I2C_MUTEX_UNLOCK(); // release i2c bus access
  } else
    ESP_LOGE(TAG, "I2c bus busy - scan error");

  return devices;
}

// mutexed functions for i2c r/w access
uint8_t i2c_readBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
  if (I2C_MUTEX_LOCK()) {

    uint8_t ret = 0;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    uint8_t cnt = Wire.requestFrom(addr, (uint8_t)len, (uint8_t)1);
    if (!cnt)
      ret = 0xFF;
    uint16_t index = 0;
    while (Wire.available()) {
      if (index > len) {
        ret = 0xFF;
        goto finish;
      }
      data[index++] = Wire.read();
    }

  finish:
    I2C_MUTEX_UNLOCK(); // release i2c bus access
    return ret;
  } else {
    ESP_LOGW(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
    return 0xFF;
  }
}

uint8_t i2c_writeBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
  if (I2C_MUTEX_LOCK()) {

    uint8_t ret = 0;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    for (uint16_t i = 0; i < len; i++) {
      Wire.write(data[i]);
    }
    ret = Wire.endTransmission();

    I2C_MUTEX_UNLOCK(); // release i2c bus access
    return ret ? ret : 0xFF;
  } else {
    ESP_LOGW(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
    return 0xFF;
  }
}
