//
// Created by mark on 9/28/25.
//
#include <stdio.h>
#include <iostream>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"



#ifndef RP2040_FREERTOS_IRQ_ALL_TASKS_H
#define RP2040_FREERTOS_IRQ_ALL_TASKS_H

#include "FreeRTOS.h"
#//include "tasks.h"
#include "tasks_data.h"

#define WIFI_SSID "markfernando"
#define WIFI_PASS "markfernando"


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
void co2_injecting_task(void *param);
void eeprom_task(void *param);
void wifi_task(void *param);
void cloud_task(void *param);
void encoder_task(void *param);
void button_task(void *param);

//Display
void ui_task(void *param);
//void watchdog_task(void *pvParameters);

#endif //RP2040_FREERTOS_IRQ_ALL_TASKS_H