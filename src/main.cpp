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
#include "globals.h"

// std::set for unified array functions
#include <set>

// OLED driver
#include <U8x8lib.h>
#include <Wire.h> // Does nothing and avoid any compilation error with I2C

// LMIC-Arduino LoRaWAN Stack
#include "loraconf.h"
#include <lmic.h>
#include <hal/hal.h>

// ESP32 Functions
#include <esp_event_loop.h> // needed for Wifi event handler
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes
#include <esp32-hal-log.h> // needed for ESP_LOGx on arduino framework

configData_t cfg; // struct holds current device configuration
osjob_t sendjob, initjob; // LMIC

// Initialize global variables
uint8_t channel = 0;
int macnum = 0;
uint64_t uptimecounter = 0;
bool joinstate = false;

std::set<uint16_t> macs; // associative container holds total of unique MAC adress hashes (Wifi + BLE)
std::set<uint16_t> wifis; // associative container holds unique Wifi MAC adress hashes

#ifdef BLECOUNTER
    std::set<uint16_t> bles; // associative container holds unique BLE MAC adresses hashes
    int scanTime;
#endif

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

#ifdef HAS_LED
    void set_onboard_led(int st);
#endif

/* begin LMIC specific parts ------------------------------------------------------------ */

// defined in lorawan.cpp
void gen_lora_deveui(uint8_t * pdeveui);
void RevBytes(unsigned char* b, size_t c);
void get_hard_deveui(uint8_t *pdeveui);


#ifdef VERBOSE
    void printKeys(void);
#endif // VERBOSE

// LMIC callback functions
void os_getDevKey (u1_t *buf) { 
    memcpy(buf, APPKEY, 16);
}

void os_getArtEui (u1_t *buf) { 
    memcpy(buf, APPEUI, 8);
    RevBytes(buf, 8); // TTN requires it in LSB First order, so we swap bytes
}

void os_getDevEui (u1_t* buf) {
    int i=0, k=0;
    memcpy(buf, DEVEUI, 8); // get fixed DEVEUI from loraconf.h 
    for (i=0; i<8 ; i++) {
        k += buf[i];
    }
    if (k) {
        RevBytes(buf, 8); // use fixed DEVEUI and swap bytes to LSB format
    } else {
        gen_lora_deveui(buf); // generate DEVEUI from device's MAC
    }

    // Get MCP 24AA02E64 hardware DEVEUI (override default settings if found)
    #ifdef MCP_24AA02E64_I2C_ADDRESS
        get_hard_deveui(buf); 
        RevBytes(buf, 8); // swap bytes to LSB format
    #endif
}

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

