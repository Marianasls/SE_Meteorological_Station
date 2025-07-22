#ifndef GLOBALS_H
#define GLOBALS_H

// --- Includes Comuns do Sistema ---
#include "hardware/i2c.h"
#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Sensores na I2C0
#define I2C_PORT i2c0               // i2c0 pinos 0 e 1, i2c1 pinos 2 e 3
#define I2C_SDA 0                   // 0 ou 2
#define I2C_SCL 1                   // 1 ou 3
#define SEA_LEVEL_PRESSURE 101325.0 // Pressão ao nível do mar em Pa
// Display na I2C1
#define I2C_PORT_DISP i2c1   	// Define que o barramento I2C usado será o "i2c0"
#define I2C_SDA_DISP 14      	// Define que o pino GPIO 14 será usado como SDA (linha de dados do I2C)
#define I2C_SCL_DISP 15      	// Define que o pino GPIO 15 será usado como SCL (linha de clock do I2C)
#define DISPLAY_ADDRESS 0x3C	    // Define o endereço I2C do dispositivo (0x3C é o endereço padrão de muitos displays OLED SSD1306)
#define DISPLAY_WIDTH       128
#define DISPLAY_HEIGHT      64

#define BTNA 5          	// Botão A
#define BTNB 6          	// Botão B
#define SW_BNT 22           // Botão SW
#define WS2812_PIN 7    	// Matriz de LEDs 5x5
#define LED_GREEN 11        // Led verde
#define LED_RED 13          // Led vermelho
#define LED_BLUE 12          // Led azul

// Pinos de Hardware
#define BUZZER_PIN          10

// Variáveis de configuração (limites que podem ser alterados pela web)
extern bool SHUTDOWN;
extern uint8_t MENU_MODE;

// --- tempos do buzzer ---
#define BUZZER_ALERT_FREQ   1500 // F#6 (mais agudo e rápido)
#define BUZZER_ALERT_ON_MS  150

// Frequências das notas musicais (em Hz)
enum Notes {
    C = 2640,  // C
    D = 2970,  // D
    E = 3300,  // E
    F = 3520,  // F
    G = 3960,  // G
    A = 4400,  // A
    B = 4950,  // B
    C5 = 5280, // C5 (uma oitava acima)
    A = 880    // A5 
};

#endif // GLOBALS_H