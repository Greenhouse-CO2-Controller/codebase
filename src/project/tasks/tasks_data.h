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
#include "project/eeprom/EEPROM.h"
#include "project/eeprom/EEPROM_data.h"

#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600
#define STOP_BITS 2
#define EEPROM_ADDRESS 0x50

#define ROTARY_A   10    // no pull
#define ROTARY_B   11    // no pull
#define ROTARY_SW  12    // with pull-up
#define BUTTON_DEBOUNCE_MS 250
#define BUTTON_2 7
#define BUTTON_1 8
#define BUTTON_0 9

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

typedef struct {
    bool led_state;
    int led_frequency;
    uint32_t last_button_time;
    TaskHandle_t blink_task_handle;
    QueueHandle_t queue;
    QueueHandle_t gpio_semaphore;
} led_data_s;


enum ButtonEvent {
    BTN_UP,
    BTN_DOWN,
    BTN_OK,
    //enable disable edit for navigating C02, SSID & PWD chedel
    BTN_EDIT_ON,
    BTN_EDIT_OFF
    //enable disable edit for navigating C02, SSID & PWD chedel
};


struct SystemObjects {

    //enable disable edit for navigating C02, SSID & PWD chedel
    volatile bool edit_enabled = false;


    std::shared_ptr<PicoOsUart> uart;
    std::shared_ptr<ModbusClient> rtu_client;

    std::shared_ptr<ModbusRegister> co2_sensor;
    std::shared_ptr<ModbusRegister> fan_control;
    std::shared_ptr<ModbusRegister> fan_counter;
    std::shared_ptr<ModbusRegister> rh_sensor;
    std::shared_ptr<ModbusRegister> t_sensor;

    // Mutex to protect Modbus bus
    QueueHandle_t buttonQueue;
    QueueHandle_t encoderQueue;
    float temperature;
    float humidity;
    float fanSpeed;
    float co2_setpoint=800;
    bool waiting=false;
    bool injecting = false;
    bool fan_running=false;
    SemaphoreHandle_t modbus_mutex;
    Settings settings;
    EEPROM eeprom;
    //float co2_setpoint = 1200.0f; // for testing only
    float confirmed_co2_setpoint=800;

    SystemObjects() {
        uart = std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS);
        rtu_client = std::make_shared<ModbusClient>(uart);

        co2_sensor   = std::make_shared<ModbusRegister>(rtu_client, 240, 257); // try with 0
        fan_control  = std::make_shared<ModbusRegister>(rtu_client, 1, 0);
        fan_counter  = std::make_shared<ModbusRegister>(rtu_client, 1, 30005);
        rh_sensor    = std::make_shared<ModbusRegister>(rtu_client, 241, 256);
        t_sensor     = std::make_shared<ModbusRegister>(rtu_client, 241, 257);

        modbus_mutex = xSemaphoreCreateMutex();
        eeprom.init(EEPROM_ADDRESS, &settings, sizeof(settings));
        eeprom.eeprom_read_state();
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