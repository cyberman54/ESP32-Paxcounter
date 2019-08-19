#ifdef HAS_MATRIX_DISPLAY

#include "globals.h"

#define MATRIX_DISPLAY_PAGES (2) // number of display pages
#define LINE_DIAGRAM_DIVIDER (2) // scales pax numbers to led rows

// local Tag for logging
static const char TAG[] = __FILE__;

uint8_t MatrixDisplayIsOn = 0;
static uint8_t displaybuf[LED_MATRIX_WIDTH * LED_MATRIX_HEIGHT / 8] = {0};
static unsigned long ulLastNumMacs = 0;
static time_t ulLastTime = myTZ.toLocal(now());

LEDMatrix matrix(LED_MATRIX_LA_74138, LED_MATRIX_LB_74138, LED_MATRIX_LC_74138,
                 LED_MATRIX_LD_74138, LED_MATRIX_EN_74138, LED_MATRIX_DATA_R1,
                 LED_MATRIX_LATCHPIN, LED_MATRIX_CLOCKPIN);

// --- SELECT YOUR FONT HERE ---
const FONT_INFO *ActiveFontInfo = &digital7_18ptFontInfo;
// const FONT_INFO *ActiveFontInfo = &arialNarrow_17ptFontInfo;
// const FONT_INFO *ActiveFontInfo = &gillSansMTCondensed_18ptFontInfo;
// const FONT_INFO *ActiveFontInfo = &gillSansMTCondensed_16ptFontInfo;

const uint8_t *iaActiveFont = ActiveFontInfo->Bitmap;
const FONT_CHAR_INFO *ActiveFontCharInfo = ActiveFontInfo->Descriptors;

void init_matrix_display(bool reverse) {
  ESP_LOGI(TAG, "Initializing LED Matrix display");
  matrix.begin(displaybuf, LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT);

  if (MatrixDisplayIsOn)
    matrix.on();
  else
    matrix.off();

  if (reverse)
    matrix.reverse();
  matrix.clear();
  matrix.drawPoint(0, LED_MATRIX_HEIGHT - 1, 1);
} // init_display

void refreshTheMatrixDisplay(bool nextPage) {
  static uint8_t DisplayPage = 0, col = 0, row = 0;
  uint8_t level;
  char buff[16];

  // if Matrixdisplay is switched off we don't refresh it to relax cpu
  if (!MatrixDisplayIsOn && (MatrixDisplayIsOn == cfg.screenon))
    return;

  // set display on/off according to current device configuration
  if (MatrixDisplayIsOn != cfg.screenon) {
    MatrixDisplayIsOn = cfg.screenon;
    if (MatrixDisplayIsOn)
      matrix.on();
    else
      matrix.off();
  }

  if (nextPage) {
    DisplayPage =
        (DisplayPage >= MATRIX_DISPLAY_PAGES - 1) ? 0 : (DisplayPage + 1);
    matrix.clear();
    col = 0;
  }

  switch (DisplayPage % MATRIX_DISPLAY_PAGES) {

    // page 0: number of current pax OR footfall line diagram
    // page 1: time of day

  case 0:

    if (cfg.countermode == 1)

    { // cumulative counter mode -> display total number of pax
      if (ulLastNumMacs != macs.size()) {
        ulLastNumMacs = macs.size();
        matrix.clear();
        DrawNumber(String(ulLastNumMacs));
      }
    }

    else { // cyclic counter mode -> plot a line diagram

      if (ulLastNumMacs != macs.size()) {

        // next count cycle?
        if (macs.size() == 0) {

          // matrix full? then scroll left 1 dot, else increment column
          if (col < (LED_MATRIX_WIDTH - 1))
            col++;
          else
            ScrollLeft(displaybuf, LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT);

        } else
          matrix.drawPoint(col, row, 0); // clear current dot

        // scale and set new dot
        ulLastNumMacs = macs.size();
        level = ulLastNumMacs / LINE_DIAGRAM_DIVIDER;
        row = level <= LED_MATRIX_HEIGHT
                  ? LED_MATRIX_HEIGHT - 1 - level % LED_MATRIX_HEIGHT
                  : 0;
        matrix.drawPoint(col, row, 1);
      }
    }
    break;

  case 1:

    const time_t t = myTZ.toLocal(now());
    if (ulLastTime != t) {
      ulLastTime = t;
      matrix.clear();
      snprintf(buff, sizeof(buff), "%02d:%02d:%02d", hour(t), minute(t),
               second(t));
      DrawNumber(String(buff));
    }
    break;

  } // switch page

  matrix.scan();
}

// (x, y) top-left position, x should be multiple of 8
void DrawChar(uint16_t x, uint16_t y, char cChar) {
  // Get address of char in font char descriptor from font descriptor
  auto CharDescAddress = (cChar - ActiveFontInfo->StartChar);
  // Get offset of char into font bitmap
  uint16_t FontBitmapOffset = ActiveFontCharInfo[CharDescAddress].offset;
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

  for (int i = 0; i < iNumLength; i++) {
    DrawChar(iDigitPos, 0, strNum.charAt(i));
    if (i + 1 == iDotPos) {
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
  return CharDescriptor.width;
}

void ScrollLeft(uint8_t *buf, const uint16_t cols, const uint16_t rows) {
  uint32_t i, k, idx;
  const uint32_t x = cols / 8;

  for (k = 0; k < rows; k++) {
    // scroll a line with x bytes one dot to the left
    for (i = 0; i < x - 1; ++i) {
      idx = i + k * x;
      buf[idx] = (buf[idx] << 1) | ((buf[idx + 1] >> 7) & 1);
    }
    buf[idx + 1] <<= 1;
  }
}

#endif // HAS_MATRIX_DISPLAY