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
 * @file	bsec.h
 * @date	31 Jan 2018
 * @version	1.0
 *
 */

#ifndef BSEC_CLASS_H
#define BSEC_CLASS_H

/* Includes */
#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"
#include "bsec_datatypes.h"
#include "bsec_interface.h"
#include "bme680.h"

/* BSEC class definition */
class Bsec
{
public:
	/* Public variables */
	bsec_version_t version;		// Stores the version of the BSEC algorithm
	int64_t nextCall;			// Stores the time when the algorithm has to be called next in ms
	int8_t bme680Status;		// Placeholder for the BME680 driver's error codes
	bsec_library_return_t status;
	float iaqEstimate, rawTemperature, pressure, rawHumidity, gasResistance, stabStatus, runInStatus, temperature, humidity,
	      staticIaq, co2Equivalent, breathVocEquivalent, compGasValue, gasPercentage;
	uint8_t iaqAccuracy, staticIaqAccuracy, co2Accuracy, breathVocAccuracy, compGasAccuracy, gasPercentageAcccuracy;
	int64_t outputTimestamp;	// Timestamp in ms of the output
	static TwoWire *wireObj;
	static SPIClass *spiObj;

	/* Public APIs */
	/**
	 * @brief Constructor
	 */
	Bsec();

	/**
	 * @brief Function to initialize the BSEC library and the BME680 sensor
	 * @param devId		: Device identifier parameter for the read/write interface functions
	 * @param intf		: Physical communication interface
	 * @param read		: Pointer to the read function
	 * @param write		: Pointer to the write function
	 * @param idleTask	: Pointer to the idling task
	 */
	void begin(uint8_t devId, enum bme680_intf intf, bme680_com_fptr_t read, bme680_com_fptr_t write, bme680_delay_fptr_t idleTask);
	
	/**
	 * @brief Function to initialize the BSEC library and the BME680 sensor
	 * @param i2cAddr	: I2C address
	 * @param i2c		: Pointer to the TwoWire object
	 */
	void begin(uint8_t i2cAddr, TwoWire &i2c);
	
	/**
	 * @brief Function to initialize the BSEC library and the BME680 sensor
	 * @param chipSelect	: SPI chip select
	 * @param spi			: Pointer to the SPIClass object
	 */
	void begin(uint8_t chipSelect, SPIClass &spi);

	/**
	 * @brief Function that sets the desired sensors and the sample rates
	 * @param sensorList	: The list of output sensors
	 * @param nSensors		: Number of outputs requested
	 * @param sampleRate	: The sample rate of requested sensors
	 */
	void updateSubscription(bsec_virtual_sensor_t sensorList[], uint8_t nSensors, float sampleRate = BSEC_SAMPLE_RATE_ULP);

	/**
	 * @brief Callback from the user to trigger reading of data from the BME680, process and store outputs
	 * @return true if there are new outputs. false otherwise
	 */
	bool run(void);

	/**
	 * @brief Function to get the state of the algorithm to save to non-volatile memory
	 * @param state			: Pointer to a memory location that contains the state
	 */
	void getState(uint8_t *state);

	/**
	 * @brief Function to set the state of the algorithm from non-volatile memory
	 * @param state			: Pointer to a memory location that contains the state
	 */
	void setState(uint8_t *state);

	/**
	 * @brief Function to set the configuration of the algorithm from memory
	 * @param state			: Pointer to a memory location that contains the configuration
	 */
	void setConfig(const uint8_t *config);

	/**
	 * @brief Function to set the temperature offset
	 * @param tempOffset	: Temperature offset in degree Celsius
	 */
	void setTemperatureOffset(float tempOffset)
	{
		_tempOffset = tempOffset;
	}
	
		
	/**
	 * @brief Function to calculate an int64_t timestamp in milliseconds
	 */
	int64_t getTimeMs(void);
	
	/**
	* @brief Task that delays for a ms period of time
	* @param period	: Period of time in ms
	*/
	static void delay_ms(uint32_t period);

	/**
	* @brief Callback function for reading registers over I2C
	* @param devId		: Library agnostic parameter to identify the device to communicate with
	* @param regAddr	: Register address
	* @param regData	: Pointer to the array containing the data to be read
	* @param length	: Length of the array of data
	* @return	Zero for success, non-zero otherwise
	*/
	static int8_t i2cRead(uint8_t devId, uint8_t regAddr, uint8_t *regData, uint16_t length);

	/**
	* @brief Callback function for writing registers over I2C
	* @param devId		: Library agnostic parameter to identify the device to communicate with
	* @param regAddr	: Register address
	* @param regData	: Pointer to the array containing the data to be written
	* @param length	: Length of the array of data
	* @return	Zero for success, non-zero otherwise
	*/
	static int8_t i2cWrite(uint8_t devId, uint8_t regAddr, uint8_t *regData, uint16_t length);

	/**
	* @brief Callback function for reading and writing registers over SPI
	* @param devId		: Library agnostic parameter to identify the device to communicate with
	* @param regAddr	: Register address
	* @param regData	: Pointer to the array containing the data to be read or written
	* @param length	: Length of the array of data
	* @return	Zero for success, non-zero otherwise
	*/
	static int8_t spiTransfer(uint8_t devId, uint8_t regAddr, uint8_t *regData, uint16_t length);

private:
	/* Private variables */
	struct bme680_dev _bme680;
	struct bme680_field_data _data;
	float _tempOffset;
	// Global variables to help create a millisecond timestamp that doesn't overflow every 51 days.
	// If it overflows, it will have a negative value. Something that should never happen.
	uint32_t millisOverflowCounter;
	uint32_t lastTime;

	/* Private APIs */
	/**
	 * @brief Get the version of the BSEC library
	 */
	void getVersion(void);

	/**
	 * @brief Read data from the BME680 and process it
	 * @param currTimeNs: Current time in ns
	 * @param bme680Settings: BME680 sensor's settings
	 * @return true if there are new outputs. false otherwise
	 */
	bool readProcessData(int64_t currTimeNs, bsec_bme_settings_t bme680Settings);

	/**
	 * @brief Set the BME680 sensor's configuration
	 * @param bme680Settings: Settings to configure the BME680
	 * @return BME680 return code. BME680_OK for success, failure otherwise
	 */
	int8_t setBme680Config(bsec_bme_settings_t bme680Settings);

	/**
	 * @brief Common code for the begin function
	 */
	void beginCommon(void);

	/**
	 * @brief Function to zero the outputs
	 */
	void zeroOutputs(void);
};

#endif
