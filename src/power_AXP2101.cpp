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

        pmu->getIrqStatus();

        if (pmu->isVbusInsertIrq())
            ESP_LOGI(TAG, "USB plugged, %.2fV", pmu->getVbusVoltage() / 1000);
        if (pmu->isVbusRemoveIrq())
            ESP_LOGI(TAG, "USB unplugged.");
        if (pmu->isBatInsertIrq())
            ESP_LOGI(TAG, "Battery is connected.");
        if (pmu->isBatRemoveIrq())
            ESP_LOGI(TAG, "Battery was removed.");
        if (pmu->isBatChagerStartIrq())
            ESP_LOGI(TAG, "Battery charging started.");
        if (pmu->isBatChagerDoneIrq())
            ESP_LOGI(TAG, "Battery charging done.");

        // PEK button handling:
        // long press -> shutdown power, must be exited by another longpress
        if (pmu->isPekeyLongPressIrq())
            AXP2101_power(pmu_power_off); // switch off Lora, GPS, display
        #ifdef HAS_BUTTON
        // short press -> esp32 deep sleep mode, must be exited by user button
        if (pmu->isPekeyShortPressIrq())
            enter_deepsleep(0UL, HAS_BUTTON);
        #endif

        pmu->clearIrqStatus();

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
            pmu->setChargingLedMode(XPOWERS_CHG_LED_OFF);
            pmu->shutdown();
            break;
        case pmu_power_sleep:
            pmu->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
            pmu->disablePowerOutput(XPOWERS_DCDC3); // oled off
            pmu->disablePowerOutput(XPOWERS_ALDO4); // gps off
            pmu->disablePowerOutput(XPOWERS_VBACKUP); // charge GPS backup battery off
            pmu->disablePowerOutput(XPOWERS_ALDO3); // lora off
            pmu->enableSleep();
            break;
        case pmu_power_on:
        default:
            pmu->enablePowerOutput(XPOWERS_DCDC3); // oled on
            pmu->enablePowerOutput(XPOWERS_ALDO4); // gps on
            pmu->enablePowerOutput(XPOWERS_VBACKUP); // charge GPS backup battery
            pmu->enablePowerOutput(XPOWERS_ALDO3); // lora on
            pmu->setChargingLedMode(XPOWERS_CHG_LED_ON);
            break;
        }
    }
  }
}

void AXP2101_showstatus(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP2101) {
        XPowersAXP2101* axp2101 = static_cast<XPowersAXP2101*>(pmu);

        if (pmu->isBatteryConnect())
            if (pmu->isCharging())
            ESP_LOGI(TAG, "Battery charging, %.2fV",
                    pmu->getBattVoltage() / 1000.0);
            else
            ESP_LOGI(TAG, "Battery not charging");
        else
            ESP_LOGI(TAG, "Battery not present");

        if (pmu->isVbusIn())
            ESP_LOGI(TAG, "USB powered, %.0fmV",
                    pmu->getVbusVoltage());
        else
            ESP_LOGI(TAG, "USB not present");
    }
  }
}

void AXP2101_init(void) {
  if (pmu != nullptr) {
    if (pmu->getChipModel() == XPOWERS_AXP2101) {
        XPowersAXP2101* axp2101 = static_cast<XPowersAXP2101*>(pmu);

        ESP_LOGD(TAG, "AXP2101 ChipID:0x%x", pmu->getChipID());

        #ifdef _TTGOBEAM_1_2_H
        // Settings for Lilygo T-Beam V1.2 (yet untested!)

        // gnss
        pmu->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO3);
        // lora
        pmu->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO2);
        // oled
        pmu->setPowerChannelVoltage(XPOWERS_DCDC3, 3300);
        pmu->enablePowerOutput(XPOWERS_DCDC3);
        // gnss backup battery
        pmu->setPowerChannelVoltage(XPOWERS_VBACKUP, 3300);
        pmu->enablePowerOutput(XPOWERS_VBACKUP); // charge GPS backup battery

        #endif // T-Beam v1.2

        #ifdef _TTGOTSUPREMES3_H 
        // Settings for Lilygo T-Beam Supreme S3

        // gnss
        pmu->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO4);

        // gnss backup battery
        pmu->setPowerChannelVoltage(XPOWERS_VBACKUP, 3300);
        pmu->enablePowerOutput(XPOWERS_VBACKUP); // charge GPS backup battery
        // lora
        pmu->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO3);
        // oled
        pmu->setPowerChannelVoltage(XPOWERS_DCDC3, 3300);
        pmu->enablePowerOutput(XPOWERS_DCDC3);
        /**
         * ALDO2 cannot be turned off.
         * It is a necessary condition for sensor communication.
         * It must be turned on to properly access the sensor and screen
         * It is also responsible for the power supply of PCF8563
         */
        pmu->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO2);
        // 6-axis, magnetomete, bme280, oled screen power channel
        pmu->setPowerChannelVoltage(XPOWERS_ALDO1, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO1);
        // sdcard power channel
        pmu->setPowerChannelVoltage(XPOWERS_BLDO1, 3300);
        pmu->enablePowerOutput(XPOWERS_BLDO1);
        #endif // TTGO Supreme

        // set pmu operating voltages
        pmu->setSysPowerDownVoltage(2700);
        pmu->setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1000MA);

        // configure PEK button settings
        pmu->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
        pmu->setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

        // set battery temperature sensing pin off to save power
        pmu->disableTSPinMeasure();

        // Enable internal ADC detection
        pmu->enableBattDetection();
        pmu->enableVbusVoltageMeasure();
        pmu->enableBattVoltageMeasure();
        pmu->enableSystemVoltageMeasure();

        #ifdef PMU_INT
        pinMode(PMU_INT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PMU_INT), PMUIRQ, FALLING);
        // disable all interrupts
        pmu->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
        // clear all interrupt flags
        pmu->clearIrqStatus();
        // enable the required interrupt function
        pmu->enableIRQ(XPOWERS_AXP2101_BAT_INSERT_IRQ |
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
        pmu->setChargerConstantCurr(PMU_CHG_CURRENT);
        pmu->setChargeTargetVoltage(PMU_CHG_CUTOFF);
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