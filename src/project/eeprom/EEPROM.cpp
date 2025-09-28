//
// Created by mark on 9/26/25.
//

#include "EEPROM.h"
#include "FreeRTOS.h"
#include "task.h"

EEPROM::EEPROM(uint16_t address, const void *data, size_t length)
    : state_address(address), data(data), length(length), i2c(I2C), eeprom_address(EEPROM_ADDRESS), address_size(ADDRESS_SIZE) {}

void EEPROM::eeprom_write_state(void *write_data) {
    uint8_t buffer[address_size + length];
    buffer[0] = (state_address >> 8) & 0xFF;
    buffer[1] = state_address & 0xFF;
    memcpy(&buffer[2], write_data, length);
    i2c_write_blocking(i2c, eeprom_address, buffer, sizeof(buffer), false);
    //sleep_ms(50); // we have to use freertos delay
    vTaskDelay(50);
}

void EEPROM::eeprom_read_state() {
    uint8_t addr_buffer[address_size];
    addr_buffer[0] = (state_address >> 8) & 0xFF;
    addr_buffer[1] = state_address & 0xFF;
    i2c_write_blocking(i2c, eeprom_address, addr_buffer, 2, true);
    i2c_read_blocking(i2c, eeprom_address, (uint8_t *)data, length, false);
}