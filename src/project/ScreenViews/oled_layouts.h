//
// Created by Chedel on 10/4/2025.
//

#ifndef PROJECT_OLED_LAYOUTS_H
#define PROJECT_OLED_LAYOUTS_H

#include <memory>
#include "i2c/PicoI2C.h"
#include "ssd1306os.h"


class OledLayouts : public ssd1306os {

public:
    explicit OledLayouts(std::shared_ptr<PicoI2C> i2c);


    void welcomeScreen();
    void mainMenu(); // selectedIndex is just a bandaid solution still figuring out the encoder isr.
    void mainMenu_C02();
    void mainMenu_STATUS(); // Dummy values only. configure its code pipeline to call real values
    void mainMenu_NETWORK();
    void setCo2Screen(int ppm = 1305); // Dummy values only. configure its code pipeline to call real C02 values
    void setCo2Screen_SAVE(int ppm = 1305); // Dummy values only. configure its code pipeline to call real C02 values
    void setCo2Screen_EXIT(int ppm =1305); // Dummy values only. configure its code pipeline to call real C02 values
    void statusScreen(int co2_read = 0, int co2_set = 0,int rh = 0, int temp = 0, int fan = 0); // test values only
    void networkScreen(const char* ssid = "", const char* pass = "", bool highlightLeftArrow = true);      // i dont know
};

#endif
