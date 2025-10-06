#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306.h"
#include "timers.h"
#include "project/tasks/tasks_data.h"
#include "project/tasks/tasks.h"
#include "hardware/timer.h"
//my includes starts
#include "project/eeprom/EEPROM.h"
#include "project/eeprom/EEPROM_data.h"

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

#include "blinker.h"
static led_data_s *led_data_for_isr = NULL;

#define TESTING_EEPROM 0x01
int main()
{
    static led_params lp1 = { .pin = 20, .delay = 300 };
    stdio_init_all();
    printf("\nBoot\n");
    co2Queue = xQueueCreate(1, sizeof(float));

    //UI event Queue -Chedel
    uiQueue = xQueueCreate(10, sizeof(gpio_event_s));

    Uart_s ptr; // this is for uart function
    Data sensor_ptr;
    static SystemObjects sys;

    gpio_sem = xSemaphoreCreateBinary();
    //xTaskCreate(blink_task, "LED_1", 256, (void *) &lp1, tskIDLE_PRIORITY + 1, nullptr);
    //xTaskCreate(gpio_task, "BUTTON", 256, (void *) nullptr, tskIDLE_PRIORITY + 1, nullptr);
    //xTaskCreate(serial_task, "UART1", 256, (void *) nullptr,
    //            tskIDLE_PRIORITY + 1, nullptr);

    // ALL tasks from here

#if 1 //controller task
    xTaskCreate(controller_task,"controller_task",512,&sys,tskIDLE_PRIORITY+3,NULL);

#endif
#if 1 // modbus task--> control fan and reding sensors
    xTaskCreate(modbus_task, "Modbus", 512, &sys,
                tskIDLE_PRIORITY + 2, nullptr); // changed (void *) nullptr --> &t_ptr

    //xTaskCreate(AI1_counter_task, "AI1_counter", 512, nullptr,
                //tskIDLE_PRIORITY+1,nullptr);
#endif

#if 0 // co2 injecting
    xTaskCreate(co2_injecting_task, "CO2_Inject", 512, &sys, tskIDLE_PRIORITY + 2, nullptr);

#endif
#if 0 // not using now.
    xTaskCreate(i2c_task, "i2c test", 512, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);
#endif

#if 1 //UI task
    xTaskCreate(ui_task, "SSD1306", 512, &sensor_ptr,
                tskIDLE_PRIORITY + 1, nullptr); // changed (void *) nullptr --> &t_ptr
#endif

#if 0 // enable this to enter commands. But we have to set this in UI and cloud
    xTaskCreate(uartTask, "UART", 512, &ptr, 1, nullptr);
#endif

#if 0 // not using now.
    xTaskCreate(tls_task, "tls test", 6000, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);
#endif


    // network task
    // EEPROM task
    // Thinkspeak task

    vTaskStartScheduler();

    while(true){};
}














//EEPROM testing
/*
uint8_t eeprom_testing = 0xFF; // 0xFF just a initialize value.
uint8_t testing_write = 0x01;
EEPROM testing(TESTING_EEPROM, &eeprom_testing,sizeof(eeprom_testing));
testing.eeprom_read_state(); // this will store the data in eeprom_testing
testing.eeprom_write_state(&testing_write); // write data to eerpom
testing.eeprom_read_write(&testing_write); // do both read and write. This is under construction.
*/
// EEPROM testing ends














