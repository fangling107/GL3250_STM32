#include "usart.h"
#include "delay.h"
#include "crc16.h"
#include "24cxx.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��os,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os ʹ��	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/8/18
//�汾��V1.5
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
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
//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��
////////////////////////////////////////////////////////////////////////////////// 	  
struct USARTDataStream   U1DS,U2DS,U5DS;   //;Uart2���������Ŀ��Ʋ���
struct Buf_Type AllBuf;      //;������
struct sensors Sensors;				
struct run_setting Run_Setting;
struct  disinfect Disinfect;

union floatto_hex FtoHex;
u16 AQMD3610_FLOW=0;
//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART2->SR&0X40)==0);//ѭ������,ֱ���������
		//delay_xms(4);
    USART2->DR = (u8) ch;      
	return ch;
}
#endif 

/*ʹ��microLib�ķ���*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/

/*----------------------------------------------------------------------------*
 |  ����1��ʼ��������
 *----------------------------------------------------------------------------*/
void SendFirstByte_USART1(void)
{
    U1DS.TBusy = 20;
    USART_SendData(USART1, (u16)*(U1DS.BufAdr + 0));
    U1DS.TIndex = 1;
    //ֻҪ���ͼĴ���Ϊ�գ��ͻ�һֱ���жϣ���ˣ�Ҫ�ǲ���������ʱ���ѷ����жϹرգ�ֻ�ڿ�ʼ����ʱ���Ŵ򿪡� 
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE); // //����TXE�жϣ�����ʼ���͹���
}

/*----------------------------------------------------------------------------*
 |  ����1�жϷ�������
 *----------------------------------------------------------------------------*/
void SendByte_USART1(void)
{
    if((USART1->SR & USART_FLAG_TXE))
    {
        U1DS.TBusy = 20;
        if(U1DS.TIndex >= U1DS.TLong)
        {
            U1DS.TLong = 0;
            U1DS.TBusy = 10;
            U1DS.SedOrd = 0; //;���������֡���
            U1DS.BufAdr = 0;
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);           
            return ;
        }
        USART_SendData(USART1, (u16)*(U1DS.BufAdr + U1DS.TIndex));
        U1DS.TIndex++;
    }
}


/*----------------------------------------------------------------------------*
 |  ����1�жϽ�������
 *----------------------------------------------------------------------------*/
void ReceiveByte_USART1(void)
{
    int temp;
    u8 ch;

    if(USART1->SR & USART_FLAG_RXNE)
    {
        U1DS.RecRun = true;
        U1DS.RBet2Byte = 0x00;
        temp = USART_ReceiveData(USART1);
        ch = (u8)temp;
        AllBuf.Rx1_1[U1DS.RIndex] = ch;
        U1DS.RIndex++;
        if(U1DS.RIndex >= U1BUFSIZE)
        {
            U1DS.RIndex = 0;
        }
    }
}


/*----------------------------------------------------------------------------*
 |  ����1��������
 *----------------------------------------------------------------------------*/
void USART1_Rece(void)
{
    if(U1DS.RBet2Byte >= U1FRAMEDIST)     //;����1�������һ���ֽ��Ѿ�50ms,�Խ��յ������ݽ���Ӧ��
    {
        U1DS.RecRun = false;
        U1DS.RBet2Byte = 0x00;

        if(U1DS.RIndex > U1BUFSIZE)
        {
            U1DS.RIndex = 0;
            return;
        }
        
        U1DS.RecLen = U1DS.RIndex;
        U1DS.RIndex = 0;
        U1DS.RecOrd = true; 
        U1DS.BufAdr = AllBuf.Rx1_1; 
    }
}


/*----------------------------------------------------------------------------*
 |  ����1��������
 *----------------------------------------------------------------------------*/
void USART1_Send(void)
{
    if(U1DS.TLong > U1BUFSIZE)
    {
        return; 
    }    
    
    if(U1DS.TBusy == 0)
    {
        if(U1DS.SedOrd == 'E') //;��������
        {
            U1DS.SedOrd = 'S'; //;���ڷ���
            SendFirstByte_USART1();
        }
    }
}



/*----------------------------------------------------------------------------*
 |  ����2��ʼ��������
 *----------------------------------------------------------------------------*/
void SendFirstByte_USART2(void)
{
    //U2DS.TBusy = 10;
    USART_SendData(USART2, (u16)*(U2DS.BufAdr + 0));
    U2DS.TIndex = 1;  
    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);   
}


/*----------------------------------------------------------------------------*
 |  ����2�жϷ�������
 *----------------------------------------------------------------------------*/
void SendByte_USART2(void)
{  
		
    if((USART2->SR & USART_FLAG_TXE))
    {
        //U2DS.TBusy =10;
        if(U2DS.TIndex >= U2DS.TLong)
        {
            U2DS.TLong = 0;
            U2DS.SedOrd = 0; //;���������֡���
            U2DS.TBusy =10;
            U2DS.BufAdr = 0;
            USART_ITConfig(USART2, USART_IT_TXE, DISABLE);  
						
            return ;
        }
        USART_SendData(USART2, (u16)*(U2DS.BufAdr + U2DS.TIndex));
        U2DS.TIndex++;
    }
}
/*----------------------------------------------------------------------------*
 |  ����2�жϽ�������
 *----------------------------------------------------------------------------*/
void ReceiveByte_USART2(void)
{
    int temp;
    u8 ch;
    
    if(USART2->SR & USART_FLAG_RXNE)
    {
        U2DS.RecRun = true;
        U2DS.RBet2Byte = 0x00;
        temp = USART_ReceiveData(USART2);
        ch = (u8)temp;
        AllBuf.Rx2_1[U2DS.RIndex] = ch;
        U2DS.RIndex++;
        if(U2DS.RIndex >= U2BUFSIZE)
        {
            U2DS.RIndex = 0; 
        }
    }
}



/*----------------------------------------------------------------------------*
 | ����2��������
 *----------------------------------------------------------------------------*/
void USART2_Rece(void)
{  	
    if(U2DS.RBet2Byte >= U2FRAMEDIST) //;����2�������һ���ֽ��Ѿ�20ms,�Խ��յ������ݽ���Ӧ��
    {
        U2DS.RecRun = false;
        U2DS.RBet2Byte = 0x00;

        if(U2DS.RIndex > U2BUFSIZE)
        {
            U2DS.RIndex = 0;
            return;
        }
        
        U2DS.RecLen = U2DS.RIndex;
        U2DS.RIndex = 0;
        U2DS.RecOrd = true;         
        U2DS.BufAdr = AllBuf.Rx2_1; 
    }
}


