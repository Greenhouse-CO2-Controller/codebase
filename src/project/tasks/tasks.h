//
// Created by mark on 9/28/25.
//
#include <stdio.h>
#include <iostream>

#ifndef RP2040_FREERTOS_IRQ_ALL_TASKS_H
#define RP2040_FREERTOS_IRQ_ALL_TASKS_H

#include "FreeRTOS.h"
#//include "tasks.h"
#include "tasks_data.h"


void modbus_task(void *pvParameters);
void AI1_counter_task(void *pvParameters);
void display_task(void *pvParameters);
void i2c_task(void *param);
void controller_task(void *param);
void uartTask(void *param);
void processCommand(Uart_s *ptr, const std::string &cmd);
void blink_task(void *param);
void gpio_task(void *param);
void gpio_callback(uint gpio, uint32_t events);
void relay_task(void *param);
void co2_injecting_task(void *param);

//Display
void ui_task(void *param);
//void watchdog_task(void *pvParameters);

#endif //RP2040_FREERTOS_IRQ_ALL_TASKS_H