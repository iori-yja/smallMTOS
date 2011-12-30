#include"lpc23xx.h"
/******************************************************************************
** Function name:		GPIOResetInit
**
** Descriptions:		Initialize the target board before running the main() 
**				function; User may change it as needed, but may not 
**				deleted it.
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void GPIOResetInit( void )
{
	/* Reset all GPIO pins to default: primary function */
    PINSEL0 = 0x0FA00050;
    PINSEL1 = 0x00154000;
    PINSEL2 = 0x00000000;
    PINSEL3 = 0xF0000000;
    PINSEL4 = 0x00000000;
    PINSEL5 = 0x00000000;
    PINSEL6 = 0x00000000;
    PINSEL7 = 0x00000000;
    PINSEL8 = 0x00000000;
    PINSEL9 = 0x00000000;
   PINSEL10 = 0x00000000;
    
     IODIR0 = 0x00000000;
     IODIR1 = 0x00000000;
     IOSET0 = 0x00000000;
     IOSET1 = 0x00000000;
    
    FIO0DIR = 0x00000000;
    FIO1DIR = 0x00000000;
    FIO2DIR = 0x00000201;
    FIO3DIR = 0x00000000;
    FIO4DIR = 0x0000FFFF;
    
	FIO0MASK = 0x00000000;
	FIO0PIN = 0x00000000;
	FIO0SET = 0x00000000;
	FIO0CLR = 0x00000000;
	
	FIO1MASK = 0x00000000;
	FIO1PIN = 0x00000000;
	FIO1SET = 0x00000000;
	FIO1CLR = 0x00000000;
	
	FIO2MASK = 0x00000000;
	FIO2PIN = 0x00000000;
	FIO2SET = 0x00000000;
	FIO2CLR = 0x00000000;

	FIO3MASK = 0x00000000;
	FIO3PIN = 0x00000000;
	FIO3SET = 0x00000000;
	FIO3CLR = 0x00000000;
	
	FIO4MASK = 0x00000000;
	FIO4PIN = 0x00000000;
	FIO4SET = 0x00000000;
	FIO4CLR = 0x00000000;
	
    return;        
}

