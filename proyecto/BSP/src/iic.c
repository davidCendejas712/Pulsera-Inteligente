
#include <s3c44b0x.h>
#include <s3cev40.h>
#include <timers.h>
#include <iic.h>

void iic_init( void )
{
	 IICADD = 0x0; // Direccion como escalavo, no tiene direccion porque nunca se va a ver como esclavo
	        /**
	        *  IICCON[6] = 0    :: IICCLK = MCLK/16
	        *  IICCON[3:0] = 15 :: TxCLK = IICCLK / (15+1) = 64MHz / 256
	        */
	 IICCON = 0xAF; //frecuencia de transmision a 250KHz


	         //IICSTAT[4] = 1    :: Serial output enable

	 IICSTAT = 0X10; // Modo de transmision lectura/escritura
}

void iic_start( uint8 mode, uint8 byte )
{
    IICDS   = byte; //dato en el registro de transmision
    IICSTAT = (((mode & 0x3) << 6) | (1 << 5) | (1 << 4)); //ponemos a 1 ICCSTAT[5]
    IICCON &= ~(1 << 4); //0 en iicon[4]
    while((IICCON & (1 << 4)) == 0); //espera a q acabe la transmison
}


void iic_putByte( uint8 byte )
{
	IICDS   = byte;
	IICCON &= ~(1 << 4);
	while( (IICCON & (1 << 4)) == 0 );
}

uint8 iic_getByte( uint8 ack )
{
	IICCON &= ~(1 << 7); // ponemos a 0 el bit 7
	IICCON |= ((ack & 1) << 7); // conf ack

	IICCON &= ~(1 << 4);
	while((IICCON & (1 << 4)) == 0); // hasta recibir un dato
	return IICDS;
}

void iic_stop( uint16 ms )
{
	IICSTAT &= ~(1 << 5);
	IICCON  &= ~(1 << 4);
	sw_delay_ms( ms );
}
