//
// Created by mark on 9/26/25.
//

#include "EEPROM_data.h"

#include <cstring>
#include "EEPROM.h"

EEPROM_data::EEPROM_data():eeprom(0x0000, this, sizeof(EEPROM_data)){
    eeprom.eeprom_read_state();
    if (co2_setpoint == 0xFFFF || co2_setpoint == 0) { // this need when device is boot for the first time
        co2_setpoint = 800;
        co2 = 0;
        strcpy(wifi_ssid, "");
        strcpy(wifi_pass, "");
        eeprom.eeprom_write_state(this);
    }

}

uint16_t EEPROM_data::getCO2() const {
    return co2;
}

void EEPROM_data::setCO2(uint16_t value) {
    co2_setpoint = value;
}

const char* EEPROM_data::getSSID() const {
    return wifi_ssid;
}

void EEPROM_data::setSSID(const char* ssid) {
    strncpy(wifi_ssid, ssid, sizeof(wifi_ssid)-1);
    wifi_ssid[sizeof(wifi_ssid)-1] = 0;
    eeprom.eeprom_write_state(this);

}

