#include "lpc23xx.h"
#include "interrupt.h"
#include "target.h"
#include "i2c.h"
#include "i2cErr.h"
#include "uart.h"
#include "integer.h"
#include <stdio.h>

#define TIMER0_INT_BIT  (0x00000010)

extern void vi2c(void);
extern void i2enable(void);
extern void ymzwrite0(int,int);
extern void ymzwrite1(int,int);
extern void GPIOResetInit(void);

int i2cErr;



void UARTint (void)
{
  DWORD Fdiv, i = 0;
  U0LCR = 0x83;			/* 8 bits, no Parity, 1 Stop bit */
  Fdiv = (Fpclk / 16) / UART_BAUD;	/*baud rate */
  U0DLM = Fdiv / 256;
  U0DLL = Fdiv % 256;
  U0LCR = 0x03;			/* DLAB = 0 */
  U0FCR = 0x07;			/* Enable and reset TX and RX FIFO. */
}

void Delay(volatile unsigned long nCount)
{
	for(; nCount != 0; nCount--);
}

int issrgetbit(void)
{
	int ret;
	static int bit;
	static int twel = 0;
	static int state = 0;
	if(state){
		state = 0;
		FIO2SET0 = 1;
		Delay(10);
		bit = (FIO2PIN0 & 3)>>1;
		twel = (twel << 1) | bit;
		ret = -1;
	}
	else {
		state = 1;
		FIO2CLR0 = 1;
		Delay(10);
		if (bit == ((FIO2PIN0 & 3)>>1))
			ret = -1;
		else {
			ret = twel;
			twel = 0;
		}
	}
	return ret;
}
/* IRQ 割り込み処理 */
void Isr_TIMER0 (void)
{
	int twel;
	FIO1PIN ^=0x00040000;
	twel = issrgetbit();
	if ( twel != -1)
		ymzwrite0(twel,0);
	T0IR = 1;
}
void TIMER0_handler (void)
{
	FIO1PIN ^=0x00040000;
	printf("1");
	T0IR = 1;
}

void ymzinit(void)
{
	ymzwrite1(0xfe,7);
	ymzwrite1(0xF,0x8);
	ymzwrite1(0xFe,0x7);
}
void fiqregist(void)
{
	/* Use IRQ instead of FIQ */
	VICIntSelect = 0;

	T0PR = 0x00000000;
	/*  *TIMER0_MatchRegister0       = 0x00080000; */
	T0MR0 = 0x00010000;
	T0MCR = 0x00000003;      /* Match時にTCクリア & 割り込み */
	VICIntEnable = TIMER0_INT_BIT;

	RegisterVector(TIMER0_INT, TIMER0_handler, PRI_LOWEST, CLASS_IRQ);
	IrqEnable();

	T0TCR = 1;
}

int main(void)
{
	SCS = SCS | 1;

	TargetResetInit();
	GPIOResetInit();
	UARTint();
	FIO2DIR1 =2;
	FIO1DIR =0x00040000;
	FIO1PIN =0x00000000;
	FIO1MASK=0x00000000;

	printf("hy");
	//ymzinit();
	//ymzwrite0(0x6ff,0);
	//i2enable();
	PCONP |= 0x00001000;
	FIO1PIN =0x00000000;
	fiqregist();
	for(int i;i!=100;i++)
		printf("Hello,World%d\n",i);
	while(1);
}
