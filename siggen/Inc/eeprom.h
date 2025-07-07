#ifndef __EEPROM__H_
#define __EEPROM__H_

#include <stdint.h>

#define EEPROM_PAGE_SIZE (16)

uint8_t eeprom_read_byte (uint16_t address);
void eeprom_write_byte (uint16_t address, uint8_t data);
int eeprom_write_page (uint16_t address, uint8_t *data);

#endif
