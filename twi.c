#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <string.h>
#include "twi.h"
//-----------------------------------------------------------------------------
static volatile BYTE busy;
static struct
{
    BYTE buffer[TWI_BUFFER_SIZE];
    BYTE size;
    BYTE index;
    void (*callback)(BYTE, BYTE*);
} transmission;
//-----------------------------------------------------------------------------
void twi_init()
{
    TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
    TWSR = 0; // prescaler = 1
    busy = 0;
    TWCR = _bit(TWEN);
}
//-----------------------------------------------------------------------------
BYTE* twi_wait()
{
   while(busy);
   return &transmission.buffer[1];
}
//-----------------------------------------------------------------------------
void twi_start()
{
    TWCR = _bit(TWINT) | _bit(TWEN) | _bit(TWIE) | _bit(TWSTA);
}
//-----------------------------------------------------------------------------
void twi_stop()
{
    TWCR = _bit(TWINT) | _bit(TWEN) | _bit(TWSTO);
}
//-----------------------------------------------------------------------------
void twi_ack()
{
    TWCR = _bit(TWINT) | _bit(TWEN) | _bit(TWIE) | _bit(TWEA);
}
//-----------------------------------------------------------------------------
void twi_nack()
{
    TWCR = _bit(TWINT) | _bit(TWEN) | _bit(TWIE);
}
//-----------------------------------------------------------------------------
void twi_send(BYTE data)
{
    TWDR = data;
}
//-----------------------------------------------------------------------------
void twi_recv()
{
    transmission.buffer[transmission.index++] = TWDR;
}
//-----------------------------------------------------------------------------
void twi_reply()
{
    if(transmission.index < (transmission.size - 1))
    {
        twi_ack();
    }
    else
    {
        twi_nack();
    }
}
//-----------------------------------------------------------------------------
void twi_done()
{
    BYTE address = transmission.buffer[0] >> 1;
    BYTE* data = &transmission.buffer[1];
    busy = 0;
    if(transmission.callback)
    {
        transmission.callback(address, data);
    }
}
//-----------------------------------------------------------------------------
void twi_write(BYTE address, BYTE* data, BYTE size, void (*callback)(BYTE, BYTE*))
{
    twi_wait();
    busy = 1;
    transmission.buffer[0] = (address << 1) | TW_WRITE;
    memcpy(transmission.buffer + 1, data, size);
    transmission.size = size + 1;
    transmission.index = 0;
    transmission.callback = callback;
    twi_start();
}
//-----------------------------------------------------------------------------
void twi_read(BYTE address, BYTE size, void (*callback)(BYTE, BYTE *))
{
    twi_wait();
    busy = 1;
    transmission.buffer[0] = (address << 1) | TW_READ;
    transmission.size = size + 1;
    transmission.index = 0;
    transmission.callback = callback;
    twi_start();
}
//-----------------------------------------------------------------------------
ISR(TWI_vect)
{
    switch (TW_STATUS)
    {
    case TW_START:
    case TW_REP_START:
    case TW_MT_SLA_ACK:
    case TW_MT_DATA_ACK:
        if (transmission.index < transmission.size)
        {
            twi_send(transmission.buffer[transmission.index++]);
            twi_nack();
        }
        else
        {
            twi_stop();
            twi_done();
        }
        break;
    case TW_MR_DATA_ACK:
        twi_recv();
        twi_reply();
        break;
    case TW_MR_SLA_ACK:
        twi_reply();
        break;
    case TW_MR_DATA_NACK:
        twi_recv();
        twi_stop();
        twi_done();
        break;
    case TW_MT_SLA_NACK:
    case TW_MR_SLA_NACK:
    case TW_MT_DATA_NACK:
    default:
        twi_stop();
        twi_done();
    }
}
//-----------------------------------------------------------------------------
