// remote command interpreter
// parses multiple number of command / value pairs from LoRaWAN remote command port (RCMDPORT)  
// checks commands and executes each command with 1 argument per command

// Basic Config
#include "main.h"
#include "globals.h"

// LMIC-Arduino LoRaWAN Stack
#include <lmic.h>
#include <hal/hal.h>

// Local logging tag
static const char *TAG = "rcommand";

// table of remote commands and assigned functions
typedef struct {
    const int nam;
    void (*func)(int);
    const bool store;
} cmd_t;

// functions defined in configmanager.cpp
void eraseConfig(void);
void saveConfig(void);

// function defined in antenna.cpp
#ifdef HAS_ANTENNA_SWITCH
    void antenna_select(const int8_t _ant);
#endif

// help function to assign LoRa datarates to numeric spreadfactor values
void switch_lora (int sf, int tx) {
    if ( tx > 20 ) return;
    cfg.txpower = tx;
    switch (sf) {
        case 7: LMIC_setDrTxpow(DR_SF7,tx); cfg.lorasf=sf; break;
        case 8: LMIC_setDrTxpow(DR_SF8,tx); cfg.lorasf=sf; break;
        case 9: LMIC_setDrTxpow(DR_SF9,tx); cfg.lorasf=sf; break;
        case 10: LMIC_setDrTxpow(DR_SF10,tx); cfg.lorasf=sf; break;
        case 11: 
        #if defined(CFG_eu868)
            LMIC_setDrTxpow(DR_SF11,tx); cfg.lorasf=sf; break;
        #elif defined(CFG_us915)
            LMIC_setDrTxpow(DR_SF11CR,tx); cfg.lorasf=sf; break;
        #endif
        case 12: 
        #if defined(CFG_eu868)
            LMIC_setDrTxpow(DR_SF12,tx); cfg.lorasf=sf; break;
        #elif defined(CFG_us915)
            LMIC_setDrTxpow(DR_SF12CR,tx); cfg.lorasf=sf; break;
        #endif
        default: break;
    }
}

// set of functions that can be triggered by remote commands
void set_reset(int val) {
    switch (val) {
        case 0: // restart device
            ESP_LOGI(TAG, "Remote command: restart device");
            u8x8.clearLine(5);
            u8x8.setCursor(0, 5);
            u8x8.printf("Reset pending   ");
            vTaskDelay(10000/portTICK_PERIOD_MS); // wait for LMIC to confirm LoRa downlink to server
            esp_restart();
            break;
        case 1: // reset MAC counter
            ESP_LOGI(TAG, "Remote command: reset MAC counter");
            macs.clear(); // clear macs container
            macnum = 0;
            salt = rand() % 256; // get new random int between 0 and 255 for salting MAC hashes
            u8x8.clearLine(0); u8x8.clearLine(1); // clear Display counter
            u8x8.clearLine(5);
            u8x8.setCursor(0, 5);
            u8x8.printf("Reset counter   ");
            break;
        case 2: // reset device to factory settings
            ESP_LOGI(TAG, "Remote command: reset device to factory settings");
            u8x8.clearLine(5);
            u8x8.setCursor(0, 5);
            u8x8.printf("Factory reset   ");
            eraseConfig();
            break;
        }
};

void set_rssi(int val) {
    cfg.rssilimit = val * -1;
    ESP_LOGI(TAG, "Remote command: set RSSI limit to %i", cfg.rssilimit);
    u8x8.clearLine(5);
    u8x8.setCursor(0, 5);
    u8x8.printf(!cfg.rssilimit ? "RLIM:  off" : "RLIM: -%4i", cfg.rssilimit);
};    

void set_wifiscancycle(int val) {
    cfg.wifiscancycle = val;
    ESP_LOGI(TAG, "Remote command: set Wifi scan cycle duration to %i seconds", cfg.wifiscancycle*2);
};    

void set_wifichancycle(int val) {
    cfg.wifichancycle = val;
    ESP_LOGI(TAG, "Remote command: set Wifi channel switch interval to %i seconds", cfg.wifichancycle/100);
};    

void set_blescancycle(int val) {
    cfg.blescancycle = val;
    ESP_LOGI(TAG, "Remote command: set Wifi channel cycle duration to %i seconds", cfg.blescancycle);
};    

void set_countmode(int val) {
    switch (val) {
        case 0: // cyclic unconfirmed
            cfg.countermode = 0; 
            ESP_LOGI(TAG, "Remote command: set counter mode to cyclic unconfirmed");
            break; 
        case 1: // cumulative
            cfg.countermode = 1; 
            ESP_LOGI(TAG, "Remote command: set counter mode to cumulative");
            break; 
        default: // cyclic confirmed
            cfg.countermode = 2; 
            ESP_LOGI(TAG, "Remote command: set counter mode to cyclic confirmed");
            break; 
        }
};

