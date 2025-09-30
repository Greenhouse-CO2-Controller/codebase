//
// Created by mark on 9/28/25.
//

#ifndef RP2040_FREERTOS_IRQ_TASKS_DATA_H
#define RP2040_FREERTOS_IRQ_TASKS_DATA_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "PicoOsUart.h"
#include <string>
#include "queue.h"

extern SemaphoreHandle_t gpio_sem;
extern QueueHandle_t co2Queue;

struct led_params{
    uint pin;
    uint delay;
};

struct tasks_return {
    float produal_return;
    float rh_return;
    float t_return;
    float co2_return;
    float pressure_return;
    uint pulse_count;
};

struct Program {
    PicoOsUart uart;
    TimerHandle_t inactivityTimer; // timer object
    TimerHandle_t ledTimer; // timer object
    std::string inputBuffer;
    TickType_t lastLedToggleTick;

    Program() : uart(0, 0, 1, 115200), inactivityTimer(nullptr), ledTimer(nullptr), lastLedToggleTick(0) {}
};

#endif //RP2040_FREERTOS_IRQ_TASKS_DATA_H