/*----------------------------------------------------------------------------*
 |  ����2��������
 *----------------------------------------------------------------------------*/
void USART2_Send(void)
{
	
    if(U2DS.TLong > U2BUFSIZE)
    {
        return;
    }    
    
    if(U2DS.TBusy == 0)
    {
        if(U2DS.SedOrd == 'E') //;��������
        { 
            U2DS.SedOrd = 'S'; //;���ڷ���
					
            SendFirstByte_USART2();
        }
    }
}
/*----------------------------------------------------------------------------*
 |  ����5��ʼ��������
 *----------------------------------------------------------------------------*/
void SendFirstByte_UART5(void)
{
    U5DS.TBusy = 20;
    USART_SendData(UART5, (u16)*(U5DS.BufAdr + 0));
    U5DS.TIndex = 1;
    USART_ITConfig(UART5, USART_IT_TXE, ENABLE); 
}


/*----------------------------------------------------------------------------*
 |  ����5�жϷ�������
 *----------------------------------------------------------------------------*/
void SendByte_UART5(void)
{
    if ((UART5->SR &USART_FLAG_TXE))
    {
        U5DS.TBusy = 20;
        if (U5DS.TIndex >= U5DS.TLong)
        {
            U5DS.TLong = 0;
            U5DS.SedOrd = 0; //;���������֡���
						U5DS.TBusy = 10;
            U5DS.BufAdr = 0;
            USART_ITConfig(UART5, USART_IT_TXE, DISABLE);
            return ;
        }
        USART_SendData(UART5, (u16)*(U5DS.BufAdr + U5DS.TIndex));
        U5DS.TIndex++;
    }
}


/*----------------------------------------------------------------------------*
 |  ����5�жϽ�������
 *----------------------------------------------------------------------------*/
void ReceiveByte_UART5(void)
{
    int temp;
    u8 ch;
    
    if(UART5->SR & USART_FLAG_RXNE)
    {
        U5DS.RecRun = true;  //;�Ƿ��ڽ���
        U5DS.RBet2Byte = 0x00;
        temp = USART_ReceiveData(UART5);
        ch = (u8)temp;
        AllBuf.Rx5_1[U5DS.RIndex] = ch;
        U5DS.RIndex++;
        if(U5DS.RIndex >= U5BUFSIZE)
        {
            U5DS.RIndex = 0; 
        }
    }
}

/*----------------------------------------------------------------------------*
 |  ����5��������
 *----------------------------------------------------------------------------*/
void UART5_Rece(void)
{
    if(U5DS.RBet2Byte >=U5FRAMEDIST)        //;����2�������һ���ֽ��Ѿ�50ms,�Խ��յ������ݽ���Ӧ��
    {
        U5DS.RecRun = false;
        U5DS.RBet2Byte = 0x00;

        if(U5DS.RIndex > U5BUFSIZE)
        {
            U5DS.RIndex = 0;
            return;
        }
        
        U5DS.RecLen = U5DS.RIndex;
        U5DS.RIndex = 0;
        U5DS.RecOrd = true;         
        U5DS.BufAdr = AllBuf.Rx5_1;         
    }
}


/*----------------------------------------------------------------------------*
 |  ����5��������
 *----------------------------------------------------------------------------*/
void UART5_Send(void)
{
    if(U5DS.TLong > U5BUFSIZE)
    {
        return;
    }    
    
    if(U5DS.TBusy == 0)
    {
        if(U5DS.SedOrd == 'E') //;��������
        {
            U5DS.SedOrd = 'S'; //;���ڷ���
            SendFirstByte_UART5();
        }
    }
}


void USART_Com(void)
{  
    USART1_Rece();
    USART1_Send();    
    USART2_Rece();
    USART2_Send();
    UART5_Rece();
    UART5_Send();    
}
void GPIO_Configuration(void)
{	
  GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE, ENABLE);
	//PB6 ����pwm����
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_6;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	GPIO_SetBits(GPIOB,GPIO_Pin_6);
	//PD13 PD14 PD15 ����24V���
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOD, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOD,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
	//PE1 PE2 PE3 PE4 PE6 PE8 PE9 PE10 PE11 ����24V���
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_6|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOE, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOE,GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_6|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11);
	//PD0 PD1 PD3 PD4 PD5 PD6 PD7 PD8 PD9 PD10 PD11 PD12	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //���ó����룬Ĭ������	  
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOD,GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12);

}

//��ʼ��
//USART1:J2 485 	2������������ 1������ÿ�����
//USART2:P1 232 	���Ӵ�����
//USART5:J1 485		2��ѹ����� 1�������� 3��ѹ��������
void uart_init(void)
{
    //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* ��������1ʱ��*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 , ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2|RCC_APB1Periph_UART5, ENABLE);	//ʹ��ʱ��	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC |RCC_APB2Periph_GPIOD, ENABLE);	//ʹ��GPIOAʱ��
 	/*USART1 PA9  PA10*/
	USART_DeInit(USART1);  //��λ����1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //;��������ģʽ
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //;���ù����������ģʽ
	GPIO_Init(GPIOA, &GPIO_InitStructure); 
	//Usart1 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
	USART_InitStructure.USART_BaudRate = 9600; //;������
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity =USART_Parity_Even ;//żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART1, &USART_InitStructure);
	/*����USART1�жϽ���*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	/*����USART1����*/
	USART_Cmd(USART1, ENABLE);
	
	//USART2_TX   PA.2 //USART2_RX	  PA.3
	USART_DeInit(USART2);  //��λ����2	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA2	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��PA3
   //Usart2 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
   //USART2 ��ʼ������
	USART_InitStructure.USART_BaudRate = 19200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

	USART_Init(USART2, &USART_InitStructure); //��ʼ������
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//�����ж�
	USART_Cmd(USART2, ENABLE);                    //ʹ�ܴ��� 
	
	
	/*USART5*/
	USART_DeInit(UART5);  //��λ����5	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	//Usart5 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART5, &USART_InitStructure);
	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
	USART_Cmd(UART5, ENABLE);

}

void USART1_IRQHandler(void)
{
		#ifdef SYSTEM_SUPPORT_OS	 	
		OSIntEnter();    
		#endif
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        ReceiveByte_USART1();
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);   //;����յ��жϱ�־
    }
    if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
    {
        SendByte_USART1();
        USART_ClearITPendingBit(USART1, USART_IT_TXE);   //;�巢�͵��жϱ�־
    }
		#ifdef SYSTEM_SUPPORT_OS	 
		OSIntExit();  											 
		#endif
}

