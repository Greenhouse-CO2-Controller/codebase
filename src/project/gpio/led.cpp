//
// Created by mark on 10/7/25.
//

#include "led.h"
#include <iostream>
#include "gpiopin.h"

LED::LED(GPIOPin *led1, GPIOPin *led2, GPIOPin *led3)
    : led1(led1), led2(led2), led3(led3) {}

void LED::turn_all_on() {
    led1->write(true);
    led2->write(true);
    led3->write(true);
}

void LED::turn_all_off() {
    led1->write(false);
    led2->write(false);
    led3->write(false);
}

void LED::set_custom(bool state1, bool state2, bool state3) {
    led1->write(state1);
    led2->write(state2);
    led3->write(state3);
}


void blinking(LED& led_controller, Button& button_sw0, Button& button_sw2) {
    while (true) {
        if (button_sw0.is_pressed() && button_sw2.is_pressed()) {
            break;
        }

        led_controller.turn_all_on();
        sleep_ms(100);

        if (button_sw0.is_pressed() && button_sw2.is_pressed()) {
            break;
        }

        led_controller.turn_all_off();
        sleep_ms(100);
    }
}