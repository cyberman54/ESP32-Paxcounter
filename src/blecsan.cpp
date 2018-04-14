#ifdef BLECOUNTER

/* code snippets taken from
https://github.com/nkolban/esp32-snippets/tree/master/BLE/scanner
*/

// Basic Config
#include "globals.h"

// Bluetooth specific includes
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_blufi_api.h>
#include <bt_types.h>

#define BT_BD_ADDR_HEX(addr)   addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

// local Tag for logging
static const char *TAG = "bt_loop";

// defined in macsniff.cpp
bool mac_add(uint8_t *paddr, int8_t rssi, bool sniff_type);

// Prototypes
static const char *bt_event_type_to_string(uint32_t eventType);
static const char *bt_gap_search_event_type_to_string(uint32_t searchEvt);
static const char *bt_addr_t_to_string(esp_ble_addr_type_t type);
static const char *bt_dev_type_to_string(esp_bt_dev_type_t type);
static const char *btsig_gap_type(uint32_t gap_type);


static void gap_callback_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	ESP_LOGD(TAG, "Received a GAP event: %s", bt_event_type_to_string(event));
	esp_ble_gap_cb_param_t *p = (esp_ble_gap_cb_param_t *)param;
	esp_err_t status;	

	switch (event) 
	{
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
		{
			ESP_LOGD(TAG, "Start Scan, status: %d", p->scan_param_cmpl.status);
		
			// This procedure keep the device scanning the peer device which advertising on the air.
			status = esp_ble_gap_start_scanning(BLESCANTIME); 
			if (status != ESP_OK) 
			{
				ESP_LOGE(TAG, "esp_ble_gap_start_scanning: rc=%d", status);
			}
		}
		break;
		
		case ESP_GAP_BLE_SCAN_RESULT_EVT:
		{	
		
			ESP_LOGI(TAG, "Device address (bda): %02x:%02x:%02x:%02x:%02x:%02x", BT_BD_ADDR_HEX(p->scan_rst.bda));
			ESP_LOGI(TAG, "Device type         : %s", bt_dev_type_to_string(p->scan_rst.dev_type));
			ESP_LOGI(TAG, "Search_evt          : %s", bt_gap_search_event_type_to_string(p->scan_rst.search_evt));
			ESP_LOGI(TAG, "Addr_type           : %s", bt_addr_t_to_string(p->scan_rst.ble_addr_type));
			ESP_LOGI(TAG, "RSSI                : %d", p->scan_rst.rssi);
			ESP_LOGI(TAG, "Flag                : %d", p->scan_rst.flag);
			  
			//bit 0 (OFF) LE Limited Discoverable Mode
   			//bit 1 (OFF) LE General Discoverable Mode
   			//bit 2 (ON) BR/EDR Not Supported
   			//bit 3 (OFF) Simultaneous LE and BR/EDR to Same Device Capable (controller)
   			//bit 4 (OFF) Simultaneous LE and BR/EDR to Same Device Capable (Host)
			
		  	ESP_LOGI(TAG, "num_resps           : %d", p->scan_rst.num_resps);

			if ( p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) // Inquiry complete, scan is done
			{	// restart scan
				status = esp_ble_gap_start_scanning	(BLESCANTIME); 
				if (status != ESP_OK) 
				{
					ESP_LOGE(TAG, "esp_ble_gap_start_scanning: rc=%d", status);
				}
				return;
			}
			
			if (p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) // Inquiry result for a peer device
			{

				#ifdef VENDORFILTER
					if ( 	(p->scan_rst.ble_addr_type == BLE_ADDR_TYPE_PUBLIC) ||
							(p->scan_rst.ble_addr_type == BLE_ADDR_TYPE_RPA_PUBLIC)
						)
					 {
				#endif

					// add this device and show new count total if it was not previously added
            		mac_add((uint8_t *) p->scan_rst.bda, p->scan_rst.rssi, MAC_SNIFF_BLE);	

				#ifdef VENDORFILTER
					}
					else
					{
						ESP_LOGI(TAG, "BLE device filtered");	
					}
				#endif
					

				/* to be improved in vendorfilter if:
				
				// you can search for elements in the payload using the
				// function esp_ble_resolve_adv_data()
				//
				// Like this, that scans for the "Complete name" (looking inside the payload buffer)
				// uint8_t len;
				// uint8_t *data = esp_ble_resolve_adv_data(p->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &len);

				filter BLE devices using their advertisements to get filter alternative to vendor OUI
				if vendorfiltering is on, we ...
				- want to count: mobile phones and tablets
				- don't want to count: beacons, peripherals (earphones, headsets, printers), cars and machines
				see
				https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/src/BLEAdvertisedDevice.cpp

				http://www.libelium.com/products/meshlium/smartphone-detection/

				https://www.question-defense.com/2013/01/12/bluetooth-cod-bluetooth-class-of-deviceclass-of-service-explained

				https://www.bluetooth.com/specifications/assigned-numbers/baseband

				"The Class of Device (CoD) in case of Bluetooth which allows us to differentiate the type of 
				device (smartphone, handsfree, computer, LAN/network AP). With this parameter we can 
				differentiate among pedestrians and vehicles."

				*/

			}

		}
		break;

		default:
        break;
	}
} // gap_callback_handler