void USART2_IRQHandler(void)                	//����1�жϷ������
{
	#ifdef SYSTEM_SUPPORT_OS	 	
		OSIntEnter();    
	#endif
		if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
		{
				ReceiveByte_USART2();
				USART_ClearITPendingBit(USART2, USART_IT_RXNE);   //;����յ��жϱ�־
		}
		if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
		{
				SendByte_USART2();
				USART_ClearITPendingBit(USART2, USART_IT_TXE);   //;�巢�͵��жϱ�־
		}
	#ifdef SYSTEM_SUPPORT_OS	 
		OSIntExit();  											 
	#endif
} 

void UART5_IRQHandler(void)
{
		#ifdef SYSTEM_SUPPORT_OS	 	
		OSIntEnter();    
		#endif
    if (USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)
    {
        ReceiveByte_UART5();
        USART_ClearITPendingBit(UART5, USART_IT_RXNE);
    }
    if (USART_GetITStatus(UART5, USART_IT_TXE) != RESET)
    {
        SendByte_UART5();
        USART_ClearITPendingBit(UART5, USART_IT_TXE);
    }
		#ifdef SYSTEM_SUPPORT_OS	 
		OSIntExit();  											 
		#endif
}

void ZD_Port(void)  
{
		u8 i=0;
		u8 len=0;	
		//***********************����1��ͨѶ**********************//
    if(U1DS.RecOrd == true) //;����1���յ�����
    {
        U1DS.RecOrd = false;
				for(i=0;i<U1DS.RecLen;i++)			
					AllBuf.Rx1_2[i]=*(U1DS.BufAdr+i);
			#if	Azbil_Sensor_Use
				switch(AllBuf.Rx1_2[0])//�ж�������ַ
				{
					case 0x02:
					if(AllBuf.Rx1_2[2]==0x38||AllBuf.Rx1_2[2]==0x39) 	//��·����������
					{
						if(AllBuf.Rx1_2[6]==0x30&&AllBuf.Rx1_2[7]==0x30)
							U1DS.RecStatus='R';//���յ�����
					}
							break;
					case Aerosol_AQMD3610://���������������	
							len=6;				
 							Cal_CRC(AllBuf.Rx1_2,len);	// ���� CRC16 У��
 							if((CRCHigh == AllBuf.Rx1_2[len])&&(CRCLow == AllBuf.Rx1_2[len+1]))	// CRCУ����ȷ,����������Ч							
								U1DS.RecStatus='R';//���յ�����	
							break;
					default:break;
				}
				
			#else
				switch(AllBuf.Rx1_2[0])//�ж�������ַ
				{											
					case Left_FlowCont_ADDR:	//��·����������
					case Right_FlowCont_ADDR: //��·����������	
							if(AllBuf.Rx1_2[1]==0x03) len=7;
							else											len=6;				
 							Cal_CRC(AllBuf.Rx1_2,len);	// ���� CRC16 У��
 							if((CRCHigh == AllBuf.Rx1_2[len])&&(CRCLow == AllBuf.Rx1_2[len+1]))	// CRCУ����ȷ,����������Ч							
								U1DS.RecStatus='R';//���յ�����												
							break;
					case Aerosol_AQMD3610://���������������
							len=6;				
 							Cal_CRC(AllBuf.Rx1_2,len);	// ���� CRC16 У��
 							if((CRCHigh == AllBuf.Rx1_2[len])&&(CRCLow == AllBuf.Rx1_2[len+1]))	// CRCУ����ȷ,����������Ч							
								U1DS.RecStatus='R';//���յ�����												
							break;			
					default:break;
				}
			#endif	
    }
		//***********************����2��ͨѶ**********************//
    if(U2DS.RecOrd == true) //;����2���յ�����
    {
        U2DS.RecOrd = false;
				for(i=0;i<U2DS.RecLen;i++)			
					AllBuf.Rx2_2[i]=*(U2DS.BufAdr+i);
				switch(AllBuf.Rx2_2[1])//�жϵ�ַ
				{											
					case 0x03://��11���ֽڵ�����
					case 0x05://����	
							len=6;
 							Cal_CRC(AllBuf.Rx2_2,len);	// ���� CRC16 У��
 							if((CRCHigh == AllBuf.Rx2_2[len])&&(CRCLow == AllBuf.Rx2_2[len+1]))	// CRCУ����ȷ,����������Ч							
								U2DS.RecStatus='R';//���յ�����												
							break;
					case 0x10:
							len=17;//ɱ��������������ò�������
							Cal_CRC(AllBuf.Rx2_2,len);	// ���� CRC16 У��
 							if((CRCHigh == AllBuf.Rx2_2[len])&&(CRCLow == AllBuf.Rx2_2[len+1]))	// CRCУ����ȷ,����������Ч							
								U2DS.RecStatus='R';//���յ�����	
							break;
					default:break;
				}
    }
				//***********************����5��ͨѶ**********************//
    if(U5DS.RecOrd == true) //;����5���յ�����
    {
        U5DS.RecOrd = false;
				for(i=0;i<U5DS.RecLen;i++)			
					AllBuf.Rx5_2[i]=*(U5DS.BufAdr+i);
				switch(AllBuf.Rx5_2[0])//�ж�������ַ
				{											
					case Box_Press_ADDR://���帺ѹ
					case Aerosol_Press_ADDR://�����Ҹ�ѹ
					case Atomizer_Press_ADDR://��������ѹ��
					case Left_Press_ADDR://��·��ѹ��	
					case Right_Press_ADDR://��·��ѹ��	
							len=5;					
 							Cal_CRC(AllBuf.Rx5_2,len);	// ���� CRC16 У��
 							if((CRCHigh == AllBuf.Rx5_2[len])&&(CRCLow == AllBuf.Rx5_2[len+1]))	// CRCУ����ȷ,����������Ч							
								U5DS.RecStatus='R';//���յ�����												
							break;
					case Aerosol_Flow_ADDR://������
							len=7;					
 							Cal_CRC(AllBuf.Rx5_2,len);	// ���� CRC16 У��
 							if((CRCHigh == AllBuf.Rx5_2[len])&&(CRCLow == AllBuf.Rx5_2[len+1]))	// CRCУ����ȷ,����������Ч							
								U5DS.RecStatus='R';//���յ�����		
							break;
					default:break;
				}
    }
}
/*----------------------------------------------------------------------------*
**�Դ����յ������ݽ��д���
 *----------------------------------------------------------------------------*/
