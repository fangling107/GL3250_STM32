#include "ledbeep.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "includes.h"
#include "24cxx.h"
#include "PWM.h"
#include "crc16.h"

//UCOSIII���������ȼ��û�������ʹ��
//����Щ���ȼ��������UCOSIII��5��ϵͳ�ڲ�����
//���ȼ�0���жϷ������������� OS_IntQTask()
//���ȼ�1��ʱ�ӽ������� OS_TickTask()
//���ȼ�2����ʱ���� OS_TmrTask()
//���ȼ�OS_CFG_PRIO_MAX-2��ͳ������ OS_StatTask()
//���ȼ�OS_CFG_PRIO_MAX-1���������� OS_IdleTask()
//AT24C02��ַ���� 0-9ϵͳ���� 10-19ɱ������

//�������ȼ�
#define START_TASK_PRIO		3
//�����ջ��С	
#define START_STK_SIZE 		256
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);

//�����������ȼ�
#define UARTSenddata_TASK_PRIO		4
//�����ջ��С
#define UARTSenddata_STK_SIZE		256
//������ƿ�
OS_TCB	UartSenddataTaskTCB;
//�����ջ
__align(8) CPU_STK	UARTSenddata_TASK_STK[UARTSenddata_STK_SIZE];
//������
void UartSenddata_task(void *p_arg);


//�������ȼ�
#define UARTRecdata_TASK_PRIO		5
//�����ջ��С
#define UARTRecdata_STK_SIZE		256
//������ƿ�
OS_TCB	UartRecdataTaskTCB;
//�����ջ
__align(8) CPU_STK	UARTRecdata_TASK_STK[UARTRecdata_STK_SIZE];
//������
void UartRecdata_task(void *p_arg);


//�������ȼ�
#define LED0_TASK_PRIO		9
//�����ջ��С	
#define LED0_STK_SIZE 		256
//������ƿ�
OS_TCB Led0TaskTCB;
//�����ջ	
CPU_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *p_arg);


//��ʱ��
OS_TMR 	tmr1;		//��ʱ��1
OS_TMR 	tmr2;		//��ʱ��2
OS_TMR 	tmr3;		//��ʱ��3
OS_TMR 	tmr4;		//��ʱ��4
OS_TMR 	tmr5;		//��ʱ��5
void tmr1_callback(void *p_tmr, void *p_arg); 	//��ʱ��1�ص�����
void tmr2_callback(void *p_tmr, void *p_arg); 	//��ʱ��2�ص�����
void tmr3_callback(void *p_tmr, void *p_arg); 	//��ʱ��3�ص�����
void tmr4_callback(void *p_tmr, void *p_arg); 	//��ʱ��4�ص�����
void tmr5_callback(void *p_tmr, void *p_arg); 	//��ʱ��5�ص�����
//��������
void Read_savedata(void);
void PressOrstatus_Check(void);
//ȫ�ֱ���
u8 Start_Off_Flag=0;//��ͣ��־
u8 Sec_count=0;//���ʱ
u16 Sys_Status=0;//��״̬��־λ

