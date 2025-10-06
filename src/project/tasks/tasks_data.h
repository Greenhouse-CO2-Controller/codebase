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

// Interrupt defs -Chedel
#define ROTARY_A   10    // no pull
#define ROTARY_B   11    // no pull
#define ROTARY_SW  12    // with pull-up
#define BUTTON_DEBOUNCE_MS 250

// UI event queue -Chedel
extern QueueHandle_t uiQueue;


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

typedef enum {
    rot_rotate_clockwise,
    rot_rotate_counterclockwise
}rotation_event_t;

typedef enum {
    event_button,
    event_encoder
}event_type_t;

typedef struct {
    rotation_event_t direction;
    event_type_t type;
    uint32_t timestamp;
}gpio_event_s;

typedef struct {
    bool led_state;
    int led_frequency;
    uint32_t last_button_time;
    TaskHandle_t blink_task_handle;
    QueueHandle_t queue;
    QueueHandle_t gpio_semaphore;
} led_data_s;

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

    float co2_setpoint = 1200.0f; // for testing only
    //float co2_setpoint; // for user input

    SystemObjects() {
        uart = std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS);
        rtu_client = std::make_shared<ModbusClient>(uart);

        co2_sensor   = std::make_shared<ModbusRegister>(rtu_client, 240, 257);
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