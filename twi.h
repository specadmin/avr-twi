#ifndef TWI_H
#define TWI_H
//-----------------------------------------------------------------------------
#include "../avr-misc/avr-misc.h"
#include <stdint.h>
//-----------------------------------------------------------------------------
#ifndef TWI_FREQ
#define TWI_FREQ 100000UL
#endif

#ifndef TWI_BUFFER_SIZE
#define TWI_BUFFER_SIZE 32
#endif
//-----------------------------------------------------------------------------
void twi_init();
void twi_write(BYTE address, BYTE* data, uint8_t length, void (*callback)(BYTE, BYTE *));
void twi_read(BYTE address, BYTE length, void (*callback)(BYTE, BYTE *));
uint8_t *twi_wait();
//-----------------------------------------------------------------------------
#endif