void set_screensaver(int val) {
    ESP_LOGI(TAG, "Remote command: set screen saver to %s ", val ? "on" : "off");
    switch (val) {
        case 1: cfg.screensaver = val; break;
        default: cfg.screensaver = 0; break;
        }
    u8x8.setPowerSave(cfg.screensaver); // set display 0=on / 1=off
};

void set_display(int val) {
    ESP_LOGI(TAG, "Remote command: set screen to %s", val ? "on" : "off");
    switch (val) {
        case 1: cfg.screenon = val; break;
        default: cfg.screenon = 0; break;
        }
    u8x8.setPowerSave(!cfg.screenon); // set display 0=on / 1=off                          
};

void set_lorasf(int val) {
    ESP_LOGI(TAG, "Remote command: set LoRa SF to %i", val);
    switch_lora(val, cfg.txpower);
};

void set_loraadr(int val) {
    ESP_LOGI(TAG, "Remote command: set LoRa ADR mode to %s", val ? "on" : "off");
    switch (val) {
        case 1: cfg.adrmode = val; break;
        default: cfg.adrmode = 0; break;
        }
    LMIC_setAdrMode(cfg.adrmode); 
};

void set_blescan(int val) {
    ESP_LOGI(TAG, "Remote command: set BLE scan mode to %s", val ? "on" : "off");
    switch (val) {
        case 1: cfg.blescan = val; break;
        default: 
            cfg.blescan = 0; 
            btStop();
            u8x8.clearLine(3); // clear BLE results from display
            break;
        }
};

void set_wifiant(int val) {
    ESP_LOGI(TAG, "Remote command: set Wifi antenna to %s", val ? "external" : "internal");
    switch (val) {
        case 1: cfg.wifiant = val; break;
        default: cfg.wifiant = 0; break;
        }
    #ifdef HAS_ANTENNA_SWITCH
        antenna_select(cfg.wifiant);
    #endif
};

void set_lorapower(int val) {
    ESP_LOGI(TAG, "Remote command: set LoRa TXPOWER to %i", val);
    switch_lora(cfg.lorasf, val);
};

void set_noop (int val) { 
    ESP_LOGI(TAG, "Remote command: noop - doing nothing");
};

void get_config (int val) {
    ESP_LOGI(TAG, "Remote command: get configuration");
    int size = sizeof(configData_t);
    // declare send buffer (char byte array)
    unsigned char *sendData = new unsigned char[size];
    // copy current configuration (struct) to send buffer
    memcpy(sendData, &cfg, size);
    LMIC_setTxData2(RCMDPORT, sendData, size-1, 0); // send data unconfirmed on RCMD Port
    delete sendData; // free memory
    ESP_LOGI(TAG, "%i bytes queued in send queue", size-1);
};

void get_uptime (int val) {
    ESP_LOGI(TAG, "Remote command: get uptime");
    int size = sizeof(uptimecounter);
    unsigned char *sendData = new unsigned char[size];
    memcpy(sendData, (unsigned char*)&uptimecounter, size);
    LMIC_setTxData2(RCMDPORT, sendData, size-1, 0); // send data unconfirmed on RCMD Port
    delete sendData; // free memory
    ESP_LOGI(TAG, "%i bytes queued in send queue", size-1);
};

void get_cputemp (int val) {
    ESP_LOGI(TAG, "Remote command: get cpu temperature");
    float temp = temperatureRead();
    int size = sizeof(temp);
    unsigned char *sendData = new unsigned char[size];
    memcpy(sendData, (unsigned char*)&temp, size);
    LMIC_setTxData2(RCMDPORT, sendData, size-1, 0); // send data unconfirmed on RCMD Port
    delete sendData; // free memory
    ESP_LOGI(TAG, "%i bytes queued in send queue", size-1);
};

// assign previously defined functions to set of numeric remote commands
// format: opcode, function, flag (1 = do make settings persistent / 0 = don't)
// 
cmd_t table[] = {
                {0x01, set_rssi, true},
                {0x02, set_countmode, true},
                {0x03, set_screensaver, true},
                {0x04, set_display, true},
                {0x05, set_lorasf, true},
                {0x06, set_lorapower, true},
                {0x07, set_loraadr, true},
                {0x08, set_noop, false},
                {0x09, set_reset, false},
                {0x0a, set_wifiscancycle, true},
                {0x0b, set_wifichancycle, true},
                {0x0c, set_blescancycle, true},
                {0x0d, set_blescan, true},
                {0x0e, set_wifiant, true},
                {0x80, get_config, false},
                {0x81, get_uptime, false},
                {0x82, get_cputemp, false}
                };

// check and execute remote command
void rcommand(int cmd, int arg) {         
    int i = sizeof(table) / sizeof(table[0]); // number of commands in command table
    bool store_flag = false;
    while(i--) { 
        if(cmd == table[i].nam) { // check if valid command
            table[i].func(arg); // then execute assigned function
            if ( table[i].store ) store_flag = true; // set save flag if function needs to store configuration
            break; // exit check loop, since command was found
        }
    }
    if (store_flag) saveConfig(); // if save flag is set: store new configuration in NVS to make it persistent
}