static const char *bt_dev_type_to_string(esp_bt_dev_type_t type) {
	switch(type) {
	case ESP_BT_DEVICE_TYPE_BREDR:
		return "ESP_BT_DEVICE_TYPE_BREDR";
	case ESP_BT_DEVICE_TYPE_BLE:
		return "ESP_BT_DEVICE_TYPE_BLE";
	case ESP_BT_DEVICE_TYPE_DUMO:
		return "ESP_BT_DEVICE_TYPE_DUMO";
	default:
		return "Unknown";
	}
} // bt_dev_type_to_string

static const char *bt_addr_t_to_string(esp_ble_addr_type_t type) {
	switch(type) {
		case BLE_ADDR_TYPE_PUBLIC:
			return "BLE_ADDR_TYPE_PUBLIC";
		case BLE_ADDR_TYPE_RANDOM:
			return "BLE_ADDR_TYPE_RANDOM";
		case BLE_ADDR_TYPE_RPA_PUBLIC:
			return "BLE_ADDR_TYPE_RPA_PUBLIC";
		case BLE_ADDR_TYPE_RPA_RANDOM:
			return "BLE_ADDR_TYPE_RPA_RANDOM";
		default:
			return "Unknown addr_t";
	}
} // bt_addr_t_to_string

static const char *bt_gap_search_event_type_to_string(uint32_t searchEvt) {
	switch(searchEvt) {
		case ESP_GAP_SEARCH_INQ_RES_EVT:
			return "ESP_GAP_SEARCH_INQ_RES_EVT";
		case ESP_GAP_SEARCH_INQ_CMPL_EVT:
			return "ESP_GAP_SEARCH_INQ_CMPL_EVT";
		case ESP_GAP_SEARCH_DISC_RES_EVT:
			return "ESP_GAP_SEARCH_DISC_RES_EVT";
		case ESP_GAP_SEARCH_DISC_BLE_RES_EVT:
			return "ESP_GAP_SEARCH_DISC_BLE_RES_EVT";
		case ESP_GAP_SEARCH_DISC_CMPL_EVT:
			return "ESP_GAP_SEARCH_DISC_CMPL_EVT";
		case ESP_GAP_SEARCH_DI_DISC_CMPL_EVT:
			return "ESP_GAP_SEARCH_DI_DISC_CMPL_EVT";
		case ESP_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT:
			return "ESP_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT";
		default:
			return "Unknown event type";
	}
} // bt_gap_search_event_type_to_string

/*
 * Convert a BT GAP event type to a string representation.
 */
static const char *bt_event_type_to_string(uint32_t eventType) {
	switch(eventType) {
		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
			return "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT";
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
			return "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT";
		case ESP_GAP_BLE_SCAN_RESULT_EVT:
			return "ESP_GAP_BLE_SCAN_RESULT_EVT";
		case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
			return "ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT";
		default:
			return "Unknown event type";
	}
} // bt_event_type_to_string

