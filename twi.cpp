#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <string.h>
#include "../avr-debug/debug.h"
#include "twi.h"
//-----------------------------------------------------------------------------
static volatile BYTE busy;
static struct
{
    BYTE address;
    BYTE tx_buffer[TWI_MASTER_BUFFER_SIZE];
    BYTE* rx_buffer;
    BYTE size;
    BYTE index;
    BYTE result;
    BYTE tries_left;
    void (*callback)(BYTE result);
} master;
static struct
{
    BYTE (*receiver)(BYTE action);
    BYTE (*transmitter)(BYTE action);
} slave;
//-----------------------------------------------------------------------------
void twi_reset();
void twi_start();
//-----------------------------------------------------------------------------
/**
 * @brief Initilize and configure TWI
 * @param config A pointer to a slave configuration structure. The TWI will not
 * become slave, if this parameter is NULL or ommited.
 */
void twi_init(struct twi_slave_config* config)
{
    TWBR = ((F_CPU / TWI_MASTER_FREQ) - 16) / 2;
    TWSR = 0; // prescaler = 1
    busy = 0;
    if(config)
    {
        // configure slave
        slave.receiver = config->slave_receive_callback;
        slave.transmitter = config->slave_transmit_callback;
        TWAR = config->address << 1 | config->receive_broadcasts;
    }
    else
    {
        TWAR = 0;
    }
    twi_reset();
}
//-----------------------------------------------------------------------------
/**
 * @brief Block futher processing until the last non-blocking call of twi_send()
 * or twi_receveive() will complete it's action.
 */
void twi_wait()
{
   while(busy);
}
//-----------------------------------------------------------------------------
/**
 * @brief Send a packet via TWI as a master
 * @param address Destination slave address or 0 for a broadcast packet
 * @param size Data size
 * @data data Data pointer
 * @param callback (optional) A method to be called after transmission will be
 * completed. This method should be as fast as possible, since it is called in
 * the interrupt and the TWI bus would NOT be freed until this method is running.
 * A slow method could cause data hold and data lost in your TWI network.
 * The method receives two parameters: result - transmission result (error code)
 * and size - actual number of bytes sent. If callback parameter is NULL or
 * ommited, then twi_send() does not return until the transmission is completed.
 */
BYTE twi_send(BYTE address, size_t size, BYTE* data, void (*callback)(BYTE result))
{
    if(busy)
    {
        return TWI_ERR_BUSY;
    }
    if(size == 0 || size > TWI_MASTER_BUFFER_SIZE)
    {
        return TWI_ERR_BAD_PARAMETER;
    }
    busy = 1;
    master.address = (address << 1) | TW_WRITE;
    memcpy(master.tx_buffer, data, size);
    master.size = size;
    master.index = 0;
    master.tries_left = TWI_MAX_TRIES_COUNT;
    master.callback = callback;
    master.result = TWI_ERR_UNKNOWN;
    twi_start();
    if(callback)
    {
        return TWI_ERR_OK;
    }
    else
    {
        while(busy);
        return master.result;
    }
}
//-----------------------------------------------------------------------------
/**
 * @brief Receive a packet via TWI as a master
 * @param address Source slave address
 * @param size Data size
 * @data data Data pointer to store the received data in
 * @param callback (optional) A method to be called after the receiption will be
 * completed. This method should be as fast as possible, since it is called in
 * the interrupt and the TWI bus would NOT be freed until this method is running.
 * A slow method could cause data hold and data lost in your TWI network.
 * The method receives two parameters: result - receiption result (error code)
 * and size - actual number of bytes received. If callback parameter is NULL or
 * ommited, then twi_receive() does not return until the receiption is completed.
 */
