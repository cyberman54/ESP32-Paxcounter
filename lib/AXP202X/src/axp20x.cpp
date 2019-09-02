/////////////////////////////////////////////////////////////////
/*
MIT License

Copyright (c) 2019 lewis he

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

axp20x.cpp - Arduino library for X-Power AXP202 chip.
Created by Lewis he on April 1, 2019.
github:https://github.com/lewisxhe/AXP202X_Libraries
*/
/////////////////////////////////////////////////////////////////

#include "axp20x.h"
#include <math.h>

const uint8_t AXP20X_Class::startupParams[] = {
    0b00000000,
    0b01000000,
    0b10000000,
    0b11000000
};

const uint8_t AXP20X_Class::longPressParams[] = {
    0b00000000,
    0b00010000,
    0b00100000,
    0b00110000
};

const uint8_t AXP20X_Class::shutdownParams[] = {
    0b00000000,
    0b00000001,
    0b00000010,
    0b00000011
};

const uint8_t AXP20X_Class::targetVolParams[] = {
    0b00000000,
    0b00100000,
    0b01000000,
    0b01100000
};


// Power Output Control register
uint8_t AXP20X_Class::_outputReg;

int AXP20X_Class::begin(TwoWire &port, uint8_t addr)
{
    _i2cPort = &port;   //Grab which port the user wants us to use
    _address = addr;
    _readByte(AXP202_IC_TYPE, 1, &_chip_id);
    AXP_DEBUG("chip id detect 0x%x\n", _chip_id);
    if (_chip_id == AXP202_CHIP_ID || _chip_id == AXP192_CHIP_ID) {
        AXP_DEBUG("Detect CHIP :%s\n", _chip_id == AXP202_CHIP_ID ? "AXP202" : "AXP192");
        _readByte(AXP202_LDO234_DC23_CTL, 1, &_outputReg);
        AXP_DEBUG("OUTPUT Register 0x%x\n", _outputReg);
        _init = true;
    }
    return _init ? AXP_PASS : AXP_FAIL;
}

//Only axp192 chip
bool AXP20X_Class::isDCDC1Enable()
{
    if (_chip_id == AXP192_CHIP_ID)
        return IS_OPEN(_outputReg, AXP192_DCDC1);
    return false;
}
//Only axp192 chip
bool AXP20X_Class::isExtenEnable()
{
    if (_chip_id == AXP192_CHIP_ID)
        return IS_OPEN(_outputReg, AXP192_EXTEN);
    return false;
}

bool AXP20X_Class::isLDO2Enable()
{
    //axp192 same axp202 ldo2 bit
    return IS_OPEN(_outputReg, AXP202_LDO2);
}

bool AXP20X_Class::isLDO3Enable()
{
    if (_chip_id == AXP192_CHIP_ID)
        return IS_OPEN(_outputReg, AXP192_LDO3);
    else if (_chip_id == AXP202_CHIP_ID)
        return IS_OPEN(_outputReg, AXP202_LDO3);
    return false;
}

bool AXP20X_Class::isLDO4Enable()
{
    if (_chip_id == AXP202_CHIP_ID)
        return IS_OPEN(_outputReg, AXP202_LDO4);
    return false;
}

bool AXP20X_Class::isDCDC2Enable()
{
    //axp192 same axp202 dc2 bit
    return IS_OPEN(_outputReg, AXP202_DCDC2);
}

bool AXP20X_Class::isDCDC3Enable()
{
    //axp192 same axp202 dc3 bit
    return IS_OPEN(_outputReg, AXP202_DCDC3);
}

int AXP20X_Class::setPowerOutPut(uint8_t ch, bool en)
{
    uint8_t data;
    uint8_t val = 0;
    if (!_init)return AXP_NOT_INIT;

    _readByte(AXP202_LDO234_DC23_CTL, 1, &data);
    if (en) {
        data |= (1 << ch);
    } else {
        data &= (~(1 << ch));
    }

    FORCED_OPEN_DCDC3(data);    //! Must be forced open in T-Watch

    _writeByte(AXP202_LDO234_DC23_CTL, 1, &data);
    delay(1);
    _readByte(AXP202_LDO234_DC23_CTL, 1, &val);
    if (data == val) {
        _outputReg = val;
        return AXP_PASS;
    }
    return AXP_FAIL;
}

bool AXP20X_Class::isChargeing()
{
    uint8_t reg;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_MODE_CHGSTATUS, 1, &reg);
    return IS_OPEN(reg, 6);
}

bool AXP20X_Class::isBatteryConnect()
{
    uint8_t reg;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_MODE_CHGSTATUS, 1, &reg);
    return IS_OPEN(reg, 5);
}

float AXP20X_Class::getAcinVoltage()
{
    if (!_init)return AXP_NOT_INIT;
    return _getRegistResult(AXP202_ACIN_VOL_H8, AXP202_ACIN_VOL_L4) * AXP202_ACIN_VOLTAGE_STEP;
}