static const char *btsig_gap_type(uint32_t gap_type) {
	switch (gap_type)
	{
		case 0x01: return "Flags";
		case 0x02: return "Incomplete List of 16-bit Service Class UUIDs";
		case 0x03: return "Complete List of 16-bit Service Class UUIDs";
		case 0x04: return "Incomplete List of 32-bit Service Class UUIDs";
		case 0x05: return "Complete List of 32-bit Service Class UUIDs";
		case 0x06: return "Incomplete List of 128-bit Service Class UUIDs";
		case 0x07: return "Complete List of 128-bit Service Class UUIDs";
		case 0x08: return "Shortened Local Name";
		case 0x09: return "Complete Local Name";
		case 0x0A: return "Tx Power Level";
		case 0x0D: return "Class of Device";
		case 0x0E: return "Simple Pairing Hash C/C-192";
		case 0x0F: return "Simple Pairing Randomizer R/R-192";
		case 0x10: return "Device ID/Security Manager TK Value";
		case 0x11: return "Security Manager Out of Band Flags";
		case 0x12: return "Slave Connection Interval Range";
		case 0x14: return "List of 16-bit Service Solicitation UUIDs";
		case 0x1F: return "List of 32-bit Service Solicitation UUIDs";
		case 0x15: return "List of 128-bit Service Solicitation UUIDs";
		case 0x16: return "Service Data - 16-bit UUID";
		case 0x20: return "Service Data - 32-bit UUID";
		case 0x21: return "Service Data - 128-bit UUID";
		case 0x22: return "LE Secure Connections Confirmation Value";
		case 0x23: return "LE Secure Connections Random Value";
		case 0x24: return "URI";
		case 0x25: return "Indoor Positioning";
		case 0x26: return "Transport Discovery Data";
		case 0x17: return "Public Target Address";
		case 0x18: return "Random Target Address";
		case 0x19: return "Appearance";
		case 0x1A: return "Advertising Interval";
		case 0x1B: return "LE Bluetooth Device Address";
		case 0x1C: return "LE Role";
		case 0x1D: return "Simple Pairing Hash C-256";
		case 0x1E: return "Simple Pairing Randomizer R-256";
		case 0x3D: return "3D Information Data";
		case 0xFF: return "Manufacturer Specific Data";
		
		default: 
			return "Unknown type";
	}
}
	
esp_err_t register_ble_functionality(void)
{
	esp_err_t status;	
	
	ESP_LOGI(TAG, "Register GAP callback");
	
	// This function is called to occur gap event, such as scan result.
	//register the scan callback function to the gap module
	status = esp_ble_gap_register_callback(gap_callback_handler);
	if (status != ESP_OK) 
	{
		ESP_LOGE(TAG, "esp_ble_gap_register_callback: rc=%d", status);
		return ESP_FAIL;
	}

	static esp_ble_scan_params_t ble_scan_params = 
	{	
		.scan_type              = BLE_SCAN_TYPE_PASSIVE,
		.own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
		.scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_UND_RPA_DIR,
		// .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
		.scan_interval          = (uint16_t) (BLESCANINTERVAL / 0.625),		// Time = N * 0.625 msec
		.scan_window            = (uint16_t) (BLESCANWINDOW / 0.625)		// Time = N * 0.625 msec	
	};

	ESP_LOGI(TAG, "Set GAP scan parameters");

	// This function is called to set scan parameters.			
 	status = esp_ble_gap_set_scan_params(&ble_scan_params);		
 	if (status != ESP_OK) 		
 	{		
 		ESP_LOGE(TAG, "esp_ble_gap_set_scan_params: rc=%d", status);		
 		return ESP_FAIL;		
 	}		
	
	return ESP_OK ;
}


// Main start code running in its own Xtask
void bt_loop(void *ignore)
{
	esp_err_t status;
	
	// Initialize BT controller to allocate task and other resource. 
	ESP_LOGI(TAG, "Enabling Bluetooth Controller");
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	if (esp_bt_controller_init(&bt_cfg) != ESP_OK) 
	{
		ESP_LOGE(TAG, "Bluetooth controller initialize failed");
		goto end; 
	}

	// Enable BT controller
	if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) 
	{
		ESP_LOGE(TAG, "Bluetooth controller enable failed");
		goto end; 
	}
	
	// Init and alloc the resource for bluetooth, must be prior to every bluetooth stuff
	ESP_LOGI(TAG, "Init Bluetooth stack");
	status = esp_bluedroid_init(); 
	if (status != ESP_OK)
	{ 
		ESP_LOGE(TAG, "%s init bluetooth failed\n", __func__); 
		goto end; 
	} 
	
	// Enable bluetooth, must after esp_bluedroid_init()
	status = esp_bluedroid_enable(); 
	if (status != ESP_OK) 
	{ 
		ESP_LOGE(TAG, "%s enable bluetooth failed\n", __func__); 
		goto end;
	} 

	ESP_LOGI(TAG, "Register BLE functionality");
	status = register_ble_functionality();
	if (status != ESP_OK)
	{
		ESP_LOGE(TAG, "Register BLE functionality failed");
		goto end;
	}

	while(1)
    {
        vTaskDelay(500);
        yield();
    }

end:
	ESP_LOGI(TAG, "Terminating BT logging task");
	vTaskDelete(NULL);
		
} // bt_loop

#endif // BLECOUNTER