//
// Created by mark on 9/26/25.
//

#ifndef RP2040_FREERTOS_IRQ_EEPROM_DATA_H
#define RP2040_FREERTOS_IRQ_EEPROM_DATA_H

#include <cstdint>
#include "EEPROM.h"

struct Settings {
    float max_co2_setpoint;
    float min_co2_setpoint;
    float co2_setpoint;
    // add more later (fan speed calibration, mode flags, etc.)
};


#endif //RP2040_FREERTOS_IRQ_EEPROM_DATA_H