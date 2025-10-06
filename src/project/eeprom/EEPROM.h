//
// Created by mark on 9/26/25.
//

#ifndef RP2040_FREERTOS_IRQ_EEPROM_H
#define RP2040_FREERTOS_IRQ_EEPROM_H


#include <cstring>
#include "hardware/i2c.h"

#define EEPROM_ADDRESS 0x50 // check the address
#define I2C i2c0
#define ADDRESS_SIZE 2

class EEPROM {
public:
    EEPROM(uint16_t address, const void *data, size_t length);
    void eeprom_write_state(void *write_data);
    void eeprom_read_state();
    void eeprom_read_write(void *write_data);

private:
    uint16_t state_address;
    const void *data;
    size_t length;
    i2c_inst_t *i2c;
    uint8_t eeprom_address;
    size_t address_size;
};


#endif //RP2040_FREERTOS_IRQ_EEPROM_H