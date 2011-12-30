#include "lpc23xx.h"
#include "interrupt.h"
#include "i2c.h"
#include "i2cErr.h"

#define TIMER0_INT_BIT  (0x00000010)

extern void vi2c(void);
extern void i2enable(void);
extern void ymzwrite0(int,int);
extern void ymzwrite1(int,int);
extern void GPIOResetInit(void);

int i2cErr;

void CPU_Initialize(void)
{
	/*** PLLクロック発振初期化 ***/

	/* すでにPLLが動作中だった場合は停止させる */
	if ( PLLSTAT & (1<<25) ) {
		/* PLLCON - PLL Enable & disconnected */
		PLLCON   =0x00000001;
		/* PLL Feed operation */
		PLLFEED  =0x000000AA;
		PLLFEED  =0x00000055;
	}
	/* PLLCON - PLL Disable & disconnected*/
	PLLCON   =0x00000000;
	/* PLL Feed operation */
	PLLFEED  =0x000000AA;
	PLLFEED  =0x00000055;

	SCS = 0x21;	/* 12MHz OSC Enable & FGPIO Select */
	while((SCS&0x40) == 0){}	/* OSCSTAT Wait */

	/* CLKSRCSEL - MASTER oscillator select */
	CLKSRCSEL=0x00000001;

	/* PLLCFG - MCLK=12MHz use, FCC0 = 288MHz M=144,N=12 */
	PLLCFG   =0x000B008F;
	/* PLL Feed operation. */
	PLLFEED  =0x000000AA;
	PLLFEED  =0x00000055;

	/* PLLCON - PLL Enable & disconnected */
	PLLCON   =0x00000001;
	/* PLL Feed operation */
	PLLFEED  =0x000000AA;
	PLLFEED  =0x00000055;

	/* CPU Clock Divider 1/4 */
	CCLKCFG  =0x00000003;
	/* USB Clock Divider 1/6 */
	USBCLKCFG=0x00000005;

	while ( ((PLLSTAT & (1<<26)) == 0) ); /* Check lock bit status */

	/* PLLCON - PLL Enable & Connected */
	PLLCON   =0x00000003;
	/* PLL Feed operation. */
	PLLFEED  =0x000000AA;
	PLLFEED  =0x00000055;
	while ( ((PLLSTAT & (1<<25)) == 0) ); /* Check connect bit status */
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

	RegisterVector(TIMER0_INT, Isr_TIMER0, PRI_LOWEST, CLASS_IRQ);
	IrqEnable();

	T0TCR = 1;
}

int main(void)
{
	SCS = SCS | 1;

	CPU_Initialize();
	GPIOResetInit();
	FIO2DIR1 =2;
	FIO1DIR =0x00040000;
	FIO1MASK=0x00000000;

	ymzinit();
	ymzwrite0(0x6ff,0);
	i2enable();
	PCONP |= 0x00001000;
	FIO1PIN =0x00040000;
	vi2c();
	FIO1PIN =0x00000000;
	fiqregist();
	while(1);
}
