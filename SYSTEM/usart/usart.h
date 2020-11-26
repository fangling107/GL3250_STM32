#ifndef __USART_H
#define __USART_H

#include "stdio.h"	
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//Mini STM32������
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.csom
//�޸�����:2011/6/14
//�汾��V1.4
//��Ȩ���У�����ؾ���
//Copyright(C) ����ԭ�� 2009-2019
//All rights reserved
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_REC_LEN,���ڶ��崮�����������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
////////////////////////////////////////////////////////////////////////////////// 	

//����봮���жϽ��գ��벻Ҫע�����º궨��


void uart_init(void);
void USART_Com(void);
void ZD_Port(void);
void DataProcess(void);
void Uart1_SendDataMsg(u8 PLC_Add,u8 Flag,float  Data_Flow);
void Uart5_SendDataMsg(u8 PLC_Add);
void GPIO_Configuration(void);

#if	Azbil_Sensor_Use
u8 Azbil_Sensor_485connect(u8 Azbil_Addr,u8 ReadOrWR,u8 OnOroff);
#endif

#endif


