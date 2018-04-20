
// program version - note: increment version after modifications to configData_t struct!!
#define PROGVERSION                     "1.3.2"    // use max 10 chars here!
#define PROGNAME                        "PAXCNT"

//--- Declarations ---

enum states { 
  LED_OFF,
  LED_ON
};

//--- Prototypes ---

// defined in main.cpp
void blink_LED (uint16_t set_color, uint16_t set_blinkduration, uint16_t set_interval);
void reset_counters();

// defined in configmanager.cpp
void eraseConfig(void);
void saveConfig(void);
void loadConfig(void);

// defined in lorawan.cpp
void onEvent(ev_t ev);
void do_send(osjob_t* j);
void gen_lora_deveui(uint8_t * pdeveui);
void RevBytes(unsigned char* b, size_t c);
void get_hard_deveui(uint8_t *pdeveui);

// defined in wifisniffer.cpp
void wifi_sniffer_init(void);
void wifi_sniffer_set_channel(uint8_t channel);
void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

// defined in blescan.cpp
void bt_loop(void *ignore);

// defined in main.cpp
void reset_counters(void);