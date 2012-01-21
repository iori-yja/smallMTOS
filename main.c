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

int i2cErr;

typedef struct {
	int pid;
	int*pc;
	int regsp[15];
	int sr;
	int*entrypoint;
}process_t;

typedef struct _run{
	struct _run*next;
	process_t p;
	int status;
}run_t;

run_t*currentpr;
run_t zeropr;
run_t onepr;
/*
int switchp(void){
	run_t*next;
	next = currentpr->next;
	return next->p.pc;
}
*/

int create_task(int*functionpointer)
{
	int pid=1;
	return pid;
}
int kill_task(int pid)
{
	return 1;
}
int run_task(int pid)
{
	return 1;
}
void UARTint (void)
{
  DWORD Fdiv = 0;
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
void TIMER0_handler (void)
{
	FIO1PIN ^=0x00040000;
	printf("1");
	T0IR = 1;
}
void TIMER1_handler (void)
{
	FIO1PIN ^=0x00040000;
	printf("Timer 2 message\n");
	T1IR = 1;
}

void ymzinit(void)
{
	ymzwrite1(0xfe,7);
	ymzwrite1(0xF,0x8);
	ymzwrite1(0xFe,0x7);
}
void timer0setup(void)
{
	T0PR = 0x00000000;
	/*  *TIMER0_MatchRegister0       = 0x00080000; */
	T0MR0 = 0x00080000;
	T0MCR = 0x00000003;      /* Match時にTCクリア & 割り込み */
	T0TCR = 1;
}
void timer1setup(void)
{
	T1PR = 0x00000000;
	/*  *TIMER0_MatchRegister0       = 0x00080000; */
	T1MR0 = 0x00080300;
	T1MCR = 0x00000003;      /* Match時にTCクリア & 割り込み */
	T1TCR = 1;
}
void irqregist(char sourcenum,void*handler)
{
	IrqDisable();
	//Use IRQ
	VICIntSelect &= ~(1<<sourcenum);
	VICIntEnable |= 1<<sourcenum;
	RegisterVector(sourcenum, handler, PRI_LOWEST, CLASS_IRQ);
	IrqEnable();
}
void fiqregist(char sourcenum, void*handler)
{
	FiqDisable();
	//Use FIQ
	VICIntSelect |= 1<<sourcenum;
	VICIntEnable |= 1<<sourcenum;
	RegisterVector(sourcenum, handler, PRI_LOWEST, CLASS_IRQ);
	FiqEnable();
}
void func(void){
	while (1){
		Delay(18888);
		printf ("hofg\n");
	}
}
void func1(void){
	while (1){
		Delay(18888);
		printf ("aaag\n");
	}
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
	zeropr.status = 0;
	zeropr.p.pid  = 0;
	zeropr.p.pc   = (void *)func;
	zeropr.next   = &onepr;
	currentpr     = &zeropr;
	onepr.status = 0;
	onepr.p.pid  = 2;
	onepr.p.pc   = (void *)func1;
	onepr.next   = &zeropr;
	printf("hy,hello world!\n");
	//ymzinit();
	//ymzwrite0(0x6ff,0);
	//i2enable();
	irqregist(TIMER0_INT,TIMER0_handler);
	timer0setup();
	func();
	while(1);
}
