// Fonts.h

#ifndef _FONTS_h
#define _FONTS_h

#include "Arduino.h"

struct FONT_CHAR_INFO {
  uint8_t width; // width in bits
  uint8_t height;
  uint16_t offset; // offset of char into char array
};

struct FONT_INFO {
  uint8_t CharHeight;
  char StartChar;
  char EndChar;
  uint8_t SpaceWidth;
  const FONT_CHAR_INFO *Descriptors;
  const uint8_t *Bitmap;
};

// Font data for Arial Narrow 17pt
extern const uint8_t arialNarrow_17ptBitmaps[];
extern const FONT_INFO arialNarrow_17ptFontInfo;
extern const FONT_CHAR_INFO arialNarrow_17ptDescriptors[];

// Font data for Gill Sans MT Condensed 18pt
extern const uint8_t gillSansMTCondensed_18ptBitmaps[];
extern const FONT_INFO gillSansMTCondensed_18ptFontInfo;
extern const FONT_CHAR_INFO gillSansMTCondensed_18ptDescriptors[];

// Font data for Gill Sans MT Condensed 16pt
extern const uint8_t gillSansMTCondensed_16ptBitmaps[];
extern const FONT_INFO gillSansMTCondensed_16ptFontInfo;
extern const FONT_CHAR_INFO gillSansMTCondensed_16ptDescriptors[];

// Font data for Digital-7 18pt
extern const uint8_t digital7_18ptBitmaps[];
extern const FONT_INFO digital7_18ptFontInfo;
extern const FONT_CHAR_INFO digital7_18ptDescriptors[];

#endif