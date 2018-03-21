/*
 
Copyright  2018 Oliver Brandmueller <ob@sysadm.in>
Copyright  2018 Klaus Wilting <verkehrsrot@arcor.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

NOTICE: 
Parts of the source files in this repository are made available under different licenses.
Refer to LICENSE.txt file in repository for more details.

*/

// Basic Config
#include "main.h"
#include "globals.h"

// std::set for unified array functions
#include <set>

// OLED driver
#include <U8x8lib.h>

// LMIC-Arduino LoRaWAN Stack
#include "loraconf.h"
#include <lmic.h>
#include <hal/hal.h>

// WiFi Functions
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_spi_flash.h>
#include <esp32-hal-log.h> // we need this for ESP_LOGx on arduino framework

configData_t cfg; // struct holds current device configuration
osjob_t sendjob, initjob; // LMIC

// Initialize global variables
int macnum = 0, blenum = 0;
uint64_t uptimecounter = 0;
bool joinstate = false;

std::set<uint64_t, std::greater <uint64_t> > macs; // storage holds MAC frames

// this variable will be changed in the ISR, and read in main loop
static volatile bool ButtonTriggered = false;

// local Tag for logging
static const char *TAG = "paxcnt";
// Note: Log level control seems not working during runtime,
// so we need to switch loglevel by compiler build option in platformio.ini
#ifndef VERBOSE
int redirect_log(const char * fmt, va_list args) {
   //do nothing
   return 0;
}
#endif

// defined in configmanager.cpp
void eraseConfig(void);
void saveConfig(void);
void loadConfig(void);


/* begin LMIC specific parts ------------------------------------------------------------ */

// defined in lorawan.cpp
void gen_lora_deveui(uint8_t * pdeveui);
#ifdef VERBOSE
    void printKeys(void);
#endif // VERBOSE

// LMIC callback functions
void os_getArtEui (u1_t *buf) { memcpy(buf, APPEUI, 8);}
void os_getDevKey (u1_t *buf) { memcpy(buf, APPKEY, 16);}
#ifdef DEVEUI // if DEVEUI defined in loraconf.h use that and hardwire it in code ...
    void os_getDevEui (u1_t *buf) { memcpy(buf, DEVEUI, 8);}
#else // ... otherwise generate DEVEUI at runtime from devices's MAC
    void os_getDevEui (u1_t *buf) { gen_lora_deveui(buf);} 
#endif

// LMIC enhanced Pin mapping 
const lmic_pinmap lmic_pins = {
    .mosi = PIN_SPI_MOSI,
    .miso = PIN_SPI_MISO,
    .sck = PIN_SPI_SCK,
    .nss = PIN_SPI_SS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RST,
    .dio = {DIO0, DIO1, DIO2}
};

// LMIC functions
void onEvent(ev_t ev);
void do_send(osjob_t* j);

// LoRaWAN Initjob
static void lora_init (osjob_t* j) {
    // reset MAC state
    LMIC_reset();
    // This tells LMIC to make the receive windows bigger, in case your clock is 1% faster or slower.
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); 
    // start joining
    LMIC_startJoining();
}

// LMIC Task
void lorawan_loop(void * pvParameters) {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 ); // FreeRTOS check
    while(1) {
        os_runloop_once();
        vTaskDelay(10/portTICK_PERIOD_MS);
        yield();
    }    
}

/* end LMIC specific parts --------------------------------------------------------------- */



/* beginn hardware specific parts -------------------------------------------------------- */

#ifdef HAS_DISPLAY
    HAS_DISPLAY u8x8(OLED_RST, OLED_SCL, OLED_SDA);
#else
    U8X8_NULL u8x8;
#endif

#ifdef LOPY
    // defined in antenna.cpp
    void antenna_init (void);
    void antenna_select (antenna_type_t antenna_type);
#endif

#if defined BLECOUNTER
    void BLECount(void);
#else
    btStop();
#endif

void set_onboard_led(int st){
#ifdef HAS_LED
    switch (st) {
        case 1: digitalWrite(HAS_LED, HIGH); break;
        case 0: digitalWrite(HAS_LED, LOW); break;
    }
#endif
};

#ifdef HAS_BUTTON
    // Button Handling, board dependent -> perhaps to be moved to new hal.cpp
    // IRAM_ATTR necessary here, see https://github.com/espressif/arduino-esp32/issues/855
    void IRAM_ATTR isr_button_pressed(void) {
        ButtonTriggered++; }
#endif

/* end hardware specific parts -------------------------------------------------------- */


