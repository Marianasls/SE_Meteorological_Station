#ifndef BUZZER_H
#define BUZZER_H

#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "globals.h"

void set_buzzer_frequency(uint frequency);
void play_buzzer(uint frequency);
void stop_buzzer();

#endif // BUZZER_H