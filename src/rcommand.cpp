// remote command interpreter
// parses multiple number of command / value pairs from LoRaWAN remote command port (RCMDPORT)  
// checks commands and executes each command with 1 argument per command

// Basic Config
#include "globals.h"

// LMIC-Arduino LoRaWAN Stack
#include <lmic.h>
#include <hal/hal.h>

// Local logging tag
static const char TAG[] = "main";

// table of remote commands and assigned functions
typedef struct {
    const uint8_t nam;
    void (*func)(uint8_t);
    const bool store;
} cmd_t;

// function defined in antenna.cpp
#ifdef HAS_ANTENNA_SWITCH
    void antenna_select(const uint8_t _ant);
#endif

// function defined in adcread.cpp
#ifdef HAS_BATTERY_PROBE
    uint32_t read_voltage(void);
#endif

// function sends result of get commands to LoRaWAN network
void do_transmit(osjob_t* j){
    // check if there is a pending TX/RX job running, if yes then reschedule transmission
    if (LMIC.opmode & OP_TXRXPEND) {
        ESP_LOGI(TAG, "LoRa busy, rescheduling");
        sprintf(display_lmic, "LORA BUSY");
        os_setTimedCallback(&rcmdjob, os_getTime()+sec2osticks(RETRANSMIT_RCMD), do_transmit);
    }
    LMIC_setTxData2(RCMDPORT, rcmd_data, rcmd_data_size, 0); // send data unconfirmed on RCMD Port
    ESP_LOGI(TAG, "%d bytes queued to send", rcmd_data_size);
    sprintf(display_lmic, "PACKET QUEUED");
}

// help function to transmit result of get commands, since callback function do_transmit() cannot have params
void transmit(xref2u1_t mydata, u1_t mydata_size){
    rcmd_data = mydata;
    rcmd_data_size = mydata_size;
    do_transmit(&rcmdjob);
}

