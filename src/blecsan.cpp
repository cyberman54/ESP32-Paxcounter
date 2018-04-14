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

// defined in macsniff.cpp
bool mac_add(uint8_t *paddr, int8_t rssi, bool sniff_type);

#define BT_BD_ADDR_STR         "%02x:%02x:%02x:%02x:%02x:%02x"
#define BT_BD_ADDR_HEX(addr)   addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

// Prototypes
static const char *bt_event_type_to_string(uint32_t eventType);
static const char *bt_gap_search_event_type_to_string(uint32_t searchEvt);
static const char *bt_addr_t_to_string(esp_ble_addr_type_t type);
static const char *bt_dev_type_to_string(esp_bt_dev_type_t type);
static void gap_callback_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

// local Tag for logging
static const char *TAG = "bt_loop";

static void gap_callback_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	ESP_LOGD(TAG, "Received a GAP event: %s", bt_event_type_to_string(event));
	esp_ble_gap_cb_param_t *p = (esp_ble_gap_cb_param_t *)param;

	esp_err_t status;	


	switch (event) 
	{
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
		{
			ESP_LOGD(TAG, "status: %d", p->scan_param_cmpl.status);
		
			// This procedure keep the device scanning the peer device which advertising on the air.
			status = esp_ble_gap_start_scanning(BLESCANTIME); 
			if (status != ESP_OK) 
			{
				ESP_LOGE(TAG, "esp_ble_gap_start_scanning: rc=%d", status);
			}
		}
		break;

		case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
		{
			//scan start complete event to indicate scan start successfully or failed
			if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
			{
          	  ESP_LOGE(TAG, "Scan start failed");
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

            /* to be done here:
            #ifdef VENDORFILTER
            
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

            #endif
            */

            // add this device and show new count total if it was not previously added
            mac_add((uint8_t *) p->scan_rst.bda, p->scan_rst.rssi, MAC_SNIFF_BLE);
              
			if ( p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT)
			{
				// Scan is done.

				// The next 5 codelines automatically restarts the scan.
				status = esp_ble_gap_start_scanning	(BLESCANTIME); 
				if (status != ESP_OK) 
				{
					ESP_LOGE(TAG, "esp_ble_gap_start_scanning: rc=%d", status);
				}

				return;
			}

			
			if (p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) 
			{

				// you can search for elements in the payload using the
				// function esp_ble_resolve_adv_data()
				//
				// Like this, that scans for the "Complete name" (looking inside the payload buffer)
			
				uint8_t len;
				uint8_t *data = esp_ble_resolve_adv_data(p->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &len);
				ESP_LOGD(TAG, "len: %d, %.*s", len, len, data);
			
			}
			ESP_LOGI(TAG, "");
			
		}
		break;

		case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
		{
			if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
			{
				ESP_LOGE(TAG, "Scan stop failed");
			}
			else 
			{
				ESP_LOGI(TAG, "Stop scan successfully");
			}
		}

		case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
		{
			if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
			{
				ESP_LOGE(TAG, "Adv stop failed");
			}
			else 
			{
				ESP_LOGI(TAG, "Stop adv successfully");
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
		.scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
		.scan_interval          = (uint16_t) (BLESCANINTERVAL / 0.625),      // Time = N * 0.625 msec
		.scan_window            = (uint16_t) (BLESCANWINDOW / 0.625)	        // Time = N * 0.625 msec	
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
	ESP_LOGI(TAG, "Enabling Bluetooth Controller...");
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
	ESP_LOGI(TAG, "Init Bluetooth stack...");
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

	ESP_LOGI(TAG, "Register BLE functionality...");
	status = register_ble_functionality();
	if (status != ESP_OK)
	{
		ESP_LOGE(TAG, "Register BLE functionality failed");
		goto end;
	}

	while(1)
    {
        vTaskDelay(1000);
        yield();
    }

end:
	ESP_LOGI(TAG, "Terminating BT logging task");
	vTaskDelete(NULL);
		
} // bt_loop

#endif // BLECOUNTER