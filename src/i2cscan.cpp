// Basic config
#include "globals.h"
#include "i2cscan.h"

// Local logging tag
static const char TAG[] = __FILE__;

int i2c_scan(void) {

  int i2c_ret, addr;
  int devices = 0;

  ESP_LOGI(TAG, "Starting I2C bus scan...");

  for (addr = 8; addr <= 119; addr++) {

// scan i2c bus with no more to 100KHz
#ifdef HAS_DISPLAY
    Wire.begin(MY_OLED_SDA, MY_OLED_SCL, 100000);
#else
    Wire.begin(SDA, SCL, 100000);
#endif
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

  return devices;
}