float AXP20X_Class::getAcinCurrent()
{
    if (!_init)return AXP_NOT_INIT;
    float rslt;
    uint8_t hv, lv;
    return _getRegistResult(AXP202_ACIN_CUR_H8, AXP202_ACIN_CUR_L4) * AXP202_ACIN_CUR_STEP;
}

float AXP20X_Class::getVbusVoltage()
{
    if (!_init)return AXP_NOT_INIT;
    return _getRegistResult(AXP202_VBUS_VOL_H8, AXP202_VBUS_VOL_L4) * AXP202_VBUS_VOLTAGE_STEP;
}

float AXP20X_Class::getVbusCurrent()
{
    if (!_init)return AXP_NOT_INIT;
    return _getRegistResult(AXP202_VBUS_CUR_H8, AXP202_VBUS_CUR_L4) * AXP202_VBUS_CUR_STEP;
}

float AXP20X_Class::getTemp()
{
    if (!_init)return AXP_NOT_INIT;
    uint8_t hv, lv;
    _readByte(AXP202_INTERNAL_TEMP_H8, 1, &hv);
    _readByte(AXP202_INTERNAL_TEMP_L4, 1, &lv);
    float rslt =  hv << 8 | (lv & 0xF);
    return rslt / 1000;
}

float AXP20X_Class::getTSTemp()
{
    if (!_init)return AXP_NOT_INIT;
    return _getRegistResult(AXP202_TS_IN_H8, AXP202_TS_IN_L4) * AXP202_TS_PIN_OUT_STEP;
}

float AXP20X_Class::getGPIO0Voltage()
{
    if (!_init)return AXP_NOT_INIT;
    return _getRegistResult(AXP202_GPIO0_VOL_ADC_H8, AXP202_GPIO0_VOL_ADC_L4) * AXP202_GPIO0_STEP;
}

float AXP20X_Class::getGPIO1Voltage()
{
    if (!_init)return AXP_NOT_INIT;
    return _getRegistResult(AXP202_GPIO1_VOL_ADC_H8, AXP202_GPIO1_VOL_ADC_L4) * AXP202_GPIO1_STEP;
}

/*
Note: the battery power formula:
Pbat =2* register value * Voltage LSB * Current LSB / 1000.
(Voltage LSB is 1.1mV; Current LSB is 0.5mA, and unit of calculation result is mW.)
*/
float AXP20X_Class::getBattInpower()
{
    float rslt;
    uint8_t hv, mv, lv;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_BAT_POWERH8, 1, &hv);
    _readByte(AXP202_BAT_POWERM8, 1, &mv);
    _readByte(AXP202_BAT_POWERL8, 1, &lv);
    rslt =  (hv << 16) | (mv << 8) | lv;
    rslt = 2 * rslt * 1.1 * 0.5 / 1000;
    return rslt;
}

float AXP20X_Class::getBattVoltage()
{
    if (!_init)return AXP_NOT_INIT;
    return _getRegistResult(AXP202_BAT_AVERVOL_H8, AXP202_BAT_AVERVOL_L4) * AXP202_BATT_VOLTAGE_STEP;
}

float AXP20X_Class::getBattChargeCurrent()
{
    if (!_init)return AXP_NOT_INIT;
    switch (_chip_id) {
    case AXP202_CHIP_ID:
        return _getRegistResult(AXP202_BAT_AVERCHGCUR_H8, AXP202_BAT_AVERCHGCUR_L4) * AXP202_BATT_CHARGE_CUR_STEP;
    case AXP192_CHIP_ID:
        return _getRegistH8L5(AXP202_BAT_AVERCHGCUR_H8, AXP202_BAT_AVERCHGCUR_L4) * AXP202_BATT_CHARGE_CUR_STEP;
    default:
        break;
    }
}

float AXP20X_Class::getBattDischargeCurrent()
{
    float rslt;
    uint8_t hv, lv;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_BAT_AVERDISCHGCUR_H8, 1, &hv);
    _readByte(AXP202_BAT_AVERDISCHGCUR_L5, 1, &lv);
    rslt = (hv << 5) | (lv & 0x1F);
    return rslt * AXP202_BATT_DISCHARGE_CUR_STEP;
}

float AXP20X_Class::getSysIPSOUTVoltage()
{
    float rslt;
    uint8_t hv, lv;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_APS_AVERVOL_H8, 1, &hv);
    _readByte(AXP202_APS_AVERVOL_L4, 1, &lv);
    rslt = (hv << 4) | (lv & 0xF);
    return rslt;
}

