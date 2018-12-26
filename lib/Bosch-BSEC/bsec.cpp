/**
 * Copyright (C) 2017 - 2018 Bosch Sensortec GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names of the
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 * The information provided is believed to be accurate and reliable.
 * The copyright holder assumes no responsibility
 * for the consequences of use
 * of such information nor for any infringement of patents or
 * other rights of third parties which may result from its use.
 * No license is granted by implication or otherwise under any patent or
 * patent rights of the copyright holder.
 *
 * @file	bsec.cpp
 * @date	31 Jan 2018
 * @version	1.0
 *
 */

#include "bsec.h"

TwoWire* Bsec::wireObj = NULL;
SPIClass* Bsec::spiObj = NULL;

/**
 * @brief Constructor
 */
Bsec::Bsec()
{
	nextCall = 0;
	version.major = 0;
	version.minor = 0;
	version.major_bugfix = 0;
	version.minor_bugfix = 0;
	millisOverflowCounter = 0;
	lastTime = 0;
	bme680Status = BME680_OK;
	outputTimestamp = 0;
	_tempOffset = 0.0f;
	status = BSEC_OK;
	zeroOutputs();
}

/**
 * @brief Function to initialize the BSEC library and the BME680 sensor
 */
void Bsec::begin(uint8_t devId, enum bme680_intf intf, bme680_com_fptr_t read, bme680_com_fptr_t write, bme680_delay_fptr_t idleTask)
{
	_bme680.dev_id = devId;
	_bme680.intf = intf;
	_bme680.read = read;
	_bme680.write = write;
	_bme680.delay_ms = idleTask;
	_bme680.amb_temp = 25;
	_bme680.power_mode = BME680_FORCED_MODE;

	beginCommon();
}

/**
 * @brief Function to initialize the BSEC library and the BME680 sensor
 */
void Bsec::begin(uint8_t i2cAddr, TwoWire &i2c)
{
	_bme680.dev_id = i2cAddr;
	_bme680.intf = BME680_I2C_INTF;
	_bme680.read = Bsec::i2cRead;
	_bme680.write = Bsec::i2cWrite;
	_bme680.delay_ms = Bsec::delay_ms;
	_bme680.amb_temp = 25;
	_bme680.power_mode = BME680_FORCED_MODE;

	Bsec::wireObj = &i2c;
	Bsec::wireObj->begin();

	beginCommon();
}

/**
 * @brief Function to initialize the BSEC library and the BME680 sensor
 */
void Bsec::begin(uint8_t chipSelect, SPIClass &spi)
{
	_bme680.dev_id = chipSelect;
	_bme680.intf = BME680_SPI_INTF;
	_bme680.read = Bsec::spiTransfer;
	_bme680.write = Bsec::spiTransfer;
	_bme680.delay_ms = Bsec::delay_ms;
	_bme680.amb_temp = 25;
	_bme680.power_mode = BME680_FORCED_MODE;

	pinMode(chipSelect, OUTPUT);
	digitalWrite(chipSelect, HIGH);
	Bsec::spiObj = &spi;
	Bsec::spiObj->begin();

	beginCommon();
}

/**
 * @brief Common code for the begin function
 */
void Bsec::beginCommon(void)
{
	status = bsec_init();

	getVersion();
	
	bme680Status = bme680_init(&_bme680);
}

/**
 * @brief Function that sets the desired sensors and the sample rates
 */
void Bsec::updateSubscription(bsec_virtual_sensor_t sensorList[], uint8_t nSensors, float sampleRate)
{
	bsec_sensor_configuration_t virtualSensors[BSEC_NUMBER_OUTPUTS],
	        sensorSettings[BSEC_MAX_PHYSICAL_SENSOR];
	uint8_t nVirtualSensors = 0, nSensorSettings = BSEC_MAX_PHYSICAL_SENSOR;

	for (uint8_t i = 0; i < nSensors; i++) {
		virtualSensors[nVirtualSensors].sensor_id = sensorList[i];
		virtualSensors[nVirtualSensors].sample_rate = sampleRate;
		nVirtualSensors++;
	}

	status = bsec_update_subscription(virtualSensors, nVirtualSensors, sensorSettings, &nSensorSettings);
	return;
}

