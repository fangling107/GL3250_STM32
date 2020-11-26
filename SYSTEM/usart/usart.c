#include "usart.h"
#include "delay.h"
#include "crc16.h"
#include "24cxx.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用os,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//串口1初始化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
struct USARTDataStream   U1DS,U2DS,U5DS;   //;Uart2的数据流的控制参数
struct Buf_Type AllBuf;      //;缓冲区
struct sensors Sensors;				
struct run_setting Run_Setting;
struct  disinfect Disinfect;

union floatto_hex FtoHex;
u16 AQMD3610_FLOW=0;
//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART2->SR&0X40)==0);//循环发送,直到发送完毕
		//delay_xms(4);
    USART2->DR = (u8) ch;      
	return ch;
}
#endif 

/*使用microLib的方法*/
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
 |  串口1开始发送数据
 *----------------------------------------------------------------------------*/
void SendFirstByte_USART1(void)
{
    U1DS.TBusy = 20;
    USART_SendData(USART1, (u16)*(U1DS.BufAdr + 0));
    U1DS.TIndex = 1;
    //只要发送寄存器为空，就会一直有中断，因此，要是不发送数据时，把发送中断关闭，只在开始发送时，才打开。 
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE); // //启用TXE中断，即开始发送过程
}

/*----------------------------------------------------------------------------*
 |  串口1中断发送数据
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
            U1DS.SedOrd = 0; //;串口命令发送帧完成
            U1DS.BufAdr = 0;
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);           
            return ;
        }
        USART_SendData(USART1, (u16)*(U1DS.BufAdr + U1DS.TIndex));
        U1DS.TIndex++;
    }
}


/*----------------------------------------------------------------------------*
 |  串口1中断接收数据
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
 |  串口1接收命令
 *----------------------------------------------------------------------------*/
void USART1_Rece(void)
{
    if(U1DS.RBet2Byte >= U1FRAMEDIST)     //;串口1接收最后一个字节已经50ms,对接收到的数据进行应答
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
 |  串口1发送命令
 *----------------------------------------------------------------------------*/
void USART1_Send(void)
{
    if(U1DS.TLong > U1BUFSIZE)
    {
        return; 
    }    
    
    if(U1DS.TBusy == 0)
    {
        if(U1DS.SedOrd == 'E') //;发送命令
        {
            U1DS.SedOrd = 'S'; //;正在发送
            SendFirstByte_USART1();
        }
    }
}



/*----------------------------------------------------------------------------*
 |  串口2开始发送数据
 *----------------------------------------------------------------------------*/
void SendFirstByte_USART2(void)
{
    //U2DS.TBusy = 10;
    USART_SendData(USART2, (u16)*(U2DS.BufAdr + 0));
    U2DS.TIndex = 1;  
    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);   
}


/*----------------------------------------------------------------------------*
 |  串口2中断发送数据
 *----------------------------------------------------------------------------*/