//������
int main(void)
{
	
	OS_ERR err;
	CPU_SR_ALLOC();
	
	delay_init();       //��ʱ��ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //�жϷ�������
	GPIO_Configuration();
	uart_init();    //���ڲ���������	
	LED_BEEP_Init();         //LED��ʼ��	
	TIM3_PWM_Int_Init(200);//0-40khz,75KHZ��Ӧ3ML/MIN����Ӳ�����ƣ�����ֻ�ܵ��Ƶ�40Khz,����ʧ��	
	
	AT24CXX_Init();	
	while(AT24CXX_Check());		//���24c02�Ƿ�����
	Read_savedata();	
	
	OSInit(&err);		//��ʼ��UCOSIII
	OS_CRITICAL_ENTER();//�����ٽ���
	//������ʼ����
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
								(CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
	OS_CRITICAL_EXIT();	//�˳��ٽ���	 
	OSStart(&err);  //����UCOSIII
	while(1);
}

//��ʼ������
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif
	
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
	 //ʹ��ʱ��Ƭ��ת���ȹ���,ʱ��Ƭ����Ϊ1��ϵͳʱ�ӽ��ģ���1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	//������ʱ��1
	OSTmrCreate((OS_TMR		*)&tmr1,		//��ʱ��1
                (CPU_CHAR	*)"tmr1",		//��ʱ������
                (OS_TICK	 )0,			//
                (OS_TICK	 )1,          //1*10=10ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //����ģʽ
                (OS_TMR_CALLBACK_PTR)tmr1_callback,//��ʱ��1�ص�����
                (void	    *)0,			//����Ϊ0
                (OS_ERR	    *)&err);		//���صĴ�����
	//������ʱ��2
	OSTmrCreate((OS_TMR		*)&tmr2,		//��ʱ��2
                (CPU_CHAR	*)"tmr2",		//��ʱ������
                (OS_TICK	 )0,			//
                (OS_TICK	 )100,          //100*10=1s
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //����ģʽ
                (OS_TMR_CALLBACK_PTR)tmr2_callback,//��ʱ��1�ص�����
                (void	    *)0,			//����Ϊ0
                (OS_ERR	    *)&err);		//���صĴ�����
	//������ʱ��3
	OSTmrCreate((OS_TMR		*)&tmr3,		//��ʱ��3
                (CPU_CHAR	*)"tmr3",		//��ʱ������
                (OS_TICK	 )0,			//
                (OS_TICK	 )100,          //100*10=100ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //����ģʽ
                (OS_TMR_CALLBACK_PTR)tmr3_callback,//��ʱ��1�ص�����
                (void	    *)0,			//����Ϊ0
                (OS_ERR	    *)&err);		//���صĴ�����
		//������ʱ��4
	OSTmrCreate((OS_TMR		*)&tmr4,		//��ʱ��4
                (CPU_CHAR	*)"tmr4",		//��ʱ������
                (OS_TICK	 )0,			//
                (OS_TICK	 )25,          //25*10=250ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //����ģʽ
                (OS_TMR_CALLBACK_PTR)tmr4_callback,//��ʱ��4�ص�����
                (void	    *)0,			//����Ϊ0
                (OS_ERR	    *)&err);		//���صĴ�����		
			//������ʱ��5
	OSTmrCreate((OS_TMR		*)&tmr5,		//��ʱ��5
                (CPU_CHAR	*)"tmr5",		//��ʱ������
                (OS_TICK	 )0,			//
                (OS_TICK	 )100,          //100*10=1s
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //����ģʽ
                (OS_TMR_CALLBACK_PTR)tmr5_callback,//��ʱ��4�ص�����
                (void	    *)0,			//����Ϊ0
                (OS_ERR	    *)&err);		//���صĴ�����										
	OSTmrStart(&tmr1,&err);	//������ʱ��1
	OSTmrStart(&tmr2,&err);	//������ʱ��2	
	OSTmrStart(&tmr3,&err);	//������ʱ��3
	OSTmrStart(&tmr4,&err);	//������ʱ��4		
	OSTmrStart(&tmr5,&err);	//������ʱ��5
	OS_CRITICAL_ENTER();	//�����ٽ���

	//����LED����
	OSTaskCreate((OS_TCB 	* )&Led0TaskTCB,		
				 (CPU_CHAR	* )"led0 task", 		
                 (OS_TASK_PTR )led0_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED0_TASK_PRIO,     
                 (CPU_STK   * )&LED0_TASK_STK[0],	
                 (CPU_STK_SIZE)LED0_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED0_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);			 
	//�������ڵ����ݷ�������
	OSTaskCreate((OS_TCB 	* )&UartSenddataTaskTCB,		
							(CPU_CHAR	* )"UART Senddata task", 		
                 (OS_TASK_PTR )UartSenddata_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )UARTSenddata_TASK_PRIO,     	
                 (CPU_STK   * )&UARTSenddata_TASK_STK[0],	
                 (CPU_STK_SIZE)UARTSenddata_STK_SIZE/10,	
                 (CPU_STK_SIZE)UARTSenddata_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);	
								 
	//�������ڵĽ������ݴ�������
	OSTaskCreate((OS_TCB 	* )&UartRecdataTaskTCB,		
								(CPU_CHAR	* )"UART Recdata task", 		
                 (OS_TASK_PTR )UartRecdata_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )UARTRecdata_TASK_PRIO,     	
                 (CPU_STK   * )&UARTRecdata_TASK_STK[0],	
                 (CPU_STK_SIZE)UARTRecdata_STK_SIZE/10,	
                 (CPU_STK_SIZE)UARTRecdata_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);		
							 
	OS_CRITICAL_EXIT();	//�˳��ٽ���								
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//����ʼ����			 
	
}
//��ʱ��1�Ļص�����
//���ڿ��Ʒ��ͺͽ�������֮֡���ʱ����
void tmr1_callback(void *p_tmr, void *p_arg)
{
	if(U1DS.RecRun)   //;����1���ڽ����ֽ�
	{ 
			U1DS.RBet2Byte+=10;      //;��¼����1���������ֽ�֮���ʱ�����
	}
	if(U1DS.TBusy)  				  //;����1����1���ֽ��Ժ����ʱʱ��
    { 
        U1DS.TBusy-=10; 
    }
	if(U2DS.RecRun)   //;����2���ڽ����ֽ�
    { 
        U2DS.RBet2Byte+=10;      //;��¼����2���������ֽ�֮���ʱ�����
    }
	if(U2DS.TBusy)  				  //;����2����1���ֽ��Ժ����ʱʱ��
    { 
        U2DS.TBusy-=10; 
    }
		
	if(U5DS.RecRun)   //;����5���ڽ����ֽ�
	{ 
			U5DS.RBet2Byte+=10;      //;��¼����5���������ֽ�֮���ʱ�����
	}
	if(U5DS.TBusy)  				  //;����5����1���ֽ��Ժ����ʱʱ��
    { 
        U5DS.TBusy-=10; 
    }	
}
//��ʱ��2�Ļص�����
//���ڼ�ʱ
void tmr2_callback(void *p_tmr, void *p_arg)
{	
	if(((Sensors.Status&0x0018)==0x0008||(Sensors.Status&0x8002)==0x0002||(Sensors.Status&0x8020)==0x0020||\
		  (Sensors.Status&0x8100)==0x0100||(Sensors.Status&0x8200)==0x0200||(Sensors.Status&0x8400)==0x0400))	
		Sec_count++;
	if(((Sensors.Status&0x8008)==0x0008||(Sensors.Status&0x8002)==0x0002||(Sensors.Status&0x8020)==0x0020||\
		  (Sensors.Status&0x8100)==0x0100||(Sensors.Status&0x8200)==0x0200||(Sensors.Status&0x8400)==0x0400)&&Sec_count==2)
	{
		Sec_count=0;
		if((Sensors.Status&0x8008)==0x0008)
			Sensors.Status|=0x8008;//��ʼ�������������������Ķ�ʱ��
		else if((Sensors.Status&0x8002)==0x0002)
			Sensors.Status|=0x8002;//���ڽ�����·ƽ�⣬�����������Ķ�ʱ��
		else if((Sensors.Status&0x8020)==0x0020)
			Sensors.Status|=0x8020;//������ϴ��ʱ��
		else if((Sensors.Status&0x8100)==0x0100)
			Sensors.Status|=0x8100;//��������Һɱ����ʱ��
		else if((Sensors.Status&0x8200)==0x0200)
			Sensors.Status|=0x8200;//��������ˮɱ����ʱ��
		else if((Sensors.Status&0x8400)==0x0400)
			Sensors.Status|=0x8400;//��������ɱ����ʱ��
		
	}
	else if(((Sensors.Status&0x8008)==0x8008)&&Sec_count==Run_Setting.Liquid_Time)
	{
		Sec_count=0;
		Sensors.Status&=0x0;
		Sensors.Status|=0x8018;//��ʽ�����У���Һʱ�䵽		
	}
}
//����1�����ݷ��Ϳ��� 1S����һ������
//PLC_Add:������ַ
//Flag:��־ 0����ȡ������������������������ 1�����ÿ�����������
//����������������������������1����������������
//��ַ8����·����������Left_FlowCont_ADDR
//��ַ9����·����������Right_FlowCont_ADDR
//��ַ10������õ�����������
//����������������������Ӧʱ��Ϊ1S���ٶȲ���̫��
void tmr3_callback(void *p_tmr, void *p_arg)
{
	if(Start_Off_Flag==1)//����״̬
	{
		//���ø�������������
		if(U1DS.RecData_Process==0)//������·����������
			Uart1_SendDataMsg(Left_FlowCont_ADDR,1,28.3f);
		else if(U1DS.RecData_Process==1)//������·��������������
			Uart1_SendDataMsg(Right_FlowCont_ADDR,1,28.3f);
		else if(U1DS.RecData_Process==2)//��������������ת��
			Uart1_SendDataMsg(Aerosol_AQMD3610,0,AQMD3610_FLOW);//80% ��Ӧ10L/MIN
		//��ȡ��������������������
		else if(U1DS.RecData_Process==3)
			Uart1_SendDataMsg(Left_FlowCont_ADDR,0,0);//��ȡ��·����������������
		else if(U1DS.RecData_Process==4)
			Uart1_SendDataMsg(Right_FlowCont_ADDR,0,0);//��ȡ��·����������������
		else if(U1DS.RecData_Process==5)
			Uart1_SendDataMsg(Left_FlowCont_ADDR,0,0);//��ȡ��·����������������
	}
 if((Start_Off_Flag==0&&U1DS.RecData_Process>0)||((Sensors.Status&0x0040)==0x0040))//����ֹͣ����������ɣ���Ҫ�ر�����������������������
	{
			if(U1DS.RecData_Process>3)//������·����������
				Uart1_SendDataMsg(Left_FlowCont_ADDR,1,0);//�ر���·����������
			else if(U1DS.RecData_Process==1)//�ر���·����������
				Uart1_SendDataMsg(Right_FlowCont_ADDR,1,0);
			else if(U1DS.RecData_Process==2)//��������������ת��
				Uart1_SendDataMsg(Aerosol_AQMD3610,0,0);//0 stop
			else if(U1DS.RecData_Process==3)
			{
				U1DS.RecData_Process=0;
				Out5=1;//��·ƽ���ŷ���
				Out6=1;//������ŷ���
				Out7=1;//���������	
				Out8=1;//���������
				Sensors.Left_Flow=0;
				Sensors.Right_Flow=0;
				Sensors.Aerosol_Flow=0;	
			}
	}
}
//����5�����ݷ��Ϳ���
//PLC_Add:������ַ
//��������ѹ����������2��ѹ�������1��������
//��ַ2������ѹ�����	Box_Press_ADDR		      200ms
//��ַ3��������ѹ����� Aerosol_Press_ADDR		200ms
//��ַ4�������������Aerosol_Flow_ADDR					10ms
//��ַ5��������·ѹ�� Atomizer_Press_ADDR			1s					
//��ַ6����·ѹ�������� Left_Press_ADDR					1s
//��ַ7����·ѹ�������� Right_Press_ADDR				1s

