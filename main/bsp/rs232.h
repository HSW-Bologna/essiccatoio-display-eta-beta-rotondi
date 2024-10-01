#ifndef BSP_RS232_H_INCLUDED
#define BSP_RS232_H_INCLUDED


#include <stdint.h>
#include <stdlib.h>


void bsp_rs232_init(void);
void bsp_rs232_flush(void);
int  bsp_rs232_write(uint8_t *buffer, size_t len);
int  bsp_rs232_read(uint8_t *buffer, size_t len);


#endif