/*
Coulomb calculation formula:
C= 65536 * current LSB *（charge coulomb counter value - discharge coulomb counter value） /
3600 / ADC sample rate. Refer to REG84H setting for ADC sample rate；the current LSB is
0.5mA；unit of the calculation result is mAh. ）
*/
uint32_t AXP20X_Class::getBattChargeCoulomb()
{
    uint8_t buffer[4];
    if (!_init)return AXP_NOT_INIT;
    _readByte(0xB1, 4, buffer);
    return buffer[0] << 24 + buffer[1] << 16 + buffer[2] << 8 + buffer[3];
}

uint32_t AXP20X_Class::getBattDischargeCoulomb()
{
    uint8_t buffer[4];
    if (!_init)return AXP_NOT_INIT;
    _readByte(0xB4, 4, buffer);
    return buffer[0] << 24 + buffer[1] << 16 + buffer[2] << 8 + buffer[3];
}

float AXP20X_Class::getCoulombData()
{
    if (!_init)return AXP_NOT_INIT;
    uint32_t charge = getBattChargeCoulomb();
    uint32_t discharge = getBattDischargeCoulomb();
    uint8_t rate = getAdcSamplingRate();
    float result = 65536.0 * 0.5 * (charge - discharge) / 3600.0 / rate;
    return result;
}

uint8_t AXP20X_Class::getAdcSamplingRate()
{
    //axp192 same axp202 aregister address 0x84
    if (!_init)return AXP_NOT_INIT;
    uint8_t val;
    _readByte(AXP202_ADC_SPEED, 1, &val);
    return 25 * (int)pow(2, (val & 0xC0) >> 6);
}

int AXP20X_Class::setAdcSamplingRate(axp_adc_sampling_rate_t rate)
{
    //axp192 same axp202 aregister address 0x84
    if (!_init)return AXP_NOT_INIT;
    if (rate > AXP_ADC_SAMPLING_RATE_200HZ) return AXP_FAIL;
    uint8_t val;
    _readByte(AXP202_ADC_SPEED, 1, &val);
    uint8_t rw = rate;
    val &= 0x3F;
    val |= (rw << 6);
    _writeByte(AXP202_ADC_SPEED, 1, &val);
    return AXP_PASS;
}


int AXP20X_Class::adc1Enable(uint16_t params, bool en)
{
    if (!_init)return AXP_NOT_INIT;
    uint8_t val;
    _readByte(AXP202_ADC_EN1, 1, &val);
    if (en)
        val |= params;
    else
        val &= ~(params);
    _writeByte(AXP202_ADC_EN1, 1, &val);

    _readByte(AXP202_ADC_EN1, 1, &val);
    return AXP_PASS;
}

int AXP20X_Class::adc2Enable(uint16_t params, bool en)
{
    if (!_init)return AXP_NOT_INIT;
    uint8_t val;
    _readByte(AXP202_ADC_EN2, 1, &val);
    if (en)
        val |= params;
    else
        val &= ~(params);
    _writeByte(AXP202_ADC_EN2, 1, &val);
    return AXP_PASS;
}



int AXP20X_Class::enableIRQ(uint32_t params, bool en)
{
    if (!_init)return AXP_NOT_INIT;
    uint8_t val, val1;
    if (params & 0xFF) {
        val1 = params & 0xFF;
        _readByte(AXP202_INTEN1, 1, &val);
        if (en)
            val |= val1;
        else
            val &= ~(val1);
        AXP_DEBUG("%s [0x%x]val:0x%x\n", en ? "enable" : "disable", AXP202_INTEN1, val);
        _writeByte(AXP202_INTEN1, 1, &val);
    }
    if (params & 0xFF00) {
        val1 = params >> 8;
        _readByte(AXP202_INTEN2, 1, &val);
        if (en)
            val |= val1;
        else
            val &= ~(val1);
        AXP_DEBUG("%s [0x%x]val:0x%x\n", en ? "enable" : "disable", AXP202_INTEN2, val);
        _writeByte(AXP202_INTEN2, 1, &val);
    }

    if (params & 0xFF0000) {
        val1 = params >> 16;
        _readByte(AXP202_INTEN3, 1, &val);
        if (en)
            val |= val1;
        else
            val &= ~(val1);
        AXP_DEBUG("%s [0x%x]val:0x%x\n", en ? "enable" : "disable", AXP202_INTEN3, val);
        _writeByte(AXP202_INTEN3, 1, &val);
    }

    if (params & 0xFF000000) {
        val1 = params >> 24;
        _readByte(AXP202_INTEN4, 1, &val);
        if (en)
            val |= val1;
        else
            val &= ~(val1);
        AXP_DEBUG("%s [0x%x]val:0x%x\n", en ? "enable" : "disable", AXP202_INTEN4, val);
        _writeByte(AXP202_INTEN4, 1, &val);
    }
    return AXP_PASS;
}