void DataProcess(void)
{		
		u8 m=0;	
		u16 Box_Press=0;		
		u32 Aerosol_Flow=0;
		u16	Period=0;
		float Gas_Flow=0;
		u8 data[4]={0};
		s16		Press_Data=0;
	//2������������������1������õĿ�����
	if(U1DS.RecStatus=='R')
		{	
			U1DS.RecStatus=0;
			#if	Azbil_Sensor_Use
			switch(AllBuf.Rx1_2[0])	// �жϵ�ַ��
			{
				case 0x02:				
				if(AllBuf.Rx1_2[2]==0x38)
				{
					if(AllBuf.Rx1_2[5]==0x58)//����·����
					{
					if(AllBuf.Rx1_2[8]>0x39) 	data[0]=AllBuf.Rx1_2[8]-0x37;
					else										 	data[0]=AllBuf.Rx1_2[8]-0x30;
					if(AllBuf.Rx1_2[9]>0x39) 	data[1]=AllBuf.Rx1_2[9]-0x37;
					else										 	data[1]=AllBuf.Rx1_2[9]-0x30;
					if(AllBuf.Rx1_2[10]>0x39) data[2]=AllBuf.Rx1_2[10]-0x37;
					else										 	data[2]=AllBuf.Rx1_2[10]-0x30;
					if(AllBuf.Rx1_2[11]>0x39) data[3]=AllBuf.Rx1_2[11]-0x37;
					else										 	data[3]=AllBuf.Rx1_2[11]-0x30;
					Sensors.Left_Flow=data[3]+(data[2]<<4)+(data[1]<<8)+(data[0]<<12);
					U1DS.RecData_Process=4;	//��·������ȡ��ɱ�־
					}
				else
					U1DS.RecData_Process=1;	//��·����������ɱ�־
			}
			else if(AllBuf.Rx1_2[2]==0x39)
			{
				if(AllBuf.Rx1_2[5]==0x58)//����·����
					{
					if(AllBuf.Rx1_2[8]>0x39) 	data[0]=AllBuf.Rx1_2[8]-0x37;
					else										 	data[0]=AllBuf.Rx1_2[8]-0x30;
					if(AllBuf.Rx1_2[9]>0x39) 	data[1]=AllBuf.Rx1_2[9]-0x37;
					else										 	data[1]=AllBuf.Rx1_2[9]-0x30;
					if(AllBuf.Rx1_2[10]>0x39) data[2]=AllBuf.Rx1_2[10]-0x37;
					else										 	data[2]=AllBuf.Rx1_2[10]-0x30;
					if(AllBuf.Rx1_2[11]>0x39) data[3]=AllBuf.Rx1_2[11]-0x37;
					else										 	data[3]=AllBuf.Rx1_2[11]-0x30;
					Sensors.Right_Flow=data[3]+(data[2]<<4)+(data[1]<<8)+(data[0]<<12);
					U1DS.RecData_Process=5;	//��·������ȡ��ɱ�־
					}
					else
						U1DS.RecData_Process=2;	//��·����������ɱ�־
				}
					break ;
				case Aerosol_AQMD3610://������������
						U1DS.RecData_Process=3;	//��������������ɱ�־	
					break ;
				
			}
			#else
			switch(AllBuf.Rx1_2[0])	// �жϵ�ַ��
			{
				case Left_FlowCont_ADDR:
					if(AllBuf.Rx1_2[1]==0x03)//����·����
					{
						Gas_Flow=(AllBuf.Rx1_2[5]<<24)+(AllBuf.Rx1_2[6]<<16)+(AllBuf.Rx1_2[3]<<8)+(AllBuf.Rx1_2[4]);						
						Sensors.Left_Flow=Gas_Flow*10;
						U1DS.RecData_Process=4;	//��·������ȡ��ɱ�־
					}
					else
						U1DS.RecData_Process=1;	//��·����������ɱ�־
					break ;
				case Right_FlowCont_ADDR:
					if(AllBuf.Rx1_2[1]==0x03)//����·����
					{					
						Gas_Flow=(AllBuf.Rx1_2[5]<<24)+(AllBuf.Rx1_2[6]<<16)+(AllBuf.Rx1_2[3]<<8)+(AllBuf.Rx1_2[4]);
						Sensors.Right_Flow=Gas_Flow*10;
						U1DS.RecData_Process=5;	//��·������ȡ��ɱ�־
					}
					else
						U1DS.RecData_Process=2;	//��·����������ɱ�־
					break ;
				case Aerosol_AQMD3610://������������
						U1DS.RecData_Process=3;	//��������������ɱ�־
					break ;
			}
			#endif
		}
	
	//***********************�Դ���2�յ������ݽ��д���**********************//
		if(U2DS.RecStatus=='R')
		{	
			U2DS.RecStatus=0;
			
			switch(AllBuf.Rx2_2[1])	// �жϵ�ַ��
			{				
				case 0x03:					
					if(AllBuf.Rx2_2[3]==0x0e)//��������,�������в�����ɱ������
					{
						AllBuf.Tx2_1[m++]=WEINVIEW_ADDR;
						AllBuf.Tx2_1[m++]=0x03;	
						AllBuf.Tx2_1[m++]=0x16;//11����22���ֽ�
						AllBuf.Tx2_1[m++]=(Run_Setting.Peristaltic_Flow>>8);		
						AllBuf.Tx2_1[m++]=Run_Setting.Peristaltic_Flow;
						
						AllBuf.Tx2_1[m++]=(Run_Setting.Aerosol_Flow>>8);	
						AllBuf.Tx2_1[m++]=(Run_Setting.Aerosol_Flow);
						
						AllBuf.Tx2_1[m++]=(Run_Setting.Sampling_Time>>8);	
						AllBuf.Tx2_1[m++]=(Run_Setting.Sampling_Time);
						
						AllBuf.Tx2_1[m++]=(Run_Setting.Liquid_Time>>8);	
						AllBuf.Tx2_1[m++]=(Run_Setting.Liquid_Time);
						
						AllBuf.Tx2_1[m++]=(Run_Setting.Clear_Time>>8);	
						AllBuf.Tx2_1[m++]=(Run_Setting.Clear_Time);
						
						AllBuf.Tx2_1[m++]=0;	
						AllBuf.Tx2_1[m++]=0;
						
						AllBuf.Tx2_1[m++]=(Disinfect.Peristaltic_Flow>>8);	
						AllBuf.Tx2_1[m++]=(Disinfect.Peristaltic_Flow);
						
						AllBuf.Tx2_1[m++]=(Disinfect.Aerosol_Flow>>8);	
						AllBuf.Tx2_1[m++]=(Disinfect.Aerosol_Flow);
						
						AllBuf.Tx2_1[m++]=(Disinfect.Disclear_Time>>8);	
						AllBuf.Tx2_1[m++]=(Disinfect.Disclear_Time);
						
						AllBuf.Tx2_1[m++]=(Disinfect.Waterclear_Time>>8);	
						AllBuf.Tx2_1[m++]=(Disinfect.Waterclear_Time);
						
						AllBuf.Tx2_1[m++]=(Disinfect.Airclear_Time>>8);	
						AllBuf.Tx2_1[m++]=(Disinfect.Airclear_Time);					 

					}
					else if(AllBuf.Rx2_2[3]==0x00)//����������
					{
						
						AllBuf.Tx2_1[m++]=WEINVIEW_ADDR;
						AllBuf.Tx2_1[m++]=0x03;	
						AllBuf.Tx2_1[m++]=0x16;//11����22���ֽ�
						AllBuf.Tx2_1[m++]=(Sensors.Box_Press>>8);	//����ѹ��	
						AllBuf.Tx2_1[m++]=Sensors.Box_Press;
						
						AllBuf.Tx2_1[m++]=(Sensors.Aerosol_Press>>8);	//������ѹ��
						AllBuf.Tx2_1[m++]=(Sensors.Aerosol_Press);
						
						AllBuf.Tx2_1[m++]=(Sensors.Atomizer_Press>>8);	//����ѹ��
						AllBuf.Tx2_1[m++]=(Sensors.Atomizer_Press);						
						
						AllBuf.Tx2_1[m++]=(Sensors.Aerosol_Flow>>8);//����������	
						AllBuf.Tx2_1[m++]=(Sensors.Aerosol_Flow);
						
						AllBuf.Tx2_1[m++]=(Sensors.Peristaltic_Flow>>8);//�䶯������	
						AllBuf.Tx2_1[m++]=(Sensors.Peristaltic_Flow);
						
						AllBuf.Tx2_1[m++]=(Sensors.Left_Press>>8);//��·ѹ��
						AllBuf.Tx2_1[m++]=Sensors.Left_Press;
						
						AllBuf.Tx2_1[m++]=(Sensors.Left_Flow>>8);	//��·����
						AllBuf.Tx2_1[m++]=(Sensors.Left_Flow);
						
						AllBuf.Tx2_1[m++]=(Sensors.Right_Press>>8);	//��·ѹ��
						AllBuf.Tx2_1[m++]=(Sensors.Right_Press);
						
						AllBuf.Tx2_1[m++]=(Sensors.Right_Flow>>8);//��·����	
						AllBuf.Tx2_1[m++]=(Sensors.Right_Flow);
						
						
						AllBuf.Tx2_1[m++]=(Sensors.Pre_Set_Time>>8);	//Ԥ��ʱ��
						AllBuf.Tx2_1[m++]=(Sensors.Pre_Set_Time);
						
					
						AllBuf.Tx2_1[m++]=(Sensors.Status>>8);//ϵͳ״̬	
						AllBuf.Tx2_1[m++]=(Sensors.Status);
					}
						Cal_CRC(AllBuf.Tx2_1,m);	// ���� CRC16
						AllBuf.Tx2_1[m++]=CRCHigh;//crc
						AllBuf.Tx2_1[m++]=CRCLow;		
						 
						U2DS.TLong=m;    
						U2DS.SedOrd = 'E';  
						U2DS.BufAdr = AllBuf.Tx2_1;
						break;
				case 0x05:					
					if(AllBuf.Rx2_2[3]==0x00)//����						
					{
							if(AllBuf.Rx2_2[4]==0xFF)//��ʼ����
							{
									Start_Off_Flag=1;	
									Sensors.Status&=0x0;//�رն�ʱ��
									Sensors.Status|=0x0002;//��ʾ���ڽ�����·ƽ��
									Sensors.Pre_Set_Time=Balence_Time;//��·ƽ��ʱ��Ԥ��ʱ��
									Sec_count=0;//���ʱ����
									U1DS.RecData_Process=0;
									U5DS.RecData_Process=0;
									Out5=0;//��·ƽ���ŷ���
									Out7=0;//���������
									Out8=0;//���������
									Out1=0;//�͵�ƽ��ͨ�������ķ��
									AQMD3610_FLOW=Run_Setting.Aerosol_Flow*8;
									Sensors.Aerosol_Flow=Run_Setting.Aerosol_Flow;
							}
							else//ֹͣ����
							{
									Start_Off_Flag=0;
									Sensors.Status&=0;
									Sensors.Status|=0x0001;//�������״̬
									Sensors.Pre_Set_Time=0;//Ԥ��ʱ������
									Sensors.Left_Flow=0;
									Sensors.Right_Flow=0;
									Sensors.Aerosol_Flow=0;
							}
					}
					else if(AllBuf.Rx2_2[3]==0x01)//���		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//�����
							{
								Out1=0;//�͵�ƽ��ͨ
							}
							else//�ط��
							{
								Out1=1;//�ߵ�ƽ�ر�
							}

					}	
					else if(AllBuf.Rx2_2[3]==0x02)//�����		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//�������
							{
								Out2=0;//�͵�ƽ��ͨ
							}
							else//�������
							{
								Out2=1;
							}

					}
					else if(AllBuf.Rx2_2[3]==0x03)//����		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//������
							{
								Out3=0;	
							}
							else//������
							{
								Out3=1;
							}

					}
					//�ֶ���ͣ�䶯��
					else if(AllBuf.Rx2_2[3]==0x04)//�䶯��		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//���䶯��
							{								
								if(Start_Off_Flag==1)//����״̬
								{
									if((Sensors.Status&0x0008)==0x0008)	//��������״̬	
									{
										if(Run_Setting.Peristaltic_Flow<=1000)
										{
											Period=40000/Run_Setting.Peristaltic_Flow;
											TIM_SetAutoreload(TIM3,Period-1);//1ML/MIN(25KHZ)												
										}
										else
										{
											Period=40;
											TIM_SetAutoreload(TIM3,Period-1);//25khz											
										}
										Sensors.Peristaltic_Flow=	Run_Setting.Peristaltic_Flow;
									}
									else//ϵͳɱ��
									{
										if(Disinfect.Peristaltic_Flow<=1000)
										{
											Period=40000/Disinfect.Peristaltic_Flow;
											TIM_SetAutoreload(TIM3,Period-1);//1ML/MIN(25KHZ)
										}
										else
										{
											Period=40;
											TIM_SetAutoreload(TIM3,Period-1);//40khz	
										}
										Sensors.Peristaltic_Flow=	Disinfect.Peristaltic_Flow;
									}								
								}
								else//�ֶ�״̬����Ϊ�������	
								{
										Period=40;
										TIM_SetAutoreload(TIM3,Period-1);//40khz
										Sensors.Peristaltic_Flow=1000;
								}
								TIM_SetCompare2(TIM3,Period/5);
								TIM_Cmd(TIM3, ENABLE);  //ʹ�ܶ�ʱ��TIM3	
							}
							else//���䶯��
							{
								TIM_Cmd(TIM3, DISABLE);  //ʧ�ܶ�ʱ��TIM3
								Sensors.Peristaltic_Flow=0;
							}
					}

					else if(AllBuf.Rx2_2[3]==0x06&&AllBuf.Rx2_2[4]==0xFF)//��ʱ����		
					{
						if(Start_Off_Flag==1&&((Sensors.Status&0x08)==0x08))	
						{
							Sensors.Status&=0x0;
							Sensors.Status|=0x0020;//2min������ɣ���ʼ���й�·��ϴ	
							Sensors.Pre_Set_Time=Run_Setting.Clear_Time;//Ԥ��ʱ��
							Sec_count=0;//���ʱ����
							Out5=0;//��·ƽ���ŷ���
							Out6=1;//������ŷ���
						}
						else if(Start_Off_Flag==1&&((Sensors.Status&0x20)==0x20))	//��ϴʱ�䵽
						{
							Sensors.Status&=0x0;
							Sensors.Status|=0x0040;//��ʾ�������
						}
						else if(Start_Off_Flag==1&&((Sensors.Status&0x02)==2))//��·ƽ�ⳬʱ
						{
							Sensors.Status&=0x0;//�رն�ʱ��
							Sensors.Status|=0x0080;//��ʾ��ʱ
							//��ʱ��Ҫ�ر�һЩ����ִ������
							
						}
						//ϵͳɱ��
						else if((Sensors.Status&0x0100)==0x0100||(Sensors.Status&0x0200)==0x0200||(Sensors.Status&0x0400)==0x0400)	//ɱ��ʱ�䵽
						{
							Sensors.Status&=0;
							Sensors.Status|=0x0001;//�������״̬
							Sensors.Pre_Set_Time=0;//Ԥ��ʱ������
							Start_Off_Flag=0;	
							AQMD3610_FLOW=0;
							Sensors.Left_Flow=0;
							Sensors.Right_Flow=0;
							Sensors.Aerosol_Flow=0;	
						}
					}
					else if(AllBuf.Rx2_2[3]==0x08)//����Һ��ϴ		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//��
							{
								Sensors.Status&=0x0;
								Sensors.Status|=0x0100;//����Һ��ϴ
								Sensors.Pre_Set_Time=Disinfect.Disclear_Time;//Ԥ��ʱ��
								Sec_count=0;//���ʱ����
								Start_Off_Flag=1;
								Out5=0;//��·ƽ���ŷ���
								Out7=0;//���������
								Out8=0;//���������
								AQMD3610_FLOW=Disinfect.Aerosol_Flow*8;
								Sensors.Aerosol_Flow=Disinfect.Aerosol_Flow;
							}
							else//��
							{
								Sensors.Status&=0;
								Sensors.Status|=0x0001;//�������״̬
								Sensors.Pre_Set_Time=0;//Ԥ��ʱ������
								Start_Off_Flag=0;
								Sensors.Left_Flow=0;
								Sensors.Right_Flow=0;	
								Sensors.Aerosol_Flow=0;
							}
					}
					else if(AllBuf.Rx2_2[3]==0x09)//����ˮ��ϴ		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//��
							{
								Sensors.Status&=0x0;
								Sensors.Status|=0x0200;//����Һ��ϴ
								Sensors.Pre_Set_Time=Disinfect.Waterclear_Time;//Ԥ��ʱ��
								Sec_count=0;//���ʱ����
								Start_Off_Flag=1;
								Out5=0;//��·ƽ���ŷ���
								Out7=0;//���������
								Out8=0;//���������
								AQMD3610_FLOW=Disinfect.Aerosol_Flow*8;
								Sensors.Aerosol_Flow=Disinfect.Aerosol_Flow;	
							}
							else//��
							{
								Sensors.Status&=0;
								Sensors.Status|=0x0001;//�������״̬
								Sensors.Pre_Set_Time=0;//Ԥ��ʱ������
								Start_Off_Flag=0;	
								Sensors.Left_Flow=0;
								Sensors.Right_Flow=0;	
								Sensors.Aerosol_Flow=0;
							}
					}
					else if(AllBuf.Rx2_2[3]==0x0A)//������ϴ		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//��
							{
								Sensors.Status&=0x0;
								Sensors.Status|=0x0400;//����Һ��ϴ
								Sensors.Pre_Set_Time=Disinfect.Airclear_Time;//Ԥ��ʱ��
								Sec_count=0;//���ʱ����
								Start_Off_Flag=1;
								Out5=0;//��·ƽ���ŷ���
								Out7=0;//���������	
								Out8=0;//���������
								AQMD3610_FLOW=Disinfect.Aerosol_Flow*8;
								Sensors.Aerosol_Flow=Disinfect.Aerosol_Flow;																
							}
							else//��
							{
								Sensors.Status&=0;
								Sensors.Status|=0x0001;//�������״̬
								Sensors.Pre_Set_Time=0;//Ԥ��ʱ������
								Start_Off_Flag=0;
								Sensors.Left_Flow=0;
								Sensors.Right_Flow=0;
								Sensors.Aerosol_Flow=0;	
							}
					}
					else if(AllBuf.Rx2_2[3]==0x0B&&AllBuf.Rx2_2[4]==0xFF)//������ʽ��ʼ���鰴ť		
					{
						Sensors.Status&=0x0;						
						Sensors.Status|=0x0008;//���ڽ�������	
						Sensors.Pre_Set_Time=Run_Setting.Sampling_Time;//Ԥ��ʱ��
						Sec_count=0;//���ʱ����						
						Out5=1;//��·ƽ���ŷ���
						Out6=0;//������ŷ���							
					}
					for(m=0;m<8;m++)	
						AllBuf.Tx2_1[m]=AllBuf.Rx2_2[m];
					U2DS.TLong=8;    
					U2DS.SedOrd = 'E';  
					U2DS.BufAdr = AllBuf.Tx2_1;
					break;	
				case 0x10:					
					if(AllBuf.Rx2_2[3]==0x0e)//���Բ�������
					{
						Run_Setting.Peristaltic_Flow=(AllBuf.Rx2_2[7]<<8)+AllBuf.Rx2_2[8];
						Run_Setting.Aerosol_Flow=(AllBuf.Rx2_2[9]<<8)+AllBuf.Rx2_2[10];
						Run_Setting.Sampling_Time=(AllBuf.Rx2_2[11]<<8)+AllBuf.Rx2_2[12];
						Run_Setting.Liquid_Time=(AllBuf.Rx2_2[13]<<8)+AllBuf.Rx2_2[14];
						Run_Setting.Clear_Time=(AllBuf.Rx2_2[15]<<8)+AllBuf.Rx2_2[16];
						AT24CXX_Write(&AllBuf.Rx2_2[7],0,10);
					}
					else if(AllBuf.Rx2_2[3]==0x14)//ɱ����������
					{
						Disinfect.Peristaltic_Flow=(AllBuf.Rx2_2[7]<<8)+AllBuf.Rx2_2[8];
						Disinfect.Aerosol_Flow=(AllBuf.Rx2_2[9]<<8)+AllBuf.Rx2_2[10];
						Disinfect.Disclear_Time=(AllBuf.Rx2_2[11]<<8)+AllBuf.Rx2_2[12];
						Disinfect.Waterclear_Time=(AllBuf.Rx2_2[13]<<8)+AllBuf.Rx2_2[14];
						Disinfect.Airclear_Time=(AllBuf.Rx2_2[15]<<8)+AllBuf.Rx2_2[16];
						AT24CXX_Write(&AllBuf.Rx2_2[7],10,10);
					}
					for(m=0;m<6;m++)	
						AllBuf.Tx2_1[m]=AllBuf.Rx2_2[m];
					Cal_CRC(AllBuf.Tx2_1,m);	// ���� CRC16
					AllBuf.Tx2_1[m++]=CRCHigh;//crc
					AllBuf.Tx2_1[m++]=CRCLow;		
					 
					U2DS.TLong=m;    
					U2DS.SedOrd = 'E';  
					U2DS.BufAdr = AllBuf.Tx2_1;
					break;
				default:break;
			}
				
		}
	if(U5DS.RecStatus=='R')
		{	
			U5DS.RecStatus=0;
			switch(AllBuf.Rx5_2[0])	// �жϵ�ַ��
			{
				case Box_Press_ADDR://���帺ѹ
					Box_Press=(AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4];
					Sensors.Box_Press=300*Box_Press/2000;//��λPa
					U5DS.RecData_Process=1;	//���帺ѹ��ȡ��ɱ�־
					break ;	
				case Aerosol_Press_ADDR://�����Ҹ�ѹ
					Box_Press=(AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4];
					Sensors.Aerosol_Press=300*Box_Press/2000;//��λPa
					U5DS.RecData_Process=2;	//�����Ҹ�ѹ��ȡ��ɱ�־
					break ;
				case Aerosol_Flow_ADDR://�����������
					Aerosol_Flow=(AllBuf.Rx5_2[3]<<24)+(AllBuf.Rx5_2[4]<<16)+(AllBuf.Rx5_2[5]<<8)+(AllBuf.Rx5_2[6]);
					//Sensors.Aerosol_Flow=Aerosol_Flow/100;
					U5DS.RecData_Process=3;	//����������ƶ�ȡ��ɱ�־
					break ;
				case Atomizer_Press_ADDR://����ѹ��������
					//Sensors.Atomizer_Press=(AllBuf.Rx5_2[3]<<8)+(AllBuf.Rx5_2[4]);
					Press_Data=(AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4];				
					Sensors.Atomizer_Press=100*Press_Data/2000;//��λKPa
					U5DS.RecData_Process=4;
					break ;
				case Left_Press_ADDR://��·ѹ��������
					//Sensors.Left_Press=(AllBuf.Rx5_2[3]<<8)+(AllBuf.Rx5_2[4]);
					Press_Data=((AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4]);					
					Sensors.Left_Press=200-((20*Press_Data/200));//��λKPa
					U5DS.RecData_Process=5;	
					break ;
				case Right_Press_ADDR://��·ѹ��������
					//Sensors.Right_Press=(AllBuf.Rx5_2[3]<<8)+(AllBuf.Rx5_2[4]);					
					Press_Data=((AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4]);
					Sensors.Right_Press=200-((20*Press_Data/200));//��λKPa
					U5DS.RecData_Process=6;	
					break ;
			}	
		}
}
//����1�����ݷ��Ϳ���
//PLC_Add:������ַ
//Flag:��־ 0����ȡ������������������������ 1�����ÿ�����������
//����������������������������1����������������
//��ַ8����·����������Left_FlowCont_ADDR
//��ַ9����·����������Right_FlowCont_ADDR
//��ַ10������õ�����������
//Azbil_Addr:��������ַ
//ReadOrWR��16���ƶ�д���� 1��ʾд 0��ʾ��
//OnOroff��1�趨28.3L/MIN   0�趨��0
void Uart1_SendDataMsg(u8 PLC_Add,u8 Flag,float Data)
{
	u8 m=0;	
	#if	Azbil_Sensor_Use
	if(PLC_Add==Left_FlowCont_ADDR)//��·������������������
	{
		if(Flag==0)//����������������������
			m=Azbil_Sensor_485connect(Left_FlowCont_ADDR,0,0);
		else//����������������������������
		{
			if(Data==28.3f)
				m=Azbil_Sensor_485connect(Left_FlowCont_ADDR,1,1);//��������Ϊ28.3
			else
				m=Azbil_Sensor_485connect(Left_FlowCont_ADDR,1,0);//��������Ϊ0
		}
	}
	else if(PLC_Add==Right_FlowCont_ADDR)//��·������������������
	{
		if(Flag==0)//����������������������
			m=Azbil_Sensor_485connect(Right_FlowCont_ADDR,0,0);
		else//����������������������������
		{
			if(Data==28.3f)
				m=Azbil_Sensor_485connect(Right_FlowCont_ADDR,1,1);//��������Ϊ28.3
			else
				m=Azbil_Sensor_485connect(Right_FlowCont_ADDR,1,0);//��������Ϊ0
		}
	}
	else if(PLC_Add==Aerosol_AQMD3610)//������������ DATAΪ0ֹͣ
	{
			AllBuf.Tx1_1[m++]=Aerosol_AQMD3610;
			AllBuf.Tx1_1[m++]=0x06;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x40;
			AllBuf.Tx1_1[m++]=((u16)Data)>>8;
			AllBuf.Tx1_1[m++]=((u16)Data);
			Cal_CRC(AllBuf.Tx1_1,m);	// ���� CRC16
			AllBuf.Tx1_1[m++]=CRCHigh;//crc
			AllBuf.Tx1_1[m++]=CRCLow;
	}
	
	#else
	if(PLC_Add==Left_FlowCont_ADDR)//��·������������������
	{		
		AllBuf.Tx1_1[m++]=Left_FlowCont_ADDR;
		if(Flag==0)//����������������������
		{
			AllBuf.Tx1_1[m++]=0x03;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x10;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x02;		
		}
		else//����������������������������
		{			
			AllBuf.Tx1_1[m++]=0x10;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x6A;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x02;	
			AllBuf.Tx1_1[m++]=0x04;
			//1��������
			FtoHex.f=Data;
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>8;
			AllBuf.Tx1_1[m++]=(FtoHex.m);
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>24;
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>16;
		}
	}
	else if(PLC_Add==Right_FlowCont_ADDR)//��·������������������
	{		
		AllBuf.Tx1_1[m++]=Right_FlowCont_ADDR;
		if(Flag==0)//����������������������
		{
			AllBuf.Tx1_1[m++]=0x03;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x10;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x02;		
		}
		else//����������������������������
		{
			AllBuf.Tx1_1[m++]=0x10;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x6A;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x02;	
			AllBuf.Tx1_1[m++]=0x04;
			//1��������
			FtoHex.f=Data;
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>8;
			AllBuf.Tx1_1[m++]=(FtoHex.m);
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>24;
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>16;
			
		}
	}
	else if(PLC_Add==Aerosol_AQMD3610)//������������ DATAΪ0ֹͣ
	{
			AllBuf.Tx1_1[m++]=Aerosol_AQMD3610;
			AllBuf.Tx1_1[m++]=0x06;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x40;
			AllBuf.Tx1_1[m++]=((u16)Data)>>8;
			AllBuf.Tx1_1[m++]=((u16)Data);	
	}
	Cal_CRC(AllBuf.Tx1_1,m);	// ���� CRC16
	AllBuf.Tx1_1[m++]=CRCHigh;//crc
	AllBuf.Tx1_1[m++]=CRCLow;
	#endif
	if(U1DS.SedOrd == 0)
		{  
				U1DS.TLong=m;    
				U1DS.SedOrd = 'E';  
				U1DS.BufAdr = AllBuf.Tx1_1;      
		}

}