void tmr4_callback(void *p_tmr, void *p_arg)
{	
	if(U5DS.RecData_Process==0)
		Uart5_SendDataMsg(Box_Press_ADDR);//��ȡ�����ѹ����������
	else if(U5DS.RecData_Process==1)
		Uart5_SendDataMsg(Aerosol_Press_ADDR);//��ȡ�����Ҳ�ѹ����������
	else if(U5DS.RecData_Process==2)
		Uart5_SendDataMsg(Aerosol_Flow_ADDR);//��ȡ���������������	
	else if(U5DS.RecData_Process==6)	
		Uart5_SendDataMsg(Box_Press_ADDR);//��ȡ�����ѹ����������	
}
void tmr5_callback(void *p_tmr, void *p_arg)
{	
	if(U5DS.RecData_Process==3)	
		Uart5_SendDataMsg(Atomizer_Press_ADDR);//��ȡ�����ѹ������������
	else if(U5DS.RecData_Process==4)	
		Uart5_SendDataMsg(Left_Press_ADDR);//��ȡ��·ѹ������������
	else if(U5DS.RecData_Process==5)	
		Uart5_SendDataMsg(Right_Press_ADDR);//��ȡ��·ѹ������������	
}
//led0������
void led0_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	while(1)
	{
		PressOrstatus_Check();
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); 		
	}
}
//�������ݷ�������
void UartSenddata_task(void *p_arg)
{
	OS_ERR err;
	
	CPU_SR_ALLOC();
	p_arg = p_arg;	
	while(1)
	{	
		OS_CRITICAL_ENTER();	//�����ٽ���
		USART_Com();		
		OS_CRITICAL_EXIT();	//�˳��ٽ���		
		OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_HMSM_STRICT,&err); 
		
	}
}
//���ڽ������ݴ���
void UartRecdata_task(void *p_arg)
{
	OS_ERR err;
	
	CPU_SR_ALLOC();
	p_arg = p_arg;	
	while(1)
	{	
		OS_CRITICAL_ENTER();	//�����ٽ���
		ZD_Port();	
		DataProcess();
		OS_CRITICAL_EXIT();	//�˳��ٽ���		
		OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_HMSM_STRICT,&err);		
	}
}