int AXP20X_Class::readIRQ()
{
    if (!_init)return AXP_NOT_INIT;
    switch (_chip_id) {
    case AXP192_CHIP_ID:
        for (int i = 0; i < 4; ++i) {
            _readByte(AXP192_INTSTS1 + i, 1, &_irq[i]);
        }
        _readByte(AXP192_INTSTS5, 1, &_irq[4]);
        return AXP_PASS;

    case AXP202_CHIP_ID:
        for (int i = 0; i < 5; ++i) {
            _readByte(AXP202_INTSTS1 + i, 1, &_irq[i]);
        }
        return AXP_PASS;
    default:
        return AXP_FAIL;
    }
}

void AXP20X_Class::clearIRQ()
{
    uint8_t val = 0xFF;
    switch (_chip_id) {
    case AXP192_CHIP_ID:
        for (int i = 0; i < 3; i++) {
            _writeByte(AXP192_INTSTS1 + i, 1, &val);
        }
        _writeByte(AXP192_INTSTS5, 1, &val);
        break;
    case AXP202_CHIP_ID:
        for (int i = 0; i < 5; i++) {
            _writeByte(AXP202_INTSTS1 + i, 1, &val);
        }
        break;
    default:
        break;
    }
    memset(_irq, 0, sizeof(_irq));
}


bool AXP20X_Class::isAcinOverVoltageIRQ()
{
    return (bool)(_irq[0] & BIT_MASK(7));
}

bool AXP20X_Class::isAcinPlugInIRQ()
{
    return (bool)(_irq[0] & BIT_MASK(6));
}

bool AXP20X_Class::isAcinRemoveIRQ()
{
    return (bool)(_irq[0] & BIT_MASK(5));
}

bool AXP20X_Class::isVbusOverVoltageIRQ()
{
    return (bool)(_irq[0] & BIT_MASK(4));
}

bool AXP20X_Class::isVbusPlugInIRQ()
{
    return (bool)(_irq[0] & BIT_MASK(3));
}

bool AXP20X_Class::isVbusRemoveIRQ()
{
    return (bool)(_irq[0] & BIT_MASK(2));
}

bool AXP20X_Class::isVbusLowVHOLDIRQ()
{
    return (bool)(_irq[0] & BIT_MASK(1));
}

bool AXP20X_Class::isBattPlugInIRQ()
{
    return (bool)(_irq[1] & BIT_MASK(7));
}
bool AXP20X_Class::isBattRemoveIRQ()
{
    return (bool)(_irq[1] & BIT_MASK(6));
}
bool AXP20X_Class::isBattEnterActivateIRQ()
{
    return (bool)(_irq[1] & BIT_MASK(5));
}
bool AXP20X_Class::isBattExitActivateIRQ()
{
    return (bool)(_irq[1] & BIT_MASK(4));
}
bool AXP20X_Class::isChargingIRQ()
{
    return (bool)(_irq[1] & BIT_MASK(3));
}
bool AXP20X_Class::isChargingDoneIRQ()
{
    return (bool)(_irq[1] & BIT_MASK(2));
}
bool AXP20X_Class::isBattTempLowIRQ()
{
    return (bool)(_irq[1] & BIT_MASK(1));
}
bool AXP20X_Class::isBattTempHighIRQ()
{
    return (bool)(_irq[1] & BIT_MASK(0));
}

bool AXP20X_Class::isPEKShortPressIRQ()
{
    return (bool)(_irq[2] & BIT_MASK(1));
}

bool AXP20X_Class::isPEKLongtPressIRQ()
{
    return (bool)(_irq[2] & BIT_MASK(0));
}

bool AXP20X_Class::isVBUSPlug()
{
    if (!_init)return AXP_NOT_INIT;
    uint8_t val;
    _readByte(AXP202_STATUS, 1, &val);
    return (bool)(_irq[2] & BIT_MASK(5));
}

int AXP20X_Class::setDCDC2Voltage(uint16_t mv)
{
    if (!_init)return AXP_NOT_INIT;
    if (mv < 700) {
        AXP_DEBUG("DCDC2:Below settable voltage:700mV~2275mV");
        mv = 700;
    }
    if (mv > 2275) {
        AXP_DEBUG("DCDC2:Above settable voltage:700mV~2275mV");
        mv = 2275;
    }
    uint8_t val = (mv - 700) / 25;
    _writeByte(AXP202_DC2OUT_VOL, 1, &val);
    return AXP_PASS;
}



uint16_t AXP20X_Class::getDCDC2Voltage()
{
    uint8_t val = 0;
    _readByte(AXP202_DC2OUT_VOL, 1, &val);
    return val * 25 + 700;
}

uint16_t AXP20X_Class::getDCDC3Voltage()
{
    uint8_t val = 0;
    _readByte(AXP202_DC3OUT_VOL, 1, &val);
    return val * 25 + 700;
}


int AXP20X_Class::setDCDC3Voltage(uint16_t mv)
{
    if (!_init)return AXP_NOT_INIT;
    if (mv < 700) {
        AXP_DEBUG("DCDC3:Below settable voltage:700mV~3500mV");
        mv = 700;
    }
    if (mv > 3500) {
        AXP_DEBUG("DCDC3:Above settable voltage:700mV~3500mV");
        mv = 3500;
    }
    uint8_t val = (mv - 700) / 25;
    _writeByte(AXP202_DC3OUT_VOL, 1, &val);
    return AXP_PASS;
}


