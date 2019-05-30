#ifndef _MATRIX_DISPLAY_H
#define _MATRIX_DISPLAY_H

#include "LEDMatrix.h"
#include "ledmatrixfonts.h"

extern uint8_t MatrixDisplayIsOn;

extern LEDMatrix matrix;

void init_matrix_display(bool reverse = false);
void refreshTheMatrixDisplay(void);
void DrawNumber(String strNum, uint8_t iDotPos = 0);
uint8_t GetCharFromFont(char cChar);
uint8_t GetCharWidth(char cChar);

#endif