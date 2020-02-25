# avr-twi

Universal TWI/I2C driver for Atmel AVR

## Adding to your project

Run the following command in your project's root directory

```
git submodule add https://github.com/specadmin/avr-twi lib/avr-twi
```

Include the header file

```
#include "lib/twi.h"
```


## API


###### `twi_init()` or `twi_init(struct twi_slave_config* config)`

Initilize and configure TWI driver.  Should be called before calling any other TWI functions. Could be called again to reconfigure slave, when needed.

* `config` -- a pointer to a slave configuration structure. See [twi.h](https://github.com/specadmin/avr-twi/blob/master/twi.h#L39) and [driver usage example](https://github.com/specadmin/avr-twi-example) for details. The TWI will not become slave, if this parameter is NULL or ommited.

-------------------------------------------------------------------------------

###### `BYTE twi_send(BYTE address, size_t size, BYTE* data, void (*callback)(BYTE result) = NULL)`

Sends a packet via TWI as a master.

* `address` -- a destination slave address or 0 for a broadcast packet;
* `data` -- a pointer to data buffer;
* `size` -- a numer of bytes to send from the given data buffer, should be less or equal to TWI_MASTER_BUFFER_SIZE (see Definitions for details);
* `callback` -- a function pointer to callback (called when transmission completes).

The callback function should be void and accept only one argument:

* `result` -- transmission result (error code), that could be one of the follows:

  * `TWI_ERR_OK` -- the transmission was successfull, whole the data were sent;
  * `TWI_ERR_NOT_FOUND` -- slave address not found;
  * `TWI_ERR_REJECTED` -- slave was unable to receive data;
  * `TWI_ERR_ABORTED` -- slave has aborted the reception;
  * `TWI_ERR_UNKNOWN` -- uknown error or bus state.

If callback is NULL or omitted, than twi_send() will proceed in a **blocking mode** -- it will **NOT** return until the whole data packet will be transmited or any error will happend.

In **non-blocking mode** `twi_send()` could returns one of the following call result (error code):

* `TWI_ERR_OK` -- the call was accepted successfully, the transmission is being started;
* `TWI_ERR_BUSY` -- the driver is busy by executing a previous twi_send() or twi_receive() call;
* `TWI_ERR_BAD_PARAMETER` -- bad parameter specifiend in the function call.

In **blocking mode** `twi_send()` could returns one of the following call result (error code):

* `TWI_ERR_OK` -- the transmission was successfull, whole the data were sent;
* `TWI_ERR_BUSY` -- the driver is busy by executing a previous twi_send() or twi_receive() call;
* `TWI_ERR_BAD_PARAMETER` -- bad parameter specifiend in the function call;
* `TWI_ERR_NOT_FOUND` -- slave address not found;
* `TWI_ERR_REJECTED` -- slave was unable to receive data;
* `TWI_ERR_ABORTED` -- slave has aborted the reception;
* `TWI_ERR_UNKNOWN` -- uknown error or bus state.


Please remember, that callback function should be as fast as possible. Since it is called in the interrupt routine and the TWI bus would **NOT** be freed until this function is being executed. A slow running function could cause data hold and data lost in your TWI network.

-------------------------------------------------------------------------------

## Definitions

* `F_CPU` -- CPU frequency, required
* `TWI_MASTER_FREQ` -- TWI baudrate, when being a master. Defaults to 200 kbit/s, if left undefined.
* `TWI_MASTER_BUFFER_SIZE` -- Maximum transmission buffer size, allocated in the RAM. Defaults to 32, if left undefined.