int AXP20X_Class::setLDO2Voltage(uint16_t mv)
{
    uint8_t rVal, wVal;
    if (!_init)return AXP_NOT_INIT;
    if (mv < 1800) {
        AXP_DEBUG("LDO2:Below settable voltage:1800mV~3300mV");
        mv = 1800;
    }
    if (mv > 3300) {
        AXP_DEBUG("LDO2:Above settable voltage:1800mV~3300mV");
        mv = 3300;
    }
    wVal = (mv - 1800) / 100;
    if (_chip_id == AXP202_CHIP_ID ) {
        _readByte(AXP202_LDO24OUT_VOL, 1, &rVal);
        rVal &= 0x0F;
        rVal |= (wVal << 4);
        _writeByte(AXP202_LDO24OUT_VOL, 1, &rVal);
        return AXP_PASS;
    } else if (_chip_id == AXP192_CHIP_ID) {
        _readByte(AXP192_LDO23OUT_VOL, 1, &rVal);
        rVal &= 0x0F;
        rVal |= (wVal << 4);
        _writeByte(AXP192_LDO23OUT_VOL, 1, &rVal);
        return AXP_PASS;
    }
    return AXP_FAIL;
}


uint16_t AXP20X_Class::getLDO2Voltage()
{
    uint8_t rVal;
    if (_chip_id == AXP202_CHIP_ID ) {
        _readByte(AXP202_LDO24OUT_VOL, 1, &rVal);
        rVal &= 0xF0;
        rVal >>= 4;
        return rVal * 100 + 1800;
    } else if (_chip_id == AXP192_CHIP_ID) {
        _readByte(AXP192_LDO23OUT_VOL, 1, &rVal);
        AXP_DEBUG("get result:%x\n", rVal);
        rVal &= 0xF0;
        rVal >>= 4;
        return rVal * 100 + 1800;
    }
    return 0;
}

int AXP20X_Class::setLDO3Voltage(uint16_t mv)
{
    uint8_t rVal;
    if (!_init)return AXP_NOT_INIT;
    if (_chip_id == AXP202_CHIP_ID && mv < 700) {
        AXP_DEBUG("LDO3:Below settable voltage:700mV~2275mV");
        mv = 700;
    } else if (_chip_id == AXP192_CHIP_ID && mv < 1800) {
        AXP_DEBUG("LDO3:Below settable voltage:1800mV~3300mV");
        mv = 1800;
    }

    if (_chip_id == AXP202_CHIP_ID && mv > 2275) {
        AXP_DEBUG("LDO3:Above settable voltage:700mV~2275mV");
        mv = 2275;
    } else if (_chip_id == AXP192_CHIP_ID && mv > 3300) {
        AXP_DEBUG("LDO3:Above settable voltage:1800mV~3300mV");
        mv = 3300;
    }

    if (_chip_id == AXP202_CHIP_ID ) {

        _readByte(AXP202_LDO3OUT_VOL, 1, &rVal);
        rVal &= 0x80;
        rVal |= ((mv - 700) / 25);
        _writeByte(AXP202_LDO3OUT_VOL, 1, &rVal);
        return AXP_PASS;

    } else if (_chip_id == AXP192_CHIP_ID) {

        _readByte(AXP192_LDO23OUT_VOL, 1, &rVal);
        rVal &= 0xF0;
        rVal |= ((mv - 1800) / 100);
        _writeByte(AXP192_LDO23OUT_VOL, 1, &rVal);
        return AXP_PASS;
    }
    return AXP_FAIL;
}


uint16_t AXP20X_Class::getLDO3Voltage()
{
    uint8_t rVal;
    if (!_init)return AXP_NOT_INIT;

    if (_chip_id == AXP202_CHIP_ID ) {

        _readByte(AXP202_LDO3OUT_VOL, 1, &rVal);
        if (rVal & 0x80) {
            //! According to the hardware N_VBUSEN Pin selection
            return getVbusVoltage() * 1000;
        } else {
            return (rVal & 0x7F) * 25 + 700;
        }
    } else if (_chip_id == AXP192_CHIP_ID) {
        _readByte(AXP192_LDO23OUT_VOL, 1, &rVal);
        rVal &= 0x0F;
        return rVal * 100 + 1800;
    }
    return 0;
}

//! Only axp202 support
int AXP20X_Class::setLDO4Voltage(axp_ldo4_table_t param)
{
    if (!_init)return AXP_NOT_INIT;
    if (_chip_id != AXP202_CHIP_ID ) return AXP_FAIL;
    if (param >= AXP202_LDO4_MAX)return AXP_INVALID;
    uint8_t val;
    _readByte(AXP202_LDO24OUT_VOL, 1, &val);
    val &= 0xF0;
    val |= param;
    _writeByte(AXP202_LDO24OUT_VOL, 1, &val);
    return AXP_PASS;
}