/*
/*
SemaphoreHandle_t gpio_sem;

void gpio_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // signal task that a button was pressed
    xSemaphoreGiveFromISR(gpio_sem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

struct led_params{
    uint pin;
    uint delay;
};

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

void serial_task(void *param)
{
    PicoOsUart u(0, 0, 1, 115200);
    Blinker blinky(20);
    uint8_t buffer[64];
    std::string line;
    while (true) {
        if(int count = u.read(buffer, 63, 30); count > 0) {
            u.write(buffer, count);
            buffer[count] = '\0';
            line += reinterpret_cast<const char *>(buffer);
            if(line.find_first_of("\n\r") != std::string::npos){
                u.send("\n");
                std::istringstream input(line);
                std::string cmd;
                input >> cmd;
                if(cmd == "delay") {
                    uint32_t i = 0;
                    input >> i;
                    blinky.on(i);
                }
                else if (cmd == "off") {
                    blinky.off();
                }
                line.clear();
            }
        }
    }
}


extern "C" {
    void tls_test(void);
}
void tls_task(void *param)
{
    tls_test();
    while(true) {
        vTaskDelay(100);
    }
}

// our codes starts here
void fan_task(void *param); // to control the fan speed


struct Program {
    PicoOsUart uart;
    TimerHandle_t inactivityTimer; // timer object
    TimerHandle_t ledTimer; // timer object
    std::string inputBuffer;
    TickType_t lastLedToggleTick;

    Program() : uart(0, 0, 1, 115200), inactivityTimer(nullptr), ledTimer(nullptr), lastLedToggleTick(0) {}
};

void processCommand(Program *ptr, const std::string &cmd) { // this is only for clion serial port purposes
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
    auto *ptr = static_cast<Program*>(param);
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


#include <cstdio>
#include "ModbusClient.h"
#include "ModbusRegister.h"

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

void modbus_task(void *param) {

    auto *ptr = static_cast<tasks_return*>(param);
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
    vTaskDelay(pdMS_TO_TICKS(100));
    produal.write(0); // need to set this based on the C02 level
    vTaskDelay((100));
    produal.write(0); // 100 means 10%
#endif

    while (true) {
#ifdef USE_MODBUS

        gpio_put(led_pin, !gpio_get(led_pin)); // toggle  led
        // these need to be output to oled.(and cloud if needed)
        float val = rh.read();
        printf("RH=%5.1f%%\n", val / 10.0);
        vTaskDelay(5);
        ptr->t_return = t.read();
        printf("T =%5.1f%%\n", ptr->t_return / 10.0);
        vTaskDelay(5);
        ptr->produal_return = produal.read();
        printf("fan =%5.1f%%\n", ptr->produal_return/10.0);
        vTaskDelay(5);
        ptr->co2_return = c02.read();
        printf("co2 =%5.1f\n", ptr->co2_return  /10.0);
        vTaskDelay(3000);


#endif
    }

}

#include "ssd1306os.h"
void display_task(void *param) // for led display(UI)
{
    auto *ptr = static_cast<tasks_return*>(param);
    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    ssd1306os display(i2cbus);
    display.fill(0);

    //display.text("Display is ok", 0, 0);
    // display ptr->rh.read()
    display.show();
    while(true) {
        display.fill(0);
        char buf[32];
        snprintf(buf, sizeof(buf), "RH=%5.1f%%", ptr->rt_return / 10.0);
        display.text(buf, 0, 0);
        display.show();
        vTaskDelay(100);
    }

}

void i2c_task(void *param) {
    auto i2cbus{std::make_shared<PicoI2C>(0, 100000)};

    const uint led_pin = 21;
    const uint delay = pdMS_TO_TICKS(250);
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    uint8_t buffer[64] = {0};
    i2cbus->write(0x50, buffer, 2);

    auto rv = i2cbus->read(0x50, buffer, 64);
    printf("rv=%u\n", rv);
    for(int i = 0; i < 64; ++i) {
        printf("%c", isprint(buffer[i]) ? buffer[i] : '_');
    }
    printf("\n");

    buffer[0]=0;
    buffer[1]=64;
    rv = i2cbus->transaction(0x50, buffer, 2, buffer, 64);
    printf("rv=%u\n", rv);
    for(int i = 0; i < 64; ++i) {
        printf("%c", isprint(buffer[i]) ? buffer[i] : '_');
    }
    printf("\n");

    while(true) {
        gpio_put(led_pin, 1);
        vTaskDelay(delay);
        gpio_put(led_pin, 0);
        vTaskDelay(delay);
    }


}
*/