void Read_savedata(void)
{
	u8 save_data[20]={0};
	//��ȡ����Ĳ���20���ֽ�
	AT24CXX_Read(save_data,0,20);
	if(save_data[0]==0xff)//��û�б����������ѡ��Ĭ�ϲ���
	{
		Run_Setting.Peristaltic_Flow=5;
		Run_Setting.Aerosol_Flow		=100;
		Run_Setting.Sampling_Time		=120;
		Run_Setting.Liquid_Time			=60;
		Run_Setting.Clear_Time			=60;
	}
	else//��������������������ݸ��ṹ��
	{
		Run_Setting.Peristaltic_Flow=(save_data[0]<<8)+save_data[1];
		Run_Setting.Aerosol_Flow		=(save_data[2]<<8)+save_data[3];
		Run_Setting.Sampling_Time		=(save_data[4]<<8)+save_data[5];
		Run_Setting.Liquid_Time			=(save_data[6]<<8)+save_data[7];
		Run_Setting.Clear_Time			=(save_data[8]<<8)+save_data[9];
	}
	if(save_data[10]==0xff)//��û�б����������ѡ��Ĭ�ϲ���
	{
		Disinfect.Peristaltic_Flow=2000;
		Disinfect.Aerosol_Flow		=100;
		Disinfect.Disclear_Time		=300;
		Disinfect.Waterclear_Time	=300;
		Disinfect.Airclear_Time		=300;
	}
	else
	{
		Disinfect.Peristaltic_Flow=(save_data[10]<<8)+save_data[11];
		Disinfect.Aerosol_Flow		=(save_data[12]<<8)+save_data[13];
		Disinfect.Disclear_Time		=(save_data[14]<<8)+save_data[15];
		Disinfect.Waterclear_Time	=(save_data[16]<<8)+save_data[17];
		Disinfect.Airclear_Time		=(save_data[18]<<8)+save_data[19];
	}
	//����ϵͳ��ʼ״̬
	Sensors.Status=1;//Ĭ��Ϊ����״̬
}

//ѹ����״̬���
void PressOrstatus_Check(void)
{	

	//����ѹ���������ļ��
	//����ѹ��50~200pa
	//������ѹ��-50~-200pa
	//������·����27.8-28.8
	//��������0-12
	if(Start_Off_Flag==1&&((Sensors.Status&0x0002)==0x0002)&&\
					(Sensors.Box_Press>50&&Sensors.Box_Press<200)&&\
				  (Sensors.Aerosol_Press>50&&Sensors.Aerosol_Press<200)&&\
				  (Sensors.Left_Flow>275&&Sensors.Left_Flow<290)&&\
				  (Sensors.Right_Flow>275&&Sensors.Right_Flow<290)&&\
				  (Sensors.Aerosol_Flow<120))//��·ƽ������		
			{
				Sensors.Status&=0x0;
				Sensors.Status|=0x0004;//��·ƽ�����				
			}
			
// 	if(Start_Off_Flag==1&&((Sensors.Status&0x0002)==0x0002))//test ��·ƽ������		
// 			{
// 				Sensors.Status&=0x0;
// 				Sensors.Status|=0x0004;//��·ƽ�����					
// 			}
			

}
