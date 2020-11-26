#ifndef __SYS_H
#define __SYS_H	
#include "stm32f10x.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.7
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
////////////////////////////////////////////////////////////////////////////////// 	 

//0,不支持ucos
//1,支持ucos
#define SYSTEM_SUPPORT_OS			1		//定义系统文件夹是否支持UCOS

//定义azbil传感器的开关
#define Azbil_Sensor_Use			1		//定义是否使用Azbil传感器

//定义各个传感器的通讯地址																	    
#define	WEINVIEW_ADDR					1		//定义威纶触摸屏的通讯地址
#define	Box_Press_ADDR				2		//定义箱体负压传感器的通讯地址
#define	Aerosol_Press_ADDR		3		//定义气雾室负压传感器的通讯地址
#define	Aerosol_Flow_ADDR			4		//定义雾化器流量传感器的通讯地址

#define	Atomizer_Press_ADDR		5		//定义雾化器压力传感器的通讯地址
#define	Left_Press_ADDR				6		//定义左路压力传感器的通讯地址
#define	Right_Press_ADDR			7		//定义右路压力传感器的通讯地址


#define	Left_FlowCont_ADDR		8		//定义左路气体质量流量控制器的通讯地址
#define	Right_FlowCont_ADDR		9		//定义右路气体质量流量控制器的通讯地址

#define Aerosol_AQMD3610			10	//雾化器的驱动器地址

#define Balence_Time					300	//气路平衡超时时间 300s
//位带操作,实现51类似的GPIO控制功能
//具体实现思想,参考<<CM3权威指南>>第五章(87页~92页).
//IO口操作宏定义
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
//IO口地址映射
#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C 
#define GPIOB_ODR_Addr    (GPIOB_BASE+12) //0x40010C0C 
#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C 
#define GPIOD_ODR_Addr    (GPIOD_BASE+12) //0x4001140C 
#define GPIOE_ODR_Addr    (GPIOE_BASE+12) //0x4001180C 
#define GPIOF_ODR_Addr    (GPIOF_BASE+12) //0x40011A0C    
#define GPIOG_ODR_Addr    (GPIOG_BASE+12) //0x40011E0C    

#define GPIOA_IDR_Addr    (GPIOA_BASE+8) //0x40010808 
#define GPIOB_IDR_Addr    (GPIOB_BASE+8) //0x40010C08 
#define GPIOC_IDR_Addr    (GPIOC_BASE+8) //0x40011008 
#define GPIOD_IDR_Addr    (GPIOD_BASE+8) //0x40011408 
#define GPIOE_IDR_Addr    (GPIOE_BASE+8) //0x40011808 
#define GPIOF_IDR_Addr    (GPIOF_BASE+8) //0x40011A08 
#define GPIOG_IDR_Addr    (GPIOG_BASE+8) //0x40011E08 
 
//IO口操作,只对单一的IO口!
//确保n的值小于16!
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //输出 
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //输入 

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //输出 
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //输入 

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //输出 
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //输入 

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  //输出 
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  //输入 

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  //输出 
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  //输入

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  //输出 
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  //输入

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //输出 
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //输入


//以下为汇编函数
void WFI_SET(void);		//执行WFI指令
void INTX_DISABLE(void);//关闭所有中断
void INTX_ENABLE(void);	//开启所有中断
void MSR_MSP(u32 addr);	//设置堆栈地址


//板子上的输出口和引脚对应关系
//out1__Electro3  out2__Electro2 out3__Electro1 out4__Electro6 out5__Electro5 out6__Electro4
//out7__Electro12  out8__Electro11 out9__Electro10 out10__Electro9 out11__Electro8 out12__Electro7
//
#define	Out3		 	PDout(13) 
#define	Out2 			PDout(14)
#define	Out1		 	PDout(15)
#define	Out6		 	PEout(1) 
#define	Out5			PEout(2)
#define	Out4		 	PEout(3)
#define	Out12		 	PEout(4) 
#define	Out11		 	PEout(6)
#define	Out10		 	PEout(8)
#define	Out9		  PEout(9) 
#define	Out8		  PEout(10)
#define	Out7		  PEout(11)
#define PWM_Dir		PBout(6)