//! Only AXP202 support
// 0 : LDO  1 : DCIN
int AXP20X_Class::setLDO3Mode(uint8_t mode)
{
    uint8_t val;
    if (_chip_id != AXP202_CHIP_ID ) return AXP_FAIL;
    _readByte(AXP202_LDO3OUT_VOL, 1, &val);
    if (mode) {
        val |= BIT_MASK(7);
    } else {
        val &= (~BIT_MASK(7));
    }
    _writeByte(AXP202_LDO3OUT_VOL, 1, &val);
    return AXP_PASS;

}

int AXP20X_Class::setStartupTime(uint8_t param)
{
    uint8_t val;
    if (!_init)return AXP_NOT_INIT;
    if (param > sizeof(startupParams) / sizeof(startupParams[0]))return AXP_INVALID;
    _readByte(AXP202_POK_SET, 1, &val);
    val &= (~0b11000000);
    val |= startupParams[param];
    _writeByte(AXP202_POK_SET, 1, &val);
}

int AXP20X_Class::setlongPressTime(uint8_t param)
{
    uint8_t val;
    if (!_init)return AXP_NOT_INIT;
    if (param > sizeof(longPressParams) / sizeof(longPressParams[0]))return AXP_INVALID;
    _readByte(AXP202_POK_SET, 1, &val);
    val &= (~0b00110000);
    val |= longPressParams[param];
    _writeByte(AXP202_POK_SET, 1, &val);
}

int AXP20X_Class::setShutdownTime(uint8_t param)
{
    uint8_t val;
    if (!_init)return AXP_NOT_INIT;
    if (param > sizeof(shutdownParams) / sizeof(shutdownParams[0]))return AXP_INVALID;
    _readByte(AXP202_POK_SET, 1, &val);
    val &= (~0b00000011);
    val |= shutdownParams[param];
    _writeByte(AXP202_POK_SET, 1, &val);
}

int AXP20X_Class::setTimeOutShutdown(bool en)
{
    uint8_t val;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_POK_SET, 1, &val);
    if (en)
        val |= (1 << 3);
    else
        val &= (~(1 << 3));
    _writeByte(AXP202_POK_SET, 1, &val);
}


int AXP20X_Class::shutdown()
{
    uint8_t val;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_OFF_CTL, 1, &val);
    val |= (1 << 7);
    _writeByte(AXP202_OFF_CTL, 1, &val);
}


float AXP20X_Class::getSettingChargeCurrent()
{
    uint8_t val;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_CHARGE1, 1, &val);
    val &= 0b00000111;
    float cur = 300.0 + val * 100.0;
    AXP_DEBUG("Setting Charge current : %.2f mA\n", cur);
    return cur;
}

bool AXP20X_Class::isChargeingEnable()
{
    uint8_t val;
    if (!_init)return false;
    _readByte(AXP202_CHARGE1, 1, &val);
    if (val & (1 << 7)) {
        AXP_DEBUG("Charging enable is enable\n");
        val = true;
    } else {
        AXP_DEBUG("Charging enable is disable\n");
        val = false;
    }
    return val;
}

int AXP20X_Class::enableChargeing(bool en)
{
    uint8_t val;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_CHARGE1, 1, &val);
    val |= (1 << 7);
    _writeByte(AXP202_CHARGE1, 1, &val);
    return AXP_PASS;
}

int AXP20X_Class::setChargingTargetVoltage(axp_chargeing_vol_t param)
{
    uint8_t val;
    if (!_init)return AXP_NOT_INIT;
    if (param > sizeof(targetVolParams) / sizeof(targetVolParams[0]))return AXP_INVALID;
    _readByte(AXP202_CHARGE1, 1, &val);
    val &= ~(0b01100000);
    val |= targetVolParams[param];
    _writeByte(AXP202_CHARGE1, 1, &val);
    return AXP_PASS;
}

int AXP20X_Class::getBattPercentage()
{
    if (!_init)return AXP_NOT_INIT;
    uint8_t val;
    _readByte(AXP202_BATT_PERCENTAGE, 1, &val);
    if (!(val & BIT_MASK(7))) {
        return val & (~BIT_MASK(7));
    }
    return 0;
}