// LMIC FreeRTos Task
void lorawan_loop(void * pvParameters) {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 ); // FreeRTOS check

    static bool led_state;
    bool new_led_state;

    while(1) {
        uint16_t color;
        os_runloop_once();

        // All follow is Led management
        // Let join at the begining of if sequence,
        // is prior to send because joining state send data
        if ( LMIC.opmode & (OP_JOINING | OP_REJOIN) )  {
            color = COLOR_YELLOW;
            // quick blink 20ms on each 1/5 second
            new_led_state = ((millis() % 200) < 20) ? HIGH : LOW;
            
        // TX data pending
        } else if (LMIC.opmode & (OP_TXDATA | OP_TXRXPEND)) {
            color = COLOR_BLUE;
            // small blink 10ms on each 1/2sec (not when joining)
            new_led_state = ((millis() % 500) < 20) ? HIGH : LOW;
        
        // This should not happen so indicate a problem
        } else  if ( LMIC.opmode & (OP_TXDATA | OP_TXRXPEND | OP_JOINING | OP_REJOIN) == 0 ) {
            color = COLOR_RED;
            // heartbeat long blink 200ms on each 2 seconds
            new_led_state = ((millis() % 2000) < 200) ? HIGH : LOW;
        } else {
            // led off
            rgb_set_color(COLOR_NONE);
        }
        // led need to change state? avoid digitalWrite() for nothing
        if (led_state != new_led_state) {
            if (new_led_state == HIGH) {
                set_onboard_led(1);
                rgb_set_color(color);
            } else {
                set_onboard_led(0);
                rgb_set_color(COLOR_NONE);
            }
            led_state = new_led_state;
        }

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

#ifdef HAS_ANTENNA_SWITCH
    // defined in antenna.cpp
    void antenna_init();
    void antenna_select(const int8_t _ant);
#endif

#ifndef BLECOUNTER
    bool btstop = btStop();
#endif

void set_onboard_led(int st){
#ifdef HAS_LED
    switch (st) {
        #ifdef LED_ACTIVE_LOW
          case 1: digitalWrite(HAS_LED, LOW); break;
          case 0: digitalWrite(HAS_LED, HIGH); break;
        #else
          case 1: digitalWrite(HAS_LED, HIGH); break;
          case 0: digitalWrite(HAS_LED, LOW); break;
        #endif
    }
#endif
};


#ifdef HAS_BUTTON
    // Button Handling, board dependent -> perhaps to be moved to hal/<$board.h>
    // IRAM_ATTR necessary here, see https://github.com/espressif/arduino-esp32/issues/855
    void IRAM_ATTR isr_button_pressed(void) {
        ButtonTriggered = true; }
#endif

/* end hardware specific parts -------------------------------------------------------- */


/* begin wifi specific parts ---------------------------------------------------------- */

// defined in wifisniffer.cpp
void wifi_sniffer_init(void);
void wifi_sniffer_set_channel(uint8_t channel);
void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

// defined in blescan.cpp
void bt_loop(void *ignore);

// Sniffer Task
void sniffer_loop(void * pvParameters) {

    configASSERT( ( ( uint32_t ) pvParameters ) == 1 ); // FreeRTOS check
    channel=0;
    char buff[16];
    int nloop=0, lorawait=0;
   
  	while (true) {

        nloop++; // actual number of wifi loops, controls cycle when data is sent

        vTaskDelay(cfg.wifichancycle*10 / portTICK_PERIOD_MS);
        yield();
        channel = (channel % WIFI_CHANNEL_MAX) + 1;     // rotates variable channel 1..WIFI_CHANNEL_MAX
        wifi_sniffer_set_channel(channel);
        ESP_LOGD(TAG, "Wifi set channel %d", channel);

        // duration of one wifi scan loop reached? then send data and begin new scan cycle
        if ( nloop >= ( (100 / cfg.wifichancycle) * (cfg.wifiscancycle * 2)) +1 ) {
            u8x8.setPowerSave(!cfg.screenon);           // set display on if enabled
            nloop=0; channel=0;                         // reset wifi scan + channel loop counter           
            do_send(&sendjob);                          // Prepare and execute LoRaWAN data upload
            vTaskDelay(500/portTICK_PERIOD_MS);
            yield();

            // clear counter if not in cumulative counter mode
            if (cfg.countermode != 1) {
                macs.clear();                           // clear all macs container
                wifis.clear();                          // clear Wifi macs couner
                #ifdef BLECOUNTER
                    bles.clear();                         // clear BLE macs counter
                #endif 
                salt_reset(); // get new salt for salting hashes
                u8x8.clearLine(0); // clear Display counter 
                u8x8.clearLine(1); 
            }      

            // wait until payload is sent, while wifi scanning and mac counting task continues
            lorawait = 0;
            while(LMIC.opmode & OP_TXRXPEND) {
                if(!lorawait) 
                    u8x8.drawString(0,6,"LoRa wait       ");
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

            // TBD: need to check if long 2000ms pause causes stack problems while scanning continues
            if (cfg.screenon && cfg.screensaver) {
                vTaskDelay(2000/portTICK_PERIOD_MS);   // pause for displaying results
                yield();
                u8x8.setPowerSave(1 && cfg.screensaver); // set display off if screensaver is enabled
            }          
        } // end of send data cycle
        
    } // end of infinite wifi channel rotation loop
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

    #ifdef DISPLAY_FLIP
        u8x8.setFlipMode(1);
    #endif

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

    // disable brownout detection
#ifdef DISABLE_BROWNOUT
    // register with brownout is at address DR_REG_RTCCNTL_BASE + 0xd4
  (*((volatile uint32_t *)ETS_UNCACHED_ADDR((DR_REG_RTCCNTL_BASE+0xd4)))) = 0;
#endif

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
    rgb_set_color(COLOR_NONE);
    
    // initialize system event handler for wifi task, needed for wifi_sniffer_init()
    esp_event_loop_init(NULL, NULL);

    // print chip information on startup if in verbose mode
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
#endif

    // read settings from NVRAM
    loadConfig(); // includes initialize if necessary

    // initialize led if needed
#ifdef HAS_LED
    pinMode(HAS_LED, OUTPUT);
    digitalWrite(HAS_LED, LOW);
#endif

    // initialize button handling if needed
#ifdef HAS_BUTTON
    #ifdef BUTTON_PULLUP
        // install button interrupt (pullup mode)
        pinMode(HAS_BUTTON, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), isr_button_pressed, RISING); 
    #else
        // install button interrupt (pulldown mode)
        pinMode(HAS_BUTTON, INPUT_PULLDOWN);
        attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), isr_button_pressed, FALLING); 
    #endif
