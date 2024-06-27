#include "power.h"



#ifdef HAS_PMU


/*

On board power management by AXP2101 PMU chip:

DC1        1.5-3.4V /2A                      
DC2        0.5-1.2V,1.22-1.54V /2A
DC3        0.5-1.2V,1.22-1.54V,1.6-3.4V /2A  
DC4        0.5-1.2V,1.22-1.84V /1.5A
DC5        1.2V,1.4-3.7V /1A
LDO1(VRTC) 1.8V /30mA                        

ALDO1      0.5-3.5V /300mA
ALDO2      0.5-3.5V /300mA                   
ALDO3      0.5-3.5V /300mA                   
ALDO4      0.5-3.5V /300mA
BLDO1      0.5-3.5V /300mA
BLDO2      0.5-3.5V /300mA
DLDO1      0.5-3.3V/300mA (LDO Mode) (Dependent on DC1)
DC1SW      0.5-3.3V/300mA (SW  Mode) (Dependent on DC1)
DLDO2      0.5-1.4V/300mA (LDO Mode) (Dependent on DC4)
DC4SW      0.5-1.4V/300mA (SW  Mode) (Dependent on DC4)
CPUSLDO    0.5-1.4V /30mA            (Dependent on DC4)

N.B.
There are 2 different versions of the AXP2101:
- LDO mode:    DLDO1 / DLDO2
- Switch mode: DC1SW / DC4SW

Note:
    Whether DLDO1/DLDO2/RTCLDO2/GPIO1 can be used depends on the chip.
    It is not available by default.
    RTCLDO1 has a default voltage, which is generally 1.8V by default.
    The voltage value cannot be changed or turned off through the register.

*/

#include <XPowersLib.h>
#if defined(XPOWERS_CHIP_AXP2101)

void AXP2101_powerevent_IRQ(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP2101) {
        XPowersAXP2101* axp2101 = static_cast<XPowersAXP2101*>(pmu);

        axp2101->getIrqStatus();

        if (axp2101->isBatfetOverCurrentIrq())
            ESP_LOGI(TAG, "Battery current too high.");
        if (axp2101->isVbusInsertIrq())
            ESP_LOGI(TAG, "USB plugged, %.2fV", axp2101->getVbusVoltage() / 1000);
        if (axp2101->isVbusRemoveIrq())
            ESP_LOGI(TAG, "USB unplugged.");
        if (axp2101->isBatInsertIrq())
            ESP_LOGI(TAG, "Battery is connected.");
        if (axp2101->isBatRemoveIrq())
            ESP_LOGI(TAG, "Battery was removed.");
        if (axp2101->isBatChagerStartIrq())
            ESP_LOGI(TAG, "Battery charging started.");
        if (axp2101->isBatChagerDoneIrq())
            ESP_LOGI(TAG, "Battery charging done.");
        if (axp2101->isBatWorkUnderTemperatureIrq())
            ESP_LOGI(TAG, "Battery low temperature.");
        if (axp2101->isBatWorkOverTemperatureIrq())
            ESP_LOGI(TAG, "Battery high temperature.");
        if (axp2101->isBatChargerUnderTemperatureIrq())
            ESP_LOGI(TAG, "Battery charge low temperature.");
        if (axp2101->isBatChargerOverTemperatureIrq())
            ESP_LOGI(TAG, "Battery charge hgh temperature.");

        // PEK button handling:
        // long press -> shutdown power, must be exited by another longpress
        if (axp2101->isPekeyLongPressIrq())
            AXP2101_power(pmu_power_off); // switch off Lora, GPS, display
        #ifdef HAS_BUTTON
        // short press -> esp32 deep sleep mode, must be exited by user button
        if (axp2101->isPekeyShortPressIrq())
            enter_deepsleep(0UL, HAS_BUTTON);
        #endif

        axp2101->clearIrqStatus();

        // refresh stored voltage value
        read_battlevel();
    }
  }
}

