/*
 * =====================================================================================
 *
 *       Filename:  i2clib.c
 *
 *    Description:  i2c library
 *
 *        Version:  1.0
 *        Created:  2011年09月27日 19時26分11秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Iori-yja (), 
 *        Company:  
 *
 * =====================================================================================
 */
/****************************************************************************
This is i2c source for LPC23xx/24xx micro processor.
Author		:iorivr
History	:30th,Jan,2010 stared to write.
function	:i2c initialize, data sending, and data resieving.
NOTE		:It is NOT to eat.
		 distributed on LGPL license.
		 Also, I will adjust even for SCCB.

****************************************************************************
|\  | |""""| """|""" |"""" 
After 502page on user manual is very usefull. Please reference for it!!
| \ | |    |    |    |====	
|  \| |____|    |    |____
******************************************************************************/
#include"lpc23xx.h"
#include"i2c.h"
#include"i2cErr.h"
#include"OV7670userconfig.h"


#define  RESETREG SCCBByteWrite(0x1280);
/****************************************************************************
function name	:i2enable
about function:as name.
Problem	:
****************************************************************************/
void
i2enable ()
{
  PINMODE0 = 0x00000000;
}

/****************************************************************************
function name	:i2cmasterset
discribe	:the i2c periphrals which given number will be master
****************************************************************************/
int
i2cmasterset (int a)
{
  switch (a)
    {
    case 0:
      I20CONSET = 0x40;
      return 0;
      break;
    case 1:
      I21CONSET = 0x40;
      return 0;
      break;
    case 2:
      I22CONSET = 0x40;
      return 0;
      break;
    default:
      return 1;
    }
}

/****************************************************************************
function name	:i2cstart
discribe	:the i2c will start
****************************************************************************/
int
i2cstart (int a)
{
  switch (a)
    {
    case 0:
      I20CONSET = 0x60;
      while (I20STAT != 0x08);
      return 0;
      break;
    case 1:
      I21CONSET = 0x60;
      while (I21STAT != 0x08);
      return 0;
      break;
    case 2:
      I22CONSET = 0x60;
      while (I22STAT != 0x08);
      return 0;
      break;
    default:
      return 1;
    }
}

/****************************************************************************
function name	:i2crestart
discribe	:the i2c will restart
****************************************************************************/
int
i2crestart (int a)
{
  switch (a)
    {
    case 0:
      I20CONSET = 0x14;
      I20CONCLR = 0x08;
      I20CONSET = 0x60;
      while ((I20STAT != 0x10) && (I20STAT != 0x08));
      return 0;
      break;
    case 1:
      I21CONSET = 0x14;
      I21CONCLR = 0x08;
      I21CONSET = 0x60;
      while ((I21STAT != 0x10) && (I21STAT != 0x08));
      return 0;
      break;
    case 2:
      I22CONSET = 0x14;
      I22CONCLR = 0x08;
      I22CONSET = 0x60;
      while ((I22STAT != 0x10) && (I22STAT != 0x08));
      return 0;
      break;
    default:
      return 1;
    }
}

/****************************************************************************
function name	:i2stop
discribe	:the i2c will stop
****************************************************************************/
void
i2cstop (int a)
{
      I22CONSET = 0x50;
      I22CONCLR = 0x2C;
}


void
i2csender (int Continue, unsigned int Data, int size)
{
  if (Continue == 0)
    {
      I22DAT = Data;
      I22CONSET |= 0x04;
      I22CONCLR = 0x08;
      while (I22STAT != 0x18 && I22STAT != 0x20)
      if (I22STAT == 0x20)
	{
	  i2cErr = 0x22;
	}
    }
  else
    {
      for (; size != 0; size -= 8)
	{
	  I22DAT = (Data & 0xFF);
	  I22CONCLR = 0x28;
	  while (I22STAT != 0x28);
	  Data = Data >> 8;
	}
    }
}

int
SCCBwrite (int subadr, int size, int Data)
{
  i2cstart (2);
//      vTaskDelay(1 / portTICK_RATE_MS);
  FIO2CLR1=2; 
  i2cErr = 0;
  i2csender (0, 0x42, 8);
  if (i2cErr == 0x22)
    {
      i2cstop (2);
      return -1;
    }
  i2csender (1, subadr, 8);	//3byte? 
  i2csender (1, Data, size);
  i2cstop (2);
  return 0;
}

int
SCCBByteWrite (int inp)
{
	int ret;
      FIO2PIN1 =0;
    Delay(100);
	ret = SCCBwrite ((inp >> 8 & 0xFF), 8, (inp & 0xFF)) ;
      if(ret==-1)FIO2PIN1 =2;
	return ret;
}
void
vi2c (void)
{
  int regpointer = 0;
  char Loopy;
  i2cmasterset (2);
  I22SCLH = 80;
  I22SCLL = 180;
  for (regpointer = 0; regpointer != 129;)
    {
    Delay(100);
      if (SCCBByteWrite (OV7670[regpointer]) == -1) continue;
      regpointer++;
    }
}
