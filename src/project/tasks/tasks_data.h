//
// Created by mark on 9/28/25.
//

#ifndef RP2040_FREERTOS_IRQ_TASKS_DATA_H
#define RP2040_FREERTOS_IRQ_TASKS_DATA_H

#include <memory>
#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "PicoOsUart.h"
#include <string>
#include "queue.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"

#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600
#define STOP_BITS 2

extern SemaphoreHandle_t gpio_sem;
extern QueueHandle_t co2Queue;

struct led_params{
    uint pin;
    uint delay;
};

struct Data {
    float produal_return;
    float rh_return;
    float t_return;
    float co2_return;
    float pressure_return;
    uint pulse_count;
    float co2_setpoint;
};



struct SystemObjects {
    std::shared_ptr<PicoOsUart> uart;
    std::shared_ptr<ModbusClient> rtu_client;

    std::shared_ptr<ModbusRegister> co2_sensor;
    std::shared_ptr<ModbusRegister> fan_control;
    std::shared_ptr<ModbusRegister> fan_counter;
    std::shared_ptr<ModbusRegister> rh_sensor;
    std::shared_ptr<ModbusRegister> t_sensor;

    // Mutex to protect Modbus bus
    SemaphoreHandle_t modbus_mutex;

    float co2_setpoint = 1800.0f; // for testing only
    //float co2_setpoint; // for user input

    SystemObjects() {
        uart = std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS);
        rtu_client = std::make_shared<ModbusClient>(uart);

        co2_sensor   = std::make_shared<ModbusRegister>(rtu_client, 240, 5);
        fan_control  = std::make_shared<ModbusRegister>(rtu_client, 1, 0);
        fan_counter  = std::make_shared<ModbusRegister>(rtu_client, 1, 30005);
        rh_sensor    = std::make_shared<ModbusRegister>(rtu_client, 241, 256);
        t_sensor     = std::make_shared<ModbusRegister>(rtu_client, 241, 257);

        modbus_mutex = xSemaphoreCreateMutex();
    }
};

struct Uart_s {
    PicoOsUart uart;
    TimerHandle_t inactivityTimer; // timer object
    TimerHandle_t ledTimer; // timer object
    std::string inputBuffer;
    TickType_t lastLedToggleTick;

    Uart_s() : uart(0, 0, 1, 115200), inactivityTimer(nullptr), ledTimer(nullptr), lastLedToggleTick(0) {}
};

#endif //RP2040_FREERTOS_IRQ_TASKS_DATA_H