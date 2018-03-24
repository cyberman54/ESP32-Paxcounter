// Basic Config
#include "main.h"
#include "globals.h"

// LMIC-Arduino LoRaWAN Stack
#include "loraconf.h"
#include <lmic.h>
#include <hal/hal.h>

uint8_t mydata[] = "0000";

// Local logging Tag
static const char *TAG = "lorawan";

// function defined in main.cpp
void set_onboard_led(int state);

// functions defined in rcommand.cpp
void rcommand(int cmd, int arg);
void switch_lora(int sf, int tx);

// DevEUI generator using devices's MAC address
void gen_lora_deveui(uint8_t *pdeveui) {
    uint8_t *p = pdeveui, dmac[6];
    int i = 0;
    esp_efuse_mac_get_default(dmac);
    // deveui is LSB, we reverse it so TTN DEVEUI display 
    // will remain the same as MAC address
    // MAC is 6 bytes, devEUI 8, set first 2 ones 
    // with an arbitrary value
    *p++ = 0xFF;
    *p++ = 0xFE;
    // Then next 6 bytes are mac address reversed
                for ( i=0; i<6 ; i++) {
                  *p++ = dmac[5-i];
                }
}

// Function to do a byte swap in a byte array
void RevBytes(unsigned char* b, size_t c)
{
  u1_t i;
  for (i = 0; i < c / 2; i++)
  { unsigned char t = b[i];
    b[i] = b[c - 1 - i];
    b[c - 1 - i] = t; }
}

#ifdef VERBOSE

// Display a key
void printKey(const char * name, const uint8_t * key, uint8_t len, bool lsb) {
  uint8_t start=lsb?len:0;
  uint8_t end = lsb?0:len;
  const uint8_t * p ;
  char keystring[len+1] = "?", keybyte[3];
  for (uint8_t i=0; i<len ; i++) {
     p = lsb ? key+len-i-1 : key+i;
     sprintf(keybyte, "%02X", * p);
     strncat(keystring, keybyte, 2);
    }
  ESP_LOGI(TAG, "%s: %s", name, keystring);
}

// Display OTAA keys
void printKeys(void) {
	// LMIC may not have used callback to fill 
	// all EUI buffer so we do it here to a temp
	// buffer to be able to display them
	uint8_t buf[32];
	os_getDevEui((u1_t*) buf);
	printKey("DevEUI", buf, 8, true);
	os_getArtEui((u1_t*) buf);
	printKey("AppEUI", buf, 8, true);
	os_getDevKey((u1_t*) buf);
	printKey("AppKey", buf, 16, false);
}

#endif // VERBOSE