#endif

    // initialize wifi antenna if needed
#ifdef HAS_ANTENNA_SWITCH
    antenna_init();
#endif

// initialize display  
    init_display(PROGNAME, PROGVERSION);     
    u8x8.setPowerSave(!cfg.screenon); // set display off if disabled
    u8x8.draw2x2String(0, 0, "PAX:0");
    u8x8.setCursor(0,4);
    u8x8.printf("WIFI: 0");
    #ifdef BLECOUNTER
        u8x8.setCursor(0,3);
        u8x8.printf("BLTH: 0");
    #endif
    u8x8.setCursor(0,5);
    u8x8.printf(!cfg.rssilimit ? "RLIM: off" : "RLIM: %d", cfg.rssilimit);
    u8x8.drawString(0,6,"Join Wait       ");

// output LoRaWAN keys to console
#ifdef VERBOSE
    printKeys();
#endif

os_init(); // setup LMIC
os_setCallback(&initjob, lora_init); // setup initial job & join network 
wifi_sniffer_init(); // setup wifi in monitor mode and start MAC counting

// initialize salt value using esp_random() called by random() in arduino-esp32 core
// note: do this *after* wifi has started, since gets it's seed from RF noise
salt_reset(); // get new 16bit for salting hashes
 
// run wifi task on core 0 and lora task on core 1 and bt task on core 0
ESP_LOGI(TAG, "Starting Lora task on core 1");
xTaskCreatePinnedToCore(lorawan_loop, "loratask", 2048, ( void * ) 1,  ( 5 | portPRIVILEGE_BIT ), NULL, 1);  
ESP_LOGI(TAG, "Starting Wifi task on core 0");
xTaskCreatePinnedToCore(sniffer_loop, "wifisniffer", 16384, ( void * ) 1, 1, NULL, 0);
#ifdef BLECOUNTER
    if (cfg.blescan) { // start BLE task only if BLE function is enabled in NVRAM configuration
        ESP_LOGI(TAG, "Starting Bluetooth task on core 0");
        xTaskCreatePinnedToCore(bt_loop, "btscan", 16384, NULL, 5, NULL, 0);
    }
#endif
    
// Finally: kickoff first sendjob and join, then send initial payload "0000"
uint8_t mydata[] = "0000";
do_send(&sendjob);
}

/* end Aruino SETUP ------------------------------------------------------------ */


/* begin Aruino LOOP ------------------------------------------------------------ */

// Arduino main moop, runs on core 1
// https://techtutorialsx.com/2017/05/09/esp32-get-task-execution-core/
void loop() {
    
    #ifdef HAS_BUTTON
        if (ButtonTriggered) {
            ButtonTriggered = false;
            ESP_LOGI(TAG, "Button pressed, resetting device to factory defaults");
            eraseConfig();
            esp_restart();
        }
    #endif
    
    #ifdef HAS_DISPLAY
        // display counters(lines 0-4)
        char buff[16];
        snprintf(buff, sizeof(buff), "PAX:%-4d", (int) macs.size()); // convert 16-bit MAC counter to decimal counter value
        u8x8.draw2x2String(0, 0, buff);          // display number on unique macs total Wifi + BLE
        u8x8.setCursor(0,4);
        u8x8.printf("WIFI: %-4d", (int) wifis.size());
        #ifdef BLECOUNTER
            u8x8.setCursor(0,3);
            u8x8.printf("BLTH: %-4d", (int) bles.size());
        #endif
        // display actual wifi channel (line 4)
        u8x8.setCursor(11,4);
        u8x8.printf("ch:%02i", channel);
        // display RSSI status (line 5)
        u8x8.setCursor(0,5);
        u8x8.printf(!cfg.rssilimit ? "RLIM: off" : "RLIM: %-3d", cfg.rssilimit);
    #endif

    vTaskDelay(DISPLAYREFRESH/portTICK_PERIOD_MS);
    uptimecounter = uptime() / 1000; // count uptime seconds

}

/* end Aruino LOOP ------------------------------------------------------------ */
