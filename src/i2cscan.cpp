// Basic config
#include "globals.h"
#include "i2cscan.h"

// Local logging tag
static const char TAG[] = __FILE__;

#define SSD1306_PRIMARY_ADDRESS (0x3D)
#define SSD1306_SECONDARY_ADDRESS (0x3C)
#define BME_PRIMARY_ADDRESS (0x77)
#define BME_SECONDARY_ADDRESS (0x76)
#define AXP192_PRIMARY_ADDRESS (0x34)
#define MCP_24AA02E64_PRIMARY_ADDRESS (0x50)

int i2c_scan(void) {

  int i2c_ret, addr;
  int devices = 0;

  ESP_LOGI(TAG, "Starting I2C bus scan...");

  for (addr = 8; addr <= 119; addr++) {

    // scan i2c bus with no more to 100KHz
    Wire.begin(SDA, SCL, 100000);
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
#ifdef HAS_PMU
        AXP192_init();
#endif
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

#ifdef HAS_PMU

void AXP192_init(void) {

  AXP20X_Class axp;

  if (axp.begin(Wire, AXP192_PRIMARY_ADDRESS))
    ESP_LOGI(TAG, "AXP192 PMU initialization failed");
  else {

    axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
    axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
    axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);

    /*
    axp.setChgLEDMode(LED_BLINK_4HZ);
    axp.setDCDC1Voltage(3300);

    pinMode(PMU_IRQ, INPUT_PULLUP);
    attachInterrupt(PMU_IRQ, [] { pmu_irq = true; }, FALLING);

    axp.adc1Enable(AXP202_BATT_CUR_ADC1, 1);
    axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ |
                      AXP202_BATT_REMOVED_IRQ | AXP202_BATT_CONNECT_IRQ,
                  1);
    axp.clearIRQ();
    */

    ESP_LOGI(TAG, "AXP192 PMU initialized.");
  }
#endif // HAS_PMU
}