void do_send(osjob_t* j){
    mydata[0] = (macnum & 0x0000ff00) >> 8;
    mydata[1] = macnum  & 0x000000ff;
    mydata[2] = (blenum & 0x0000ff00) >> 8;
    mydata[3] = blenum  & 0x000000ff;

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        ESP_LOGI(TAG, "OP_TXRXPEND, not sending");
        u8x8.clearLine(7); 
        u8x8.drawString(0, 7, "LORA BUSY");
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, (cfg.countermode & 0x02));
        ESP_LOGI(TAG, "Packet queued");
        u8x8.clearLine(7);
        u8x8.drawString(0, 7, "PACKET QUEUED");
        set_onboard_led(1);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            ESP_LOGI(TAG, "EV_SCAN_TIMEOUT");
            u8x8.clearLine(7); 
            u8x8.drawString(0, 7, "SCAN TIMEOUT");
            break;
        case EV_BEACON_FOUND:
            ESP_LOGI(TAG, "EV_BEACON_FOUND");
            u8x8.clearLine(7); 
            u8x8.drawString(0, 7, "BEACON FOUND");
            break;
        case EV_BEACON_MISSED:
            ESP_LOGI(TAG, "EV_BEACON_MISSED");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "BEACON MISSED");
            break;
        case EV_BEACON_TRACKED:
            ESP_LOGI(TAG, "EV_BEACON_TRACKED");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "BEACON TRACKED");
            break;
        case EV_JOINING:
            ESP_LOGI(TAG, "EV_JOINING");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "JOINING");
            break;
        case EV_JOINED:
            ESP_LOGI(TAG, "EV_JOINED");
            u8x8.clearLine(6); // erase "Join Wait" message from display, see main.cpp
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "JOINED");
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            // set data rate adaptation
            LMIC_setAdrMode(cfg.adrmode);
            // Set data rate and transmit power (note: txpower seems to be ignored by the library)
            switch_lora(cfg.lorasf,cfg.txpower);
            joinstate=true;
            // show effective LoRa parameters after join
            ESP_LOGI(TAG, "ADR=%i, SF=%i, TXPOWER=%i", cfg.adrmode, cfg.lorasf, cfg.txpower);
            break;
        case EV_RFU1:
            ESP_LOGI(TAG, "EV_RFU1");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "RFUI");
            break;
        case EV_JOIN_FAILED:
            ESP_LOGI(TAG, "EV_JOIN_FAILED");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "JOIN FAILED");
            break;
        case EV_REJOIN_FAILED:
            ESP_LOGI(TAG, "EV_REJOIN_FAILED");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "REJOIN FAILED");
            break;
        case EV_TXCOMPLETE:
            ESP_LOGI(TAG, "EV_TXCOMPLETE (includes waiting for RX windows)");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "TX COMPLETE");
            set_onboard_led(0);
            if (LMIC.txrxFlags & TXRX_ACK) {
              ESP_LOGI(TAG, "Received ack");
              u8x8.clearLine(7);
              u8x8.drawString(0, 7, "RECEIVED ACK");
            }   
            if (LMIC.dataLen) {
                ESP_LOGI(TAG, "Received %i bytes of payload", LMIC.dataLen);
                u8x8.clearLine(6); 
                u8x8.setCursor(0, 6);
                u8x8.printf("Rcvd %i bytes", LMIC.dataLen);
                u8x8.clearLine(7);
                u8x8.setCursor(0, 7);
                // LMIC.snr = SNR twos compliment [dB] * 4
                // LMIC.rssi = RSSI [dBm] (-196...+63)              
                u8x8.printf("RSSI %d SNR %d", LMIC.rssi, (signed char)LMIC.snr / 4);
                // check if payload received on command port, then call remote command interpreter
                if ( (LMIC.txrxFlags & TXRX_PORT) && (LMIC.frame[LMIC.dataBeg-1] == RCMDPORT ) ) {
                    // caution: buffering LMIC values here because rcommand() can modify LMIC.frame
                    unsigned char* buffer = new unsigned char[MAX_LEN_FRAME];
                    memcpy(buffer, LMIC.frame, MAX_LEN_FRAME); //Copy data from cfg to char*
                    int i, k = LMIC.dataBeg, l = LMIC.dataBeg+LMIC.dataLen-2;
                    for (i=k; i<=l; i+=2)
                        rcommand(buffer[i], buffer[i+1]);
                    delete[] buffer; //free memory
                }
            }
            break;
        case EV_LOST_TSYNC:
            ESP_LOGI(TAG, "EV_LOST_TSYNC");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "LOST TSYNC");
            break;
        case EV_RESET:
            ESP_LOGI(TAG, "EV_RESET");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "RESET");
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            ESP_LOGI(TAG, "EV_RXCOMPLETE");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "RX COMPLETE");
            break;
        case EV_LINK_DEAD:
            ESP_LOGI(TAG, "EV_LINK_DEAD");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "LINK DEAD");
            break;
        case EV_LINK_ALIVE:
            ESP_LOGI(TAG, "EV_LINK_ALIVE");
            u8x8.clearLine(7);
            u8x8.drawString(0, 7, "LINK ALIVE");
            break;
        default:
            ESP_LOGI(TAG, "Unknown event");
            u8x8.clearLine(7);
            u8x8.setCursor(0, 7);
            u8x8.printf("UNKNOWN EVENT %d", ev);
            break;
    }
}