//����5�����ݷ��Ϳ���
//PLC_Add:������ַ
//��������ѹ����������2��ѹ�������1��������
//��ַ2������ѹ�����	Box_Press_ADDR
//��ַ3��������ѹ����� Aerosol_Press_ADDR
//��ַ4�������������Aerosol_Flow_ADDR
//��ַ5��������·ѹ�� Atomizer_Press_ADDR
//��ַ6����·ѹ�������� Left_Press_ADDR
//��ַ7����·ѹ�������� Right_Press_ADDR

void Uart5_SendDataMsg(u8 PLC_Add)
{
	u8 m=0;
	if(PLC_Add==Box_Press_ADDR)//����ѹ�����0-300pa
	{		
		AllBuf.Tx5_1[m++]=Box_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}	
	else if(PLC_Add==Aerosol_Press_ADDR)//������ѹ�����0-300pa
	{		
		AllBuf.Tx5_1[m++]=Aerosol_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}	
	else if(PLC_Add==Aerosol_Flow_ADDR)//�����������
	{		
		AllBuf.Tx5_1[m++]=Aerosol_Flow_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x02;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x02;		
	}
	else if(PLC_Add==Atomizer_Press_ADDR)//������ѹ��0-300kpa
	{		
		AllBuf.Tx5_1[m++]=Atomizer_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}	
	else if(PLC_Add==Left_Press_ADDR)//��·ѹ����������ѹ��-100kpa-0
	{		
		AllBuf.Tx5_1[m++]=Left_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}
	else if(PLC_Add==Right_Press_ADDR)//��·ѹ����������ѹ��-100kpa-0
	{		
		AllBuf.Tx5_1[m++]=Right_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}

	Cal_CRC(AllBuf.Tx5_1,m);	// ���� CRC16
	AllBuf.Tx5_1[m++]=CRCHigh;//crc
	AllBuf.Tx5_1[m++]=CRCLow;		
	if(U5DS.SedOrd == 0)
		{  
				U5DS.TLong=m;    
				U5DS.SedOrd = 'E';  
				U5DS.BufAdr = AllBuf.Tx5_1;    
		}

}