//串口结构体
struct USARTDataStream  //;串口数据流的控制
{    
    u16 RIndex;    		//;接收序列号
    u16 TIndex;   		//;发送序列号
    u16 TLong;     		//;发送长度
    u16 TBusy;     		//;发送忙碌
    u8  RecRun;    		//;是否在接受
    u8  RBet2Byte; 		//;接收两个字节的时间间隔
    u8  RecOrd;    		//;串口收到数据帧
		u8	RecData_Process;//接收数据处理
		u8  RecStatus;    //;串口收到数据帧标志
    u16 RecLen;    		//;接收长度
    u8  SedOrd;    		//;串口在发送帧		
    u8  *BufAdr;   	//;发送与接收缓存的指针
};
extern struct USARTDataStream   U1DS,U2DS,U5DS;   //;Uart的数据流的控制参数


#define PORT1REVBUF     AllBuf.Rx1_1
#define PORT1SEDBUF     AllBuf.Tx1_1
#define U1BUFSIZE      	50
#define U1FRAMEDIST    	20

#define PORT2REVBUF     AllBuf.Rx2_1
#define PORT2SEDBUF     AllBuf.Tx2_1
#define U2BUFSIZE      	50
#define U2FRAMEDIST    	20

#define PORT5REVBUF    	AllBuf.Rx5_1
#define PORT5SEDBUF    	AllBuf.Tx5_1
#define U5BUFSIZE      	50
#define U5FRAMEDIST    	20

#define false   0x00
#define true    0x01
struct Buf_Type   //;所有的缓冲区的分配
{	
		u8 Tx1_1[U1BUFSIZE];  //;串口1的缓冲区    
    u8 Rx1_1[U1BUFSIZE];  
		u8 Rx1_2[U1BUFSIZE];
	
    u8 Tx2_1[U2BUFSIZE];  //;串口2的缓冲区
    u8 Rx2_1[U2BUFSIZE];	//;用于保存
    u8 Rx2_2[U2BUFSIZE];

		u8 Tx5_1[U5BUFSIZE];  //;串口5的缓冲区    
    u8 Rx5_1[U5BUFSIZE];
		u8 Rx5_2[U5BUFSIZE];
};
//传感器的数据结构
struct sensors
{
	s16  Box_Press;//箱体压力
	s16  Aerosol_Press;//气雾室压力
	s16  Atomizer_Press;//雾化器压力
	s16  Aerosol_Flow;//喷雾流量
	s16  Peristaltic_Flow;//蠕动泵流量
	s16  Left_Press;//左路压力
	s16  Left_Flow;//左路流量
	s16  Right_Press;//右路压力
	s16  Right_Flow;//右路流量
	u16  Pre_Set_Time;//预设时间
	u16	 Status;//系统状态
};
extern struct sensors Sensors;

//运行参数设置
struct run_setting
{
	u16  Peristaltic_Flow;//蠕动泵流量
	u16  Aerosol_Flow;//喷雾流量
	u16  Sampling_Time;//采样时间
	u16  Liquid_Time;//供液时间
	u16  Clear_Time;//清洗时间
};
extern struct run_setting Run_Setting;
//系统杀毒设置
struct  disinfect
{
	u16  Peristaltic_Flow;//蠕动泵流量
	u16  Aerosol_Flow;//喷雾流量
	u16  Disclear_Time;//消毒液清洗时间
	u16  Waterclear_Time;//蒸馏水清洗时间
	u16  Airclear_Time;//空气清洗时间
};
extern struct  disinfect Disinfect;

//全局变量
extern u8 Start_Off_Flag;//开关标志
extern u8 Sec_count;//秒计时
extern u16 Sys_Status;//系统状态标志

//共用体
union floatto_hex
{
	u32 m;
	float f;
};
#endif
extern u16 AQMD3610_FLOW;
