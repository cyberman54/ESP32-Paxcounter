#include "power.h"



#ifdef HAS_PMU


/*
On board power management by AXP192 PMU chip:

DC1         0.7V-3.5V /1.2A     
DC2         0.7-2.275V /1.6A
DC3         0.7-3.5V /0.7A      
LDO1(VRTC)  3.3V /30mA          
LDO2        1.8V-3.3V /200mA    
LDO3        1.8-3.3V /200mA     
LDO5/IO0    1.8-3.3V /50mA

*/

#include <XPowersLib.h>
#if defined(XPOWERS_CHIP_AXP192)

void AXP192_powerevent_IRQ(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP192) {
        XPowersAXP192* axp192 = static_cast<XPowersAXP192*>(pmu);

        axp192->getIrqStatus();

        if (axp192->isVbusOverVoltageIrq())
            ESP_LOGI(TAG, "USB voltage %.2fV too high.", axp192->getVbusVoltage() / 1000);
        if (axp192->isVbusInsertIrq())
            ESP_LOGI(TAG, "USB plugged, %.2fV @ %.0mA", axp192->getVbusVoltage() / 1000,
                    axp192->getVbusCurrent());
        if (axp192->isVbusRemoveIrq())
            ESP_LOGI(TAG, "USB unplugged.");
        if (axp192->isBatInsertIrq())
            ESP_LOGI(TAG, "Battery is connected.");
        if (axp192->isBatRemoveIrq())
            ESP_LOGI(TAG, "Battery was removed.");
        if (axp192->isBatChagerStartIrq())
            ESP_LOGI(TAG, "Battery charging started.");
        if (axp192->isBatChagerDoneIrq())
            ESP_LOGI(TAG, "Battery charging done.");
        if (axp192->isBattTempLowIrq())
            ESP_LOGI(TAG, "Battery high temperature.");
        if (axp192->isBattTempHighIrq())
            ESP_LOGI(TAG, "Battery low temperature.");

        // PEK button handling:
        // long press -> shutdown power, must be exited by another longpress
        if (axp192->isPekeyLongPressIrq())
            AXP192_power(pmu_power_off); // switch off Lora, GPS, display
        #ifdef HAS_BUTTON
        // short press -> esp32 deep sleep mode, must be exited by user button
        if (axp192->isPekeyShortPressIrq())
            enter_deepsleep(0UL, HAS_BUTTON);
        #endif

        axp192->clearIrqStatus();

        // refresh stored voltage value
        read_battlevel();
    }
  }
}

void AXP192_power(pmu_power_t powerlevel) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP192) {
        XPowersAXP192* axp192 = static_cast<XPowersAXP192*>(pmu);

        switch (powerlevel) {
        case pmu_power_off:
            axp192->setChargingLedMode(XPOWERS_CHG_LED_OFF);
            axp192->shutdown();
            break;
        case pmu_power_sleep:
            axp192->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
            // we don't cut off DCDC1, because OLED display will then block i2c bus
            // axp192->disableDC1(); // OLED off
            axp192->disableLDO3(); // gps off
            axp192->disableLDO2(); // lora off
            axp192->enableSleep();
            break;
        case pmu_power_on:
        default:
            axp192->enableLDO2(); // Lora on T-Beam V1.0/1.1
            axp192->enableLDO3(); // Gps on T-Beam V1.0/1.1
            axp192->enableDC1();  // OLED on T-Beam v1.0/1.1
            axp192->setChargingLedMode(XPOWERS_CHG_LED_ON);
            break;
        }
    }
  }
}

void AXP192_showstatus(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP192) {
        XPowersAXP192* axp192 = static_cast<XPowersAXP192*>(pmu);

        if (axp192->isBatteryConnect())
            if (axp192->isCharging())
            ESP_LOGI(TAG, "Battery charging, %.2fV @ %.0fmAh",
                    axp192->getBattVoltage() / 1000.0, axp192->getBatteryChargeCurrent());
            else
            ESP_LOGI(TAG, "Battery not charging");
        else
            ESP_LOGI(TAG, "Battery not present");

        if (axp192->isVbusIn())
            ESP_LOGI(TAG, "USB powered, %.0fmW",
                    axp192->getVbusVoltage() / 1000 * axp192->getVbusCurrent());
        else
            ESP_LOGI(TAG, "USB not present");
    }
  }
}

