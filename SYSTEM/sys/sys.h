#ifndef __SYS_H
#define __SYS_H	
#include "stm32f10x.h"

//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/8/18
//�汾��V1.7
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
////////////////////////////////////////////////////////////////////////////////// 	 

//0,��֧��ucos
//1,֧��ucos
#define SYSTEM_SUPPORT_OS			1		//����ϵͳ�ļ����Ƿ�֧��UCOS

//����azbil�������Ŀ���
#define Azbil_Sensor_Use			1		//�����Ƿ�ʹ��Azbil������

//���������������ͨѶ��ַ																	    
#define	WEINVIEW_ADDR					1		//�������ڴ�������ͨѶ��ַ
#define	Box_Press_ADDR				2		//�������帺ѹ��������ͨѶ��ַ
#define	Aerosol_Press_ADDR		3		//���������Ҹ�ѹ��������ͨѶ��ַ
#define	Aerosol_Flow_ADDR			4		//��������������������ͨѶ��ַ

#define	Atomizer_Press_ADDR		5		//��������ѹ����������ͨѶ��ַ
#define	Left_Press_ADDR				6		//������·ѹ����������ͨѶ��ַ
#define	Right_Press_ADDR			7		//������·ѹ����������ͨѶ��ַ


#define	Left_FlowCont_ADDR		8		//������·��������������������ͨѶ��ַ
#define	Right_FlowCont_ADDR		9		//������·��������������������ͨѶ��ַ

#define Aerosol_AQMD3610			10	//��������������ַ

#define Balence_Time					300	//��·ƽ�ⳬʱʱ�� 300s
//λ������,ʵ��51���Ƶ�GPIO���ƹ���
//����ʵ��˼��,�ο�<<CM3Ȩ��ָ��>>������(87ҳ~92ҳ).
//IO�ڲ����궨��
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
//IO�ڵ�ַӳ��
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
 
//IO�ڲ���,ֻ�Ե�һ��IO��!
//ȷ��n��ֵС��16!
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //��� 
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //���� 

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //��� 
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //���� 

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //��� 
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //���� 

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  //��� 
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  //���� 

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  //��� 
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  //����

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  //��� 
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  //����

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //��� 
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //����


//����Ϊ��ຯ��
void WFI_SET(void);		//ִ��WFIָ��
void INTX_DISABLE(void);//�ر������ж�
void INTX_ENABLE(void);	//���������ж�
void MSR_MSP(u32 addr);	//���ö�ջ��ַ


//�����ϵ�����ں����Ŷ�Ӧ��ϵ
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


//���ڽṹ��
struct USARTDataStream  //;�����������Ŀ���
{    
    u16 RIndex;    		//;�������к�
    u16 TIndex;   		//;�������к�
    u16 TLong;     		//;���ͳ���
    u16 TBusy;     		//;����æµ
    u8  RecRun;    		//;�Ƿ��ڽ���
    u8  RBet2Byte; 		//;���������ֽڵ�ʱ����
    u8  RecOrd;    		//;�����յ�����֡
		u8	RecData_Process;//�������ݴ���
		u8  RecStatus;    //;�����յ�����֡��־
    u16 RecLen;    		//;���ճ���
    u8  SedOrd;    		//;�����ڷ���֡		
    u8  *BufAdr;   	//;��������ջ����ָ��
};
extern struct USARTDataStream   U1DS,U2DS,U5DS;   //;Uart���������Ŀ��Ʋ���


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
struct Buf_Type   //;���еĻ������ķ���
{	
		u8 Tx1_1[U1BUFSIZE];  //;����1�Ļ�����    
    u8 Rx1_1[U1BUFSIZE];  
		u8 Rx1_2[U1BUFSIZE];
	
    u8 Tx2_1[U2BUFSIZE];  //;����2�Ļ�����
    u8 Rx2_1[U2BUFSIZE];	//;���ڱ���
    u8 Rx2_2[U2BUFSIZE];

		u8 Tx5_1[U5BUFSIZE];  //;����5�Ļ�����    
    u8 Rx5_1[U5BUFSIZE];
		u8 Rx5_2[U5BUFSIZE];
};
//�����������ݽṹ
struct sensors
{
	s16  Box_Press;//����ѹ��
	s16  Aerosol_Press;//������ѹ��
	s16  Atomizer_Press;//����ѹ��
	s16  Aerosol_Flow;//��������
	s16  Peristaltic_Flow;//�䶯������
	s16  Left_Press;//��·ѹ��
	s16  Left_Flow;//��·����
	s16  Right_Press;//��·ѹ��
	s16  Right_Flow;//��·����
	u16  Pre_Set_Time;//Ԥ��ʱ��
	u16	 Status;//ϵͳ״̬
};
extern struct sensors Sensors;

//���в�������
struct run_setting
{
	u16  Peristaltic_Flow;//�䶯������
	u16  Aerosol_Flow;//��������
	u16  Sampling_Time;//����ʱ��
	u16  Liquid_Time;//��Һʱ��
	u16  Clear_Time;//��ϴʱ��
};
extern struct run_setting Run_Setting;
//ϵͳɱ������
struct  disinfect
{
	u16  Peristaltic_Flow;//�䶯������
	u16  Aerosol_Flow;//��������
	u16  Disclear_Time;//����Һ��ϴʱ��
	u16  Waterclear_Time;//����ˮ��ϴʱ��
	u16  Airclear_Time;//������ϴʱ��
};
extern struct  disinfect Disinfect;

//ȫ�ֱ���
extern u8 Start_Off_Flag;//���ر�־
extern u8 Sec_count;//���ʱ
extern u16 Sys_Status;//ϵͳ״̬��־

//������
union floatto_hex
{
	u32 m;
	float f;
};
#endif
extern u16 AQMD3610_FLOW;
