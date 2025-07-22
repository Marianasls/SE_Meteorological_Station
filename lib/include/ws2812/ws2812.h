/*
    Aluno: Lucas Carneiro de Araújo Lima
*/

#ifndef WS2812_H
#define WS2812_H

#include "ws2812.pio.h"
#include "hardware/pio.h"
#include <string.h>

#define NUM_PIXELS 25   // 5x5 = 25
#define IS_RGBW false

extern PIO pio;
extern int sm;

// Definição de tipo da estrutura que irá controlar a cor dos LED's
typedef struct {
    double red;
    double green;
    double blue;
} Color;

// Definição de tipo da matriz de leds
typedef Color Led_Matrix[5][5];

void symbol(char symbol);
void ws2812_draw_row(float r, float g, float b, int row, bool clear, bool print);

#endif