/* begin wifi specific parts ---------------------------------------------------------- */

// defined in wifisniffer.cpp
void wifi_sniffer_init(void);
void wifi_sniffer_set_channel(uint8_t channel);
void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

//WiFi Sniffer Task
void wifi_sniffer_loop(void * pvParameters) {

    configASSERT( ( ( uint32_t ) pvParameters ) == 1 ); // FreeRTOS check
    uint8_t channel = 1;
    int nloop=0, lorawait=0;

  	while (true) {
        nloop++;
		vTaskDelay(cfg.wifichancycle*10 / portTICK_PERIOD_MS);
        yield();
		wifi_sniffer_set_channel(channel);
		channel = (channel % WIFI_CHANNEL_MAX) + 1;
        
        // duration of one wifi scan loop reached? then send data and begin new scan cycle
        if( nloop >= ((100 / cfg.wifichancycle) * (cfg.wifiscancycle * 2)) ) {
            u8x8.setPowerSave(!cfg.screenon); // set display on if enabled
            nloop = 0; // reset wlan sniffing loop counter
            
            // execute BLE count if BLE function is enabled
            #ifdef BLECOUNTER
                if ( cfg.blescan )
                    BLECount();
            #endif
            
            // Prepare and execute LoRaWAN data upload
            u8x8.setCursor(0,4);
            u8x8.printf("MAC#: %4i", macnum);
            do_send(&sendjob); // send payload
            vTaskDelay(500/portTICK_PERIOD_MS);
            yield();

            // clear counter if not in cumulative counter mode
            if ( cfg.countermode != 1 ) {
                macs.erase(macs.begin(), macs.end()); // clear RAM
                macnum = 0;
                u8x8.clearLine(0); u8x8.clearLine(1); // clear Display counter
            }      

            // wait until payload is sent, while wifi scanning and mac counting task continues
            lorawait = 0;
            while(LMIC.opmode & OP_TXRXPEND) {
                if(!lorawait) u8x8.drawString(0,6,"LoRa wait       ");
                lorawait++;
                // in case sending really fails: reset and rejoin network
                if( (lorawait % MAXLORARETRY ) == 0) {
                    ESP_LOGI(TAG, "Payload not sent, trying reset and rejoin");
                    esp_restart();
                };
                vTaskDelay(1000/portTICK_PERIOD_MS);
                yield();
            }

            u8x8.clearLine(6);
	                    
            if (cfg.screenon && cfg.screensaver) vTaskDelay(2000/portTICK_PERIOD_MS); // pause for displaying results
            yield();
            u8x8.setPowerSave(1 && cfg.screensaver); // set display off if screensaver is enabled
        }
    }
}

/* end wifi specific parts ------------------------------------------------------------ */

// uptime counter 64bit to prevent millis() rollover after 49 days
uint64_t uptime() {
    static uint32_t low32, high32;
    uint32_t new_low32 = millis();
    if (new_low32 < low32) high32++;
    low32 = new_low32;
    return (uint64_t) high32 << 32 | low32;
}

// Print a key on display
void DisplayKey(const uint8_t * key, uint8_t len, bool lsb) {
  uint8_t start=lsb?len:0;
  uint8_t end = lsb?0:len;
  const uint8_t * p ;
  for (uint8_t i=0; i<len ; i++) {
    p = lsb ? key+len-i-1 : key+i;
    u8x8.printf("%02X", *p);
  }
  u8x8.printf("\n");
}