/**
 * @brief Callback from the user to trigger reading of data from the BME680, process and store outputs
 */
bool Bsec::run(void)
{
	bool newData = false;
	/* Check if the time has arrived to call do_steps() */
	int64_t callTimeMs = getTimeMs();
	
	if (callTimeMs >= nextCall) {
	
		bsec_bme_settings_t bme680Settings;

		int64_t callTimeNs = callTimeMs * INT64_C(1000000);

		status = bsec_sensor_control(callTimeNs, &bme680Settings);
		if (status < BSEC_OK)
			return false;

		nextCall = bme680Settings.next_call / INT64_C(1000000); // Convert from ns to ms

		bme680Status = setBme680Config(bme680Settings);
		if (bme680Status != BME680_OK) {
			return false;
		}

		bme680Status = bme680_set_sensor_mode(&_bme680);
		if (bme680Status != BME680_OK) {
			return false;
		}

		/* Wait for measurement to complete */
		uint16_t meas_dur = 0;

		bme680_get_profile_dur(&meas_dur, &_bme680);
		delay_ms(meas_dur);

		newData = readProcessData(callTimeNs, bme680Settings);	
	}
	
	return newData;
}

/**
 * @brief Function to get the state of the algorithm to save to non-volatile memory
 */
void Bsec::getState(uint8_t *state)
{
	uint8_t workBuffer[BSEC_MAX_STATE_BLOB_SIZE];
	uint32_t n_serialized_state = BSEC_MAX_STATE_BLOB_SIZE;
	status = bsec_get_state(0, state, BSEC_MAX_STATE_BLOB_SIZE, workBuffer, BSEC_MAX_STATE_BLOB_SIZE, &n_serialized_state);
}

/**
 * @brief Function to set the state of the algorithm from non-volatile memory
 */
void Bsec::setState(uint8_t *state)
{
	uint8_t workBuffer[BSEC_MAX_STATE_BLOB_SIZE];

	status = bsec_set_state(state, BSEC_MAX_STATE_BLOB_SIZE, workBuffer, BSEC_MAX_STATE_BLOB_SIZE);
}

/**
 * @brief Function to set the configuration of the algorithm from memory
 */
void Bsec::setConfig(const uint8_t *state)
{
	uint8_t workBuffer[BSEC_MAX_PROPERTY_BLOB_SIZE];

	status = bsec_set_configuration(state, BSEC_MAX_PROPERTY_BLOB_SIZE, workBuffer, sizeof(workBuffer));
}

/* Private functions */

/**
 * @brief Get the version of the BSEC library
 */
void Bsec::getVersion(void)
{
	bsec_get_version(&version);
}

/**
 * @brief Read data from the BME680 and process it
 */
