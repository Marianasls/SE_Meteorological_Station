#include <stdio.h>
#include "pico/stdlib.h"
#include "globals.h"

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

int main()
{
    stdio_init_all();
    return
}
