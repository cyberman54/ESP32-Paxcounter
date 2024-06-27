#include "power.h"



#ifdef HAS_PMU


/*
On board power management by AXP202 PMU chip:

DC2         0.7-2.275V /1.6A  
DC3         0.7-3.5V /1.2A    
LDO1(VRTC)  3.3V /30mA        
LDO2        1.8V-3.3V /200mA  
LDO3        0.7-3.5V /200mA   
LDO4        1.8V-3.3V /200mA  
LDO5/IO0    1.8-3.3V /50mA    

*/



#include <XPowersLib.h>
#if defined(XPOWERS_CHIP_AXP202)

void AXP202_powerevent_IRQ(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP202) {
        XPowersAXP202* axp202 = static_cast<XPowersAXP202*>(pmu);

        axp202->getIrqStatus();

        if (axp202->isVbusOverVoltageIrq())
            ESP_LOGI(TAG, "USB voltage %.2fV too high.", axp202->getVbusVoltage() / 1000);
        if (axp202->isVbusInsertIrq())
            ESP_LOGI(TAG, "USB plugged, %.2fV @ %.0mA", axp202->getVbusVoltage() / 1000,
                    axp202->getVbusCurrent());
        if (axp202->isVbusRemoveIrq())
            ESP_LOGI(TAG, "USB unplugged.");
        if (axp202->isBatInsertIrq())
            ESP_LOGI(TAG, "Battery is connected.");
        if (axp202->isBatRemoveIrq())
            ESP_LOGI(TAG, "Battery was removed.");
        if (axp202->isBatChagerStartIrq())
            ESP_LOGI(TAG, "Battery charging started.");
        if (axp202->isBatChagerDoneIrq())
            ESP_LOGI(TAG, "Battery charging done.");
        if (axp202->isBattTempLowIrq())
            ESP_LOGI(TAG, "Battery high temperature.");
        if (axp202->isBattTempHighIrq())
            ESP_LOGI(TAG, "Battery low temperature.");

        // PEK button handling:
        // long press -> shutdown power, must be exited by another longpress
        if (axp202->isPekeyLongPressIrq())
            AXP202_power(pmu_power_off); // switch off Lora, GPS, display
        #ifdef HAS_BUTTON
        // short press -> esp32 deep sleep mode, must be exited by user button
        if (axp202->isPekeyShortPressIrq())
            enter_deepsleep(0UL, HAS_BUTTON);
        #endif

        axp202->clearIrqStatus();

        // refresh stored voltage value
        read_battlevel();
    }
  }
}

void AXP202_power(pmu_power_t powerlevel) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP202) {
        XPowersAXP202* axp202 = static_cast<XPowersAXP202*>(pmu);

        switch (powerlevel) {
        case pmu_power_off:
            axp202->setChargingLedMode(XPOWERS_CHG_LED_OFF);
            axp202->shutdown();
            break;
        case pmu_power_sleep:
            axp202->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
/*            
            axp202->disableLDO3(); // gps off
            axp202->disableLDO2(); // lora off
            axp202->enableSleep();
*/
            break;
        case pmu_power_on:
        default:
/*
            axp202->enableLDO2(); // Lora on T-Beam V1.0/1.1
            axp202->enableLDO3(); // Gps on T-Beam V1.0/1.1
            axp202->enableDC2();  // OLED on T-Beam v1.0/1.1
            axp202->setChargingLedMode(XPOWERS_CHG_LED_ON);
*/
            break;
        }
    }
  }
}

void AXP202_showstatus(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP202) {
        XPowersAXP202* axp202 = static_cast<XPowersAXP202*>(pmu);

        if (axp202->isBatteryConnect())
            if (axp202->isCharging())
            ESP_LOGI(TAG, "Battery charging, %.2fV @ %.0fmAh",
                    axp202->getBattVoltage() / 1000.0, axp202->getBatteryChargeCurrent());
            else
            ESP_LOGI(TAG, "Battery not charging");
        else
            ESP_LOGI(TAG, "Battery not present");

        if (axp202->isVbusIn())
            ESP_LOGI(TAG, "USB powered, %.0fmW",
                    axp202->getVbusVoltage() / 1000 * axp202->getVbusCurrent());
        else
            ESP_LOGI(TAG, "USB not present");
    }
  }
}

