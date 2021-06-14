#ifndef _sensorcayenne_h_
#define _sensorcayenne_h_


//Main functions
void setup_serial();
void read_serial();
void save_data();

//Sensor calculations
void calculate_digital_input();
void calculate_digital_output();
void calculate_analog_input();
void calculate_analog_output();
void calculate_illuminance();
void calculate_presence();
void calculate_temperature();
void calculate_humidity();
void calculate_accelerometer();
void calculate_barometer();
void calculate_gyrometer();
void calculate_gps();

//Conversions
int16_t convertFrom8to16 (uint8_t new_data []);
int32_t convertFrom8to32(uint8_t new_data []);

#endif