int AXP20X_Class::setChgLEDMode(uint8_t mode)
{
    uint8_t val;
    _readByte(AXP202_OFF_CTL, 1, &val);
    val |= BIT_MASK(3);
    switch (mode) {
    case AXP20X_LED_OFF:
        val &= 0b11001111;
        _writeByte(AXP202_OFF_CTL, 1, &val);
        break;
    case AXP20X_LED_BLINK_1HZ:
        val &= 0b11001111;
        val |= 0b00010000;
        _writeByte(AXP202_OFF_CTL, 1, &val);
        break;
    case AXP20X_LED_BLINK_4HZ:
        val &= 0b11001111;
        val |= 0b00100000;
        _writeByte(AXP202_OFF_CTL, 1, &val);
        break;
    case AXP20X_LED_LOW_LEVEL:
        val &= 0b11001111;
        val |= 0b00110000;
        _writeByte(AXP202_OFF_CTL, 1, &val);
        break;
    default:
        break;
    }
    return AXP_PASS;
}

int AXP20X_Class::debugCharging()
{
    uint8_t val;
    _readByte(AXP202_CHARGE1, 1, &val);
    AXP_DEBUG("SRC REG:0x%x\n", val);
    if (val & (1 << 7)) {
        AXP_DEBUG("Charging enable is enable\n");
    } else {
        AXP_DEBUG("Charging enable is disable\n");
    }
    AXP_DEBUG("Charging target-voltage : 0x%x\n", ((val & 0b01100000) >> 5 ) & 0b11);
    if (val & (1 << 4)) {
        AXP_DEBUG("end when the charge current is lower than 15%% of the set value\n");
    } else {
        AXP_DEBUG(" end when the charge current is lower than 10%% of the set value\n");
    }
    val &= 0b00000111;
    float cur = 300.0 + val * 100.0;
    AXP_DEBUG("Charge current : %.2f mA\n", cur);
}


int AXP20X_Class::debugStatus()
{
    if (!_init)return AXP_NOT_INIT;
    uint8_t val, val1, val2;
    _readByte(AXP202_STATUS, 1, &val);
    _readByte(AXP202_MODE_CHGSTATUS, 1, &val1);
    _readByte(AXP202_IPS_SET, 1, &val2);
    AXP_DEBUG("AXP202_STATUS:   AXP202_MODE_CHGSTATUS   AXP202_IPS_SET\n");
    AXP_DEBUG("0x%x\t\t\t 0x%x\t\t\t 0x%x\n", val, val1, val2);
}


int AXP20X_Class::limitingOff()
{
    if (!_init)return AXP_NOT_INIT;
    uint8_t val;
    _readByte(AXP202_IPS_SET, 1, &val);
    if (_chip_id == AXP202_CHIP_ID) {
        val |= 0x03;
    } else {
        val &= ~(1 << 1);
    }
    _writeByte(AXP202_IPS_SET, 1, &val);
    return AXP_PASS;
}

// Only AXP129 chip
int AXP20X_Class::setDCDC1Voltage(uint16_t mv)
{
    if (!_init)return AXP_NOT_INIT;
    if (_chip_id != AXP192_CHIP_ID) return AXP_FAIL;
    if (mv < 700) {
        AXP_DEBUG("DCDC1:Below settable voltage:700mV~3500mV");
        mv = 700;
    }
    if (mv > 3500) {
        AXP_DEBUG("DCDC1:Above settable voltage:700mV~3500mV");
        mv = 3500;
    }
    uint8_t val = (mv - 700) / 25;
    _writeByte(AXP192_DC1_VLOTAGE, 1, &val);
    return AXP_PASS;
}

// Only AXP129 chip
uint16_t AXP20X_Class::getDCDC1Voltage()
{
    if (_chip_id != AXP192_CHIP_ID) return AXP_FAIL;
    uint8_t val = 0;
    _readByte(AXP192_DC1_VLOTAGE, 1, &val);
    return val * 25 + 700;
}


int AXP20X_Class::setGPIO0Voltage(uint8_t param)
{
    uint8_t params[] = {
        0b11111000,
        0b11111001,
        0b11111010,
        0b11111011,
        0b11111100,
        0b11111101,
        0b11111110,
        0b11111111,
    };
    if (!_init)return AXP_NOT_INIT;
    if (param > sizeof(params) / sizeof(params[0]))return AXP_INVALID;
    uint8_t val = 0;
    _readByte(AXP202_GPIO0_VOL, 1, &val);
    val &= 0b11111000;
    val |= params[param];
    _writeByte(AXP202_GPIO0_VOL, 1, &val);
    return AXP_PASS;
}

int AXP20X_Class::setGPIO0Level(uint8_t level)
{
    uint8_t val = 0;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_GPIO0_CTL, 1, &val);
    val = level ? val & 0b11111000 : (val & 0b11111000) | 0b00000001;
    _writeByte(AXP202_GPIO0_CTL, 1, &val);
    return AXP_PASS;
}
int AXP20X_Class::setGPIO1Level(uint8_t level)
{
    uint8_t val = 0;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_GPIO1_CTL, 1, &val);
    val = level ? val & 0b11111000 : (val & 0b11111000) | 0b00000001;
    _writeByte(AXP202_GPIO1_CTL, 1, &val);
    return AXP_PASS;
}