#if	Azbil_Sensor_Use
//azbil��������д����
//Azbil_Addr:��������ַ
//ReadOrWR��16���ƶ�д���� 1��ʾд 0��ʾ��
//OnOroff��1�趨28.3L/MIN   0�趨��0

u8 Azbil_Sensor_485connect(u8 Azbil_Addr,u8 ReadOrWR,u8 OnOroff)
{	
	u8 m=0;
	AllBuf.Tx1_1[m++]=0x02;//STX
	AllBuf.Tx1_1[m++]=0x30;
	AllBuf.Tx1_1[m++]=Azbil_Addr+0x30;	
	AllBuf.Tx1_1[m++]=0x30;
	AllBuf.Tx1_1[m++]=0x30;//�ӵ�ַ
	
	
	if(ReadOrWR==0)//��
	{
	AllBuf.Tx1_1[m++]=0x58;//�̶�	
	AllBuf.Tx1_1[m++]=0x52;
	AllBuf.Tx1_1[m++]=0x44;
	//��˲ʱ�����Ĵ���04B7	
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x34;//4
	AllBuf.Tx1_1[m++]=0x42;//B
	AllBuf.Tx1_1[m++]=0x37;//7
	//�������ݸ���  1��
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x31;//1		
	}
	else if(ReadOrWR==1)//д
	{
	AllBuf.Tx1_1[m++]=0x78;//�̶�	
	AllBuf.Tx1_1[m++]=0x57;
	AllBuf.Tx1_1[m++]=0x44;
	//���������Ĵ���0579	
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x35;//5
	AllBuf.Tx1_1[m++]=0x37;//7
	AllBuf.Tx1_1[m++]=0x39;//9
	//��������
	if(OnOroff)	
		{
			AllBuf.Tx1_1[m++]=0x30;//
			AllBuf.Tx1_1[m++]=0x31;//
			AllBuf.Tx1_1[m++]=0x31;//
			AllBuf.Tx1_1[m++]=0x42;//	
		}
		else
		{
			AllBuf.Tx1_1[m++]=0x30;//0
			AllBuf.Tx1_1[m++]=0x30;//0
			AllBuf.Tx1_1[m++]=0x30;//0
			AllBuf.Tx1_1[m++]=0x30;//0	
		}
	}
	AllBuf.Tx1_1[m++]=0x03;//ETX
	Azbil_sensor_sum(AllBuf.Tx1_1,m);
	AllBuf.Tx1_1[m++]=Sum_High;
	AllBuf.Tx1_1[m++]=Sum_Low;
	AllBuf.Tx1_1[m++]=0x0D;
	AllBuf.Tx1_1[m++]=0x0A;
	
	return m;
}
#endif
