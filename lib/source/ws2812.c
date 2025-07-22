/*
    Aluno: Lucas Carneiro de Araújo Lima
*/

#include "ws2812.h"

PIO pio = pio0;
int sm = 0;

//Frames fixos
Led_Matrix blank = {
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, // Linha 0
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, // Linha 1
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, // Linha 2
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, // Linha 3
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}  // Linha 4
};
Led_Matrix mid_row = {
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, // Linha 0
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, // Linha 1
    {{0.0, 0.025, 0.0}, {0.0, 0.025, 0.0}, {0.0, 0.025, 0.0}, {0.0, 0.025, 0.0}, {0.0, 0.025, 0.0}}, // Linha 2
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, // Linha 3
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},  // Linha 4
};

//Definição da cor escolhida
uint32_t color(double b, double r, double g) {
  return (((unsigned char)(g * 255)) << 24) | (((unsigned char)(r * 255)) << 16) | (((unsigned char)(b * 255)) << 8);
}

//Imprimindo o frame do número na matriz de led
void printer(Led_Matrix* frame){
    for (int i = 4; i >= 0; i--){
        if(i % 2) {
            for (int j = 0; j < 5; j ++) 
                pio_sm_put_blocking((PIO)pio, sm, color((*frame)[i][j].blue, (*frame)[i][j].red, (*frame)[i][j].green));
        }else {
            for (int j = 4; j >= 0; j --)
                pio_sm_put_blocking((PIO)pio, sm, color((*frame)[i][j].blue, (*frame)[i][j].red, (*frame)[i][j].green));
        }
    }
}

//Forma o frame do número
void framer(Led_Matrix* frame,Led_Matrix* base,float r, float g, float b, int row, int col, bool clear, bool print) {
    if(clear) memcpy(*frame, *base, sizeof(Led_Matrix));
    (*frame)[row][col].red = r;
    (*frame)[row][col].blue = b;
    (*frame)[row][col].green = g;
    if(print) printer(frame); 
}

void symbol(char symbol) {
    Led_Matrix frame;
   
    if(symbol == 'x') {
        framer(&frame,&blank,0.025,0,0,0,0,true,false);
        framer(&frame,&blank,0.025,0,0,0,4,false,false);
        framer(&frame,&blank,0.025,0,0,4,0,false,false);
        framer(&frame,&blank,0.025,0,0,4,4,false,false);
        framer(&frame,&blank,0.025,0,0,3,1,false,false);
        framer(&frame,&blank,0.025,0,0,3,3,false,false);
        framer(&frame,&blank,0.025,0,0,2,2,false,false);
        framer(&frame,&blank,0.025,0,0,1,1,false,false);
        framer(&frame,&blank,0.025,0,0,1,3,false,true);
    } else if(symbol == 'w') {
        framer(&frame,&blank,0.025,0.025,0,1,2,true,false);
        framer(&frame,&blank,0.025,0.025,0,2,1,false,false);
        framer(&frame,&blank,0.025,0.025,0,2,2,false,false);
        framer(&frame,&blank,0.025,0.025,0,2,3,false,false);
        framer(&frame,&blank,0.025,0.025,0,3,0,false,false);
        framer(&frame,&blank,0.025,0.025,0,3,1,false,false);
        framer(&frame,&blank,0.025,0.025,0,3,2,false,false);
        framer(&frame,&blank,0.025,0.025,0,3,3,false,false);
        framer(&frame,&blank,0.025,0.025,0,3,4,false,true);
    } else if(symbol == 'v') {
        framer(&frame,&mid_row,0,0.025,0,0,2,true,false);
        framer(&frame,&blank,0,0.025,0,1,1,false,false);
        framer(&frame,&blank,0,0.025,0,1,2,false,false);
        framer(&frame,&blank,0,0.025,0,1,3,false,false);
        framer(&frame,&blank,0,0.025,0,3,1,false,false);
        framer(&frame,&blank,0,0.025,0,3,1,false,false);
        framer(&frame,&blank,0,0.025,0,3,2,false,false);
        framer(&frame,&blank,0,0.025,0,3,3,false,false);
        framer(&frame,&blank,0,0.025,0,4,2,false,true);
    }
    else if(symbol == '*') {
        framer(&frame,&blank,0,0,0,0,0,true,true);
    }
}

void ws2812_draw_row(float r, float g, float b, int row, bool clear, bool print) {
    Led_Matrix frame;

    framer(&frame,&blank,r,g,b,row,0,clear,false);
    framer(&frame,&blank,r,g,b,row,1,false,false);
    framer(&frame,&blank,r,g,b,row,2,false,false);
    framer(&frame,&blank,r,g,b,row,3,false,false);
    framer(&frame,&blank,r,g,b,row,4,false,print);
}