int AXP20X_Class::readGpioStatus()
{
    uint8_t val = 0;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_GPIO012_SIGNAL, 1, &val);
    _gpio[0] = val & BIT_MASK(4);
    _gpio[1] = val & BIT_MASK(5);
    _gpio[2] = val & BIT_MASK(6);
    _readByte(AXP202_GPIO3_CTL, 1, &val);
    _gpio[3] = val & 1;
    return AXP_PASS;
}

int AXP20X_Class::readGpio0Level()
{
    return _gpio[0];
}

int AXP20X_Class::readGpio1Level()
{
    return _gpio[1];
}

int AXP20X_Class::readGpio2Level()
{
    return _gpio[2];
}


int AXP20X_Class::setGpio2Mode(uint8_t mode)
{
    uint8_t params[] = {
        0b11111000,
        0b11111001,
        0b11111010,
    };
    if (!_init)return AXP_NOT_INIT;
    if (mode > sizeof(params) / sizeof(params[0]))return AXP_INVALID;
    uint8_t val = 0;
    _readByte(AXP202_GPIO2_CTL, 1, &val);
    val &= params[0];
    val |= params[mode];
    _writeByte(AXP202_GPIO2_CTL, 1, &val);
    return AXP_PASS;
}



int AXP20X_Class::setGpio3Mode(uint8_t mode)
{
    uint8_t val = 0;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_GPIO3_CTL, 1, &val);
    if (mode == AXP202_GPIO3_DIGITAL_INPUT) {
        val |= BIT_MASK(2);
    } else if (mode == AXP202_GPIO3_OPEN_DRAIN_OUTPUT) {
        val &= ~BIT_MASK(2);
    } else {
        return AXP_INVALID;
    }
    return AXP_PASS;
}

int AXP20X_Class::setGpio3Level(uint8_t level)
{
    uint8_t val = 0;
    if (!_init)return AXP_NOT_INIT;
    _readByte(AXP202_GPIO3_CTL, 1, &val);
    if (!(val & BIT_MASK(2))) {
        return AXP_FAIL;
    }
    val = level ? val & (~BIT_MASK(1)) : val |  BIT_MASK(1);
    _writeByte(AXP202_GPIO3_CTL, 1, &val);
}

int AXP20X_Class::_setGpioInterrupt(uint8_t *val, int mode, bool en)
{
    switch (mode) {
    case RISING:
        *val = en ? *val | BIT_MASK(7) : *val & (~BIT_MASK(7));
        break;
    case FALLING:
        *val = en ? *val | BIT_MASK(6) : *val & (~BIT_MASK(6));
        break;
    default:
        break;
    }
}

int AXP20X_Class::setGpioInterruptMode(uint8_t gpio, int mode, bool en)
{
    uint8_t val = 0;
    if (!_init)return AXP_NOT_INIT;
    switch (gpio) {
    case AXP202_GPIO0:
        _readByte(AXP202_GPIO0_CTL, 1, &val);
        _setGpioInterrupt(&val, mode, en);
        break;
    case AXP202_GPIO1:
        _readByte(AXP202_GPIO1_CTL, 1, &val);
        _setGpioInterrupt(&val, mode, en);
        break;
    case AXP202_GPIO2:
        _readByte(AXP202_GPIO2_CTL, 1, &val);
        _setGpioInterrupt(&val, mode, en);
        break;
    case AXP202_GPIO3:
        _readByte(AXP202_GPIO3_CTL, 1, &val);
        _setGpioInterrupt(&val, mode, en);
        break;
    }
}


int AXP20X_Class::gpio0Setting(axp192_gpio0_mode_t mode)
{
    uint8_t rVal;
    if (!_init)return AXP_NOT_INIT;
    if (_chip_id == AXP192_CHIP_ID ) {
        _readByte(AXP192_GPIO0_CTL, 1, &rVal);
        rVal &= 0xF8;
        rVal |= mode;
        _writeByte(AXP192_GPIO0_CTL, 1, &rVal);
        return AXP_PASS;
    }
    return AXP_FAIL;
}


int AXP20X_Class::gpio0SetVoltage(axp192_gpio_voltage_t vol)
{
    uint8_t rVal;
    if (!_init)return AXP_NOT_INIT;
    if (_chip_id == AXP192_CHIP_ID) {
        _readByte(AXP192_GPIO0_VOL, 1, &rVal);
        rVal &= 0x0F;
        rVal |= (vol << 4);
        _writeByte(AXP192_GPIO0_VOL, 1, &rVal);
        return AXP_PASS;
    }
    return AXP_FAIL;
}

uint16_t AXP20X_Class::gpio0GetVoltage()
{
    uint8_t rVal;
    if (!_init)return AXP_NOT_INIT;
    if (_chip_id == AXP192_CHIP_ID) {
        _readByte(AXP192_GPIO0_VOL, 1, &rVal);
        rVal &= 0xF0;
        rVal >>= 4;
        return rVal;
    }
    return 0;
}