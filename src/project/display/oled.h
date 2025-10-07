//
// Created by mark on 10/3/25.
//

#include <memory>
#include "PicoI2C.h"
#include "ssd1306os.h"

#ifndef RP2040_FREERTOS_IRQ_OLED_H
#define RP2040_FREERTOS_IRQ_OLED_H

class Oled {
public:
    Oled(std::shared_ptr<PicoI2C> bus);

    void clear();
    void show();
    void drawText(int x, int y, const std::string& text, bool color = true);
    void drawRect(int x, int y, int w, int h, bool color = true, bool filled = true);

    // New combined display
    void updateDisplay(int selectedIndex, int co2Value, float temp, float rh, float fan);

private:
    ssd1306os display;
};


#endif //RP2040_FREERTOS_IRQ_OLED_H