#include <CayenneLPP.h>
#include <stdio.h>  // printf y fgets
#include <string.h> // strlen
#include <ctype.h>  // toupper y isdigit
#include <math.h>   // pow
#include "sensorcayenne.h"
#include <Arduino.h> 

#define LONGITUD_MAXIMA 1000
#define BASE 16 

#define num_sensores 12 //------------------------------------------------------------------------------------------------------------------------------
unsigned char sensor_type[num_sensores];

char raw_in_char[200];
uint8_t raw_in_bytes[200];

//Contadores
int cont_bytes = 1; //Nos colocamos en el primer prefijo (saltamos el canal)
int i = 0;

//Definimos las longitudes de los datos que nos llegan por cada sensor
#define length_digital_input 1
#define length_digital_output 1
#define length_analog_input 2
#define length_analog_output 2
#define length_illuminance 2
#define length_presence 1
#define length_temperature 2
#define length_humidity 1
#define length_accelerometer_x 2
#define length_accelerometer_y 2
#define length_accelerometer_z 2
#define length_barometer 2
#define length_gyrometer_x 2
#define length_gyrometer_y 2
#define length_gyrometer_z 2
#define length_gps_x 3
#define length_gps_y 3
#define length_gps_z 3

//Definimos los distintos arrays, uno por cada tipo de sensor (de momento tamaño payload*5)
unsigned char array_digital_input [5]; 
unsigned char array_digital_output [5]; 
unsigned char array_analog_input [10];
unsigned char array_analog_output [10];
unsigned char array_illuminance [10];
unsigned char array_presence [5];
unsigned char array_temperature [10];
unsigned char array_humidity [5];
unsigned char array_accelerometer [30];
unsigned char array_barometer [10];
unsigned char array_gyrometer [30];
unsigned char array_gps [45];

//Definimos los distintos arrays, para almacenaar solo el valor del sensor que estamos recibiendo, luego se va a sobreescribir
uint8_t new_digital_input [length_digital_input]; 
uint8_t new_digital_output [length_digital_output]; 
uint8_t new_analog_input [length_analog_input];
uint8_t new_analog_output [length_analog_output];
uint8_t new_illuminance [length_illuminance];
uint8_t new_presence [length_presence];
uint8_t new_temperature [length_temperature];
uint8_t new_humidity [length_humidity];
uint8_t new_accelerometer_x [length_accelerometer_x];
uint8_t new_accelerometer_y [length_accelerometer_y];
uint8_t new_accelerometer_z [length_accelerometer_z];
uint8_t new_barometer [length_barometer];
uint8_t new_gyrometer_x [length_gyrometer_x];
uint8_t new_gyrometer_y [length_gyrometer_y];
uint8_t new_gyrometer_z [length_gyrometer_z];
uint8_t new_gps_x [length_gps_x];
uint8_t new_gps_y [length_gps_y];
uint8_t new_gps_z [length_gps_z];

//Definimos las variables de 16 bits donde guardaremos la conversión
uint16_t new_digital_input_16; 
uint16_t new_digital_output_16; 
int16_t new_analog_input_16;
int16_t new_analog_output_16;
uint16_t new_illuminance_16;
uint16_t new_presence_16;
int16_t new_temperature_16;
uint16_t new_humidity_16;
int16_t new_accelerometer_x_16;
int16_t new_accelerometer_y_16;
int16_t new_accelerometer_z_16;
uint16_t new_barometer_16;
int16_t new_gyrometer_x_16;
int16_t new_gyrometer_y_16;
int16_t new_gyrometer_z_16;
int32_t new_gps_x_32;
int32_t new_gps_y_32;
int32_t new_gps_z_32;