//-----------------------------------------------------------------------------
BYTE twi_receive(BYTE address, size_t size, BYTE *data, void (*callback)(BYTE result))
{
    if(busy)
    {
        return TWI_ERR_BUSY;
    }
    if(address == 0 || size == 0)
    {
        return TWI_ERR_BAD_PARAMETER;
    }
    busy = 1;
    master.address = (address << 1) | TW_READ;
    master.rx_buffer = data;
    master.size = size;
    master.index = 0;
    master.tries_left = TWI_MAX_TRIES_COUNT;
    master.callback = callback;
    master.result = TWI_ERR_UNKNOWN;
    twi_start();
    if(callback)
    {
        return TWI_ERR_OK;
    }
    else
    {
        while(busy);
        return master.result;
    }
}
//-----------------------------------------------------------------------------
__inline void twi_reset()
{
    if(TWAR)
    {
        // enter slave mode
        TWCR = _bit(TWINT) | _bit(TWEA) | _bit(TWEN) | _bit(TWIE);
    }
    else
    {
        // enable the TWI bus only
        TWCR = _bit(TWEN);
    }
    busy = 0;
}
//-----------------------------------------------------------------------------
__inline void twi_start()
{
    TWCR = _bit(TWINT) | _bit(TWEN) | _bit(TWIE) | _bit(TWSTA);
}
//-----------------------------------------------------------------------------
__inline void twi_stop()
{
    TWCR = _bit(TWINT) | _bit(TWEN) | _bit(TWSTO);
    busy = 0;
}
//-----------------------------------------------------------------------------
__inline void twi_return(BYTE result)
{
    if(master.callback)
    {
        master.callback(result);
        // reset callback pointer to prevent any more calls
        master.callback = NULL;
    }
    else
    {
        master.result = result;
    }
}
//-----------------------------------------------------------------------------
__inline void twi_ack()
{
    TWCR = _bit(TWINT) | _bit(TWEN) | _bit(TWIE) | _bit(TWEA);
}
//-----------------------------------------------------------------------------
__inline void twi_nack()
{
    TWCR = _bit(TWINT) | _bit(TWEN) | _bit(TWIE);
}
//-----------------------------------------------------------------------------
__inline void twi_send(BYTE data)
{
    TWDR = data;
}
//-----------------------------------------------------------------------------
__inline void twi_recv()
{
    *(master.rx_buffer + master.index++) = TWDR;
}
//-----------------------------------------------------------------------------
ISR(TWI_vect)
{
    switch (TW_STATUS)
    {
    //////////////////
    // master events //
    //////////////////
    case TW_START:
    case TW_REP_START:
        twi_send(master.address);
        twi_ack();
        break;
    case TW_MT_SLA_ACK:
    case TW_MT_DATA_ACK:
        if(master.index < master.size)
        {
            twi_send(master.tx_buffer[master.index++]);
            twi_nack();
        }
        else
        {
            // complete the transmission session
            twi_stop();
            twi_return(TWI_ERR_OK);
        }
        break;
    case TW_MR_SLA_ACK:
        if(master.size > 1)
        {
            twi_ack();
        }
        else
        {
            twi_nack();
        }
        break;
    case TW_MR_DATA_ACK:
        twi_recv();
        if(master.index < (master.size - 1))
        {
            twi_ack();
        }
        else
        {
            // the next byte will be the last
            twi_nack();
        }
        break;
    case TW_MT_ARB_LOST:
        // restart transmission when the bus becomes free
        twi_start();
        break;
    case TW_MT_SLA_NACK:
    case TW_MR_SLA_NACK:
        // slave does not confirmed it's address, this means that the slave
        // is not present or it is temporary detached from the bus
        if(--master.tries_left)
        {
            // try again - send repeated start
            twi_start();
        }
        else
        {
            twi_stop();
            twi_return(TWI_ERR_NOT_FOUND);
        }
        break;
    case TW_MR_DATA_NACK:
        twi_recv();
        // the slave does not confirmed the last transmited byte,
        // this means that the slave has completed the transmition
        twi_stop();
        twi_return(TWI_ERR_OK);
        break;
    case TW_MT_DATA_NACK:
        twi_stop();
        if(master.index > 1)
        {
            // Some data bytes were sent successfully, but the
            // slave does not confirmed the last received byte.
            // This means, that the slave was unable to continue the reception.
            twi_return(TWI_ERR_ABORTED);
        }
        else
        {
            // No data bytes were sent yet. This means,
            // that the slave has rejected the call.
            twi_return(TWI_ERR_REJECTED);
        }
        break;

    //////////////////
    // slave events //
    //////////////////
    case TW_SR_SLA_ACK:
        if(slave.receiver)
        {
            if(slave.receiver(TWI_SLAVE_START) == TWI_ERR_OK)
            {
                twi_ack();
            }
            else
            {
                twi_nack();
            }
        }
        break;
    case TW_SR_GCALL_ACK:
        if(slave.receiver)
        {
            if(slave.receiver(TWI_SLAVE_BROADCAST_START) == TWI_ERR_OK)
            {
                twi_ack();
            }
            else
            {
                twi_nack();
            }
        }
        break;
    case TW_SR_DATA_ACK:
    case TW_SR_GCALL_DATA_ACK:
        if(slave.receiver)
        {
            slave.receiver(TWI_SLAVE_DATA);
        }
        twi_ack();
        break;
    case TW_ST_SLA_ACK:
        if(slave.transmitter)
        {
            if(slave.transmitter(TWI_SLAVE_START) == TWI_ERR_OK)
            {
                twi_ack();
            }
            else
            {
                twi_nack();
            }
        }
        break;
    case TW_ST_DATA_ACK:
        if(slave.transmitter)
        {
            if(slave.transmitter(TWI_SLAVE_DATA) == TWI_ERR_OK)
            {
                twi_ack();
            }
            else
            {
                twi_nack();
            }
        }
        break;
    case TW_SR_STOP:
        clr_bit(PORTB, 0);
        if(slave.receiver)
        {
            slave.receiver(TWI_SLAVE_END);
        }
        twi_reset();
        break;
    case TW_ST_DATA_NACK:
        // The master informs, that the previous transmited byte was
        // the last in the session and it would not request any more bytes.
        if(slave.transmitter)
        {
            slave.transmitter(TWI_SLAVE_END);
        }
        twi_reset();
        break;
    case TW_ST_LAST_DATA:
        // The master continues requesting futher data while we have nothing
        // to transmit. Inform the slave receiver and detach from the session.
        // The master will receive 0xFF values after that
        if(slave.transmitter)
        {
            slave.transmitter(TWI_SLAVE_MORE);
        }
        twi_reset();
        break;
    case TW_SR_DATA_NACK:
    case TW_NO_INFO:
    case TW_BUS_ERROR:
    default:
        twi_reset();
        twi_return(TWI_ERR_UNKNOWN);
    }
}
//-----------------------------------------------------------------------------
