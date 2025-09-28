//
// Created by mark on 9/26/25.
//

#ifndef RP2040_FREERTOS_IRQ_EEPROM_DATA_H
#define RP2040_FREERTOS_IRQ_EEPROM_DATA_H

#include <cstring>
#include "EEPROM.h"

class EEPROM_data {
public:
    EEPROM_data();

    uint16_t getCO2() const;
    void setCO2(uint16_t value);

    const char* getSSID() const;
    void setSSID(const char* ssid);

    const char* getPassword() const;
    void setPassword(const char* pass);

private:
    uint16_t co2;
    uint16_t co2_setpoint;
    char wifi_ssid[32]{};
    char wifi_pass[32]{};

    EEPROM eeprom;
};


#endif //RP2040_FREERTOS_IRQ_EEPROM_DATA_H