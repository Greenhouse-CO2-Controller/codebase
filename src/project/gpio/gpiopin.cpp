//
// Created by mark on 10/7/25.
//

#include "gpiopin.h"
#include "led.h"


GPIOPin::GPIOPin(int pin, bool input, bool pullup, bool invert)
    : pin_number(pin), is_inverted(invert) {
    gpio_init(pin);
    if (input) {
        gpio_set_dir(pin, GPIO_IN);
        if (pullup) {
            gpio_pull_up(pin);
        }
        if (invert) {
            gpio_set_inover(pin, GPIO_OVERRIDE_INVERT);
        }
    } else {
        gpio_set_dir(pin, GPIO_OUT);
        if (invert) {
            gpio_set_outover(pin, GPIO_OVERRIDE_INVERT);
        }
    }
}

bool GPIOPin::read() {
    return gpio_get(pin_number);
}

void GPIOPin::write(bool value) {
    gpio_put(pin_number, value);
}

GPIOPin::operator int() {
    return pin_number;
}

Button::Button(GPIOPin *gpio) : pin(gpio), previous_state(false), last_press_time(0){}

bool Button::is_pressed() {
    bool current_state = pin->read();
    uint32_t current_time = time_us_64() / 1000;

    if (current_state && !previous_state) {
        if (current_time - last_press_time > debounce_time) {
            last_press_time = current_time;
            previous_state = true;
            return true;
        }
    } else if (!current_state) {
        previous_state = false;
    }

    return false;
    /*
    bool current_state = pin->read();
    if (current_state && !previous_state) {
        uint32_t current_time = time_us_64() / 1000;
        if (current_time - last_press_time > debounce_time) {
            last_press_time = current_time;
            previous_state = current_state;
            return true;
        }
    }
    previous_state = current_state;
    return false;*/
}

bool Button::pin_read() {
    return pin->read();
}

ButtonManager::ButtonManager(Button *b1, Button *b2) : button1(b1), button2(b2)  {

}

bool ButtonManager::single_press(Button *btn) {
    return btn->is_pressed();
}


bool ButtonManager::both_pressed() {
    //return button1->is_pressed() && button2->is_pressed();
    return button1->pin_read() && button2->pin_read();
}


/*
void blinking(GPIOPin& led1, GPIOPin& led2, GPIOPin& led3, GPIOPin& sw0, GPIOPin& sw2) {
    while (true) {
        if (sw0.read() && sw2.read()) {
            break;
        }
        led1.write(true);
        led2.write(true);
        led3.write(true);
        sleep_ms(100);

        if (sw0.read() && sw2.read()) {
            break;
        }
        led1.write(false);
        led2.write(false);
        led3.write(false);
        sleep_ms(100);
    }
}
*/
