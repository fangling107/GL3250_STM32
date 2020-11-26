#ifndef __crc16_H__
#define __crc16_H__
#include "sys.h"
#include "stm32f10x.h"
extern u8 CRCHigh ;	
extern u8	CRCLow  ;
void Cal_CRC(unsigned char *Data, unsigned char Len);

#if	Azbil_Sensor_Use
void  Azbil_sensor_sum (unsigned char *Data, unsigned char Len);
extern u8 Sum_High;
extern u8	Sum_Low;
#endif

#endif















