
// Basic Config
#include "globals.h"

#ifdef VENDORFILTER
    #include <array>
    #include <algorithm>
    #include "vendor_array.h"
#endif

// Local logging tag
static const char *TAG = "macsniff";

static wifi_country_t wifi_country = {.cc=WIFI_MY_COUNTRY, .schan=WIFI_CHANNEL_MIN, .nchan=WIFI_CHANNEL_MAX, .policy=WIFI_COUNTRY_POLICY_MANUAL};

// globals
uint16_t salt;

uint16_t reset_salt(void) {
    salt = random(65536); // get new 16bit random for salting hashes and set global salt var
    return salt;
}

bool mac_add(uint8_t *paddr, int8_t rssi, bool sniff_type) {

    char buff[16];                          // temporary buffer for printf
    bool added = false;
    uint32_t addr2int, vendor2int;          // temporary buffer for MAC and Vendor OUI
	uint16_t hashedmac;                     // temporary buffer for generated hash value

    // only last 3 MAC Address bytes are used for MAC address anonymization
    // but since it's uint32 we take 4 bytes to avoid 1st value to be 0
    addr2int =  ( (uint32_t)paddr[2] ) | ( (uint32_t)paddr[3] << 8 ) | ( (uint32_t)paddr[4] << 16 ) | ( (uint32_t)paddr[5] << 24 );

    #ifdef VENDORFILTER
        vendor2int = ( (uint32_t)paddr[2] ) | ( (uint32_t)paddr[1] << 8 ) | ( (uint32_t)paddr[0] << 16 );
        // use OUI vendor filter list only on Wifi, not on BLE
        if ( (sniff_type==MAC_SNIFF_BLE) || std::find(vendors.begin(), vendors.end(), vendor2int) != vendors.end() ) 
    {
    #endif

    // salt and hash MAC, and if new unique one, store identifier in container and increment counter on display
	// https://en.wikipedia.org/wiki/MAC_Address_Anonymization
			
	addr2int += (uint32_t)salt;		    // add 16-bit salt to pseudo MAC
	snprintf(buff, sizeof(buff), "%08X", addr2int);	// convert unsigned 32-bit salted MAC to 8 digit hex string
	hashedmac = rokkit(&buff[3], 5);	    // hash MAC last string value, use 5 chars to fit hash in uint16_t container
	auto newmac = macs.insert(hashedmac);	// add hashed MAC to total container if new unique
    added = newmac.second ? true:false;     // true if hashed MAC is unique in container

    // Count only if MAC was not yet seen
    if (added) {
        // increment counter and one blink led
        if (sniff_type == MAC_SNIFF_WIFI ) {
            macs_wifi++; // increment Wifi MACs counter
            #if (HAS_LED != NOT_A_PIN) || defined (HAS_RGB_LED)
                blink_LED(COLOR_GREEN, 50); 
            #endif
        }   
        #ifdef BLECOUNTER
        else if (sniff_type == MAC_SNIFF_BLE ) {
            macs_ble++; // increment BLE Macs counter
            #if (HAS_LED != NOT_A_PIN) || defined (HAS_RGB_LED)
                blink_LED(COLOR_MAGENTA, 50);
            #endif
        }
        #endif
    } 

    // Log scan result
    ESP_LOGI(TAG, "%s %s RSSI %ddBi -> MAC %s -> Hash %04X -> WiFi:%d  BLTH:%d -> %d Bytes left",
        added ? "new  " : "known", 
        sniff_type==MAC_SNIFF_WIFI ? "WiFi":"BLTH", 
        rssi, buff, hashedmac, macs_wifi, macs_ble,
        ESP.getFreeHeap());

    #ifdef VENDORFILTER
    } else {
        // Very noisy
        // ESP_LOGD(TAG, "Filtered MAC %02X:%02X:%02X:%02X:%02X:%02X", paddr[0],paddr[1],paddr[2],paddr[3],paddr[5],paddr[5]);
    }
    #endif

    // True if MAC WiFi/BLE was new
    return added; // function returns bool if a new and unique Wifi or BLE mac was counted (true) or not (false)
}

void wifi_sniffer_init(void) {
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		cfg.nvs_enable = 0; // we don't need any wifi settings from NVRAM
		wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT}; // we need only MGMT frames
    	ESP_ERROR_CHECK(esp_wifi_init(&cfg));						// configure Wifi with cfg
    	ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country));		// set locales for RF and channels
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));	// we don't need NVRAM
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));	// set MAC frame filter
    	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));			// now switch on monitor mode
}

void wifi_sniffer_set_channel(uint8_t channel) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    
    if (( cfg.rssilimit == 0 ) || (ppkt->rx_ctrl.rssi > cfg.rssilimit )) { // rssi is negative value
        uint8_t *p = (uint8_t *) hdr->addr2;
        mac_add(p, ppkt->rx_ctrl.rssi, MAC_SNIFF_WIFI) ;
    } else {
        ESP_LOGI(TAG, "WiFi RSSI %d -> ignoring (limit: %d)", ppkt->rx_ctrl.rssi, cfg.rssilimit);
    }
}