void AXP192_init(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP192) {
        XPowersAXP192* axp192 = static_cast<XPowersAXP192*>(pmu);

        ESP_LOGD(TAG, "AXP192 ChipID:0x%x", axp192->getChipID());

        // set pmu operating voltages
        axp192->setSysPowerDownVoltage(2700);
        axp192->setVbusVoltageLimit(XPOWERS_AXP192_VBUS_VOL_LIM_4V5);
        axp192->setVbusCurrentLimit(XPOWERS_AXP192_VBUS_CUR_LIM_OFF);

        // set device operating voltages
        axp192->setDC1Voltage(3300);  // for external OLED display
        axp192->setLDO2Voltage(3300); // LORA VDD 3v3
        axp192->setLDO3Voltage(3300); // GPS VDD 3v3

        // configure PEK button settings
        axp192->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
        axp192->setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

        // set battery temperature sensing pin off to save power
        axp192->disableTSPinMeasure();

        // Enable internal ADC detection
        axp192->enableBattDetection();
        axp192->enableVbusVoltageMeasure();
        axp192->enableBattVoltageMeasure();
        axp192->enableSystemVoltageMeasure();

        #ifdef PMU_INT
        pinMode(PMU_INT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PMU_INT), PMUIRQ, FALLING);
        // disable all interrupts
        axp192->disableIRQ(XPOWERS_AXP192_ALL_IRQ);
        // clear all interrupt flags
        axp192->clearIrqStatus();
        // enable the required interrupt function
        axp192->enableIRQ(XPOWERS_AXP192_BAT_INSERT_IRQ |
                        XPOWERS_AXP192_BAT_REMOVE_IRQ | // BATTERY
                        XPOWERS_AXP192_VBUS_INSERT_IRQ |
                        XPOWERS_AXP192_VBUS_REMOVE_IRQ | // VBUS
                        XPOWERS_AXP192_PKEY_SHORT_IRQ |
                        XPOWERS_AXP192_PKEY_LONG_IRQ | // POWER KEY
                        XPOWERS_AXP192_BAT_CHG_DONE_IRQ |
                        XPOWERS_AXP192_BAT_CHG_START_IRQ // CHARGE
        );
        #endif // PMU_INT

        // set charging parameters according to user settings if we have (see power.h)
        #ifdef PMU_CHG_CURRENT
        axp192->setChargerConstantCurr(PMU_CHG_CURRENT);
        axp192->setChargeTargetVoltage(PMU_CHG_CUTOFF);
        axp192->enableCharge();
        #endif

        // switch power rails on
        AXP192_power(pmu_power_on);
        ESP_LOGI(TAG, "AXP192 PMU initialized");
    }
  }
}

float AXP192_getBatteryChargeCurrent()
{
  float res{};
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP192) {
        XPowersAXP192* axp192 = static_cast<XPowersAXP192*>(pmu);
        if (axp192 != nullptr)
            res = axp192->getBatteryChargeCurrent();
    }
  }
  return res;
}

float AXP192_getBattDischargeCurrent()
{
  float res{};
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP192) {
        XPowersAXP192* axp192 = static_cast<XPowersAXP192*>(pmu);
        if (axp192 != nullptr)
            res = axp192->getBattDischargeCurrent();
    }
  }
  return res;
}

float AXP192_getVbusCurrent()
{
  float res{};
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP192) {
        XPowersAXP192* axp192 = static_cast<XPowersAXP192*>(pmu);
        if (axp192 != nullptr)
            res = axp192->getVbusCurrent();
    }
  }
  return res;
}



#endif
#endif // HAS_PMU