bool Bsec::readProcessData(int64_t currTimeNs, bsec_bme_settings_t bme680Settings)
{
	bme680Status = bme680_get_sensor_data(&_data, &_bme680);
	if (bme680Status != BME680_OK) {
		return false;
	}

	bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR]; // Temp, Pres, Hum & Gas
	uint8_t nInputs = 0, nOutputs = 0;

	if (_data.status & BME680_NEW_DATA_MSK) {
		if (bme680Settings.process_data & BSEC_PROCESS_TEMPERATURE) {
			inputs[nInputs].sensor_id = BSEC_INPUT_TEMPERATURE;
			inputs[nInputs].signal = _data.temperature;
			inputs[nInputs].time_stamp = currTimeNs;
			nInputs++;
			/* Temperature offset from the real temperature due to external heat sources */
			inputs[nInputs].sensor_id = BSEC_INPUT_HEATSOURCE;
			inputs[nInputs].signal = _tempOffset;
			inputs[nInputs].time_stamp = currTimeNs;
			nInputs++;
		}
		if (bme680Settings.process_data & BSEC_PROCESS_HUMIDITY) {
			inputs[nInputs].sensor_id = BSEC_INPUT_HUMIDITY;
			inputs[nInputs].signal = _data.humidity;
			inputs[nInputs].time_stamp = currTimeNs;
			nInputs++;
		}
		if (bme680Settings.process_data & BSEC_PROCESS_PRESSURE) {
			inputs[nInputs].sensor_id = BSEC_INPUT_PRESSURE;
			inputs[nInputs].signal = _data.pressure;
			inputs[nInputs].time_stamp = currTimeNs;
			nInputs++;
		}
		if (bme680Settings.process_data & BSEC_PROCESS_GAS) {
			inputs[nInputs].sensor_id = BSEC_INPUT_GASRESISTOR;
			inputs[nInputs].signal = _data.gas_resistance;
			inputs[nInputs].time_stamp = currTimeNs;
			nInputs++;
		}
	}

	if (nInputs > 0) {
		nOutputs = BSEC_NUMBER_OUTPUTS;
		bsec_output_t _outputs[BSEC_NUMBER_OUTPUTS];

		status = bsec_do_steps(inputs, nInputs, _outputs, &nOutputs);
		if (status != BSEC_OK)
			return false;

		zeroOutputs();

		if (nOutputs > 0) {
			outputTimestamp = _outputs[0].time_stamp / 1000000; // Convert from ns to ms

			for (uint8_t i = 0; i < nOutputs; i++) {
				switch (_outputs[i].sensor_id) {
					case BSEC_OUTPUT_IAQ:
						iaqEstimate = _outputs[i].signal;
						iaqAccuracy = _outputs[i].accuracy;
						break;
					case BSEC_OUTPUT_STATIC_IAQ:
						staticIaq = _outputs[i].signal;
						staticIaqAccuracy = _outputs[i].accuracy;
						break;
					case BSEC_OUTPUT_CO2_EQUIVALENT:
						co2Equivalent = _outputs[i].signal;
						co2Accuracy = _outputs[i].accuracy;
						break;
					case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
						breathVocEquivalent = _outputs[i].signal;
						breathVocAccuracy = _outputs[i].accuracy;
						break;
					case BSEC_OUTPUT_RAW_TEMPERATURE:
						rawTemperature = _outputs[i].signal;
						break;
					case BSEC_OUTPUT_RAW_PRESSURE:
						pressure = _outputs[i].signal;
						break;
					case BSEC_OUTPUT_RAW_HUMIDITY:
						rawHumidity = _outputs[i].signal;
						break;
					case BSEC_OUTPUT_RAW_GAS:
						gasResistance = _outputs[i].signal;
						break;
					case BSEC_OUTPUT_STABILIZATION_STATUS:
						stabStatus = _outputs[i].signal;
						break;
					case BSEC_OUTPUT_RUN_IN_STATUS:
						runInStatus = _outputs[i].signal;
						break;
					case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
						temperature = _outputs[i].signal;
						break;
					case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
						humidity = _outputs[i].signal;
						break;
					case BSEC_OUTPUT_COMPENSATED_GAS:
						compGasValue = _outputs[i].signal;
						compGasAccuracy = _outputs[i].accuracy;
						break;
					case BSEC_OUTPUT_GAS_PERCENTAGE:
						gasPercentage = _outputs[i].signal;
						gasPercentageAcccuracy = _outputs[i].accuracy;
						break;
					default:
						break;
				}
			}
			return true;
		}
	}

	return false;
}

/**
 * @brief Set the BME680 sensor's configuration
 */
