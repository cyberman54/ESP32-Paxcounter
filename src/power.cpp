// Basic config
#include "globals.h"
#include "power.h"

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
    axp.setDCDC1Voltage(3300);
    axp.setChgLEDMode(AXP20X_LED_BLINK_1HZ);
    // axp.setChgLEDMode(AXP20X_LED_OFF);
    axp.adc1Enable(AXP202_BATT_CUR_ADC1, 1);

#ifdef PMU_INT
    pinMode(PMU_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PMU_INT),
                    [] {
                      ESP_LOGI(TAG, "Power source changed");
                      /* put your code here */
                    },
                    FALLING);
    axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ |
                      AXP202_BATT_REMOVED_IRQ | AXP202_BATT_CONNECT_IRQ,
                  1);
    axp.clearIRQ();
#endif // PMU_INT

    ESP_LOGI(TAG, "AXP192 PMU initialized.");
  }
}
#endif // HAS_PMU