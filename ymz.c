#include"lpc23xx.h"
 /*
 * =====================================================================================
 *
 *       Filename:  ymz.c
 *
 *    Description:  SSGsound chip driver
 *
 *        Version:  1.0
 *        Created:  2011年09月28日 19時13分30秒
 *       Revision:  none
 *       Compiler:  arm-none-eabi-gcc
 *
 *         Author:  Iori-yja (), 
 *        Company:  
 *
 * =====================================================================================
 */

int
ymzwrite0 (int frec, int adr)
{
  int highbyte;
  FIO4PIN = 0x00;
  FIO4PIN = adr;
  FIO4PIN = 0x200 + adr;
  FIO4PIN = 0x100 + (frec & 0xFF);
  FIO4PIN = 0x300 + (frec & 0xFF);
  FIO4PIN = adr + 1;
  FIO4PIN = 0x201 + adr;
  highbyte = (frec >> 8) & 0xFF;
  FIO4PIN = 0x100 + highbyte;
  FIO4PIN = 0x300 + (frec >> 8);
}

int
ymzwrite1 (int value, int adr)
{
  FIO4PIN = 0x00;
  FIO4PIN = adr;
  FIO4PIN = (0x200 + adr);
  FIO4PIN = (0x100 + value);
  FIO4PIN = (0x300 + value);
  return adr * 16 + value;
}
