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
#include "project/display/oled.h"

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
        if (now - last_display_time > pdMS_TO_TICKS(1000)) {
            float rh, t, co2, fan;

            xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
            rh = s->rh_sensor->read()/10.0f;
            t  = s->t_sensor->read()/10.0f;
            co2 = s->co2_sensor->read();
            fan = s->fan_control->read()/10.0f;
            float co2_ppm = co2*10.0f;
            xSemaphoreGive(s->modbus_mutex);
            xQueueOverwrite(co2Queue, &co2_ppm); // overwrite the old value with latest. so queue doesnt block when its full. for xQueueSend it blocks when its full.

            //printf("RH=%5.1f%%, T=%5.1fC, CO2=%5.1f ppm, Fan AO1=%5.1f%%, Pulses=%u\n",
                   //rh , t , co2_ppm, fan , pulses);

            last_display_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void controller_task(void *param) {
    auto *s = static_cast<SystemObjects*>(param);
    gpio_init(CO2_VALVE_GPIO);
    gpio_set_dir(CO2_VALVE_GPIO, true); // output
    gpio_put(CO2_VALVE_GPIO, 0);
    const TickType_t inject_time = pdMS_TO_TICKS(1500);
    TickType_t wait_time = pdMS_TO_TICKS(30000);
    float co2level; //actual co2
    float setpoint = s->confirmed_co2_setpoint;
    while (true) {
        if (xQueueReceive(co2Queue, &co2level, pdMS_TO_TICKS(1000))) {
            float fanlevel = 0.0f;
            float min_fanlevel = 300.0f;
            float max_fanlevel = 1000.0f;
            float maxCO2 = 2000.0f;
            //printf("co2 setpoint in the controller: %.0f\n", s->confirmed_co2_setpoint);
            if (co2level < s->confirmed_co2_setpoint - 50) {
                fanlevel = 0;
                s->fan_running = false;
                //xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
                //s->fan_control->write(fanlevel);
                //xSemaphoreGive(s->modbus_mutex);
                gpio_put(CO2_VALVE_GPIO, 1);
                s->injecting = true;
                vTaskDelay(inject_time);
                s->injecting = false;
                gpio_put(CO2_VALVE_GPIO, 0);
                s->fan_running = false;
                s->waiting = true;

                if (co2level > s->confirmed_co2_setpoint + 50) {
                    wait_time = pdMS_TO_TICKS(0);
                    printf("waiting time updated\n");
                }
                vTaskDelay(wait_time);
                s->waiting = false;

            } else if (co2level > s->confirmed_co2_setpoint + 50) {
                s->fan_running = true;
                if (co2level > maxCO2) {
                    s->alarm_high_co2 = true;
                    fanlevel = max_fanlevel;
                } else {
                    s->alarm_high_co2 = false;
                    fanlevel = min_fanlevel + (co2level - setpoint) * (max_fanlevel - min_fanlevel) / (maxCO2 - setpoint);
                    if (fanlevel > max_fanlevel) fanlevel = max_fanlevel;
                    if (fanlevel < min_fanlevel) fanlevel = min_fanlevel;
                }
                gpio_put(CO2_VALVE_GPIO, 0);
                //s->fan_running = false;

            } else {
                gpio_put(CO2_VALVE_GPIO, 0);
                fanlevel = 0.0f;
            }
            xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
            s->fan_control->write(fanlevel);
            xSemaphoreGive(s->modbus_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void eeprom_task(void* param) { // only dummy data
    // store wifi credential, co2 set point, max and low setpoint
    auto* s = static_cast<SystemObjects*>(param);
    /*
    vTaskDelay(pdMS_TO_TICKS(1000));
    float dummy_setpoint = 1300.0f;
    s->settings.co2_setpoint = dummy_setpoint;
    s->eeprom.eeprom_write_state(&s->settings);
    s->eeprom.eeprom_read_state();
    printf("EEPROM read: CO2 setpoint=%.2f\n", s->settings.co2_setpoint);
    */
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void button_task(void *param) {
    auto *s = static_cast<SystemObjects*>(param);
    static bool lastUp = false;
    static bool lastDown = false;
    static bool lastOk = false;
    bool lastState = false;
    const TickType_t repeatDelay = pdMS_TO_TICKS(150);
    ButtonEvent btn;

    while (true) {
        bool up_button = gpio_get(BUTTON_2);
        bool down_button = gpio_get(BUTTON_0);
        bool confirm_button = gpio_get(BUTTON_1);
        TickType_t now = xTaskGetTickCount();
#if 1
        if (!up_button) {
            if (!lastUp) {
                btn = BTN_UP;
                xQueueSend(s->buttonQueue, &btn, 0);
                lastUp = now;
            }else if ((now-lastDown) >= pdMS_TO_TICKS(50)) {
                btn = BTN_UP;
                xQueueSend(s->buttonQueue, &btn, 0);
                lastUp += pdMS_TO_TICKS(500);
            }
        }else {
            lastDown = 0;
        }

        if (!down_button) {
            if (!lastDown) {
                btn = BTN_DOWN;
                xQueueSend(s->buttonQueue, &btn, 0);
                lastDown = now;
            }else if ((now-lastOk) >= pdMS_TO_TICKS(50)) {
                btn = BTN_DOWN;
                xQueueSend(s->buttonQueue, &btn, 0);
                lastDown += pdMS_TO_TICKS(500);
            }

        }else {
            lastDown =0;
        }
        if (!confirm_button && lastOk) {
            btn = BTN_OK;
            xQueueSend(s->buttonQueue, &btn, 0);
        }
        lastOk = confirm_button;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
#endif
#if 0

        if (up_button && !lastUp) {  // button just pressed
            btn = BTN_UP;
            xQueueSend(s->buttonQueue, &btn, 0);
        }

        if (down_button && !lastDown) {
            btn = BTN_DOWN;
            xQueueSend(s->buttonQueue, &btn, 0);
        }

        if (confirm_button && !lastOk) {
            btn = BTN_OK;
            xQueueSend(s->buttonQueue, &btn, 0);
        }

        lastUp = up_button;
        lastDown = down_button;
        lastOk = confirm_button;

        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms debounce

    }

}
#endif

void ui_task(void *param) {
    auto *s = static_cast<SystemObjects*>(param);
    auto i2cbus = std::make_shared<PicoI2C>(1, 400000);
    Oled display(i2cbus);

    display.clear();
    display.drawText(0, 0, "Booting...");
    display.show();
    vTaskDelay(pdMS_TO_TICKS(1000));
    ButtonEvent btn;
    //s->co2_setpoint = 800; // initial value

    while (true) {

        // read CO2 directly from modbus
        xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
        float co2_ppm = s->co2_sensor->read() * 10.0f;
        float fanlevel = s->fan_control->read()/10.0f;
        float rh = s->rh_sensor->read()/10.0f;
        float t = s->t_sensor->read()/10.0f;
        xSemaphoreGive(s->modbus_mutex);
#if 0
        if (xQueueReceive(s->buttonQueue, &btn,0)) {
            switch (btn) {
                case BTN_UP:
                    s->co2_setpoint += 30.0f;
                    if (s->co2_setpoint > 1500.0f) s->co2_setpoint = 1500.0f;
                    break;
                case BTN_DOWN:
                    s->co2_setpoint -= 30.0f;
                    if (s->co2_setpoint < 200.0f) s->co2_setpoint = 200.0f;
                    break;
                case BTN_OK:
                    s->confirmed_co2_setpoint = s->co2_setpoint;
                    //confirmed_setpoint = setpoint; // when button press it saves the set point to confirmed_co2_setpoint
                    //s->settings.co2_setpoint = confirmed_setpoint;
                    //s->eeprom.eeprom_write_state(&s->settings);
                    //s->eeprom.eeprom_read_state();
                    //printf("eerpom set point %.2f\n", s->settings.co2_setpoint);
                    //printf("setpoint changed: %.2f\n", s->co2_setpoint);
                    printf("confiremed setpoint changed: %.2f\n", s->confirmed_co2_setpoint);
                    break;
            }
        }
#endif
#if 1
        static TickType_t last_up_apply = 0;
        static TickType_t last_down_apply = 0;
        //bool saw_up = false;
        bool up_press_detected = false;
        bool down_press_detected = false;
        while (xQueueReceive(s->buttonQueue, &btn, 0) == pdTRUE) {
            switch (btn) {
                case BTN_UP:
                    up_press_detected = true;
                    break;

                case BTN_DOWN:
                    down_press_detected = true;
                    break;

                case BTN_OK:
                    s->confirmed_co2_setpoint = s->co2_setpoint;
                    break;
            }
        }

        if (up_press_detected) {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_up_apply) >= pdMS_TO_TICKS(50)) {
                s->co2_setpoint += 10.0f;
                if (s->co2_setpoint > 1500.0f) s->co2_setpoint = 1500.0f;
                last_up_apply = now;
            }
        }

        if (down_press_detected) {
            TickType_t now_down = xTaskGetTickCount();
            if ((now_down - last_down_apply) >= pdMS_TO_TICKS(50)) {
                s->co2_setpoint -= 10.0f;
                if (s->co2_setpoint < 200.0f) s->co2_setpoint = 200.0f;
                last_down_apply = now_down;
            }
        }


#endif

        // update display
        char line1[32], line2[32], line3[32], line4[32], line5[32], line6[32], line_wait[32];
        if (s->waiting) {
            strcpy(line_wait, "W");
        }else if (s->injecting) {
            strcpy(line_wait, "I");
        }else if (s->fan_running) {
            strcpy(line_wait, "R");
        }
        else if (!s->waiting && !s->injecting){
            strcpy(line_wait, "N-WI");
        }
        snprintf(line1, sizeof(line1), "CO2: %.0fppm %s", co2_ppm, line_wait);
        snprintf(line2, sizeof(line2), "Setpt: %.0fppm", s->confirmed_co2_setpoint);
        snprintf(line3, sizeof(line3), "RH:%.0f T:%.0f F:%.0f" , rh, t, fanlevel);
        snprintf(line4, sizeof(line4), "Setpt: %.0fppm", s->co2_setpoint);
        snprintf(line5, sizeof(line5), "SSID: ");
        snprintf(line6, sizeof(line6), "PWD: ");
        display.clear();
        display.drawText(0, 0, line1);
        display.drawText(0, 10, line2);
        display.drawText(0, 20, line3);
        display.drawText(0, 30, line4);
        display.drawText(0, 40, line5);
        display.drawText(0, 50, line6);
        //display.drawRect(50, 30, 50, 10 + 1, false);
        display.show();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void blink_task(void *param)
{
    auto *s = static_cast<SystemObjects*>(param);

    while (true) {
        if (s->injecting) {
            s->led_inject->write(true);
            vTaskDelay(pdMS_TO_TICKS(200));
            s->led_inject->write(false);
            vTaskDelay(pdMS_TO_TICKS(200));
        } else {
            s->led_inject->write(false);
        }
        if (s->fan_running) {
            s->led_status->write(true);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (s->alarm_high_co2) {
            s->led_alarm->write(true);
            vTaskDelay(pdMS_TO_TICKS(50));
            s->led_alarm->write(false);
            vTaskDelay(pdMS_TO_TICKS(50));
        } else {
            s->led_alarm->write(false);
        }

        if (s->waiting) {
            s->led_status->write(true);
            vTaskDelay(pdMS_TO_TICKS(400));
            s->led_status->write(false);
            vTaskDelay(pdMS_TO_TICKS(400));
        } else {
            s->led_status->write(true);
            vTaskDelay(pdMS_TO_TICKS(50));

        }

        vTaskDelay(pdMS_TO_TICKS(50)); // avoid CPU hogging
    }
}

void wifi_task(void *param) {
    (void)param;
    printf("Wi-Fi task starting...\n");

    // Initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed!\n");
        vTaskDelete(NULL);
    }

    // Enable station mode
    cyw43_arch_enable_sta_mode();

    int retry_count = 0;
    const int max_retries = 10;      // keep retrying up to 10 times
    const int retry_delay_ms = 5000; // 5 seconds between retries

    while (true) {
        printf("[WiFi] Attempting to connect: %s (Try %d/%d)...\n",
               WIFI_SSID, retry_count + 1, max_retries);

        int result = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA2_MIXED_PSK, // safer for iPhone
            10000                       // 10 second timeout
        );

        if (result == 0) {
            printf("[WiFi] Connected!\n");
            printf("[WiFi] IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));

            // Optional: light the Pico W LED on connection
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

            break; // exit loop, connection succeeded
        } else {
            printf("[WiFi] Connection failed (error %d)\n", result);
            retry_count++;
            if (retry_count >= max_retries) {
                printf("[WiFi] Could not connect after %d attempts. Restarting Wi-Fi...\n", max_retries);
                retry_count = 0;
                //cyw43_arch_disable_sta_mode();
                cyw43_arch_enable_sta_mode();
            }
        }

        // Wait a bit before retrying
        vTaskDelay(pdMS_TO_TICKS(retry_delay_ms));
    }

    // Keep task alive (monitor Wi-Fi)
    while (true) {

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}


void cloud_task(void *param) {
    // run every 15s (limitation on free account)
    //
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
