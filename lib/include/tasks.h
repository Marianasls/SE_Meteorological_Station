#ifndef TASKS_H
#define TASKS_H

#include "stdio.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"

void vTaskWiFi(void *params);
#endif