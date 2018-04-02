
// Basic Config
#include "main.h"
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

bool mac_add(uint8_t *paddr, int8_t rssi, bool sniff_type) {

    char counter [6]; // uint16_t -> 2 byte -> 5 decimals + '0' terminator -> 6 chars
    char macbuf [21]; // uint64_t -> 8 byte -> 20 decimals + '0' terminator -> 21 chars
    char typebuff[8] ;
    bool added = false;
    uint64_t addr2int;
	uint32_t vendor2int;
	uint16_t hashedmac;
	std::pair<std::set<uint16_t>::iterator, bool> newmac;

    addr2int = ( (uint64_t)paddr[0] ) | ( (uint64_t)paddr[1] << 8 ) | ( (uint64_t)paddr[2] << 16 ) | \
    ( (uint64_t)paddr[3] << 24 ) | ( (uint64_t)paddr[4] << 32 ) | ( (uint64_t)paddr[5] << 40 );

    #ifdef VENDORFILTER
    vendor2int = ( (uint32_t)paddr[2] ) | ( (uint32_t)paddr[1] << 8 ) | ( (uint32_t)paddr[0] << 16 );
    // No vendor filter for BLE
    if ( (sniff_type==MAC_SNIFF_BLE) || std::find(vendors.begin(), vendors.end(), vendor2int) != vendors.end() ) {
    #endif

        // salt and hash MAC, and if new unique one, store identifier in container and increment counter on display
		// https://en.wikipedia.org/wiki/MAC_Address_Anonymization
			
		addr2int |= (uint64_t) salt << 48;		// prepend 16-bit salt to 48-bit MAC
		snprintf(macbuf, 21, "%llx", addr2int);	// convert unsigned 64-bit salted MAC to 16 digit hex string
		hashedmac = rokkit(macbuf, 5);			// hash MAC string, use 5 chars to fit hash in uint16_t container
		newmac = macs.insert(hashedmac);		// store hashed MAC only if first time seen

        if (sniff_type == MAC_SNIFF_WIFI ) {
            newmac = wifis.insert(hashedmac); // store hashed MAC if new unique
            strcpy(typebuff, "WiFi");
        } else if (sniff_type == MAC_SNIFF_BLE ) {
            newmac = bles.insert(hashedmac); // store hashed MAC if new unique
            strcpy(typebuff, "BLE ");
        }
     
        if (newmac.second) { // first time seen this WIFI/BLE MAC
            // Insert to global counter
            macs.insert(hashedmac);
            added = true;
            snprintf(counter, 6, "%i", macs.size());		// convert 16-bit MAC counter to decimal counter value
            //itoa(macs.size(), counter, 10); // base 10 decimal counter value
            u8x8.draw2x2String(0, 0, counter);
            ESP_LOGI(TAG, "%s RSSI %04d -> Hash %04x -> #%05i", typebuff, rssi, hashedmac, macs.size());
        } else {
            ESP_LOGI(TAG, "%s RSSI %04d -> already seen", typebuff, rssi);
        }

    #ifdef VENDORFILTER
    } else {
        // Very noisy
        //ESP_LOGI(TAG, "Filtered MAC %02X:%02X:%02X:%02X:%02X:%02X", paddr[0],paddr[1],paddr[2],paddr[3],paddr[5],paddr[5]);
    }
    #endif

    // True if MAC (WiFi/BLE was new)
    return added;
}

#ifdef BLECOUNTER

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        uint8_t *p = (uint8_t *) advertisedDevice.getAddress().getNative();

        // Current devices seen on this scan session
        currentScanDevice++;
        mac_add(p, advertisedDevice.getRSSI(), MAC_SNIFF_BLE);
        u8x8.setCursor(12,3);
        u8x8.printf("%d", currentScanDevice);
    }
};

void BLECount() {
    int blenum = 0; // Total device seen on this scan session
    currentScanDevice = 0; // Set 0 seen device on this scan session
    u8x8.clearLine(3);
    u8x8.drawString(0,3,"BLE Scan...");
    BLEDevice::init(PROGNAME);
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(cfg.blescancycle);
    blenum=foundDevices.getCount();
    u8x8.clearLine(3);
    u8x8.setCursor(0,3);
    u8x8.printf("BLE#: %-5i %-3i",bles.size(), blenum);
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
        ESP_LOGI(TAG, "WiFi RSSI %04d -> ignoring (limit: %i)", ppkt->rx_ctrl.rssi, cfg.rssilimit);
    }
    yield();
}