//Definimos los distintos arrays, uno por cada tipo de sensor (tamaño de 10 float)
float array_digital_input_DEC [10]; 
float array_digital_output_DEC [10]; 
float array_analog_input_DEC [10];
float array_analog_output_DEC [10];
float array_illuminance_DEC [10];
float array_presence_DEC [10];
float array_temperature_DEC [10];
float array_humidity_DEC [10];
float array_accelerometer_DEC [10];
float array_barometer_DEC [10];
float array_gyrometer_DEC [10];
float array_gps_DEC [10];

//Variables para los nuevos datos convertidos a decimal
float new_digital_input_DEC = 0; 
float new_digital_output_DEC = 0; 
float new_analog_input_DEC = 0;
float new_analog_output_DEC = 0;
float new_illuminance_DEC = 0;
float new_presence_DEC = 0;
float new_temperature_DEC = 0;
float new_humidity_DEC = 0;
float new_accelerometer_x_DEC = 0;
float new_accelerometer_y_DEC = 0;
float new_accelerometer_z_DEC = 0;
float new_barometer_DEC = 0;
float new_gyrometer_x_DEC = 0;
float new_gyrometer_y_DEC = 0;
float new_gyrometer_z_DEC = 0;
float new_gps_x_DEC = 0;
float new_gps_y_DEC = 0;
float new_gps_z_DEC = 0;

//Variables para operaciones
//NOTA:Pongo valor alto en los valores mínimos para que se coja la nueva como mínimo
float media_digital_input = 0;
float digital_input_max = 0;
float digital_input_min = 1000.0;

float media_digital_output = 0;
float digital_output_max = 0;
float digital_output_min = 1000.0;

float media_analog_input = 0;
float analog_input_max = 0;
float analog_input_min = 1000.0;

float media_analog_output = 0;
float analog_output_max = 0.0;
float analog_output_min = 1000.0;

float media_illuminance = 0;
float illuminance_max = 0.0;
float illuminance_min = 1000.0;

float media_presence = 0;
float presence_max = 0.0;
float presence_min = 1000.0;

float media_temperature = 0;
float temperature_max = 0;
float temperature_min = 1000.0;

float media_humidity = 0;
float humidity_max = 0;
float humidity_min = 1000.0;

float media_barometer = 0;
float barometer_max = 0;
float barometer_min = 1000.0;

//__________________________________________________________________________________________________________________________________________________________________


void setup_serial()
{
/***************************************************************************
 *  Setup para comunicación serie entre ESP8266 y ESP32
 ***************************************************************************/
    // inicialización puerto serie principal conectado a USB
    Serial.begin(115200); 
    Serial.println("Inicializando Serial2");
    // ESP32 puede remapear la UART a los pines que se quiera
    Serial2.begin(9600, SERIAL_8N1, 33, 32);   // Uso GPIO33 para RX
}


/***************************************************************************
 *  Función que lee datos del puerto serial (ESP8266) e imprime por pantalla
 ***************************************************************************/
void read_serial(){
  if(Serial2.available()) {
    byte num_bytes_leidos = Serial2.readBytesUntil('#', raw_in_char, sizeof(raw_in_char));
    raw_in_char[num_bytes_leidos] = 0;   // para eliminar el último caracter #

    //Convertimos el array que nos llega en char a un array de bytes
    for(int k = 0; k < sizeof(raw_in_char); k++) raw_in_bytes[k] = (uint8_t)raw_in_char[k];

    Serial.print("Longitud de raw_in_bytes: ");
    Serial.println(sizeof(raw_in_bytes));

  }
}

//__________________________________________________________________________________________________________________________________________________________________


