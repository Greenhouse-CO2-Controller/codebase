//
// Created by mark on 10/3/25.
//

#include "oled.h"

Oled::Oled(std::shared_ptr<PicoI2C> bus)
    : display(bus) {
    display.fill(0);
    display.show();
}

void Oled::clear() { display.fill(0); }
void Oled::show()  { display.show(); }

void Oled::drawText(int x, int y, const std::string& text, bool color) {
    display.text(text.c_str(), x, y, color ? 1 : 0);
}

void Oled::drawRect(int x, int y, int w, int h, bool color, bool filled) {
    display.rect(x, y, w, h, color ? 1 : 0, filled);
}

// Combined screen
void Oled::updateDisplay(int selectedIndex, int co2Value, float temp, float rh, float fan) {
    clear();

    // Editable CO2 line
    /*
    drawText(10, 10, "SET CO2: " + std::to_string(co2Value), selectedIndex == 0);
    if (selectedIndex == 0) drawRect(8, 9, 88, 10, true, true);
    */
    // Status lines
    drawText(10, 25, "Temp: " + std::to_string((int)temp) + "C");
    drawText(10, 40, "RH: " + std::to_string((int)rh) + "%");
    drawText(10, 55, "Fan: " + std::to_string((int)fan) + "%");

    show();
}
