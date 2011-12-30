/***************************************************************
	CPU固有 各種初期化ルーチン(LPC2388用)
***************************************************************/

/* CPU初期化関数ヘッダ */
#include "cpu_init.h"

/* システムコントローラ */
#define SYS_SCS			((volatile unsigned int *)(0xE01FC1A0))

/* クロック＆パワーコントローラ制御レジスタ */
#define PLLCON		((volatile unsigned int *)(0xE01FC080))
#define PLLCFG		((volatile unsigned int *)(0xE01FC084))
#define PLLSTAT		((volatile unsigned int *)(0xE01FC088))
#define PLLFEED		((volatile unsigned int *)(0xE01FC08C))

#define CLK_CCLKCFG		((volatile unsigned int *)(0xE01FC104))
#define CLK_USBCLKCFG	((volatile unsigned int *)(0xE01FC108))
#define CLK_CLKSRCSEL	((volatile unsigned int *)(0xE01FC10C))

#define CLK_PCLKSEL0	((volatile unsigned int *)(0xE01FC1A8))
#define CLK_PCLKSEL1	((volatile unsigned int *)(0xE01FC1AC))


/* CPU固有初期化処理 */
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
	CLK_USBCLKCFG=0x00000005;

	while ( ((PLLSTAT & (1<<26)) == 0) ); /* Check lock bit status */

	/* PLLCON - PLL Enable & Connected */
	PLLCON   =0x00000003;
	/* PLL Feed operation. */
	PLLFEED  =0x000000AA;
	PLLFEED  =0x00000055;
	while ( ((PLLSTAT & (1<<25)) == 0) ); /* Check connect bit status */
}