/***************************************************************************
 *  Función para guardar los datos brutos en arrays según el tipo de sensor
 ***************************************************************************/ 
 void save_data(){

  int size_data = sizeof(raw_in_bytes);
  cont_bytes = 1;
  for (int i = 0; i < num_sensores; i++){                                       //Bucle general que se va a repetir tantas veces como número de sensores tengamos: ¿Cómo sabemos el número de sensores? Hay que ponerlo de manera manual
    if(cont_bytes<size_data){                                                   //si j<size_data, es decir, si no hemos terminado el array raw_in_bytes
      sensor_type[i] = raw_in_bytes[cont_bytes];                                //Guardamos el prefijo en sensor_type
      switch (sensor_type[i]){                                                  //Según el tipo de sensor entraremos en un case u otro
        
        case 00: {//digital input - payload = 1 byte //00-00
        //case 00: { 
          Serial.println("/***************************************************************************");
          Serial.println(" *  DIGITAL INPUT");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_digital_input();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN DIGITAL INPUT");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;          
        }
        
        case 01: { //digital output - payload = 1 byte //01-01
        //case 01: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  DIGITAL OUTPUT");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_digital_output();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN DIGITAL OUTPUT");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;                
        }
        
        case 02: { //analog input - payload = 2 bytes //02-02
        //case 02: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  ANALOG INPUT");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_analog_input();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN ANALOG INPUT");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;         
        }

        case 03: { //analog output - payload = 2 byte //03-03
        //case 03: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  ANALOG OUTPUT");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_analog_output();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN ANALOG OUTPUT");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break; 
        }

        case 101: { //illuminance sensor - payload = 2 bytes //101-65
        //case 65: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  ILLUMINANCE");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_illuminance();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN ILLUMINANCE");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;        
        }
        
        case 102: { //presence sensor - payload = 1 byte //102-66
        //case 66: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  PRESENCE");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_presence();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN PRESENCE");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;        
        }

        case 103: { //temperature sensor - payload = 2 bytes //103-67 
        //case 67: {         
          Serial.println("/***************************************************************************");
          Serial.println(" *  TEMPERATURA");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_temperature();   
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN TEMPERATURA");
          Serial.println("***************************************************************************/");        
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;        
        }

        case 104: { //humidity sensor - payload = 1 byte //104-68
        //case 68: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  HUMEDAD");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_humidity();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN HUMEDAD");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;        
        }

        case 113: { //accelerometer - payload = 6 bytes //113-71
        //case 71: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  ACCELEROMETER");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_accelerometer();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN ACCELEROMETER");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;       
        }

        case 115: { //barometer - payload = 2 bytes //115-73
        //case 73: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  PRESIÓN BAROMÉTRICA");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_barometer();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN PRESIÓN BAROMÉTRICA");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;       
        }
        
        case 134: { //gyrometer - payload = 6 bytes //134-86
        //case 86: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  GYROMETER");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_gyrometer();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN GYROMETER");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;        
        }

        case 136: { //GPS location - payload = 9 bytes //136-88
        //case 88: {
          Serial.println("/***************************************************************************");
          Serial.println(" *  GPS");
          Serial.println("***************************************************************************/"); 
          cont_bytes++; //Nos ponemos en el principio del payload
          calculate_gps();    
          Serial.println("/***************************************************************************");
          Serial.println(" *  FIN GPS");
          Serial.println("***************************************************************************/");       
          cont_bytes++;
          Serial.print("cont_bytes: "); Serial.println(cont_bytes);
          break;        
        }
      }
    }
    else
      cont_bytes=0;
  }
}


//__________________________________________________________________________________________________________________________________________________________________

/***************************************************************************
 *  Funciones para digital_input
 ***************************************************************************/
void calculate_digital_input(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_digital_input; k++){
    new_digital_input[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_digital_input[k]);
  }
  Serial.println(); 
//___________________________________________________________________________________________________ 
  new_digital_input_DEC = (float) new_digital_input[0];
  Serial.print("Valor de digital_input en decimal: ");
  Serial.println(new_digital_input_DEC);
//___________________________________________________________________________________________________
  if(new_digital_input_DEC > digital_input_max){
    digital_input_max = new_digital_input_DEC;
  }
  if(new_digital_input_DEC < digital_input_min){
    digital_input_min = new_digital_input_DEC;
  }
