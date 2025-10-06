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

// New file: OLED layout .h insertion -Chedel
#include "project/ScreenViews/oled_layouts.h"

// One-time setup and ISR -Chedel
static void ui_inputs_init(void);
static void ui_gpio_isr(uint gpio, uint32_t events);

// queue handler defined -Chedel
QueueHandle_t uiQueue = nullptr;

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
            float co2_ppm = co2*10;
            xSemaphoreGive(s->modbus_mutex);
            xQueueOverwrite(co2Queue, &co2_ppm); // overwrite the old value with latest. so queue doesnt block when its full. for xQueueSend it blocks when its full.

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
    const TickType_t inject_time = pdMS_TO_TICKS(1500);  // 2s injection
    const TickType_t wait_time = pdMS_TO_TICKS(10000);
    float co2level;
    float setpoint = s->co2_setpoint;
    while (true) {
        if (xQueueReceive(co2Queue, &co2level, portMAX_DELAY)) {
            float fanlevel = 0.0f;
            printf("Co2 level in the controller: %f\n", co2level);
            if (co2level > 2000.0f) {
                fanlevel = 500.0f;
                gpio_put(CO2_VALVE_GPIO, 0);
                printf("ALARM!!! CO2 above 2000 fan is on\n");
            } else if (co2level > setpoint-50) { // 1500 < co2 <= 2000
                while (true) {
                    xQueueReceive(co2Queue, &co2level, pdMS_TO_TICKS(500));
                    if (co2level < setpoint - 50) {
                        gpio_put(CO2_VALVE_GPIO, 1);
                        //printf("Injecting co2 (valve ON)\n");
                        vTaskDelay(inject_time);
                        gpio_put(CO2_VALVE_GPIO, 0);
                        //printf("Valve OFF, waiting for stabilization...\n");
                        vTaskDelay(wait_time);
                        printf("Waiting done...\n");
                    } else if (co2level>setpoint + 50) {
                        //printf("CO2 above max point. Valve OFF\n");
                        fanlevel = 300.0f;

                        gpio_put(CO2_VALVE_GPIO, 0);
                    } else if (co2level > setpoint - 50 || co2level < setpoint + 50) {
                        //printf("CO2 arround set point. Valve OFF\n");
                        gpio_put(CO2_VALVE_GPIO, 0);
                        fanlevel = 0.0f;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                    xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
                    s->fan_control->write(fanlevel);
                    xSemaphoreGive(s->modbus_mutex);
                }
                // need to stop injecting and no fan
                gpio_put(CO2_VALVE_GPIO, 0);
                printf("CO2 above set point.Stopping injection\n");
            } else if (co2level < setpoint -50) { // co2 <= 1500
                while (true) {
                    xQueueReceive(co2Queue, &co2level, pdMS_TO_TICKS(500)); // here if no reading then it will use last reading
                    //if (xQueueReceive(co2Queue, &co2level, pdMS_TO_TICKS(500)) == pdTRUE){
                    //  if(){
                    //  }else{
                    //      gpio_put(CO2_VALVE_GPIO, 0); //here it wait for new reading. if not we can notify no rading so we can keep valve close
                    //  }
                    //}
                    if (co2level < setpoint - 50) {
                        gpio_put(CO2_VALVE_GPIO, 1);
                        //printf("Injecting co2 (valve ON)\n");
                        vTaskDelay(inject_time);
                        gpio_put(CO2_VALVE_GPIO, 0);
                        //printf("Valve OFF, waiting for stabilization...\n");
                        vTaskDelay(wait_time);
                        printf("Waiting done...\n");
                    } else if (co2level>setpoint + 50) {
                        //printf("CO2 above max point. Valve OFF\n");
                        fanlevel = 300.0f;

                        gpio_put(CO2_VALVE_GPIO, 0);
                    } else if (co2level > setpoint - 50 || co2level < setpoint + 50) {
                        //printf("CO2 arround set point. Valve OFF\n");
                        gpio_put(CO2_VALVE_GPIO, 0);
                        fanlevel = 0.0f;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                    xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
                    s->fan_control->write(fanlevel);
                    xSemaphoreGive(s->modbus_mutex);
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
    auto *s = static_cast<SystemObjects*>(param);
    // Initialize valve GPIO
    gpio_init(CO2_VALVE_GPIO);
    gpio_set_dir(CO2_VALVE_GPIO, true); // output
    gpio_put(CO2_VALVE_GPIO, 0);        // valve initially OFF
    float co2level;
    float fanlevel;
    float setpoint = s->co2_setpoint;
    const TickType_t inject_time = pdMS_TO_TICKS(1500);  // 2s injection
    const TickType_t wait_time = pdMS_TO_TICKS(30000); // 10s wait period

    //data->co2_setpoint = 1800;
    while (true) {
        xQueueReceive(co2Queue, &co2level, pdMS_TO_TICKS(500)); // here if no reading then it will use last reading
        //if (xQueueReceive(co2Queue, &co2level, pdMS_TO_TICKS(500)) == pdTRUE){
        //  if(){
        //  }else{
        //      gpio_put(CO2_VALVE_GPIO, 0); //here it wait for new reading. if not we can notify no rading so we can keep valve close
        //  }
        //}
        if (co2level < setpoint - 50) {
            gpio_put(CO2_VALVE_GPIO, 1);
            //printf("Injecting co2 (valve ON)\n");
            vTaskDelay(inject_time);
            gpio_put(CO2_VALVE_GPIO, 0);
            //printf("Valve OFF, waiting for stabilization...\n");
            vTaskDelay(wait_time);
        } else if (co2level>setpoint + 50) {
            //printf("CO2 above max point. Valve OFF\n");
            fanlevel = 300.0f;

            gpio_put(CO2_VALVE_GPIO, 0);
        } else if (co2level > setpoint - 50 || co2level < setpoint + 50) {
            //printf("CO2 arround set point. Valve OFF\n");
            gpio_put(CO2_VALVE_GPIO, 0);
            fanlevel = 0.0f;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        xSemaphoreTake(s->modbus_mutex, portMAX_DELAY);
        s->fan_control->write(fanlevel);
        xSemaphoreGive(s->modbus_mutex);
    }
}

// Toggled  - Mark
/*
void ui_task(void *param) {
    auto *s = static_cast<SystemObjects*>(param);
    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};

    ssd1306os display(i2cbus);
    display.fill(0);
    display.text("Group 6", 38,10);
    display.text("Mark,Visal", 27,30);
    display.text("Chedel", 42,40);
    display.show();
    vTaskDelay(pdMS_TO_TICKS(3000));
    while (true) {
        display.fill(0);
        display.rect(22,39,88,10,1,true); // SET NETWORK
        //display.rect(22,24,88,10,1,true); // when this is on we need to set color 0
        display.text("SET C02", 22, 10, 1);
        display.text("SHOW STATUS", 22, 25,1);
        display.text("SET NETWORK", 22, 40, 0);
        display.show();
        vTaskDelay(100);
    }
}
*/

// with C02 Menu Navigation and exit. hard to implement ascii logic -Chedel
void ui_task(void *param)
{
    (void)param;

    ui_inputs_init();

    auto i2cbus = std::make_shared<PicoI2C>(1, 400000);
    OledLayouts display(i2cbus);


    enum class Screen { Welcome, MainMenu, SetCo2, Status };
    enum class SetCo2Focus { C02, Exit, Save };

    Screen screen = Screen::Welcome;

    int mainSel = 0;                 // 0=CO2, 1=STATUS, 2=NETWORK
    const int mainItemCount = 3;

    SetCo2Focus focus = SetCo2Focus::C02;
    int currentPpm = 1305;           // dummy Value. Need to wire real value from sensor

    // --- Timing / filtering ---
    TickType_t ignoreEncoderUntil = 0;              // transition guard
    TickType_t lastMoveMain = 0, lastMoveSet = 0;   // per-screen throttling
    const TickType_t gapMain = pdMS_TO_TICKS(200);   // snappy in Main Menu
    const TickType_t gapSet  = pdMS_TO_TICKS(250);   // calm in Set CO2

    auto encoder_allowed = [&](Screen sc, TickType_t now)->bool {
        if (now < ignoreEncoderUntil) return false;
        TickType_t &last = (sc == Screen::MainMenu) ? lastMoveMain : lastMoveSet;
        const TickType_t gap = (sc == Screen::MainMenu) ? gapMain : gapSet;
        if (now - last < gap) return false;
        last = now;
        return true;
    };

    // MAIN MENU Draw helpers
    auto drawMainMenuBySel = [&](int sel) {
        switch (sel) {
            case 0:

                display.mainMenu_C02();
            break;

            case 1:
                display.mainMenu_STATUS();
            break;

            case 2:
                display.mainMenu_NETWORK();
            break;

            default:
                display.mainMenu_C02();
            break;
        }
    };

    auto drawSetCo2ByFocus = [&]() {
        display.setCo2Screen(currentPpm); // currentPpm is a DUmmy Value
        if (focus == SetCo2Focus::Exit) {
            display.setCo2Screen_EXIT();
        } else if (focus == SetCo2Focus::Save){
            display.setCo2Screen_SAVE();
        }
    };

    // boot screen
    display.welcomeScreen();

    // Screen Changing Speed = moveMinGap
    TickType_t lastMove = 0;
    const TickType_t moveMinGap = pdMS_TO_TICKS(200); // 200

    for(;;) {
        gpio_event_s ev{};
        if (xQueueReceive(uiQueue, &ev, portMAX_DELAY) != pdTRUE)
            continue;
        if (ev.type == event_button) {
            if (screen == Screen::Welcome) {
                display.mainMenu();
                drawMainMenuBySel(mainSel);
                screen = Screen::MainMenu;
                ignoreEncoderUntil = xTaskGetTickCount() + pdMS_TO_TICKS(120);
            }
            else if (screen == Screen::MainMenu) {
                if (mainSel == 0) { //C02
                    screen = Screen::SetCo2;
                    focus  = SetCo2Focus::C02; // Show Base 1st
                    drawSetCo2ByFocus();

                    //something wrong here
                    // Drop any already-queued encoder turns so we don't jump
                    gpio_event_s tmp;
                    while (xQueuePeek(uiQueue, &tmp, 0) == pdTRUE && tmp.type == event_encoder) {
                        xQueueReceive(uiQueue, &tmp, 0);
                    }
                    ignoreEncoderUntil = xTaskGetTickCount() + pdMS_TO_TICKS(150);
                    //something wrong here
                }
                else if (mainSel == 1) {
                    // STATUS screen
                    screen = Screen::Status;
                    display.statusScreen(1580, 1500, 45, 22, 30); // DUMMY VALUES. Wire in Sensor Val
                }
                else if (mainSel == 2) {
                    // (later) NETWORK screen ADVANCED REQ TODO: NETWORK
                }
            }

            else if (screen == Screen::SetCo2) {
                if (focus == SetCo2Focus::Exit) {
                    display.mainMenu();
                    drawMainMenuBySel(mainSel);
                    screen = Screen::MainMenu;
                    ignoreEncoderUntil = xTaskGetTickCount() + pdMS_TO_TICKS(200);
                } else if (focus == SetCo2Focus::Save) {
                    // TODO: persist currentPpm if needed
                    display.mainMenu();
                    drawMainMenuBySel(mainSel);
                    screen = Screen::MainMenu;
                    ignoreEncoderUntil = xTaskGetTickCount() + pdMS_TO_TICKS(200);
                } else {
                    // If focus is C02 (base), pressing does nothing (or later: switch to edit-ppm mode)
                }
            }
            else if (screen == Screen::Status) {
                display.mainMenu();
                drawMainMenuBySel(mainSel);
                screen = Screen::MainMenu;
                ignoreEncoderUntil = xTaskGetTickCount() + pdMS_TO_TICKS(120);
            }
        }
        // ===== PATCH 3: encoder handling =====
        TickType_t now = xTaskGetTickCount();
        if (!encoder_allowed(screen, now)) {
            // still in ignore/deadband window
            continue;
        }

        if (screen == Screen::MainMenu) {
            // Start with current event
            int net = (ev.direction == rot_rotate_clockwise) ? -1 : +1;

            // Coalesce ONLY consecutive encoder events; stop on first non-encoder
            for (;;) {
                gpio_event_s next{};
                if (xQueuePeek(uiQueue, &next, 0) != pdTRUE || next.type != event_encoder) break;
                xQueueReceive(uiQueue, &next, 0);
                net += (next.direction == rot_rotate_clockwise) ? -1 : +1;
            }

            if      (net > 0) mainSel = (mainSel + 1) % mainItemCount;
            else if (net < 0) mainSel = (mainSel + mainItemCount - 1) % mainItemCount;
            drawMainMenuBySel(mainSel);
        }
        else if (screen == Screen::SetCo2) {
            // One detent = one move (no coalescing). Also drop trailing bounces.
            int step = (ev.direction == rot_rotate_clockwise) ? -1 : +1;

            auto toIndex = [&](SetCo2Focus f) {
                return (f == SetCo2Focus::Exit) ? 0 :
                       (f == SetCo2Focus::C02)  ? 1 : 2;  // Save
            };
            auto fromIndex = [&](int i) {
                return (i == 0) ? SetCo2Focus::Exit :
                       (i == 1) ? SetCo2Focus::C02  : SetCo2Focus::Save;
            };

            int idx = toIndex(focus);
            idx = (idx + step + 3) % 3;          // 3-state ring
            focus = fromIndex(idx);

            drawSetCo2ByFocus();
            // Drop any trailing encoder events from the same detent/bounce
            gpio_event_s tmp;
            while (xQueuePeek(uiQueue, &tmp, 0) == pdTRUE && tmp.type == event_encoder) {
                xQueueReceive(uiQueue, &tmp, 0);
            }
        }
        else if (screen == Screen::Status) {
            // nothing just read // sensor values can be wired-in
        }
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
        display.text("boot", 0, 0);
        display.show();
        vTaskDelay(100);
    }
}

// ISR: kept it tiny  - Chedel
void ui_gpio_isr(uint gpio, uint32_t events)
{
    (void)events;  // keep for future
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    static uint32_t last_sw_time = 0;

    gpio_event_s ev{};
    if (gpio == ROTARY_SW) {
        // Debounce press on falling edge
        if (now - last_sw_time >= BUTTON_DEBOUNCE_MS) {
            last_sw_time = now;
            ev.type = event_button;
            ev.direction = rot_rotate_clockwise; // unused for button
            ev.timestamp = now;
            if (uiQueue) xQueueSendFromISR(uiQueue, &ev, &xHigherPriorityTaskWoken);
        }
    } else if (gpio == ROTARY_A) {
        // Simple quadrature: read B at A's edge to determine CW/CCW
        const bool b = gpio_get(ROTARY_B);
        ev.type = event_encoder;
        ev.direction = b ? rot_rotate_counterclockwise : rot_rotate_clockwise;
        ev.timestamp = now;
        if (uiQueue) xQueueSendFromISR(uiQueue, &ev, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Configure pins and register exactly one bank callback  - Chedel
void ui_inputs_init(void)
{
    if (!uiQueue) {
        uiQueue = xQueueCreate(12, sizeof(gpio_event_s));
    }

    // A/B — inputs w/o pulls
    gpio_init(ROTARY_A);
    gpio_set_dir(ROTARY_A, GPIO_IN);
    gpio_disable_pulls(ROTARY_A);

    gpio_init(ROTARY_B);
    gpio_set_dir(ROTARY_B, GPIO_IN);
    gpio_disable_pulls(ROTARY_B);

    // SW — input with pull-up
    gpio_init(ROTARY_SW);
    gpio_set_dir(ROTARY_SW, GPIO_IN);
    gpio_pull_up(ROTARY_SW);

    // Register one bank callback, then enable IRQs per pin
    gpio_set_irq_enabled_with_callback(ROTARY_A, GPIO_IRQ_EDGE_FALL, true, &ui_gpio_isr);
    gpio_set_irq_enabled(ROTARY_SW, GPIO_IRQ_EDGE_FALL, true);

    // Optional: also read ROTARY_B edges for higher resolution
    // gpio_set_irq_enabled(ROTARY_B, GPIO_IRQ_EDGE_FALL, true);
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

    // commented out for #define gpio inits to be tested if working in tasks_data.h -Chedel
    /*
    const uint ROT_A = 10;
    const uint ROT_B = 11;
    const uint ROT_SW = 12;
    */
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