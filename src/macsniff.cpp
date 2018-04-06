
// Basic Config
#include "globals.h"

#ifdef BLECOUNTER
    #include <BLEDevice.h>
    #include <BLEUtils.h>
    #include <BLEScan.h>
    #include <BLEAdvertisedDevice.h>
#endif

#ifdef VENDORFILTER
    #include <array>
    #include <algorithm>
    #include "vendor_array.h"
#endif

// Local logging tag
static const char *TAG = "macsniff";

static wifi_country_t wifi_country = {.cc=WIFI_MY_COUNTRY, .schan=WIFI_CHANNEL_MIN, .nchan=WIFI_CHANNEL_MAX, .policy=WIFI_COUNTRY_POLICY_MANUAL};

uint16_t currentScanDevice = 0;
uint16_t salt;

uint16_t salt_reset(void) {
    salt = random(65536); // get new 16bit random for salting hashes
    return salt;
}

bool mac_add(uint8_t *paddr, int8_t rssi, bool sniff_type) {

    char buff[16]; // temporary buffer for printf
    bool added = false;
    uint32_t addr2int;
	uint32_t vendor2int;
	uint16_t hashedmac;

    // only last 3 MAC Address bytes are used for MAC Address Anonymization
    // but since it's uint32 we take 4 bytes to avoid 1st value to be 0
    addr2int =  ( (uint32_t)paddr[2] ) | ( (uint32_t)paddr[3] << 8 ) | ( (uint32_t)paddr[4] << 16 ) | ( (uint32_t)paddr[5] << 24 );

    #ifdef VENDORFILTER
        vendor2int = ( (uint32_t)paddr[2] ) | ( (uint32_t)paddr[1] << 8 ) | ( (uint32_t)paddr[0] << 16 );
        // No vendor filter for BLE
        if ( (sniff_type==MAC_SNIFF_BLE) || std::find(vendors.begin(), vendors.end(), vendor2int) != vendors.end() ) {
    #endif

        // salt and hash MAC, and if new unique one, store identifier in container and increment counter on display
		// https://en.wikipedia.org/wiki/MAC_Address_Anonymization
			
		addr2int += (uint32_t) salt;		    // add 16-bit salt to pseudo MAC
		snprintf(buff, sizeof(buff), "%08X", addr2int);	// convert unsigned 32-bit salted MAC to 8 digit hex string
		hashedmac = rokkit(&buff[3], 5);	    // hash MAC last string value, use 5 chars to fit hash in uint16_t container
		auto newmac = macs.insert(hashedmac);	// add hashed MAC to total container if new unique
        added = newmac.second ? true:false;     // true if hashed MAC is unique in container

        // Insert only if it was not found on global count
        if (added) {
            if (sniff_type == MAC_SNIFF_WIFI ) {
                rgb_set_color(COLOR_GREEN);
                wifis.insert(hashedmac);   // add hashed MAC to wifi container if new unique
            } else if (sniff_type == MAC_SNIFF_BLE ) {
                rgb_set_color(COLOR_MAGENTA);
                bles.insert(hashedmac);    // add hashed MAC to BLE container if new unique
            }
            // Not sure user will have time to see the LED
            // TBD do light off further in the code
            rgb_set_color(COLOR_NONE);
        } 
        
        ESP_LOGI(TAG, "%s RSSI %ddBi -> MAC %s -> Hash %04X -> WiFi:%d  BLE:%d  %s", 
                        sniff_type==MAC_SNIFF_WIFI ? "WiFi":"BLE ", 
                        rssi, buff, hashedmac, 
                        (int) wifis.size(), (int) bles.size(),
                        added ? "New" : "Already seen");

    #ifdef VENDORFILTER
    } else {
        // Very noisy
        //ESP_LOGI(TAG, "Filtered MAC %02X:%02X:%02X:%02X:%02X:%02X", paddr[0],paddr[1],paddr[2],paddr[3],paddr[5],paddr[5]);
    }
    #endif

    // True if MAC WiFi/BLE was new
    return added; // function returns bool if a new and unique Wifi or BLE mac was counted (true) or not (false)
}

#ifdef BLECOUNTER

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        int lastcount = (int) macs.size();
        uint8_t *p = (uint8_t *) advertisedDevice.getAddress().getNative();

        // Current devices seen on this scan session
        currentScanDevice++;
        // add this device and refresh display if it was not previously added
        if ( mac_add(p, advertisedDevice.getRSSI(), MAC_SNIFF_BLE) ) {
            char buff[16];
            snprintf(buff, sizeof(buff), "PAX:%d", (int) macs.size()); // convert 16-bit MAC counter to decimal counter value
            u8x8.setCursor(0,0);
            u8x8.draw2x2String(0, 0, buff);          // display number on unique macs total Wifi + BLE
        }
        u8x8.setCursor(12,3);
        u8x8.printf("%d", currentScanDevice);
    }
};

void BLECount() {
    ESP_LOGI(TAG, "BLE scan started");
    currentScanDevice = 0; // Set 0 seen device on this scan session
    //u8x8.clearLine(3); // not needed?
    u8x8.drawString(0,3,"BLE Scan...");
    BLEDevice::init(PROGNAME);
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(cfg.blescantime);
    int blenum=foundDevices.getCount();
    ESP_LOGI(TAG, "BLE scan done, seen %d device(s)", blenum);
}
#endif

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
    //yield();
}

