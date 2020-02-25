#ifndef TWI_H
#define TWI_H
//-----------------------------------------------------------------------------
#include "../avr-misc/avr-misc.h"
#include "config.h"
//-----------------------------------------------------------------------------
#ifndef TWI_MASTER_FREQ
#define TWI_MASTER_FREQ 200000UL
#endif

#ifndef TWI_MASTER_BUFFER_SIZE
#define TWI_MASTER_BUFFER_SIZE 32
#endif

#ifndef TWI_MAX_TRIES_COUNT
#define TWI_MAX_TRIES_COUNT 3
#endif
//-----------------------------------------------------------------------------
enum twi_result
{
    TWI_ERR_OK,
    TWI_ERR_NOT_FOUND,      // slave address not found
    TWI_ERR_REJECTED,       // slave is unable to receive data
    TWI_ERR_ABORTED,        // slave has aborted the reception
    TWI_ERR_BUSY,           // the interface is busy
    TWI_ERR_BAD_PARAMETER,  // bad parameter specifiend in the function call
    TWI_ERR_UNKNOWN         // uknown error or state
};
//-----------------------------------------------------------------------------
enum twi_slave_action
{
    TWI_SLAVE_START,
    TWI_SLAVE_BROADCAST_START,
    TWI_SLAVE_DATA,
    TWI_SLAVE_END,
    TWI_SLAVE_MORE
};
//-----------------------------------------------------------------------------
struct twi_slave_config
{
    BYTE address;                   // slave address
    bool receive_broadcasts;        // also called as General call recognition
    BYTE (*slave_receive_callback)(BYTE action);    // receiption function
    BYTE (*slave_transmit_callback)(BYTE action);   // transmission function
    twi_slave_config()
    {
        receive_broadcasts = false;
    }
};
//-----------------------------------------------------------------------------
void twi_init(struct twi_slave_config* config = NULL);
BYTE twi_send(BYTE address, size_t size, BYTE* data, void (*callback)(BYTE result) = NULL);
BYTE twi_receive(BYTE address, size_t size, BYTE* data, void (*callback)(BYTE result) = NULL);
void twi_wait();
//-----------------------------------------------------------------------------
#endif
