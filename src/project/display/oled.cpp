//
// Created by mark on 10/3/25.
//

#include "oled.h"
#include <iostream>
#include "font_petme128_8x8.h"

OLED::OLED( std::shared_ptr<PicoI2C> i2c): ssd1306os(i2c) {
    width = 0;
}

void OLED::Welcome_Display() {};

void OLED::Main_Menu() {};
//Welcome_Display
#if 0
void display_task(void *param)
{
    display.fill(0);
    display.text("Group 6", 38, 10);
    display.text("Mark,Visal", 27, 30);
    display.text("Chedel", 42, 40);
    display.show();

}
#endif

// Menu Option
#if 0
void display_task(void *param)
{
    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);
    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);

    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    ssd1306os display(i2cbus);
    display.fill(0);
    //display.rect(22,9,88,10,1,true); //SET CO2
    //display.rect(22,24,88,10,1,true); // SHOW STATUS
    display.rect(22,39,88,10,1,true); // SET NETWORK
    display.text("SET C02", 22, 10, 1);
    display.text("SHOW STATUS", 22, 25,1);
    display.text("SET NETWORK", 22, 40, 0);
    display.show();
    while(true) {
        vTaskDelay(100);
    }
}

#endif


// Set C02
#if 0
void display_task(void *param)
{
    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);
    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);

    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    ssd1306os display(i2cbus);
    display.fill(0);
    display.rect(88,15,35,15,1,true);
    display.text("1305", 88, 18,0); //
    display.text("SET NEW", 9, 10);
    display.text("PPM LVL",9, 23,1);
    display.text("<-", 0, 48);
    display.text("SAVE",98, 48);
    display.show();
    while(true) {
        vTaskDelay(100);
    }

}
#endif

// View Status
#if 0
void display_task(void *param)
{
    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);
    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);

    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    ssd1306os display(i2cbus);
    display.fill(0);
    display.rect(110,52,17,15,1,true); // ->
    display.text("->", 110, 55,0);
    display.text("C02.RDG:", 0, 0); // display.text("here we need to pass our variable", 0, 0);
    display.text("SET.C02: ", 0, 13);
    display.text("R.HUM:", 0, 26);
    display.text("TEMP:", 0, 40);
    display.text("FAN.SP:", 0, 55);
    display.show();
    while(true) {
        vTaskDelay(100);
    }
}

#endif
// Set Network
#if 0
void display_task(void *param)
{
    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);
    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);

    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    ssd1306os display(i2cbus);
    display.fill(0);
    display.rect(0,52,17,15,1,true); // <-
    display.text("<-", 0, 55,0);
    display.text("SAVE", 95, 55);
    display.text("SSID: ", 0, 15);
    display.text("PSSWD:", 0, 33);
    display.show();
    while(true) {
        vTaskDelay(100);
    }
}
#endif

/*
void OLED::Main_Menu_SET_CO2() {};
void OLED::Main_Menu_SHOW_STATUS() {};
void OLED::Main_Menu_SET_NETWORK() {};
void OLED::SET_CO2_CHANGE_CO2() {};
void OLED::SET_CO2_EXIT() {};
void OLED::SET_CO2_SAVE() {};
*/

