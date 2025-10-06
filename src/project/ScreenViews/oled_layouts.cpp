//
// Created by Chedel on 10/4/2025.
//

#include "project/ScreenViews/oled_layouts.h"
#include "project/tasks/tasks_data.h"
#include "ModbusClient.h"
#include "framebuf.h"
#include "ssd1306os.h"

OledLayouts::OledLayouts(std::shared_ptr<PicoI2C> i2c)
: ssd1306os(std::move(i2c)) {}

void OledLayouts::welcomeScreen() {
    fill(0);
    text("Group 6",    38, 10);
    text("Mark,Visal", 27, 30);
    text("Chedel",     42, 40);
    show();
}

// --------- SCREEN   MAIN

void OledLayouts::mainMenu() {

    fill(0);
    text("SET CO2",      22, 10, 1);
    text("SHOW STATUS",  22, 25, 1);
    text("SET NETWORK",  22, 40, 1);
    show();
}

void OledLayouts::mainMenu_C02() {

    fill(0);
    rect(22,9,88,10,1,true); //SET CO2
    text("SET CO2",      22, 10, 0);
    text("SHOW STATUS",  22, 25, 1);
    text("SET NETWORK",  22, 40, 1);
    show();
}

void OledLayouts::mainMenu_STATUS() {

    fill(0);
    rect(22,24,88,10,1,true); // SHOW STATUS
    text("SET CO2",      22, 10, 1);
    text("SHOW STATUS",  22, 25, 0);
    text("SET NETWORK",  22, 40, 1);
    show();
}

void OledLayouts::mainMenu_NETWORK() {

    fill(0);
    rect(22,39,88,10,1,true); // SET NETWORK
    text("SET CO2",      22, 10, 1);
    text("SHOW STATUS",  22, 25, 1);
    text("SET NETWORK",  22, 40, 0);
    show();
}

// --------- SCREEN   C02

void OledLayouts::setCo2Screen(int ppm) {
    fill(0);
    char buf[8];
    rect(88, 15, 35, 15, 1, true);
    snprintf(buf, sizeof(buf), "%d", ppm);
    text(buf, 88, 18, 0);
    text("SET NEW",  9, 10);
    text("PPM LVL",  9, 23, 1);
    text("<-",     0, 48);
    text("SAVE",    98, 48);
    show();
}

void OledLayouts::setCo2Screen_SAVE(int ppm) {
    fill(0);
    char buf[8];
    rect(88, 15, 35, 15, 0, true);
    snprintf(buf, sizeof(buf), "%d", ppm);
    text(buf, 88, 18, 1);
    rect(95, 45, 32, 13, 1, true);
    text("SET NEW",  9, 10);
    text("PPM LVL",  9, 23, 1);
    text("<-",     0, 48);
    text("SAVE",    95, 48,0);
    show();
}

void OledLayouts::setCo2Screen_EXIT(int ppm) {
    fill(0);
    char buf[8];
    rect(88, 15, 35, 15, 0, true);
    snprintf(buf, sizeof(buf), "%d", ppm);
    text(buf, 88, 18, 1);
    rect(0, 45, 17, 13, 1, true);
    text("SET NEW",  9, 10);
    text("PPM LVL",  9, 23, 1);
    text("<-",     0, 48,0);
    text("SAVE",    98, 48,1);
    show();
}


// --------- SCREEN   STATUS

void OledLayouts::statusScreen(int co2_read, int co2_set, int rh, int temp, int fan) {
    fill(0);

    rect(110, 52, 17, 15, 1, true);
    text("->", 110, 55, 0);

    char line[20];

    text("CO2 RDG:", 0, 0);
    snprintf(line, sizeof(line), "%d", co2_read);
    text(line, 64, 0);

    text("SET.CO2:", 0, 13);
    snprintf(line, sizeof(line), "%d", co2_set);
    text(line, 64, 13);

    text("R.HUM:",   0, 26);
    snprintf(line, sizeof(line), "%d", rh);
    text(line, 64, 26);

    text("TEMP:",    0, 40);
    snprintf(line, sizeof(line), "%d", temp);
    text(line, 64, 40);

    text("FAN.SP:",  0, 55);
    snprintf(line, sizeof(line), "%d", fan);
    text(line, 64, 55);

    show();
}

// --------- SCREEN   NETWORK

void OledLayouts::networkScreen(const char* ssid, const char* pass, bool highlightLeftArrow) {
    fill(0);

    if (highlightLeftArrow) {
        rect(0, 52, 17, 15, 1, true);
        text("<-", 0, 55, 0);
    }

    text("SAVE",  95, 55);
    text("SSID:",  0, 15);
    text("PASS:", 0, 33);

    if (ssid && *ssid)  text(ssid, 40, 15);
    if (pass && *pass)  text(pass, 40, 33);

    show();
}



