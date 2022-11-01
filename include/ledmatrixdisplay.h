#ifndef _MATRIX_DISPLAY_H
#define _MATRIX_DISPLAY_H

#ifdef HAS_MATRIX_DISPLAY

#include "LEDMatrix.h"
#include "ledmatrixfonts.h"
#include "ledmatrixdisplay.h"
#include "configmanager.h"
#include <libpax_api.h>

extern uint8_t MatrixDisplayIsOn;
extern LEDMatrix matrix;
extern hw_timer_t *matrixDisplayIRQ;

void init_matrix_display(bool reverse = false);
void refreshTheMatrixDisplay(bool nextPage = false);
void DrawNumber(String strNum, uint8_t iDotPos = 0);
uint8_t GetCharFromFont(char cChar);
uint8_t GetCharWidth(char cChar);
void ScrollMatrixLeft(uint8_t *buf, const uint16_t cols, const uint16_t rows);

#endif

#endif