//________Media LIFO_________________________________________________________________________________
for(int i = 0; i < 10; i++){
    array_digital_input_DEC[i] = array_digital_input_DEC[i+1];  
    media_digital_input += array_digital_input_DEC[i];          
    Serial.print(array_digital_input_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_digital_input_DEC[9] = new_digital_input_DEC;           
  media_digital_input += array_digital_input_DEC[9];            
  media_digital_input = media_digital_input/10;
  if (media_digital_input > 0.5) 
    media_digital_input = 1;
  else 
    media_digital_input = 0; 
//___________________________________________________________________________________________________            
  Serial.print("Valor de digital_input máxima: ");
  Serial.println(digital_input_max);

  Serial.print("Valor de digital_input mínima: ");
  Serial.println(digital_input_min);
  
  Serial.print("Valor de digital_input media: ");
  Serial.println(media_digital_input);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para digital_output
 ***************************************************************************/
void calculate_digital_output(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_digital_output; k++){
    new_digital_output[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_digital_output[k]);
  }
  Serial.println();
//___________________________________________________________________________________________________
  new_digital_output_DEC = (float) new_digital_output[0]; 
  Serial.print("Valor de digital_output en decimal: ");
  Serial.println(new_digital_output_DEC);
//___________________________________________________________________________________________________
  if(new_digital_output_DEC > digital_output_max){
    digital_output_max = new_digital_output_DEC;
  }
  if(new_digital_output_DEC < digital_output_min){
    digital_output_min = new_digital_output_DEC;
  }
//________Media LIFO_________________________________________________________________________________
  for(int i = 0; i < 10; i++){
    array_digital_output_DEC[i] = array_digital_output_DEC[i+1];  
    media_digital_output += array_digital_output_DEC[i];          
    Serial.print(array_digital_output_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_digital_output_DEC[9] = new_digital_output_DEC;           
  media_digital_output += array_digital_output_DEC[9];            
  media_digital_output = media_digital_output/10;
  if (media_digital_output > 0.5)
    media_digital_output = 1;
  else
    media_digital_output = 0;                
//___________________________________________________________________________________________________
  Serial.print("Valor de digital_output máxima: ");
  Serial.println(digital_output_max);

  Serial.print("Valor de digital_output mínima: ");
  Serial.println(digital_output_min);
  
  Serial.print("Valor de digital_output media: ");
  Serial.println(media_digital_output);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para analog_input
 ***************************************************************************/
void calculate_analog_input(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_analog_input; k++){
    new_analog_input[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_analog_input[k]);
  }
  Serial.println();
//___________________________________________________________________________________________________
  new_analog_input_16 = convertFrom8to16(new_analog_input);
  new_analog_input_DEC = (float) new_analog_input_16;
  
  new_analog_input_DEC = new_analog_input_DEC / 100;
  Serial.print("Valor de la analog_input en decimal: ");
  Serial.println(new_analog_input_DEC);
//___________________________________________________________________________________________________
  if(new_analog_input_DEC > analog_input_max){
    analog_input_max = new_analog_input_DEC;
  }
  if(new_analog_input_DEC < analog_input_min){
    analog_input_min = new_analog_input_DEC;
  }
//________Media LIFO_________________________________________________________________________________
for(int i = 0; i < 10; i++){
    array_analog_input_DEC[i] = array_analog_input_DEC[i+1]; 
    media_analog_input += array_analog_input_DEC[i];         
    Serial.print(array_analog_input_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_analog_input_DEC[9] = new_analog_input_DEC;           
  media_analog_input += array_analog_input_DEC[9];            
  media_analog_input = media_analog_input/10; 
//___________________________________________________________________________________________________
  Serial.print("Valor de analog_input máxima: ");
  Serial.println(analog_input_max);

  Serial.print("Valor de analog_input mínima: ");
  Serial.println(analog_input_min);
  
  Serial.print("Valor de analog_input media: ");
  Serial.println(media_analog_input);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para analog_output
 ***************************************************************************/
void calculate_analog_output(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_analog_output; k++){
    new_analog_output[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_analog_output[k]);
  }
  Serial.println(); 
//___________________________________________________________________________________________________ 
  new_analog_output_16 = convertFrom8to16(new_analog_output);
  new_analog_output_DEC = (float) new_analog_output_16;
  
  new_analog_output_DEC = new_analog_output_DEC / 100;
  Serial.print("Valor de la analog_output en decimal: ");
  Serial.println(new_analog_output_DEC);
//___________________________________________________________________________________________________
  if(new_analog_output_DEC > analog_output_max){
    analog_output_max = new_analog_output_DEC;
  }
  if(new_analog_output_DEC < analog_output_min){
    analog_output_min = new_analog_output_DEC;
  }
//________Media LIFO_________________________________________________________________________________
  for(int i = 0; i < 10; i++){
    array_analog_output_DEC[i] = array_analog_output_DEC[i+1]; 
    media_analog_output += array_analog_output_DEC[i];         
    Serial.print(array_analog_output_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_analog_output_DEC[9] = new_analog_output_DEC;          
  media_analog_output += array_analog_output_DEC[9];           
  media_analog_output = media_analog_output/10; 
//___________________________________________________________________________________________________
  Serial.print("Valor de analog_output máxima: ");
  Serial.println(analog_output_max);

  Serial.print("Valor de analog_output mínima: ");
  Serial.println(analog_output_min);
  
  Serial.print("Valor de analog_output media: ");
  Serial.println(media_analog_output);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para illuminance
 ***************************************************************************/
void calculate_illuminance(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_illuminance; k++){
    new_illuminance[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_illuminance[k]);
  }
  Serial.println(); 
//___________________________________________________________________________________________________
  new_illuminance_16 = convertFrom8to16(new_illuminance);
  new_illuminance_DEC = (float) new_illuminance_16;
  Serial.print("Valor de la iluminancia en decimal: ");
  Serial.println(new_illuminance_DEC);
//___________________________________________________________________________________________________
  if(new_illuminance_DEC > illuminance_max){
    illuminance_max = new_illuminance_DEC;
  }
  if(new_illuminance_DEC < illuminance_min){
    illuminance_min = new_illuminance_DEC;
  }
//________Media LIFO_________________________________________________________________________________
  for(int i = 0; i < 10; i++){
    array_illuminance_DEC[i] = array_illuminance_DEC[i+1]; 
    media_illuminance += array_illuminance_DEC[i];         
    Serial.print(array_illuminance_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_illuminance_DEC[9] = new_illuminance_DEC;          
  media_illuminance += array_illuminance_DEC[9];           
  media_illuminance = media_illuminance/10;    
//___________________________________________________________________________________________________          
  Serial.print("Valor de la iluminancia máxima: ");
  Serial.println(illuminance_max);

  Serial.print("Valor de la iluminancia mínima: ");
  Serial.println(illuminance_min);
  
  Serial.print("Valor de la iluminancia media: ");
  Serial.println(media_illuminance);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para presence
 ***************************************************************************/
void calculate_presence(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_presence; k++){
    new_presence[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_presence[k]);
  }
  Serial.println();
//___________________________________________________________________________________________________
  new_presence_DEC = (float) new_presence[0]; 
  new_presence_DEC = new_presence_DEC / 10;
  Serial.print("Valor de la presencia en decimal: ");
  Serial.println(new_presence_DEC);
//___________________________________________________________________________________________________
  if(new_presence_DEC > presence_max){
    presence_max = new_presence_DEC;
  }
  if(new_presence_DEC < presence_min){
    presence_min = new_presence_DEC;
  }
//________Media LIFO_________________________________________________________________________________
  for(int i = 0; i < 10; i++){
    array_presence_DEC[i] = array_presence_DEC[i+1];  
    media_presence += array_presence_DEC[i];          
    Serial.print(array_presence_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_presence_DEC[9] = new_presence_DEC;           
  media_presence += array_presence_DEC[9];            
  media_presence = media_presence/10;                 
//___________________________________________________________________________________________________
  Serial.print("Valor de la presencia máxima: ");
  Serial.println(presence_max);

  Serial.print("Valor de la presencia mínima: ");
  Serial.println(presence_min);
  
  Serial.print("Valor de la presencia media: ");
  Serial.println(media_presence);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para temperature
 ***************************************************************************/
void calculate_temperature(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_temperature; k++){
    new_temperature[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_temperature[k]);
  }
  Serial.println();
//___________________________________________________________________________________________________
  new_temperature_16 = convertFrom8to16(new_temperature);
  new_temperature_DEC = (float) new_temperature_16; 
  new_temperature_DEC = new_temperature_DEC / 10;
  Serial.print("Valor de la temperatura en decimal: ");
  Serial.println(new_temperature_DEC);
//___________________________________________________________________________________________________  
  if(new_temperature_DEC > temperature_max){
    temperature_max = new_temperature_DEC;
  }
  if(new_temperature_DEC < temperature_min){
    temperature_min = new_temperature_DEC;
  }
//________Media LIFO_________________________________________________________________________________
  for(int i = 0; i < 10; i++){
    array_temperature_DEC[i] = array_temperature_DEC[i+1];  
    media_temperature += array_temperature_DEC[i];          
    Serial.print(array_temperature_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_temperature_DEC[9] = new_temperature_DEC;           
  media_temperature += array_temperature_DEC[9];            
  media_temperature = media_temperature/10;                
//___________________________________________________________________________________________________
  Serial.print("Valor de la temperatura máxima: ");
  Serial.println(temperature_max);

  Serial.print("Valor de la temperatura mínima: ");
  Serial.println(temperature_min);
  
  Serial.print("Valor de la temperatura media: ");
  Serial.println(media_temperature);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para humidity
 ***************************************************************************/
void calculate_humidity(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_humidity; k++){
    new_humidity[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_humidity[k]);
  }
  Serial.println();
//___________________________________________________________________________________________________  
  new_humidity_DEC = (float) new_humidity[0]; 
  new_humidity_DEC = new_humidity_DEC / 2;
  Serial.print("Valor de la humedad en decimal: ");
  Serial.print(new_humidity_DEC); 
  Serial.println("%");
//___________________________________________________________________________________________________
  if(new_humidity_DEC > humidity_max){
    humidity_max = new_humidity_DEC;
  }
  if(new_humidity_DEC < humidity_min){
    humidity_min = new_humidity_DEC;
  }
//________Media LIFO_________________________________________________________________________________
  for(int i = 0; i < 10; i++){
    array_humidity_DEC[i] = array_humidity_DEC[i+1]; 
    media_humidity += array_humidity_DEC[i];         
    Serial.print(array_humidity_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_humidity_DEC[9] = new_humidity_DEC;          
  media_humidity += array_humidity_DEC[9];           
  media_humidity = media_humidity/10; 
//___________________________________________________________________________________________________
  Serial.print("Valor de la humedad máxima: ");
  Serial.println(humidity_max);

  Serial.print("Valor de la humedad mínima: ");
  Serial.println(humidity_min);
  
  Serial.print("Valor de la humedad media: ");
  Serial.println(media_humidity);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para accelerometer
 ***************************************************************************/
void calculate_accelerometer(){
  for(int k = 0; k<length_accelerometer_x; k++){
    new_accelerometer_x[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_accelerometer_x[k]);
    Serial.print(" ");
  }
  Serial.println(); 
  Serial.print("Cont_bytes: ");
  Serial.println(cont_bytes);

  for(int k = 0; k<length_accelerometer_y; k++){
    new_accelerometer_y[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_accelerometer_y[k]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Cont_bytes: ");
  Serial.println(cont_bytes);

  for(int k = 0; k<length_accelerometer_z; k++){
    new_accelerometer_z[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_accelerometer_z[k]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Cont_bytes: ");
  Serial.println(cont_bytes);
//___________________________________________________________________________________________________ 
  //coordenada X 
  new_accelerometer_x_16 = convertFrom8to16(new_accelerometer_x);
  new_accelerometer_x_DEC = (float) (new_accelerometer_x_16);
  new_accelerometer_x_DEC = new_accelerometer_x_DEC / 1000;
  Serial.print("Coodenada x de la aceleración en decimal: ");
  Serial.print(new_accelerometer_x_DEC);
  Serial.println("G");
  
  //coordenada Y 
  new_accelerometer_y_16 = convertFrom8to16(new_accelerometer_y);
  new_accelerometer_y_DEC = (float) (new_accelerometer_y_16);
  new_accelerometer_y_DEC = new_accelerometer_y_DEC / 1000;
  Serial.print("Coodenada y de la aceleración en decimal: ");
  Serial.print(new_accelerometer_y_DEC);
  Serial.println("G");
  
  //coordenada Z 
  new_accelerometer_z_16 = convertFrom8to16(new_accelerometer_z);
  new_accelerometer_z_DEC = (float) (new_accelerometer_z_16);
  new_accelerometer_z_DEC = new_accelerometer_z_DEC / 1000;
  Serial.print("Coodenada z de la aceleración en decimal: ");
  Serial.print(new_accelerometer_z_DEC);
  Serial.println("G");
}

/***************************************************************************
 *  Funciones para presión barométrica
 ***************************************************************************/
void calculate_barometer(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_barometer; k++){
    new_barometer[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_barometer[k]);
  }
  Serial.println();
//___________________________________________________________________________________________________  
  new_barometer_16 = convertFrom8to16(new_barometer);
  new_barometer_DEC = (float) new_barometer_16;   
  new_barometer_DEC = new_barometer_DEC / 10;
  Serial.print("Valor de la presión barométrica en decimal: ");
  Serial.println(new_barometer_DEC);
//___________________________________________________________________________________________________ 
  if(new_barometer_DEC > barometer_max){
    barometer_max = new_barometer_DEC;
  }
  if(new_barometer_DEC < barometer_min){
    barometer_min = new_barometer_DEC;
  }
//________Media LIFO_________________________________________________________________________________
  for(int i = 0; i < 10; i++){
    array_barometer_DEC[i] = array_barometer_DEC[i+1];  
    media_barometer += array_barometer_DEC[i];          
    Serial.print(array_barometer_DEC[i]);
    Serial.print(" ");
  }
  Serial.println();
  array_barometer_DEC[9] = new_barometer_DEC;           
  media_barometer += array_barometer_DEC[9];            
  media_barometer = media_barometer/10;                
//___________________________________________________________________________________________________
  Serial.print("Valor de la presión barométrica máxima: ");
  Serial.println(barometer_max);

  Serial.print("Valor de la presión barométrica mínima: ");
  Serial.println(barometer_min);
  
  Serial.print("Valor de la presión barométrica media: ");
  Serial.println(media_barometer);
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para girómetro
 ***************************************************************************/
void calculate_gyrometer(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_gyrometer_x; k++){
    new_gyrometer_x[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_gyrometer_x[k]);
  }
  Serial.println();

  for(int k = 0; k<length_gyrometer_y; k++){
    new_gyrometer_y[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_gyrometer_y[k]);
  }
  Serial.println();

  for(int k = 0; k<length_gyrometer_z; k++){
    new_gyrometer_z[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_gyrometer_z[k]);
  }
  Serial.println();
//___________________________________________________________________________________________________ 
  //coordenada X 
  new_gyrometer_x_16 = convertFrom8to16(new_gyrometer_x);
  new_gyrometer_x_DEC = (float) (new_gyrometer_x_16);
  new_gyrometer_x_DEC = new_gyrometer_x_DEC / 100;
  Serial.print("Coodenada x del girómetro en decimal: ");
  Serial.print(new_gyrometer_x_DEC);
  Serial.println("º/s");
  
  //coordenada Y 
  new_gyrometer_y_16 = convertFrom8to16(new_gyrometer_y);
  new_gyrometer_y_DEC = (float) (new_gyrometer_y_16);
  new_gyrometer_y_DEC = new_gyrometer_y_DEC / 100;
  Serial.print("Coodenada y del girómetro en decimal: ");
  Serial.print(new_gyrometer_y_DEC);
  Serial.println("º/s");
  
  //coordenada Z 
  new_gyrometer_z_16 = convertFrom8to16(new_gyrometer_z);
  new_gyrometer_z_DEC = (float) (new_gyrometer_z_16);
  new_gyrometer_z_DEC = new_gyrometer_z_DEC / 100;
  Serial.print("Coodenada z del girómetro en decimal: ");
  Serial.print(new_gyrometer_z_DEC);
  Serial.println("º/s");
//___________________________________________________________________________________________________
}

/***************************************************************************
 *  Funciones para GPS
 ***************************************************************************/
void calculate_gps(){
//___________________________________________________________________________________________________
  for(int k = 0; k<length_gps_x; k++){
    new_gps_x[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_gps_x[k]);
  }
  Serial.println();

  for(int k = 0; k<length_gps_y; k++){
    new_gps_y[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_gps_y[k]);
  }
  Serial.println();

  for(int k = 0; k<length_gps_z; k++){
    new_gps_z[k] = raw_in_bytes[cont_bytes];
    cont_bytes++;
    Serial.print(new_gps_z[k]);
  }
  Serial.println();
//___________________________________________________________________________________________________  
  //latitud 
  new_gps_x_32 = convertFrom8to32(new_gps_x);
  new_gps_x_DEC = (float) (new_gps_x_32);
  new_gps_x_DEC = new_gps_x_DEC / 10000;
  Serial.print("Latitud de GPS en decimal: ");
  Serial.print(new_gps_x_DEC);
  Serial.println("º");
  
  //longitud
  new_gps_y_32 = convertFrom8to32(new_gps_y);
  new_gps_y_DEC = (float) (new_gps_y_32);
  new_gps_y_DEC = new_gps_y_DEC / 10000;
  Serial.print("Longitud del GPS en decimal: ");
  Serial.print(new_gps_y_DEC);
  Serial.println("º");
  
  //altitud 
  new_gps_z_32 = convertFrom8to32(new_gps_z);
  new_gps_z_DEC = (float) (new_gps_z_32);
  new_gps_z_DEC = new_gps_z_DEC / 100;
  Serial.print("Altitud del GPS en decimal: ");
  Serial.print(new_gps_z_DEC);
  Serial.println("m"); 
//___________________________________________________________________________________________________
}


/***************************************************************************
 *  Conversión de 2 uint8_t a 1 uint16_t
 *  https://www.badprog.com/c-type-converting-two-uint8-t-words-into-one-of-uint16-t-and-one-of-uint16-t-into-two-of-uint8-t
 ***************************************************************************/
int16_t convertFrom8to16 (uint8_t new_data []){
  uint16_t u16 = 0x0000;
  u16 = new_data[0];
  u16 = u16 << 8;
  u16 |= new_data[1];
  return u16;
}

/***************************************************************************
 *  Conversión de 4 uint8_t a 1 uint32_t
 ***************************************************************************/
int32_t convertFrom8to32(uint8_t new_data []) {
  int32_t u32 = (new_data[0] << 16) + (new_data[1] << 8) + new_data[2];
  //Comprobar si hay un 1 en la posición que indica número negativo
  uint32_t u32_test = 0x00800000; // '00000000100000000000000000000000';
  boolean check_negative = u32 & u32_test;
  if (check_negative) 
    //Si es negativo, rellenamos con 1s 
    u32 = u32 | 0xFF000000;   
  return u32;
}