int8_t Bsec::setBme680Config(bsec_bme_settings_t bme680Settings)
{
	_bme680.gas_sett.run_gas = bme680Settings.run_gas;
	_bme680.tph_sett.os_hum = bme680Settings.humidity_oversampling;
	_bme680.tph_sett.os_temp = bme680Settings.temperature_oversampling;
	_bme680.tph_sett.os_pres = bme680Settings.pressure_oversampling;
	_bme680.gas_sett.heatr_temp = bme680Settings.heater_temperature;
	_bme680.gas_sett.heatr_dur = bme680Settings.heating_duration;
	uint16_t desired_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_FILTER_SEL
	        | BME680_GAS_SENSOR_SEL;
	return bme680_set_sensor_settings(desired_settings, &_bme680);
}

/**
 * @brief Function to zero the outputs
 */
void Bsec::zeroOutputs(void)
{
	temperature = 0.0f;
	pressure = 0.0f;
	humidity = 0.0f;
	gasResistance = 0.0f;
	rawTemperature = 0.0f;
	rawHumidity = 0.0f;
	stabStatus = 0.0f;
	runInStatus = 0.0f;
	iaqEstimate = 0.0f;
	iaqAccuracy = 0;
	staticIaq = 0.0f;
	staticIaqAccuracy = 0;
	co2Equivalent = 0.0f;
	co2Accuracy = 0;
	breathVocEquivalent = 0.0f;
	breathVocAccuracy = 0;
	compGasValue = 0.0f;
	compGasAccuracy = 0;
	gasPercentage = 0.0f;
	gasPercentageAcccuracy = 0;
}

/**
 * @brief Function to calculate an int64_t timestamp in milliseconds
 */
int64_t Bsec::getTimeMs(void)
{
	int64_t timeMs = millis();

	if (lastTime > timeMs) { // An overflow occured
		lastTime = timeMs;
		millisOverflowCounter++;
	}

	return timeMs + (millisOverflowCounter * 0xFFFFFFFF);
}

/**
 @brief Task that delays for a ms period of time
 */
void Bsec::delay_ms(uint32_t period)
{
	// Wait for a period amount of ms
	// The system may simply idle, sleep or even perform background tasks
	delay(period);
}

/**
 @brief Callback function for reading registers over I2C
 */
int8_t Bsec::i2cRead(uint8_t devId, uint8_t regAddr, uint8_t *regData, uint16_t length)
{
	uint16_t i;
	int8_t rslt = 0;
	if(Bsec::wireObj) {
		Bsec::wireObj->beginTransmission(devId);
		Bsec::wireObj->write(regAddr);
		rslt = Bsec::wireObj->endTransmission();
		Bsec::wireObj->requestFrom((int) devId, (int) length);
		for (i = 0; (i < length) && Bsec::wireObj->available(); i++) {
			regData[i] = Bsec::wireObj->read();
		}
	} else {
		rslt = -1;
	}
	return rslt;
}

/**
 * @brief Callback function for writing registers over I2C
 */
int8_t Bsec::i2cWrite(uint8_t devId, uint8_t regAddr, uint8_t *regData, uint16_t length)
{
	uint16_t i;
	int8_t rslt = 0;
	if(Bsec::wireObj) {
		Bsec::wireObj->beginTransmission(devId);
		Bsec::wireObj->write(regAddr);
		for (i = 0; i < length; i++) {
			Bsec::wireObj->write(regData[i]);
		}
		rslt = Bsec::wireObj->endTransmission();
	} else {
		rslt = -1;
	}

	return rslt;
}

/**
 * @brief Callback function for reading and writing registers over SPI
 */
int8_t Bsec::spiTransfer(uint8_t devId, uint8_t regAddr, uint8_t *regData, uint16_t length)
{
	int8_t rslt = 0;
	if(Bsec::spiObj) {
		Bsec::spiObj->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0)); // Can be upto 10MHz
	
		digitalWrite(devId, LOW);

		Bsec::spiObj->transfer(regAddr); // Write the register address, ignore the return
		for (uint16_t i = 0; i < length; i++)
			regData[i] = Bsec::spiObj->transfer(regData[i]);

		digitalWrite(devId, HIGH);
		Bsec::spiObj->endTransaction();
	} else {
		rslt = -1;
	}

	return rslt;;
}