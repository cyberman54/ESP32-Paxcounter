// Basic Config
#include "main.h"
#include "globals.h"

// ESP32 Functions
#include <esp_wifi.h>

#ifdef VENDORFILTER
	#include <array>
	#include <algorithm>
	#include "vendor_array.h"
#endif

// Local logging tag
static const char *TAG = "wifisniffer";

// function defined in rokkithash.cpp
uint32_t rokkit(const char * , int );

static wifi_country_t wifi_country = {.cc=WIFI_MY_COUNTRY, .schan=WIFI_CHANNEL_MIN, .nchan=WIFI_CHANNEL_MAX, .policy=WIFI_COUNTRY_POLICY_MANUAL};

typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

extern void wifi_sniffer_init(void);
extern void wifi_sniffer_set_channel(uint8_t channel);
extern void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

void wifi_sniffer_init(void) {
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		cfg.nvs_enable = 0; // we don't need any wifi settings from NVRAM
		wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT}; // we need only MGMT frames
    	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    	ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country));
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM) );
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL) );
    	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter)); // set MAC frame filter
    	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
}

void wifi_sniffer_set_channel(uint8_t channel) {
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
	const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
	char counter [6]; // uint16_t -> 2 byte -> 5 decimals + '0' terminator -> 6 chars
	char macbuf [21]; // uint64_t -> 8 byte -> 20 decimals + '0' terminator -> 21 chars
	uint64_t addr2int;
	uint32_t vendor2int;
	uint16_t hashedmac; 
	std::pair<std::set<uint16_t>::iterator, bool> newmac;

	if (( cfg.rssilimit == 0 ) || (ppkt->rx_ctrl.rssi > cfg.rssilimit )) { // rssi is negative value
	    addr2int = ( (uint64_t)hdr->addr2[0] ) | ( (uint64_t)hdr->addr2[1] << 8 ) | ( (uint64_t)hdr->addr2[2] << 16 ) | \
			( (uint64_t)hdr->addr2[3] << 24 ) | ( (uint64_t)hdr->addr2[4] << 32 ) | ( (uint64_t)hdr->addr2[5] << 40 );

#ifdef VENDORFILTER
		vendor2int = ( (uint32_t)hdr->addr2[2] ) | ( (uint32_t)hdr->addr2[1] << 8 ) | ( (uint32_t)hdr->addr2[0] << 16 );

		if ( std::find(vendors.begin(), vendors.end(), vendor2int) != vendors.end() ) {
#endif
		
		//if (!(addr2int & WIFI_MAC_FILTER_MASK)) { // filter local and group MACs   

			// salt and hash MAC, and if new unique one, store hash in container and increment counter on display
			addr2int <<= 8; // left shift out msb of vendor oui
			addr2int += salt; // append salt value to MAC before hashing it
			itoa(addr2int, macbuf, 10); // convert 64 bit MAC to base 10 decimal string
			hashedmac = rokkit(macbuf, 5); // hash MAC for privacy, use 5 chars to fit in uint16_t container
			newmac = macs.insert(hashedmac); // store hashed MAC if new unique
			if (newmac.second) { // first time seen MAC
				macnum++; // increment MAC counter
				itoa(macnum, counter, 10); // base 10 decimal counter value
				u8x8.draw2x2String(0, 0, counter);
				ESP_LOGI(TAG, "RSSI %04d -> Hash %05u -> #%05i", ppkt->rx_ctrl.rssi, hashedmac, macnum);
			}
			else // already seen MAC
				ESP_LOGI(TAG, "RSSI %04d -> already seen", ppkt->rx_ctrl.rssi);
		//}

#ifdef VENDORFILTER
		}
#endif
	} else
		ESP_LOGI(TAG, "RSSI %04d -> ignoring (limit: %i)", ppkt->rx_ctrl.rssi, cfg.rssilimit);

	yield();
}