void SendByte_USART2(void)
{  
		
    if((USART2->SR & USART_FLAG_TXE))
    {
        //U2DS.TBusy =10;
        if(U2DS.TIndex >= U2DS.TLong)
        {
            U2DS.TLong = 0;
            U2DS.SedOrd = 0; //;串口命令发送帧完成
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
 |  串口2中断接收数据
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
 | 串口2接收命令
 *----------------------------------------------------------------------------*/
void USART2_Rece(void)
{  	
    if(U2DS.RBet2Byte >= U2FRAMEDIST) //;串口2接收最后一个字节已经20ms,对接收到的数据进行应答
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
 |  串口2发送命令
 *----------------------------------------------------------------------------*/
void USART2_Send(void)
{
	
    if(U2DS.TLong > U2BUFSIZE)
    {
        return;
    }    
    
    if(U2DS.TBusy == 0)
    {
        if(U2DS.SedOrd == 'E') //;发送命令
        { 
            U2DS.SedOrd = 'S'; //;正在发送
					
            SendFirstByte_USART2();
        }
    }
}
/*----------------------------------------------------------------------------*
 |  串口5开始发送数据
 *----------------------------------------------------------------------------*/
void SendFirstByte_UART5(void)
{
    U5DS.TBusy = 20;
    USART_SendData(UART5, (u16)*(U5DS.BufAdr + 0));
    U5DS.TIndex = 1;
    USART_ITConfig(UART5, USART_IT_TXE, ENABLE); 
}


/*----------------------------------------------------------------------------*
 |  串口5中断发送数据
 *----------------------------------------------------------------------------*/
void SendByte_UART5(void)
{
    if ((UART5->SR &USART_FLAG_TXE))
    {
        U5DS.TBusy = 20;
        if (U5DS.TIndex >= U5DS.TLong)
        {
            U5DS.TLong = 0;
            U5DS.SedOrd = 0; //;串口命令发送帧完成
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
 |  串口5中断接收数据
 *----------------------------------------------------------------------------*/
void ReceiveByte_UART5(void)
{
    int temp;
    u8 ch;
    
    if(UART5->SR & USART_FLAG_RXNE)
    {
        U5DS.RecRun = true;  //;是否在接受
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
 |  串口5接收命令
 *----------------------------------------------------------------------------*/
void UART5_Rece(void)
{
    if(U5DS.RBet2Byte >=U5FRAMEDIST)        //;串口2接收最后一个字节已经50ms,对接收到的数据进行应答
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
 |  串口5发送命令
 *----------------------------------------------------------------------------*/
void UART5_Send(void)
{
    if(U5DS.TLong > U5BUFSIZE)
    {
        return;
    }    
    
    if(U5DS.TBusy == 0)
    {
        if(U5DS.SedOrd == 'E') //;发送命令
        {
            U5DS.SedOrd = 'S'; //;正在发送
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
	//PB6 控制pwm方向
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_6;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	GPIO_SetBits(GPIOB,GPIO_Pin_6);
	//PD13 PD14 PD15 控制24V输出
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOD, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOD,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
	//PE1 PE2 PE3 PE4 PE6 PE8 PE9 PE10 PE11 控制24V输出
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_6|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOE, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOE,GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_6|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11);
	//PD0 PD1 PD3 PD4 PD5 PD6 PD7 PD8 PD9 PD10 PD11 PD12	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //设置成输入，默认下拉	  
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOD,GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12);

}

//初始化
//USART1:J2 485 	2个流量控制器 1个喷雾泵控制器
//USART2:P1 232 	连接触摸屏
//USART5:J1 485		2个压差传感器 1个流量计 3个压力传感器
void uart_init(void)
{
    //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* 开启串口1时钟*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 , ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2|RCC_APB1Periph_UART5, ENABLE);	//使能时钟	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC |RCC_APB2Periph_GPIOD, ENABLE);	//使能GPIOA时钟
 	/*USART1 PA9  PA10*/
	USART_DeInit(USART1);  //复位串口1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //;浮空输入模式
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //;复用功能推挽输出模式
	GPIO_Init(GPIOA, &GPIO_InitStructure); 
	//Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
	USART_InitStructure.USART_BaudRate = 9600; //;波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity =USART_Parity_Even ;//偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART1, &USART_InitStructure);
	/*允许USART1中断接收*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	/*允许USART1工作*/
	USART_Cmd(USART1, ENABLE);
	
	//USART2_TX   PA.2 //USART2_RX	  PA.3
	USART_DeInit(USART2);  //复位串口2	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA3
   //Usart2 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART2 初始化设置
	USART_InitStructure.USART_BaudRate = 19200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

	USART_Init(USART2, &USART_InitStructure); //初始化串口
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启中断
	USART_Cmd(USART2, ENABLE);                    //使能串口 
	
	
	/*USART5*/
	USART_DeInit(UART5);  //复位串口5	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	//Usart5 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
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
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);   //;清接收的中断标志
    }
    if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
    {
        SendByte_USART1();
        USART_ClearITPendingBit(USART1, USART_IT_TXE);   //;清发送的中断标志
    }
		#ifdef SYSTEM_SUPPORT_OS	 
		OSIntExit();  											 
		#endif
}

void USART2_IRQHandler(void)                	//串口1中断服务程序
{
	#ifdef SYSTEM_SUPPORT_OS	 	
		OSIntEnter();    
	#endif
		if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
		{
				ReceiveByte_USART2();
				USART_ClearITPendingBit(USART2, USART_IT_RXNE);   //;清接收的中断标志
		}
		if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
		{
				SendByte_USART2();
				USART_ClearITPendingBit(USART2, USART_IT_TXE);   //;清发送的中断标志
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
		//***********************串口1的通讯**********************//
    if(U1DS.RecOrd == true) //;串口1接收到数据
    {
        U1DS.RecOrd = false;
				for(i=0;i<U1DS.RecLen;i++)			
					AllBuf.Rx1_2[i]=*(U1DS.BufAdr+i);
			#if	Azbil_Sensor_Use
				switch(AllBuf.Rx1_2[0])//判断器件地址
				{
					case 0x02:
					if(AllBuf.Rx1_2[2]==0x38||AllBuf.Rx1_2[2]==0x39) 	//左路流量控制器
					{
						if(AllBuf.Rx1_2[6]==0x30&&AllBuf.Rx1_2[7]==0x30)
							U1DS.RecStatus='R';//接收到数据
					}
							break;
					case Aerosol_AQMD3610://喷雾泵驱动器设置	
							len=6;				
 							Cal_CRC(AllBuf.Rx1_2,len);	// 进行 CRC16 校验
 							if((CRCHigh == AllBuf.Rx1_2[len])&&(CRCLow == AllBuf.Rx1_2[len+1]))	// CRC校验正确,接收数据有效							
								U1DS.RecStatus='R';//接收到数据	
							break;
					default:break;
				}
				
			#else
				switch(AllBuf.Rx1_2[0])//判断器件地址
				{											
					case Left_FlowCont_ADDR:	//左路流量控制器
					case Right_FlowCont_ADDR: //右路流量控制器	
							if(AllBuf.Rx1_2[1]==0x03) len=7;
							else											len=6;				
 							Cal_CRC(AllBuf.Rx1_2,len);	// 进行 CRC16 校验
 							if((CRCHigh == AllBuf.Rx1_2[len])&&(CRCLow == AllBuf.Rx1_2[len+1]))	// CRC校验正确,接收数据有效							
								U1DS.RecStatus='R';//接收到数据												
							break;
					case Aerosol_AQMD3610://喷雾泵驱动器设置
							len=6;				
 							Cal_CRC(AllBuf.Rx1_2,len);	// 进行 CRC16 校验
 							if((CRCHigh == AllBuf.Rx1_2[len])&&(CRCLow == AllBuf.Rx1_2[len+1]))	// CRC校验正确,接收数据有效							
								U1DS.RecStatus='R';//接收到数据												
							break;			
					default:break;
				}
			#endif	
    }
		//***********************串口2的通讯**********************//
    if(U2DS.RecOrd == true) //;串口2接收到数据
    {
        U2DS.RecOrd = false;
				for(i=0;i<U2DS.RecLen;i++)			
					AllBuf.Rx2_2[i]=*(U2DS.BufAdr+i);
				switch(AllBuf.Rx2_2[1])//判断地址
				{											
					case 0x03://读11个字节的数据
					case 0x05://按键	
							len=6;
 							Cal_CRC(AllBuf.Rx2_2,len);	// 进行 CRC16 校验
 							if((CRCHigh == AllBuf.Rx2_2[len])&&(CRCLow == AllBuf.Rx2_2[len+1]))	// CRC校验正确,接收数据有效							
								U2DS.RecStatus='R';//接收到数据												
							break;
					case 0x10:
							len=17;//杀毒参数保存或设置参数保存
							Cal_CRC(AllBuf.Rx2_2,len);	// 进行 CRC16 校验
 							if((CRCHigh == AllBuf.Rx2_2[len])&&(CRCLow == AllBuf.Rx2_2[len+1]))	// CRC校验正确,接收数据有效							
								U2DS.RecStatus='R';//接收到数据	
							break;
					default:break;
				}
    }
				//***********************串口5的通讯**********************//
    if(U5DS.RecOrd == true) //;串口5接收到数据
    {
        U5DS.RecOrd = false;
				for(i=0;i<U5DS.RecLen;i++)			
					AllBuf.Rx5_2[i]=*(U5DS.BufAdr+i);
				switch(AllBuf.Rx5_2[0])//判断器件地址
				{											
					case Box_Press_ADDR://柜体负压
					case Aerosol_Press_ADDR://气雾室负压
					case Atomizer_Press_ADDR://喷雾器的压力
					case Left_Press_ADDR://左路的压力	
					case Right_Press_ADDR://右路的压力	
							len=5;					
 							Cal_CRC(AllBuf.Rx5_2,len);	// 进行 CRC16 校验
 							if((CRCHigh == AllBuf.Rx5_2[len])&&(CRCLow == AllBuf.Rx5_2[len+1]))	// CRC校验正确,接收数据有效							
								U5DS.RecStatus='R';//接收到数据												
							break;
					case Aerosol_Flow_ADDR://流量计
							len=7;					
 							Cal_CRC(AllBuf.Rx5_2,len);	// 进行 CRC16 校验
 							if((CRCHigh == AllBuf.Rx5_2[len])&&(CRCLow == AllBuf.Rx5_2[len+1]))	// CRC校验正确,接收数据有效							
								U5DS.RecStatus='R';//接收到数据		
							break;
					default:break;
				}
    }
}
/*----------------------------------------------------------------------------*
**对串口收到的数据进行处理
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
	//2个气体流量控制器和1个喷雾泵的控制器
	if(U1DS.RecStatus=='R')
		{	
			U1DS.RecStatus=0;
			#if	Azbil_Sensor_Use
			switch(AllBuf.Rx1_2[0])	// 判断地址码
			{
				case 0x02:				
				if(AllBuf.Rx1_2[2]==0x38)
				{
					if(AllBuf.Rx1_2[5]==0x58)//读左路流量
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
					U1DS.RecData_Process=4;	//左路流量读取完成标志
					}
				else
					U1DS.RecData_Process=1;	//左路流量设置完成标志
			}
			else if(AllBuf.Rx1_2[2]==0x39)
			{
				if(AllBuf.Rx1_2[5]==0x58)//读右路流量
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
					U1DS.RecData_Process=5;	//右路流量读取完成标志
					}
					else
						U1DS.RecData_Process=2;	//右路流量设置完成标志
				}
					break ;
				case Aerosol_AQMD3610://喷雾流量设置
						U1DS.RecData_Process=3;	//喷雾流量设置完成标志	
					break ;
				
			}
			#else
			switch(AllBuf.Rx1_2[0])	// 判断地址码
			{
				case Left_FlowCont_ADDR:
					if(AllBuf.Rx1_2[1]==0x03)//读左路流量
					{
						Gas_Flow=(AllBuf.Rx1_2[5]<<24)+(AllBuf.Rx1_2[6]<<16)+(AllBuf.Rx1_2[3]<<8)+(AllBuf.Rx1_2[4]);						
						Sensors.Left_Flow=Gas_Flow*10;
						U1DS.RecData_Process=4;	//左路流量读取完成标志
					}
					else
						U1DS.RecData_Process=1;	//左路流量设置完成标志
					break ;
				case Right_FlowCont_ADDR:
					if(AllBuf.Rx1_2[1]==0x03)//读右路流量
					{					
						Gas_Flow=(AllBuf.Rx1_2[5]<<24)+(AllBuf.Rx1_2[6]<<16)+(AllBuf.Rx1_2[3]<<8)+(AllBuf.Rx1_2[4]);
						Sensors.Right_Flow=Gas_Flow*10;
						U1DS.RecData_Process=5;	//右路流量读取完成标志
					}
					else
						U1DS.RecData_Process=2;	//右路流量设置完成标志
					break ;
				case Aerosol_AQMD3610://喷雾流量设置
						U1DS.RecData_Process=3;	//喷雾流量设置完成标志
					break ;
			}
			#endif
		}
	
	//***********************对串口2收到的数据进行处理**********************//
		if(U2DS.RecStatus=='R')
		{	
			U2DS.RecStatus=0;
			
			switch(AllBuf.Rx2_2[1])	// 判断地址码
			{				
				case 0x03:					
					if(AllBuf.Rx2_2[3]==0x0e)//参数设置,包括运行参数和杀毒参数
					{
						AllBuf.Tx2_1[m++]=WEINVIEW_ADDR;
						AllBuf.Tx2_1[m++]=0x03;	
						AllBuf.Tx2_1[m++]=0x16;//11个字22个字节
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
					else if(AllBuf.Rx2_2[3]==0x00)//传感器参数
					{
						
						AllBuf.Tx2_1[m++]=WEINVIEW_ADDR;
						AllBuf.Tx2_1[m++]=0x03;	
						AllBuf.Tx2_1[m++]=0x16;//11个字22个字节
						AllBuf.Tx2_1[m++]=(Sensors.Box_Press>>8);	//箱体压力	
						AllBuf.Tx2_1[m++]=Sensors.Box_Press;
						
						AllBuf.Tx2_1[m++]=(Sensors.Aerosol_Press>>8);	//气雾室压力
						AllBuf.Tx2_1[m++]=(Sensors.Aerosol_Press);
						
						AllBuf.Tx2_1[m++]=(Sensors.Atomizer_Press>>8);	//雾化器压力
						AllBuf.Tx2_1[m++]=(Sensors.Atomizer_Press);						
						
						AllBuf.Tx2_1[m++]=(Sensors.Aerosol_Flow>>8);//喷雾器流量	
						AllBuf.Tx2_1[m++]=(Sensors.Aerosol_Flow);
						
						AllBuf.Tx2_1[m++]=(Sensors.Peristaltic_Flow>>8);//蠕动泵流量	
						AllBuf.Tx2_1[m++]=(Sensors.Peristaltic_Flow);
						
						AllBuf.Tx2_1[m++]=(Sensors.Left_Press>>8);//左路压力
						AllBuf.Tx2_1[m++]=Sensors.Left_Press;
						
						AllBuf.Tx2_1[m++]=(Sensors.Left_Flow>>8);	//左路流量
						AllBuf.Tx2_1[m++]=(Sensors.Left_Flow);
						
						AllBuf.Tx2_1[m++]=(Sensors.Right_Press>>8);	//右路压力
						AllBuf.Tx2_1[m++]=(Sensors.Right_Press);
						
						AllBuf.Tx2_1[m++]=(Sensors.Right_Flow>>8);//右路流量	
						AllBuf.Tx2_1[m++]=(Sensors.Right_Flow);
						
						
						AllBuf.Tx2_1[m++]=(Sensors.Pre_Set_Time>>8);	//预设时间
						AllBuf.Tx2_1[m++]=(Sensors.Pre_Set_Time);
						
					
						AllBuf.Tx2_1[m++]=(Sensors.Status>>8);//系统状态	
						AllBuf.Tx2_1[m++]=(Sensors.Status);
					}
						Cal_CRC(AllBuf.Tx2_1,m);	// 计算 CRC16
						AllBuf.Tx2_1[m++]=CRCHigh;//crc
						AllBuf.Tx2_1[m++]=CRCLow;		
						 
						U2DS.TLong=m;    
						U2DS.SedOrd = 'E';  
						U2DS.BufAdr = AllBuf.Tx2_1;
						break;
				case 0x05:					
					if(AllBuf.Rx2_2[3]==0x00)//试验						
					{
							if(AllBuf.Rx2_2[4]==0xFF)//开始试验
							{
									Start_Off_Flag=1;	
									Sensors.Status&=0x0;//关闭定时器
									Sensors.Status|=0x0002;//提示正在进行气路平衡
									Sensors.Pre_Set_Time=Balence_Time;//气路平衡时间预设时间
									Sec_count=0;//秒计时清零
									U1DS.RecData_Process=0;
									U5DS.RecData_Process=0;
									Out5=0;//气路平衡电磁阀开
									Out7=0;//采样风机开
									Out8=0;//采样风机开
									Out1=0;//低电平导通，开离心风机
									AQMD3610_FLOW=Run_Setting.Aerosol_Flow*8;
									Sensors.Aerosol_Flow=Run_Setting.Aerosol_Flow;
							}
							else//停止试验
							{
									Start_Off_Flag=0;
									Sensors.Status&=0;
									Sensors.Status|=0x0001;//进入空闲状态
									Sensors.Pre_Set_Time=0;//预设时间清零
									Sensors.Left_Flow=0;
									Sensors.Right_Flow=0;
									Sensors.Aerosol_Flow=0;
							}
					}
					else if(AllBuf.Rx2_2[3]==0x01)//风机		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//开风机
							{
								Out1=0;//低电平导通
							}
							else//关风机
							{
								Out1=1;//高电平关闭
							}

					}	
					else if(AllBuf.Rx2_2[3]==0x02)//紫外灯		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//开紫外灯
							{
								Out2=0;//低电平导通
							}
							else//关紫外灯
							{
								Out2=1;
							}

					}
					else if(AllBuf.Rx2_2[3]==0x03)//照明		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//开照明
							{
								Out3=0;	
							}
							else//关照明
							{
								Out3=1;
							}

					}
					//手动启停蠕动泵
					else if(AllBuf.Rx2_2[3]==0x04)//蠕动泵		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//开蠕动泵
							{								
								if(Start_Off_Flag==1)//启动状态
								{
									if((Sensors.Status&0x0008)==0x0008)	//正在试验状态	
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
									else//系统杀毒
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
								else//手动状态，设为最大流量	
								{
										Period=40;
										TIM_SetAutoreload(TIM3,Period-1);//40khz
										Sensors.Peristaltic_Flow=1000;
								}
								TIM_SetCompare2(TIM3,Period/5);
								TIM_Cmd(TIM3, ENABLE);  //使能定时器TIM3	
							}
							else//关蠕动泵
							{
								TIM_Cmd(TIM3, DISABLE);  //失能定时器TIM3
								Sensors.Peristaltic_Flow=0;
							}
					}

					else if(AllBuf.Rx2_2[3]==0x06&&AllBuf.Rx2_2[4]==0xFF)//计时结束		
					{
						if(Start_Off_Flag==1&&((Sensors.Status&0x08)==0x08))	
						{
							Sensors.Status&=0x0;
							Sensors.Status|=0x0020;//2min试验完成，开始进行管路清洗	
							Sensors.Pre_Set_Time=Run_Setting.Clear_Time;//预设时间
							Sec_count=0;//秒计时清零
							Out5=0;//气路平衡电磁阀关
							Out6=1;//采样电磁阀关
						}
						else if(Start_Off_Flag==1&&((Sensors.Status&0x20)==0x20))	//清洗时间到
						{
							Sensors.Status&=0x0;
							Sensors.Status|=0x0040;//提示试验完成
						}
						else if(Start_Off_Flag==1&&((Sensors.Status&0x02)==2))//气路平衡超时
						{
							Sensors.Status&=0x0;//关闭定时器
							Sensors.Status|=0x0080;//提示超时
							//超时需要关闭一些其他执行器件
							
						}
						//系统杀毒
						else if((Sensors.Status&0x0100)==0x0100||(Sensors.Status&0x0200)==0x0200||(Sensors.Status&0x0400)==0x0400)	//杀毒时间到
						{
							Sensors.Status&=0;
							Sensors.Status|=0x0001;//进入空闲状态
							Sensors.Pre_Set_Time=0;//预设时间清零
							Start_Off_Flag=0;	
							AQMD3610_FLOW=0;
							Sensors.Left_Flow=0;
							Sensors.Right_Flow=0;
							Sensors.Aerosol_Flow=0;	
						}
					}
					else if(AllBuf.Rx2_2[3]==0x08)//消毒液清洗		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//开
							{
								Sensors.Status&=0x0;
								Sensors.Status|=0x0100;//消毒液清洗
								Sensors.Pre_Set_Time=Disinfect.Disclear_Time;//预设时间
								Sec_count=0;//秒计时清零
								Start_Off_Flag=1;
								Out5=0;//气路平衡电磁阀开
								Out7=0;//采样风机开
								Out8=0;//采样风机开
								AQMD3610_FLOW=Disinfect.Aerosol_Flow*8;
								Sensors.Aerosol_Flow=Disinfect.Aerosol_Flow;
							}
							else//关
							{
								Sensors.Status&=0;
								Sensors.Status|=0x0001;//进入空闲状态
								Sensors.Pre_Set_Time=0;//预设时间清零
								Start_Off_Flag=0;
								Sensors.Left_Flow=0;
								Sensors.Right_Flow=0;	
								Sensors.Aerosol_Flow=0;
							}
					}
					else if(AllBuf.Rx2_2[3]==0x09)//蒸馏水清洗		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//开
							{
								Sensors.Status&=0x0;
								Sensors.Status|=0x0200;//消毒液清洗
								Sensors.Pre_Set_Time=Disinfect.Waterclear_Time;//预设时间
								Sec_count=0;//秒计时清零
								Start_Off_Flag=1;
								Out5=0;//气路平衡电磁阀开
								Out7=0;//采样风机开
								Out8=0;//采样风机开
								AQMD3610_FLOW=Disinfect.Aerosol_Flow*8;
								Sensors.Aerosol_Flow=Disinfect.Aerosol_Flow;	
							}
							else//关
							{
								Sensors.Status&=0;
								Sensors.Status|=0x0001;//进入空闲状态
								Sensors.Pre_Set_Time=0;//预设时间清零
								Start_Off_Flag=0;	
								Sensors.Left_Flow=0;
								Sensors.Right_Flow=0;	
								Sensors.Aerosol_Flow=0;
							}
					}
					else if(AllBuf.Rx2_2[3]==0x0A)//空气清洗		
					{
							if(AllBuf.Rx2_2[4]==0xFF)//开
							{
								Sensors.Status&=0x0;
								Sensors.Status|=0x0400;//消毒液清洗
								Sensors.Pre_Set_Time=Disinfect.Airclear_Time;//预设时间
								Sec_count=0;//秒计时清零
								Start_Off_Flag=1;
								Out5=0;//气路平衡电磁阀开
								Out7=0;//采样风机开	
								Out8=0;//采样风机开
								AQMD3610_FLOW=Disinfect.Aerosol_Flow*8;
								Sensors.Aerosol_Flow=Disinfect.Aerosol_Flow;																
							}
							else//关
							{
								Sensors.Status&=0;
								Sensors.Status|=0x0001;//进入空闲状态
								Sensors.Pre_Set_Time=0;//预设时间清零
								Start_Off_Flag=0;
								Sensors.Left_Flow=0;
								Sensors.Right_Flow=0;
								Sensors.Aerosol_Flow=0;	
							}
					}
					else if(AllBuf.Rx2_2[3]==0x0B&&AllBuf.Rx2_2[4]==0xFF)//按下正式开始试验按钮		
					{
						Sensors.Status&=0x0;						
						Sensors.Status|=0x0008;//正在进行试验	
						Sensors.Pre_Set_Time=Run_Setting.Sampling_Time;//预设时间
						Sec_count=0;//秒计时清零						
						Out5=1;//气路平衡电磁阀关
						Out6=0;//采样电磁阀开							
					}
					for(m=0;m<8;m++)	
						AllBuf.Tx2_1[m]=AllBuf.Rx2_2[m];
					U2DS.TLong=8;    
					U2DS.SedOrd = 'E';  
					U2DS.BufAdr = AllBuf.Tx2_1;
					break;	
				case 0x10:					
					if(AllBuf.Rx2_2[3]==0x0e)//测试参数保存
					{
						Run_Setting.Peristaltic_Flow=(AllBuf.Rx2_2[7]<<8)+AllBuf.Rx2_2[8];
						Run_Setting.Aerosol_Flow=(AllBuf.Rx2_2[9]<<8)+AllBuf.Rx2_2[10];
						Run_Setting.Sampling_Time=(AllBuf.Rx2_2[11]<<8)+AllBuf.Rx2_2[12];
						Run_Setting.Liquid_Time=(AllBuf.Rx2_2[13]<<8)+AllBuf.Rx2_2[14];
						Run_Setting.Clear_Time=(AllBuf.Rx2_2[15]<<8)+AllBuf.Rx2_2[16];
						AT24CXX_Write(&AllBuf.Rx2_2[7],0,10);
					}
					else if(AllBuf.Rx2_2[3]==0x14)//杀毒参数保存
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
					Cal_CRC(AllBuf.Tx2_1,m);	// 计算 CRC16
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
			switch(AllBuf.Rx5_2[0])	// 判断地址码
			{
				case Box_Press_ADDR://柜体负压
					Box_Press=(AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4];
					Sensors.Box_Press=300*Box_Press/2000;//单位Pa
					U5DS.RecData_Process=1;	//柜体负压读取完成标志
					break ;	
				case Aerosol_Press_ADDR://气雾室负压
					Box_Press=(AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4];
					Sensors.Aerosol_Press=300*Box_Press/2000;//单位Pa
					U5DS.RecData_Process=2;	//气雾室负压读取完成标志
					break ;
				case Aerosol_Flow_ADDR://喷雾口流量计
					Aerosol_Flow=(AllBuf.Rx5_2[3]<<24)+(AllBuf.Rx5_2[4]<<16)+(AllBuf.Rx5_2[5]<<8)+(AllBuf.Rx5_2[6]);
					//Sensors.Aerosol_Flow=Aerosol_Flow/100;
					U5DS.RecData_Process=3;	//喷雾口流量计读取完成标志
					break ;
				case Atomizer_Press_ADDR://雾化器压力传感器
					//Sensors.Atomizer_Press=(AllBuf.Rx5_2[3]<<8)+(AllBuf.Rx5_2[4]);
					Press_Data=(AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4];				
					Sensors.Atomizer_Press=100*Press_Data/2000;//单位KPa
					U5DS.RecData_Process=4;
					break ;
				case Left_Press_ADDR://左路压力传感器
					//Sensors.Left_Press=(AllBuf.Rx5_2[3]<<8)+(AllBuf.Rx5_2[4]);
					Press_Data=((AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4]);					
					Sensors.Left_Press=200-((20*Press_Data/200));//单位KPa
					U5DS.RecData_Process=5;	
					break ;
				case Right_Press_ADDR://右路压力传感器
					//Sensors.Right_Press=(AllBuf.Rx5_2[3]<<8)+(AllBuf.Rx5_2[4]);					
					Press_Data=((AllBuf.Rx5_2[3]<<8)+AllBuf.Rx5_2[4]);
					Sensors.Right_Press=200-((20*Press_Data/200));//单位KPa
					U5DS.RecData_Process=6;	
					break ;
			}	
		}
}
//串口1的数据发送控制
//PLC_Add:器件地址
//Flag:标志 0：读取气体质量流量控制器的流量 1：设置控制器的流量
//控制两个气体质量流量控制器和1个喷雾器的驱动器
//地址8：左路流量控制器Left_FlowCont_ADDR
//地址9：右路流量控制器Right_FlowCont_ADDR
//地址10：喷雾泵的驱动控制器
//Azbil_Addr:传感器地址
//ReadOrWR：16进制读写命令 1表示写 0表示读
//OnOroff：1设定28.3L/MIN   0设定成0
void Uart1_SendDataMsg(u8 PLC_Add,u8 Flag,float Data)
{
	u8 m=0;	
	#if	Azbil_Sensor_Use
	if(PLC_Add==Left_FlowCont_ADDR)//左路气体质量流量控制器
	{
		if(Flag==0)//读气体流量控制器的流量
			m=Azbil_Sensor_485connect(Left_FlowCont_ADDR,0,0);
		else//设置气体质量流量控制器的流量
		{
			if(Data==28.3f)
				m=Azbil_Sensor_485connect(Left_FlowCont_ADDR,1,1);//设置流量为28.3
			else
				m=Azbil_Sensor_485connect(Left_FlowCont_ADDR,1,0);//设置流量为0
		}
	}
	else if(PLC_Add==Right_FlowCont_ADDR)//右路气体质量流量控制器
	{
		if(Flag==0)//读气体流量控制器的流量
			m=Azbil_Sensor_485connect(Right_FlowCont_ADDR,0,0);
		else//设置气体质量流量控制器的流量
		{
			if(Data==28.3f)
				m=Azbil_Sensor_485connect(Right_FlowCont_ADDR,1,1);//设置流量为28.3
			else
				m=Azbil_Sensor_485connect(Right_FlowCont_ADDR,1,0);//设置流量为0
		}
	}
	else if(PLC_Add==Aerosol_AQMD3610)//喷雾器控制器 DATA为0停止
	{
			AllBuf.Tx1_1[m++]=Aerosol_AQMD3610;
			AllBuf.Tx1_1[m++]=0x06;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x40;
			AllBuf.Tx1_1[m++]=((u16)Data)>>8;
			AllBuf.Tx1_1[m++]=((u16)Data);
			Cal_CRC(AllBuf.Tx1_1,m);	// 计算 CRC16
			AllBuf.Tx1_1[m++]=CRCHigh;//crc
			AllBuf.Tx1_1[m++]=CRCLow;
	}
	
	#else
	if(PLC_Add==Left_FlowCont_ADDR)//左路气体质量流量控制器
	{		
		AllBuf.Tx1_1[m++]=Left_FlowCont_ADDR;
		if(Flag==0)//读气体流量控制器的流量
		{
			AllBuf.Tx1_1[m++]=0x03;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x10;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x02;		
		}
		else//设置气体质量流量控制器的流量
		{			
			AllBuf.Tx1_1[m++]=0x10;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x6A;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x02;	
			AllBuf.Tx1_1[m++]=0x04;
			//1个浮点数
			FtoHex.f=Data;
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>8;
			AllBuf.Tx1_1[m++]=(FtoHex.m);
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>24;
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>16;
		}
	}
	else if(PLC_Add==Right_FlowCont_ADDR)//右路气体质量流量控制器
	{		
		AllBuf.Tx1_1[m++]=Right_FlowCont_ADDR;
		if(Flag==0)//读气体流量控制器的流量
		{
			AllBuf.Tx1_1[m++]=0x03;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x10;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x02;		
		}
		else//设置气体质量流量控制器的流量
		{
			AllBuf.Tx1_1[m++]=0x10;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x6A;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x02;	
			AllBuf.Tx1_1[m++]=0x04;
			//1个浮点数
			FtoHex.f=Data;
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>8;
			AllBuf.Tx1_1[m++]=(FtoHex.m);
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>24;
			AllBuf.Tx1_1[m++]=(FtoHex.m)>>16;
			
		}
	}
	else if(PLC_Add==Aerosol_AQMD3610)//喷雾器控制器 DATA为0停止
	{
			AllBuf.Tx1_1[m++]=Aerosol_AQMD3610;
			AllBuf.Tx1_1[m++]=0x06;
			AllBuf.Tx1_1[m++]=0x00;
			AllBuf.Tx1_1[m++]=0x40;
			AllBuf.Tx1_1[m++]=((u16)Data)>>8;
			AllBuf.Tx1_1[m++]=((u16)Data);	
	}
	Cal_CRC(AllBuf.Tx1_1,m);	// 计算 CRC16
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


//串口5的数据发送控制
//PLC_Add:器件地址
//控制三个压力传感器和2个压差传感器和1个流量计
//地址2：柜体压差传感器	Box_Press_ADDR
//地址3：气雾室压差传感器 Aerosol_Press_ADDR
//地址4：喷雾口流量计Aerosol_Flow_ADDR
//地址5：雾化器气路压力 Atomizer_Press_ADDR
//地址6：左路压力传感器 Left_Press_ADDR
//地址7：右路压力传感器 Right_Press_ADDR

void Uart5_SendDataMsg(u8 PLC_Add)
{
	u8 m=0;
	if(PLC_Add==Box_Press_ADDR)//柜体压差传感器0-300pa
	{		
		AllBuf.Tx5_1[m++]=Box_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}	
	else if(PLC_Add==Aerosol_Press_ADDR)//气雾室压差传感器0-300pa
	{		
		AllBuf.Tx5_1[m++]=Aerosol_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}	
	else if(PLC_Add==Aerosol_Flow_ADDR)//喷雾口流量计
	{		
		AllBuf.Tx5_1[m++]=Aerosol_Flow_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x02;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x02;		
	}
	else if(PLC_Add==Atomizer_Press_ADDR)//雾化器的压力0-300kpa
	{		
		AllBuf.Tx5_1[m++]=Atomizer_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}	
	else if(PLC_Add==Left_Press_ADDR)//左路压力传感器的压力-100kpa-0
	{		
		AllBuf.Tx5_1[m++]=Left_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}
	else if(PLC_Add==Right_Press_ADDR)//右路压力传感器的压力-100kpa-0
	{		
		AllBuf.Tx5_1[m++]=Right_Press_ADDR;
		AllBuf.Tx5_1[m++]=0x03;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x00;
		AllBuf.Tx5_1[m++]=0x01;		
	}

	Cal_CRC(AllBuf.Tx5_1,m);	// 计算 CRC16
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
//azbil传感器读写数据
//Azbil_Addr:传感器地址
//ReadOrWR：16进制读写命令 1表示写 0表示读
//OnOroff：1设定28.3L/MIN   0设定成0

u8 Azbil_Sensor_485connect(u8 Azbil_Addr,u8 ReadOrWR,u8 OnOroff)
{	
	u8 m=0;
	AllBuf.Tx1_1[m++]=0x02;//STX
	AllBuf.Tx1_1[m++]=0x30;
	AllBuf.Tx1_1[m++]=Azbil_Addr+0x30;	
	AllBuf.Tx1_1[m++]=0x30;
	AllBuf.Tx1_1[m++]=0x30;//子地址
	
	
	if(ReadOrWR==0)//读
	{
	AllBuf.Tx1_1[m++]=0x58;//固定	
	AllBuf.Tx1_1[m++]=0x52;
	AllBuf.Tx1_1[m++]=0x44;
	//读瞬时流量寄存器04B7	
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x34;//4
	AllBuf.Tx1_1[m++]=0x42;//B
	AllBuf.Tx1_1[m++]=0x37;//7
	//读出数据个数  1个
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x31;//1		
	}
	else if(ReadOrWR==1)//写
	{
	AllBuf.Tx1_1[m++]=0x78;//固定	
	AllBuf.Tx1_1[m++]=0x57;
	AllBuf.Tx1_1[m++]=0x44;
	//设置流量寄存器0579	
	AllBuf.Tx1_1[m++]=0x30;//0
	AllBuf.Tx1_1[m++]=0x35;//5
	AllBuf.Tx1_1[m++]=0x37;//7
	AllBuf.Tx1_1[m++]=0x39;//9
	//设置数据
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
