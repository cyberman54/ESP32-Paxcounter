#ifdef HAS_MATRIX_DISPLAY

#include "globals.h"

#define NUMCHARS 5

// local Tag for logging
static const char TAG[] = __FILE__;

uint8_t MatrixDisplayIsOn = 0;
static unsigned long ulLastNumMacs = 0;

LEDMatrix matrix(LED_MATRIX_LA_74138, LED_MATRIX_LB_74138, LED_MATRIX_LC_74138,
                 LED_MATRIX_LD_74138, LED_MATRIX_EN_74138, LED_MATRIX_DATA_R1,
                 LED_MATRIX_LATCHPIN, LED_MATRIX_CLOCKPIN);

// Display Buffer 128 = 64 * 16 / 8
uint8_t displaybuf[LED_MATRIX_WIDTH * LED_MATRIX_HEIGHT / NUMCHARS];

const FONT_INFO *ActiveFontInfo = &digital7_18ptFontInfo;
const uint8_t *iaActiveFont = ActiveFontInfo->Bitmap;
const FONT_CHAR_INFO *ActiveFontCharInfo = ActiveFontInfo->Descriptors;

void init_matrix_display(const char *Productname, const char *Version) {
  ESP_LOGI(TAG, "Initializing LED Matrix display");
  matrix.begin(displaybuf, LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT);
  //matrix.reverse();
  matrix.clear();
  DrawNumber(String("0"));
} // init_display

void refreshTheMatrixDisplay() {

  // if Matrixdisplay is switched off we don't refresh it to relax cpu
  if (!MatrixDisplayIsOn && (MatrixDisplayIsOn == cfg.screenon))
    return;

  // set display on/off according to current device configuration
  if (MatrixDisplayIsOn != cfg.screenon) {
    MatrixDisplayIsOn = cfg.screenon;
  }

  if (ulLastNumMacs != macs.size()) {
    ulLastNumMacs = macs.size();
    matrix.clear();
    DrawNumber(String(macs.size()));
    ESP_LOGI(TAG, "Setting display to counter: %lu", macs.size());
  }

  matrix.scan();
}

// (x, y) top-left position, x should be multiple of 8
void DrawChar(uint16_t x, uint16_t y, char cChar) {
  // Get address of char in font char descriptor from font descriptor
  auto CharDescAddress = (cChar - ActiveFontInfo->StartChar);

  // Get offset of char into font bitmap
  uint16_t FontBitmapOffset = ActiveFontCharInfo[CharDescAddress].offset;
  // Serial.printf("Address of %c is %i, bitmap offset is %u\r\n", cChar,
  //              CharDescAddress, FontBitmapOffset);

  // Check font height, if it's less than matrix height we need to
  // add some empty lines to font does not stick to the top
  if (ActiveFontInfo->CharHeight < (LED_MATRIX_HEIGHT - y)) {
    uint8_t FillerLines = (LED_MATRIX_HEIGHT - y) - ActiveFontInfo->CharHeight;
    if (FillerLines % 2 == 0) {
      // Use floor (round down) to get heighest possible divider
      // for missing lines
      y = floor(FillerLines / 2);
    }
  }

  int iDst = (x / 8) + (y * 8);
  int Shift = x % 8;
  // Serial.printf("Got hex '%x'\r\n", pSrc);
  for (uint8_t i = 0; i < ActiveFontCharInfo[CharDescAddress].height; i++) {
    int iDigitA = iaActiveFont[FontBitmapOffset];

    int Left = iDigitA >> Shift;
    int Right = iDigitA << (8 - Shift);

    displaybuf[iDst] |= Left;
    displaybuf[iDst + 1] |= Right;

    if (ActiveFontCharInfo[CharDescAddress].width > 8) {
      int iDigitB = iaActiveFont[FontBitmapOffset + 1];
      Left = iDigitB >> Shift;
      Right = iDigitB << (8 - Shift);

      displaybuf[iDst + 1] |= Left;
      displaybuf[iDst + 2] |= Right;
      FontBitmapOffset++;
    }

    FontBitmapOffset++;
    iDst += 8;
  }
}

void DrawNumber(String strNum, uint8_t iDotPos) {
  uint8_t iNumLength = strNum.length();
  uint8_t iDigitPos = 0;

  // Serial.printf("Showing number '%s' (length: %i)\r\n", strNum.c_str(),
  //              iNumLength);
  for (int i = 0; i < iNumLength; i++) {
    // Serial.printf("Showing char '%c' at x:%i y:%i\r\n",
    // strNum.charAt(i),
    //              iDigitPos, 0);
    DrawChar(iDigitPos, 0, strNum.charAt(i));
    if (i + 1 == iDotPos) {
      // matrix.drawRect((iDigitPos * 8) - 1, 15, iDigitPos * 8,
      // 16, 1);
      iDigitPos = iDigitPos + GetCharWidth(strNum.charAt(i)) +
                  ActiveFontInfo->SpaceWidth;
      DrawChar(iDigitPos, 0, '.');
      iDigitPos = iDigitPos + GetCharWidth('.') + ActiveFontInfo->SpaceWidth;
    } else {
      iDigitPos = iDigitPos + GetCharWidth(strNum.charAt(i)) +
                  ActiveFontInfo->SpaceWidth;
    }
  }
}

uint8_t GetCharFromFont(char cChar) {
  auto cStartChar = ActiveFontInfo->StartChar;
  auto iCharLocation = cChar - cStartChar;
  auto iCharBitmap = iaActiveFont[iCharLocation];

  return iCharBitmap;
}

uint8_t GetCharWidth(char cChar) {
  // Get address of char in font char descriptor from font descriptor
  auto CharDescAddress = (cChar - ActiveFontInfo->StartChar);

  // Get offset of char into font bitmap
  auto CharDescriptor = ActiveFontCharInfo[CharDescAddress];
  // Serial.printf("Char %c is %i wide\r\n", cChar, CharDescriptor.width);
  return CharDescriptor.width;
}

#endif // HAS_MATRIX_DISPLAY