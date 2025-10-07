//
// Created by mark on 10/7/25.
//

#ifndef RP2040_FREERTOS_IRQ_GPIOPIN_H
#define RP2040_FREERTOS_IRQ_GPIOPIN_H


#include "pico/stdlib.h"
//#include "led.h"

class Button;

class GPIOPin {
public:
    GPIOPin(int pin, bool input = true, bool pullup = true, bool invert = false);
    GPIOPin(const GPIOPin&) = delete;

    bool read();
    bool operator()();
    void write(bool value);
    void operator()(bool value);
    operator int();

private:
    int pin_number;
    bool is_inverted;
};

class Button {

public:
    Button(GPIOPin *gpio);

    bool is_pressed();
    bool pin_read();

private:
    GPIOPin *pin;
    bool previous_state;
    uint32_t last_press_time;
    const uint32_t debounce_time = 50;
};

class ButtonManager {

public:
    ButtonManager(Button *b1, Button *b2);

    bool single_press(Button *btn);

    bool both_pressed();

private:
    Button *button1;
    Button *button2;
};

//void blinking(GPIOPin& led1, GPIOPin& led2, GPIOPin& led3, GPIOPin& sw0, GPIOPin& sw2);


#endif //RP2040_FREERTOS_IRQ_GPIOPIN_H