void init_display(const char *Productname, const char *Version) {
    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
#ifdef HAS_DISPLAY  
    uint8_t buf[32];
    u8x8.clear();
    u8x8.setFlipMode(0);
    u8x8.setInverseFont(1);
    u8x8.draw2x2String(0, 0, Productname);
    u8x8.setInverseFont(0);
    u8x8.draw2x2String(2, 2, Productname);
    delay(1500);
    u8x8.clear();
    u8x8.setFlipMode(1);
    u8x8.setInverseFont(1);
    u8x8.draw2x2String(0, 0, Productname);
    u8x8.setInverseFont(0);
    u8x8.draw2x2String(2, 2, Productname);
    delay(1500);

    u8x8.setFlipMode(0);
    u8x8.clear();

    // Display chip information
    #ifdef VERBOSE
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        u8x8.printf("ESP32 %d cores\nWiFi%s%s\n",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
        u8x8.printf("ESP Rev.%d\n", chip_info.revision);
        u8x8.printf("%dMB %s Flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int." : "ext.");
    #endif // VERBOSE

    u8x8.print(Productname);
    u8x8.print(" v");
    u8x8.println(PROGVERSION);
    u8x8.println("DEVEUI:");
	os_getDevEui((u1_t*) buf);
    DisplayKey(buf, 8, true);
    delay(5000);
    u8x8.clear();
#endif // HAS_DISPLAY
}

/* begin Aruino SETUP ------------------------------------------------------------ */

void setup() {

    // setup debug output or silence device
#ifdef VERBOSE
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_VERBOSE);
#else
    // mute logs completely by redirecting them to silence function
    esp_log_level_set("*", ESP_LOG_NONE);
    esp_log_set_vprintf(redirect_log);
#endif
    
    ESP_LOGI(TAG, "Starting %s %s", PROGNAME, PROGVERSION);

    // system event handler for wifi task, needed for wifi_sniffer_init()
    esp_event_loop_init(NULL, NULL);

    // Print chip information on startup
#ifdef VERBOSE
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is ESP32 chip with %d CPU cores, WiFi%s%s, silicon revision %d, %dMB %s Flash",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "", 
            chip_info.revision, spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    ESP_LOGI(TAG, "ESP32 SDK: %s", ESP.getSdkVersion());
#endif // VERBOSE

    // Read settings from NVRAM
    loadConfig(); // includes initialize if necessary
      
    // initialize hardware
#ifdef HAS_LED
    // initialize LED
    pinMode(HAS_LED, OUTPUT);
    digitalWrite(HAS_LED, LOW);
#endif

#ifdef HAS_BUTTON
    // install button interrupt
    pinMode(HAS_BUTTON, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), isr_button_pressed, FALLING); 
#endif

    // initialize wifi antenna
#ifdef LOPY
    antenna_init();
    antenna_select(WIFI_LOPY_ANTENNA);
#endif

    // initialize display
    init_display(PROGNAME, PROGVERSION);     
    u8x8.setPowerSave(!cfg.screenon); // set display off if disabled
    u8x8.setCursor(0,5);
    u8x8.printf(!cfg.rssilimit ? "RLIM:  off" : "RLIM: %4i", cfg.rssilimit);
    u8x8.drawString(0,6,"Join Wait       ");
    
    // output LoRaWAN keys to console
#ifdef VERBOSE
    printKeys();
#endif // VERBOSE

    os_init(); // setup LMIC
    os_setCallback(&initjob, lora_init); // setup initial job & join network 
    wifi_sniffer_init(); // setup wifi in monitor mode and start MAC counting
 
    // Start FreeRTOS tasks
#if CONFIG_FREERTOS_UNICORE // run all tasks on core 0 and switch off core 1
    ESP_LOGI(TAG, "Starting Lora task on core 0");
    xTaskCreatePinnedToCore(lorawan_loop, "loratask", 2048, ( void * ) 1,  ( 5 | portPRIVILEGE_BIT ), NULL, 0);  
    ESP_LOGI(TAG, "Starting Wifi task on core 0");
    xTaskCreatePinnedToCore(wifi_sniffer_loop, "wifisniffer", 4096, ( void * ) 1, 1, NULL, 0);
    // to come here: code for switching off core 1
#else // run wifi task on core 0 and lora task on core 1
    ESP_LOGI(TAG, "Starting Lora task on core 1");
    xTaskCreatePinnedToCore(lorawan_loop, "loratask", 2048, ( void * ) 1,  ( 5 | portPRIVILEGE_BIT ), NULL, 1);  
    ESP_LOGI(TAG, "Starting Wifi task on core 0");
    xTaskCreatePinnedToCore(wifi_sniffer_loop, "wifisniffer", 4096, ( void * ) 1, 1, NULL, 0);
#endif
    
    // Kickoff first sendjob, use payload "0000"
    uint8_t mydata[] = "0000";
    do_send(&sendjob);
}

/* end Aruino SETUP ------------------------------------------------------------ */


/* begin Aruino LOOP ------------------------------------------------------------ */

// Arduino main moop, runs on core 1
// https://techtutorialsx.com/2017/05/09/esp32-get-task-execution-core/
void loop() {
    while(1) {

    if (ButtonTriggered) {
        ButtonTriggered = false;
        ESP_LOGI(TAG, "Button pressed, resetting device to factory defaults");
        eraseConfig();
        esp_restart();
        }
    else {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        uptimecounter = uptime() / 1000; // count uptime seconds
        }
    }
}

/* end Aruino LOOP ------------------------------------------------------------ */
