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
#define MAX_CO2_SETPOINT 1500
#define MIN_CO2_SETPOINT 200
#define I2C_SDA 16
#define I2C_SCL 17
#define FREQUENCY (100*1000)
QueueHandle_t buttonQueue;

void button_init(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}
int main()
{
    static led_params lp1 = { .pin = 20, .delay = 300 };
    stdio_init_all();
    i2c_init(I2C, FREQUENCY);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    button_init(BUTTON_2);
    button_init(BUTTON_1);
    button_init(BUTTON_0);
    printf("\nBoot\n");
    co2Queue = xQueueCreate(1, sizeof(float));
    Uart_s ptr; // this is for uart function
    static SystemObjects sys;

    //sys.eeprom.eeprom_read_state();
    //sys.co2_setpoint = sys.settings.co2_setpoint;
    //sys.confirmed_co2_setpoint = sys.settings.co2_setpoint;
    buttonQueue = xQueueCreate(5, sizeof(ButtonEvent));  // queue can hold 5 button events
    sys.buttonQueue = buttonQueue;


#if 0 //EEPROM
    uint8_t co2_max_setpoint = 0xFF; // 0xFF just a initialize value.
    uint16_t co2_setpoint_write = MAX_CO2_SETPOINT;
    EEPROM co2_setpoint_eeprom(TESTING_EEPROM, &co2_setpoint_eeprom,sizeof(co2_max_setpoint));
    co2_setpoint_eeprom.eeprom_write_state(&co2_setpoint_write); // this is to write. this only need when the program runs
    co2_setpoint_eeprom.eeprom_read_state(); // this need when we press reset

#endif

    gpio_sem = xSemaphoreCreateBinary();

    // ALL tasks from here

#if 1 //controller task
    xTaskCreate(controller_task,"controller_task",512,&sys,tskIDLE_PRIORITY+3,NULL);

#endif

#if 1 //UI task
    xTaskCreate(ui_task, "SSD1306", 512, &sys,
                tskIDLE_PRIORITY + 1, nullptr); // changed (void *) nullptr --> &t_ptr
#endif


#if 1 // modbus task--> control fan and reding sensors
    xTaskCreate(modbus_task, "Modbus", 512, &sys,
                tskIDLE_PRIORITY + 2, nullptr); // changed (void *) nullptr --> &t_ptr
#endif

#if 1
    xTaskCreate(button_task, "Button", 512, &sys,tskIDLE_PRIORITY + 1,nullptr);
#endif
#if 1
    xTaskCreate(eeprom_task, "EEPROM", 512, &sys, tskIDLE_PRIORITY + 2, nullptr);
#endif
#if 0 // connecting to wifi
    xTaskCreate(wifi_task, "WiFi", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif

#if 0 // co2 injecting
    xTaskCreate(co2_injecting_task, "CO2_Inject", 512, &sys, tskIDLE_PRIORITY + 2, nullptr);

#endif
#if 0 // not using now.
    xTaskCreate(i2c_task, "i2c test", 512, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);
#endif



#if 0 // enable this to enter commands. But we have to set this in UI and cloud
    xTaskCreate(uartTask, "UART", 512, &ptr, 1, nullptr);
#endif

#if 0 // not using now.
    xTaskCreate(tls_task, "tls test", 6000, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);
#endif


    // network task - here we need to write to eeprom and read it back
    // when we run this for first time all the important data should write to eeprom
    // Also we need to read it back. so when we press reset after run it reads from the eepro.
    // then we have data locally
    // **** eeprom need in--> network task, co2 setpoint
    // EEPROM task
    // Thinkspeak task

    vTaskStartScheduler();

    while(true){};
}