void AXP202_init(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP202) {
        XPowersAXP202* axp202 = static_cast<XPowersAXP202*>(pmu);

        ESP_LOGD(TAG, "AXP202 ChipID:0x%x", axp202->getChipID());

        // set pmu operating voltages
        axp202->setSysPowerDownVoltage(2700);
        axp202->setVbusVoltageLimit(XPOWERS_AXP202_VBUS_VOL_LIM_4V5);
        axp202->setVbusCurrentLimit(XPOWERS_AXP202_VBUS_CUR_LIM_OFF);

        // set device operating voltages
//        axp202->setDC2Voltage(3300);  // ???
//        axp202->setLDO2Voltage(3300); // ???
//        axp202->setLDO3Voltage(3300); // ???

        // configure PEK button settings
        axp202->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
        axp202->setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

        // set battery temperature sensing pin off to save power
        axp202->disableTSPinMeasure();

        // Enable internal ADC detection
        axp202->enableBattDetection();
        axp202->enableVbusVoltageMeasure();
        axp202->enableBattVoltageMeasure();
        axp202->enableSystemVoltageMeasure();

        #ifdef PMU_INT
        pinMode(PMU_INT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PMU_INT), PMUIRQ, FALLING);
        // disable all interrupts
        axp202->disableIRQ(XPOWERS_AXP202_ALL_IRQ);
        // clear all interrupt flags
        axp202->clearIrqStatus();
        // enable the required interrupt function
        axp202->enableIRQ(XPOWERS_AXP202_BAT_INSERT_IRQ |
                        XPOWERS_AXP202_BAT_REMOVE_IRQ | // BATTERY
                        XPOWERS_AXP202_VBUS_INSERT_IRQ |
                        XPOWERS_AXP202_VBUS_REMOVE_IRQ | // VBUS
                        XPOWERS_AXP202_PKEY_SHORT_IRQ |
                        XPOWERS_AXP202_PKEY_LONG_IRQ | // POWER KEY
                        XPOWERS_AXP202_BAT_CHG_DONE_IRQ |
                        XPOWERS_AXP202_BAT_CHG_START_IRQ // CHARGE
        );
        #endif // PMU_INT

        // set charging parameters according to user settings if we have (see power.h)
        #ifdef PMU_CHG_CURRENT
        axp202->setChargerConstantCurr(PMU_CHG_CURRENT);
        axp202->setChargeTargetVoltage(PMU_CHG_CUTOFF);
        axp202->enableCharge();
        #endif

        // switch power rails on
        AXP202_power(pmu_power_on);
        ESP_LOGI(TAG, "AXP202 PMU initialized");
    }
  }
}

float AXP202_getBatteryChargeCurrent()
{
  float res{};
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP202) {
        XPowersAXP202* axp202 = static_cast<XPowersAXP202*>(pmu);
        if (axp202 != nullptr)
            res = axp202->getBatteryChargeCurrent();
    }
  }
  return res;
}

float AXP202_getBattDischargeCurrent()
{
  float res{};
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP202) {
        XPowersAXP202* axp202 = static_cast<XPowersAXP202*>(pmu);
        if (axp202 != nullptr)
            res = axp202->getBattDischargeCurrent();
    }
  }
  return res;
}

float AXP202_getVbusCurrent()
{
  float res{};
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP202) {
        XPowersAXP202* axp202 = static_cast<XPowersAXP202*>(pmu);
        if (axp202 != nullptr)
            res = axp202->getVbusCurrent();
    }
  }
  return res;
}



#endif
#endif // HAS_PMU