#include "ledbeep.h"

//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������
//LED��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/2
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	   


void LED_BEEP_Init(void)
{
 
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //ʹ��PB�˿�ʱ��
	
//  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;				 //LED0-->PB.9�˿�����
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
//  GPIO_Init(GPIOB, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOB.5
//  GPIO_SetBits(GPIOB,GPIO_Pin_9);						 //PB.9 �����

 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;	    //BEEP-->PB.8 �˿�����, �������
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
 GPIO_Init(GPIOB, &GPIO_InitStructure);	  			//������� ��IO���ٶ�Ϊ50MHz
 GPIO_ResetBits(GPIOB,GPIO_Pin_8); 						 //PB.8 ����� 
}
 
