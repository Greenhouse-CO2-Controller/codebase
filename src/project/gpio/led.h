//
// Created by mark on 10/7/25.
//

#ifndef RP2040_FREERTOS_IRQ_LED_H
#define RP2040_FREERTOS_IRQ_LED_H


#include "gpiopin.h"
//class GPIOPin;

class LED {
public:
    LED(GPIOPin *led1, GPIOPin *led2, GPIOPin *led3);

    void turn_all_on();
    void turn_all_off();
    void set_custom(bool state1, bool state2, bool state3);

private:
    GPIOPin *led1;
    GPIOPin *led2;
    GPIOPin *led3;
};

void blinking(LED& led_controller, Button& button_sw0, Button& button_sw2);


#endif //RP2040_FREERTOS_IRQ_LED_H