// help function to assign LoRa datarates to numeric spreadfactor values
void switch_lora (uint8_t sf, uint8_t tx) {
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
void set_reset(uint8_t val) {
    switch (val) {
        case 0: // restart device
            ESP_LOGI(TAG, "Remote command: restart device");
            sprintf(display_lora, "Reset pending");
            vTaskDelay(10000/portTICK_PERIOD_MS); // wait for LMIC to confirm LoRa downlink to server
            esp_restart();
            break;
        case 1: // reset MAC counter
            ESP_LOGI(TAG, "Remote command: reset MAC counter");
            reset_counters();   // clear macs
            reset_salt();       // get new salt
            sprintf(display_lora, "Reset counter");
            break;
        case 2: // reset device to factory settings
            ESP_LOGI(TAG, "Remote command: reset device to factory settings");
            sprintf(display_lora, "Factory reset");
            eraseConfig();
            break;
        }
};

void set_rssi(uint8_t val) {
    cfg.rssilimit = val * -1;
    ESP_LOGI(TAG, "Remote command: set RSSI limit to %d", cfg.rssilimit);
};    

void set_sendcycle(uint8_t val) {
    cfg.sendcycle = val;
    ESP_LOGI(TAG, "Remote command: set payload send cycle to %d seconds", cfg.sendcycle*2);
};    

void set_wifichancycle(uint8_t val) {
    cfg.wifichancycle = val;
    // modify wifi channel rotation IRQ
    timerAlarmWrite(channelSwitch, cfg.wifichancycle * 10000, true); // reload interrupt after each trigger of channel switch cycle
    ESP_LOGI(TAG, "Remote command: set Wifi channel switch interval to %.1f seconds", cfg.wifichancycle/float(100));
};    

void set_blescantime(uint8_t val) {
    cfg.blescantime = val;
    ESP_LOGI(TAG, "Remote command: set BLE scan time to %.1f seconds", cfg.blescantime/float(100));
    #ifdef BLECOUNTER
        // stop & restart BLE scan task to apply new parameter
        if (cfg.blescan)
        {
            stop_BLEscan();
            start_BLEscan();
        }
    #endif
};

void set_countmode(uint8_t val) {
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

void set_screensaver(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set screen saver to %s ", val ? "on" : "off");
    switch (val) {
        case 1: cfg.screensaver = val; break;
        default: cfg.screensaver = 0; break;
        }
};

void set_display(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set screen to %s", val ? "on" : "off");
    switch (val) {
        case 1: cfg.screenon = val; break;
        default: cfg.screenon = 0; break;
        }
};

void set_gps(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set GPS to %s", val ? "on" : "off");
    switch (val) {
        case 1: cfg.gpsmode = val; break;
        default: cfg.gpsmode = 0; break;
        }
};

void set_lorasf(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set LoRa SF to %d", val);
    switch_lora(val, cfg.txpower);
};

void set_loraadr(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set LoRa ADR mode to %s", val ? "on" : "off");
    switch (val) {
        case 1: cfg.adrmode = val; break;
        default: cfg.adrmode = 0; break;
        }
    LMIC_setAdrMode(cfg.adrmode); 
};

void set_blescan(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set BLE scanner to %s", val ? "on" : "off");
    switch (val) {
        case 0:
            cfg.blescan = 0;
            macs_ble = 0; // clear BLE counter
            #ifdef BLECOUNTER
                stop_BLEscan();
            #endif
            break; 
        default: 
            cfg.blescan = 1;
             #ifdef BLECOUNTER
                start_BLEscan();
            #endif
            break; 
        }
};

void set_wifiant(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set Wifi antenna to %s", val ? "external" : "internal");
    switch (val) {
        case 1: cfg.wifiant = val; break;
        default: cfg.wifiant = 0; break;
        }
    #ifdef HAS_ANTENNA_SWITCH
        antenna_select(cfg.wifiant);
    #endif
};

void set_vendorfilter(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set vendorfilter mode to %s", val ? "on" : "off");
    switch (val) {
        case 1: cfg.vendorfilter = val; break;
        default: cfg.vendorfilter = 0; break;
        }
};

void set_rgblum(uint8_t val) {
    // Avoid wrong parameters
    cfg.rgblum = (val>=0 && val<=100) ? (uint8_t) val : RGBLUMINOSITY;
    ESP_LOGI(TAG, "Remote command: set RGB Led luminosity %d", cfg.rgblum);
};

void set_lorapower(uint8_t val) {
    ESP_LOGI(TAG, "Remote command: set LoRa TXPOWER to %d", val);
    switch_lora(cfg.lorasf, val);
};

void get_config (uint8_t val) {
    ESP_LOGI(TAG, "Remote command: get configuration");
    transmit((byte*)&cfg, sizeof(cfg));
};

void get_uptime (uint8_t val) {
    ESP_LOGI(TAG, "Remote command: get uptime");
    transmit((byte*)&uptimecounter, sizeof(uptimecounter));
};

void get_cputemp (uint8_t val) {
    ESP_LOGI(TAG, "Remote command: get cpu temperature");
    float temp = temperatureRead();
    transmit((byte*)&temp, sizeof(temp));
};

void get_voltage (uint8_t val) {
    ESP_LOGI(TAG, "Remote command: get battery voltage");
    #ifdef HAS_BATTERY_PROBE
        uint16_t voltage = read_voltage();
    #else
        uint16_t voltage = 0;
    #endif
    transmit((byte*)&voltage, sizeof(voltage));
};

void get_gps (uint8_t val) {
    ESP_LOGI(TAG, "Remote command: get gps status");
    #ifdef HAS_GPS
        gps_read();
        transmit((byte*)&gps_status, sizeof(gps_status));
        ESP_LOGI(TAG, "HDOP=%d, SATS=%d, LAT=%f, LON=%f", gps_status.hdop, gps_status.satellites, gps_status.latitude, gps_status.longitude );
    #else
        ESP_LOGE(TAG, "GPS not present");
    #endif
};

// assign previously defined functions to set of numeric remote commands
// format: opcode, function, flag (1 = do make settings persistent / 0 = don't)
// 
cmd_t table[] = {
                {0x01, set_rssi, true},
                {0x02, set_countmode, true},
                {0x03, set_gps, true},
                {0x04, set_display, true},
                {0x05, set_lorasf, true},
                {0x06, set_lorapower, true},
                {0x07, set_loraadr, true},
                {0x08, set_screensaver, true},
                {0x09, set_reset, false},
                {0x0a, set_sendcycle, true},
                {0x0b, set_wifichancycle, true},
                {0x0c, set_blescantime, true},
                {0x0d, set_vendorfilter, false},
                {0x0e, set_blescan, true},
                {0x0f, set_wifiant, true},
                {0x10, set_rgblum, true},
                {0x80, get_config, false},
                {0x81, get_uptime, false},
                {0x82, get_cputemp, false},
                {0x83, get_voltage, false},
                {0x84, get_gps, false}
                };

// check and execute remote command
void rcommand(uint8_t cmd, uint8_t arg) {
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