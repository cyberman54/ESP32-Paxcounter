#define TTGO_T5_1_2 0
#define TTGO_T5_2_0 0
#define TTGO_T5_2_1 0
#define TTGO_T5_2_2 0
#define TTGO_T5_2_3 1
#define TTGO_T5_2_4 0
#define TTGO_T5_2_8 0   //! Silk screen marking T5_V28_27

#if (TTGO_T5_1_2) || (TTGO_T5_2_4)
//#include <GxGDEP015OC1/GxGDEP015OC1.h>    // 1.54" b/w
// #include <GxGDEW0154Z04/GxGDEW0154Z04.h>  // 1./54" b/w/r 200x200
//#include <GxGDEW0154Z17/GxGDEW0154Z17.h>  // 1.54" b/w/r 152x152
// #include <GxGDEW027W3/GxGDEW027W3.h> // 2.7" b/w
#include <GxGDEH029A1/GxGDEH029A1.h>     // 2.9" b/w
//#include <GxGDEW029Z10/GxGDEW029Z10.h>    // 2.9" b/w/r
//#include <GxGDE0213B1/GxGDE0213B1.h>      // 2.13" b/w
// #include <GxGDEW0213Z16/GxGDEW0213Z16.h>  // 2.13" b/w/r
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17
#define ELINK_SS 5

#define SPI_MOSI 23
#define SPI_MISO -1 //elink no use
#define SPI_CLK 18

#define SDCARD_SS 13
#define SDCARD_MOSI 15
#define SDCARD_CLK 14

#define BUTTON_1 37
#define BUTTON_2 38
#define BUTTON_3 39

#define BUTTONS_MAP \
    {               \
        37, 38, 39  \
    }

#define SPEAKER_OUT 25

#if TTGO_T5_2_4
#define AMP_POWER_CTRL 19
#endif

#elif TTGO_T5_2_1
#include <GxGDEH029A1/GxGDEH029A1.h> // 2.9" b/w
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17
#define ELINK_SS 5

#define SPI_MOSI 23
#define SPI_MISO -1
#define SPI_CLK 18

#define SDCARD_SS 13
#define SDCARD_CLK 14

#define BUTTON_1 37
#define BUTTON_2 38
#define BUTTON_3 39

#define BUTTONS_MAP \
    {               \
        37, 38, 39  \
    }
#define SPEAKER_OUT 25

#elif (TTGO_T5_2_0) || (TTGO_T5_2_3)
// #include <GxGDE0213B1/GxGDE0213B1.h>      // 2.13" b/w
// #include <GxGDE0213B72/GxGDE0213B72.h>      // 2.13" b/w     GxGDE0213B1 升级版本
#include <GxGDE0213B72B/GxGDE0213B72B.h> // 2.13" b/w           GxGDE0213B72 升级版本 默认LilyGO的出厂屏幕都是这种

#define SPI_MOSI 23
#define SPI_MISO -1
#define SPI_CLK 18

#define ELINK_SS 5
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17

#define SDCARD_SS 13
#define SDCARD_CLK 14

#define BUTTON_3 39
#define BUTTONS_MAP \
    {               \
        39          \
    }

#define SPEAKER_OUT -1

#elif TTGO_T5_2_2
#include <GxGDEH029A1/GxGDEH029A1.h> // 2.9" b/w
#define ELINK_BUSY 4
#define ELINK_RESET 12
#define ELINK_DC 19
#define ELINK_SS 5

#define SPI_MOSI 23
#define SPI_MISO 2
#define SPI_CLK 18

#define SDCARD_SS 13
#define BUTTON_1 38
#define BUTTON_2 37
#define BUTTON_3 39

#define BUTTONS_MAP \
    {               \
        37, 38, 39  \
    }

#define SPEAKER_OUT 25

#elif TTGO_T5_XXXX
#include <GxGDEH029A1/GxGDEH029A1.h> // 2.9" b/w
#define ELINK_BUSY 4
#define ELINK_RESET 12
#define ELINK_DC 13
#define ELINK_SS 2

#define SPI_MOSI 15
#define SPI_MISO -1
#define SPI_CLK 14

#define SDCARD_SS -1

#define BUTTON_1 37
#define BUTTON_2 38
#define BUTTON_3 39

#define LORA_SS 18
#define LORA_CLK 5
#define LORA_MOSI 27
#define LORA_MISO 19
#define LORA_DIO0 26
#define LORA_DIO1 -1
#define LORA_DIO2 -1
#define LOAR_RST 23
#define BAND 868E6

#define SPEAKER_OUT -1

#elif TTGO_T5_2_8

#include <GxGDEW027W3/GxGDEW027W3.h>    // 2.7" b/w

#define SPI_MOSI 23
#define SPI_MISO -1
#define SPI_CLK 18

#define ELINK_SS 5
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17

#define SDCARD_SS 13
#define SDCARD_CLK 14

#define BUTTON_1 37
#define BUTTON_2 38
#define BUTTON_3 39
#define BUTTONS_MAP                  \
    {                                \
        BUTTON_1, BUTTON_2, BUTTON_3 \
    }

#define SPEAKER_OUT -1
#define DAC_MAX98357    
#define IIS_BCK 26
#define IIS_WS  25
#define IIS_DOUT 19

#define ICS43434
#define ICS_IIS_WS  33
#define ICS_IIS_BCK 32
#define ICS_IIS_DIN 27


// #define ENABLE_IP5306
#define I2C_SDA     21
#define I2C_SCL     22

// https://github.com/earlephilhower/ESP8266Audio
#include <HTTPClient.h> //Remove ESP8266 warning
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioFileSourceID3.h"
#include "image.h"
#else
#error "Please select board type in board_def.h"
#endif