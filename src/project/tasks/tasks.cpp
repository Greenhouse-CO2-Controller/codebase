//
// Created by mark on 9/28/25.
//

#include "tasks.h"
#include <cstdio>
#include "hardware/gpio.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"
#include "modbus/ModbusClient.h"
#include "display/ssd1306.h"
#include <stdio.h>
#include "tasks_data.h"
#include "ssd1306os.h"

SemaphoreHandle_t gpio_sem = nullptr;
QueueHandle_t co2Queue = nullptr;

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define STOP_BITS 2 // for real system (pico simualtor also requires 2 stop bits)

#define USE_MODBUS
#define CO2_VALVE_GPIO 27

void modbus_task(void *param) {
    auto *s = static_cast<SystemObjects*>(param);
    uint32_t last_display_time = 0;

    while (true) {
        uint16_t pulses;
        xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
        pulses = s->fan_counter->read();
        xSemaphoreGive(s->modbus_mutex);

        uint32_t now = xTaskGetTickCount();
        if (now - last_display_time > pdMS_TO_TICKS(1500)) {
            float rh, t, co2, fan;

            xSemaphoreTake(s->modbus_mutex, portMAX_DELAY); // look carefully on the variable with /10.0. I divided by 10 to match the values with simulater
            rh = s->rh_sensor->read()/10.0;
            t  = s->t_sensor->read()/10.0;
            co2 = s->co2_sensor->read();
            fan = s->fan_control->read()/10.0;
            float co2_ppm = co2 / 10.0f;
            xSemaphoreGive(s->modbus_mutex);
            xQueueOverwrite(co2Queue, &co2_ppm);

            printf("RH=%5.1f%%, T=%5.1fC, CO2=%5.1f ppm, Fan AO1=%5.1f%%, Pulses=%u\n",
                   rh , t , co2_ppm, fan , pulses);

            last_display_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void controller_task(void *param) {
    auto *s = static_cast<SystemObjects*>(param);
    auto *data = static_cast<Data*>(param);
    gpio_init(CO2_VALVE_GPIO);
    gpio_set_dir(CO2_VALVE_GPIO, true); // output
    gpio_put(CO2_VALVE_GPIO, 0);
    const TickType_t inject_time = pdMS_TO_TICKS(2000);  // 2s injection
    const TickType_t wait_time = pdMS_TO_TICKS(10000);
    float co2level;
    float setpoint = s->co2_setpoint;
    while (true) {
        if (xQueueReceive(co2Queue, &co2level, portMAX_DELAY)) {
            float fanlevel = 0.0f;

            if (co2level > 2000.0f) {
                fanlevel = 500.0f;
                //stop injecting and fan runs until co2 is below 2000
                gpio_put(CO2_VALVE_GPIO, 0);
                printf("ALARM!!! CO2 above 2000 fan is on\n");
            } else if (co2level > setpoint-50) { // 1500 < co2 <= 2000
                fanlevel = 0.0f;
                // need to stop injecting and no fan
                gpio_put(CO2_VALVE_GPIO, 0);
                printf("CO2 above set point.Stopping injection\n");
            } else if (co2level < setpoint -50) { // co2 <= 1500
                fanlevel = 0.0f;
                // inject CO2 and fan off

                printf("lets inject co2\n");

                while (true) {
                    xQueueReceive(co2Queue, &co2level, pdMS_TO_TICKS(500));

                    if (co2level < setpoint - 50) {
                        gpio_put(CO2_VALVE_GPIO, 1);
                        printf("Injecting co2 (valve ON)\n");
                        vTaskDelay(inject_time);
                        gpio_put(CO2_VALVE_GPIO, 0);
                        printf("Valve OFF, waiting for stabilization...\n");
                        vTaskDelay(wait_time);
                    } else {
                        printf("CO2 above set point. Valve OFF\n");
                        gpio_put(CO2_VALVE_GPIO, 0);
                    }

                    vTaskDelay(pdMS_TO_TICKS(100));
                }

            }
            xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
            s->fan_control->write(fanlevel);
            xSemaphoreGive(s->modbus_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void co2_injecting_task(void *param) {
    auto *data = static_cast<Data*>(param);

    // Initialize valve GPIO
    gpio_init(CO2_VALVE_GPIO);
    gpio_set_dir(CO2_VALVE_GPIO, true); // output
    gpio_put(CO2_VALVE_GPIO, 0);        // valve initially OFF

    const TickType_t inject_time = pdMS_TO_TICKS(2000);  // 2s injection
    const TickType_t stabilization_time = pdMS_TO_TICKS(10000); // 10s wait period
    float co2level = 0.0f;
    data->co2_setpoint = 1800;
    while (true) {
        // Wait up to 500ms for a new reading, otherwise keep last value
        xQueueReceive(co2Queue, &co2level, pdMS_TO_TICKS(500));

        if (co2level < data->co2_setpoint - 50) {
            printf("Injecting CO2... (valve ON)\n");
            // injection code...
        } else {
            printf("CO2 above set point. Valve OFF\n");
            gpio_put(CO2_VALVE_GPIO, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void display_task(void *param) // for led display(UI)
{
    auto *ptr = static_cast<Data*>(param);
    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    ssd1306os display(i2cbus);
    display.fill(0);

    //display.text("Display is ok", 0, 0);
    // display ptr->rh.read()
    display.show();
    while(true) {
        display.fill(0);
        char buf[32];
        snprintf(buf, sizeof(buf), "RH=%5.1f%%", ptr->rh_return / 10.0);
        display.text(buf, 0, 0);
        display.show();
        vTaskDelay(100);
    }
}



void AI1_counter_task(void *pvParameters) {
    /*
    auto uart{std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS)};
    auto rtu_client{std::make_shared<ModbusClient>(uart)};
    ModbusRegister fan_counter(rtu_client, 1, 30005); // AI1 counter
    while (true) {
        uint counter = fan_counter.read();
        printf("Pulse count = %u\n", counter);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    */

}



void i2c_task(void *param) {
    auto i2cbus{std::make_shared<PicoI2C>(1, 100000)};
    auto *ptr = static_cast<Data*>(param);
    const uint led_pin = 21;
    const uint delay = pdMS_TO_TICKS(250);
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    uint8_t cmd[2] = {0x36, 0x08};

    uint8_t buffer[2] = {0};

    while(true) {
        i2cbus->write(0x40, cmd, 2);
        vTaskDelay(pdMS_TO_TICKS(5));
        int p = i2cbus->read(0x40, buffer, 2);;
        if(p != 2) {
            printf("I2C read failed, pressure=%d\n", p);
        } else {
            int16_t raw = (buffer[0] << 8) | buffer[1];
            float pressure = raw / 240.0f;
            ptr->pressure_return = pressure;
            printf("Pressure=%.2f Pa\n", pressure);
        }
        vTaskDelay(pdMS_TO_TICKS(1495));
    }
}

void processCommand(Uart_s *ptr, const std::string &cmd) { // this is only for clion serial port purposes
    if (cmd.rfind("ppm", 0) == 0) {
        int val = atoi(cmd.substr(3).c_str());
        if (val > 0 && val <= 1500) { //co2 setpoint should be between 0 to 1500
            //co2SetPoint = val; // should create this
            ptr->uart.send("CO2 set point updated\r\n");
        } else {
            ptr->uart.send("Invalid CO2 value (max 1500)\r\n");
        }
    }
    else if (cmd.rfind("dec", 0) == 0) { //aCO2 dissipation--> 2.5 means 2.5 ppm/s
        int val = atoi(cmd.substr(3).c_str());
        //not complete
    }
    else if (cmd == "status") { // to view the status
        char buf[64];
        //snprintf(buf, sizeof(buf), "CO2=%d RH=%0.1f T=%0.1f\r\n", co2Read, rhRead/10.0, tRead/10.0); // after creation we can uncomment this
        ptr->uart.send(buf);
    }
    else {
        ptr->uart.send("unknown command\r\n");
    }
}

void uartTask(void *param) {
    auto *ptr = static_cast<Uart_s*>(param);
    uint8_t c;

    while (true) {
        if (ptr->uart.read(&c, 1, pdMS_TO_TICKS(100)) > 0) {
            ptr->uart.write(&c, 1);
            if (c == '\r' || c == '\n') {
                const uint8_t newline[] = {'\r', '\n'};
                ptr->uart.write(newline, sizeof(newline));
                if (!ptr->inputBuffer.empty()) {
                    processCommand(ptr, ptr->inputBuffer);
                    ptr->inputBuffer.clear();
                }
            } else {
                ptr->inputBuffer.push_back((char)c);
            }
        }
    }
}

void relay_task(void *param) {
    // open for 1 seconds and wait for 45 secs
}

void blink_task(void *param)
{
    auto lpr = (led_params *) param;
    const uint led_pin = lpr->pin;
    const uint delay = pdMS_TO_TICKS(lpr->delay);
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    while (true) {
        gpio_put(led_pin, true);
        vTaskDelay(delay);
        gpio_put(led_pin, false);
        vTaskDelay(delay);
    }
}

void gpio_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // signal task that a button was pressed
    xSemaphoreGiveFromISR(gpio_sem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void gpio_task(void *param) {
    (void) param;
    const uint button_pin = 9;
    const uint led_pin = 22;
    const uint delay = pdMS_TO_TICKS(250);
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_set_pulls(button_pin, true, false);
    gpio_set_irq_enabled_with_callback(button_pin, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    while(true) {
        if(xSemaphoreTake(gpio_sem, portMAX_DELAY) == pdTRUE) {
            //std::cout << "button event\n";
            gpio_put(led_pin, 1);
            vTaskDelay(delay);
            gpio_put(led_pin, 0);
            vTaskDelay(delay);
        }
    }
}
















/*
void modbus_task(void *param) {

    auto *ptr = static_cast<tasks_return*>(param);
    auto *s = static_cast<SystemObjects*>(param);
    const uint led_pin = 22;
    const uint button = 9;

    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);

#ifdef USE_MODBUS
    auto uart{std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS)};
    auto rtu_client{std::make_shared<ModbusClient>(uart)};
    ModbusRegister rh(rtu_client, 241, 256); //humidity
    ModbusRegister t(rtu_client, 241, 257); // temperature
    ModbusRegister c02(rtu_client, 240, 5); // C02 level
    ModbusRegister produal(rtu_client, 1, 0); // to control the fan
    ModbusRegister fan_counter(rtu_client, 1, 30005); // AI1 counter

    vTaskDelay(pdMS_TO_TICKS(100));
    produal.write(0); // move this to controller task and  set the speed based on the C02 level
    vTaskDelay((100));
    produal.write(0); // 100 means 10%

#endif
    uint32_t last_display_time =0;
    while (true) {
#ifdef USE_MODBUS

        //gpio_put(led_pin, !gpio_get(led_pin)); // toggle  led
        // these need to be output to oled.(and cloud if needed)
        uint16_t pulses = fan_counter.read();
        ptr->pulse_count = pulses;

        // Read other sensors less frequently (every 3s)
        uint32_t now = xTaskGetTickCount();
        if(now - last_display_time > pdMS_TO_TICKS(1495)) {
            ptr->rh_return = rh.read();
            ptr->t_return  = t.read();
            ptr->co2_return = c02.read();
            xQueueOverwrite(co2Queue, &ptr->co2_return);
            ptr->produal_return = produal.read();

            printf("RH=%5.1f%%, T=%5.1fC, CO2=%5.1f ppm, Fan AO1=%5.1f%%, Pulses=%u\n",
                   ptr->rh_return/10.0, ptr->t_return/10.0, ptr->co2_return/10.0,
                   ptr->produal_return/10.0, pulses);
            if (pulses>0) {
                printf("Fan is malfunctioning"); // display this in UI
            }
            gpio_put(led_pin, !gpio_get(led_pin));
            last_display_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(5));

#endif
    }
}
*/