void AXP2101_power(pmu_power_t powerlevel) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP2101) {
        XPowersAXP2101* axp2101 = static_cast<XPowersAXP2101*>(pmu);

        switch (powerlevel) {
        case pmu_power_off:
            axp2101->setChargingLedMode(XPOWERS_CHG_LED_OFF);
            axp2101->shutdown();
            break;
        case pmu_power_sleep:
            axp2101->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
            // we don't cut off DCDC1, because OLED display will then block i2c bus
            // axp2101->disableDC3(); // OLED off
            axp2101->disableALDO3(); // gps off
            axp2101->disableALDO2(); // lora off
            axp2101->enableSleep();
            break;
        case pmu_power_on:
        default:
            axp2101->enableALDO2(); // Lora on T-Beam V1.2
            axp2101->enableALDO3(); // Gps on T-Beam V1.2
            axp2101->enableDC3();   // OLED on T-Beam v1.2
            axp2101->setChargingLedMode(XPOWERS_CHG_LED_ON);
            break;
        }
    }
  }
}

void AXP2101_showstatus(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP2101) {
        XPowersAXP2101* axp2101 = static_cast<XPowersAXP2101*>(pmu);

        if (axp2101->isBatteryConnect())
            if (axp2101->isCharging())
            ESP_LOGI(TAG, "Battery charging, %.2fV",
                    axp2101->getBattVoltage() / 1000.0);
            else
            ESP_LOGI(TAG, "Battery not charging");
        else
            ESP_LOGI(TAG, "Battery not present");

        if (axp2101->isVbusIn())
            ESP_LOGI(TAG, "USB powered, %.0fmV",
                    axp2101->getVbusVoltage());
        else
            ESP_LOGI(TAG, "USB not present");
    }
  }
}

void AXP2101_init(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP2101) {
        XPowersAXP2101* axp2101 = static_cast<XPowersAXP2101*>(pmu);

        ESP_LOGD(TAG, "AXP2101 ChipID:0x%x", axp2101->getChipID());

        // set pmu operating voltages
        axp2101->setSysPowerDownVoltage(2700);
        axp2101->setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V44);
        axp2101->setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1000MA);

        // set device operating voltages
        axp2101->setDC3Voltage(3300);  // for external OLED display
        axp2101->setALDO2Voltage(3300); // LORA VDD 3v3
        axp2101->setALDO3Voltage(3300); // GPS VDD 3v3

        // configure PEK button settings
        axp2101->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
        axp2101->setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

        // set battery temperature sensing pin off to save power
        axp2101->disableTSPinMeasure();

        // Enable internal ADC detection
        axp2101->enableBattDetection();
        axp2101->enableVbusVoltageMeasure();
        axp2101->enableBattVoltageMeasure();
        axp2101->enableSystemVoltageMeasure();

        #ifdef PMU_INT
        pinMode(PMU_INT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PMU_INT), PMUIRQ, FALLING);
        // disable all interrupts
        axp2101->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
        // clear all interrupt flags
        axp2101->clearIrqStatus();
        // enable the required interrupt function
        axp2101->enableIRQ(XPOWERS_AXP2101_BAT_INSERT_IRQ |
                        XPOWERS_AXP2101_BAT_REMOVE_IRQ | // BATTERY
                        XPOWERS_AXP2101_VBUS_INSERT_IRQ |
                        XPOWERS_AXP2101_VBUS_REMOVE_IRQ | // VBUS
                        XPOWERS_AXP2101_PKEY_SHORT_IRQ |
                        XPOWERS_AXP2101_PKEY_LONG_IRQ | // POWER KEY
                        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ |
                        XPOWERS_AXP2101_BAT_CHG_START_IRQ // CHARGE
        );
        #endif // PMU_INT

        // set charging parameters according to user settings if we have (see power.h)
        #ifdef PMU_CHG_CURRENT
        axp2101->setChargerConstantCurr(PMU_CHG_CURRENT);
        axp2101->setChargeTargetVoltage(PMU_CHG_CUTOFF);
        axp2101->enableCellbatteryCharge();
        #endif

        // switch power rails on
        AXP2101_power(pmu_power_on);
        ESP_LOGI(TAG, "AXP2101 PMU initialized");
    }
  }
}

float AXP2101_getBatteryChargeCurrent()
{
  // Not supported by AXP2101
  float res{};
  return res;
}

float AXP2101_getBattDischargeCurrent()
{
  // Not supported by AXP2101
  float res{};
  return res;
}

float AXP2101_getVbusCurrent()
{
  // Not supported by AXP2101
  float res{};
  return res;
}



#endif
#endif // HAS_PMU