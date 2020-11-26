#include "ledbeep.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "includes.h"
#include "24cxx.h"
#include "PWM.h"
#include "crc16.h"

//UCOSIII中以下优先级用户程序不能使用
//将这些优先级分配给了UCOSIII的5个系统内部任务
//优先级0：中断服务服务管理任务 OS_IntQTask()
//优先级1：时钟节拍任务 OS_TickTask()
//优先级2：定时任务 OS_TmrTask()
//优先级OS_CFG_PRIO_MAX-2：统计任务 OS_StatTask()
//优先级OS_CFG_PRIO_MAX-1：空闲任务 OS_IdleTask()
//AT24C02地址分配 0-9系统参数 10-19杀毒参数

//任务优先级
#define START_TASK_PRIO		3
//任务堆栈大小	
#define START_STK_SIZE 		256
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);

//串口任务优先级
#define UARTSenddata_TASK_PRIO		4
//任务堆栈大小
#define UARTSenddata_STK_SIZE		256
//任务控制块
OS_TCB	UartSenddataTaskTCB;
//任务堆栈
__align(8) CPU_STK	UARTSenddata_TASK_STK[UARTSenddata_STK_SIZE];
//任务函数
void UartSenddata_task(void *p_arg);


//任务优先级
#define UARTRecdata_TASK_PRIO		5
//任务堆栈大小
#define UARTRecdata_STK_SIZE		256
//任务控制块
OS_TCB	UartRecdataTaskTCB;
//任务堆栈
__align(8) CPU_STK	UARTRecdata_TASK_STK[UARTRecdata_STK_SIZE];
//任务函数
void UartRecdata_task(void *p_arg);


//任务优先级
#define LED0_TASK_PRIO		9
//任务堆栈大小	
#define LED0_STK_SIZE 		256
//任务控制块
OS_TCB Led0TaskTCB;
//任务堆栈	
CPU_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *p_arg);


//定时器
OS_TMR 	tmr1;		//定时器1
OS_TMR 	tmr2;		//定时器2
OS_TMR 	tmr3;		//定时器3
OS_TMR 	tmr4;		//定时器4
OS_TMR 	tmr5;		//定时器5
void tmr1_callback(void *p_tmr, void *p_arg); 	//定时器1回调函数
void tmr2_callback(void *p_tmr, void *p_arg); 	//定时器2回调函数
void tmr3_callback(void *p_tmr, void *p_arg); 	//定时器3回调函数
void tmr4_callback(void *p_tmr, void *p_arg); 	//定时器4回调函数
void tmr5_callback(void *p_tmr, void *p_arg); 	//定时器5回调函数
//函数声明
void Read_savedata(void);
void PressOrstatus_Check(void);
//全局变量
u8 Start_Off_Flag=0;//启停标志
u8 Sec_count=0;//秒计时
u16 Sys_Status=0;//各状态标志位

//主函数
int main(void)
{
	
	OS_ERR err;
	CPU_SR_ALLOC();
	
	delay_init();       //延时初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //中断分组配置
	GPIO_Configuration();
	uart_init();    //串口波特率设置	
	LED_BEEP_Init();         //LED初始化	
	TIM3_PWM_Int_Init(200);//0-40khz,75KHZ对应3ML/MIN。受硬件限制，波形只能调制到40Khz,否则失真	
	
	AT24CXX_Init();	
	while(AT24CXX_Check());		//检测24c02是否正常
	Read_savedata();	
	
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
								(CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);  //开启UCOSIII
	while(1);
}

//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif
	
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	//创建定时器1
	OSTmrCreate((OS_TMR		*)&tmr1,		//定时器1
                (CPU_CHAR	*)"tmr1",		//定时器名字
                (OS_TICK	 )0,			//
                (OS_TICK	 )1,          //1*10=10ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //周期模式
                (OS_TMR_CALLBACK_PTR)tmr1_callback,//定时器1回调函数
                (void	    *)0,			//参数为0
                (OS_ERR	    *)&err);		//返回的错误码
	//创建定时器2
	OSTmrCreate((OS_TMR		*)&tmr2,		//定时器2
                (CPU_CHAR	*)"tmr2",		//定时器名字
                (OS_TICK	 )0,			//
                (OS_TICK	 )100,          //100*10=1s
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //周期模式
                (OS_TMR_CALLBACK_PTR)tmr2_callback,//定时器1回调函数
                (void	    *)0,			//参数为0
                (OS_ERR	    *)&err);		//返回的错误码
	//创建定时器3
	OSTmrCreate((OS_TMR		*)&tmr3,		//定时器3
                (CPU_CHAR	*)"tmr3",		//定时器名字
                (OS_TICK	 )0,			//
                (OS_TICK	 )100,          //100*10=100ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //周期模式
                (OS_TMR_CALLBACK_PTR)tmr3_callback,//定时器1回调函数
                (void	    *)0,			//参数为0
                (OS_ERR	    *)&err);		//返回的错误码
		//创建定时器4
	OSTmrCreate((OS_TMR		*)&tmr4,		//定时器4
                (CPU_CHAR	*)"tmr4",		//定时器名字
                (OS_TICK	 )0,			//
                (OS_TICK	 )25,          //25*10=250ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //周期模式
                (OS_TMR_CALLBACK_PTR)tmr4_callback,//定时器4回调函数
                (void	    *)0,			//参数为0
                (OS_ERR	    *)&err);		//返回的错误码		
			//创建定时器5
	OSTmrCreate((OS_TMR		*)&tmr5,		//定时器5
                (CPU_CHAR	*)"tmr5",		//定时器名字
                (OS_TICK	 )0,			//
                (OS_TICK	 )100,          //100*10=1s
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //周期模式
                (OS_TMR_CALLBACK_PTR)tmr5_callback,//定时器4回调函数
                (void	    *)0,			//参数为0
                (OS_ERR	    *)&err);		//返回的错误码										
	OSTmrStart(&tmr1,&err);	//开启定时器1
	OSTmrStart(&tmr2,&err);	//开启定时器2	
	OSTmrStart(&tmr3,&err);	//开启定时器3
	OSTmrStart(&tmr4,&err);	//开启定时器4		
	OSTmrStart(&tmr5,&err);	//开启定时器5
	OS_CRITICAL_ENTER();	//进入临界区

	//创建LED任务
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
	//创建串口的数据发送任务
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
								 
	//创建串口的接收数据处理任务
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
							 
	OS_CRITICAL_EXIT();	//退出临界区								
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
	
}
//定时器1的回调函数
//用于控制发送和接受两个帧之间的时间间隔
void tmr1_callback(void *p_tmr, void *p_arg)
{
	if(U1DS.RecRun)   //;串口1正在接收字节
	{ 
			U1DS.RBet2Byte+=10;      //;记录串口1接收两个字节之间的时间距离
	}
	if(U1DS.TBusy)  				  //;串口1发送1个字节以后的延时时间
    { 
        U1DS.TBusy-=10; 
    }
	if(U2DS.RecRun)   //;串口2正在接收字节
    { 
        U2DS.RBet2Byte+=10;      //;记录串口2接收两个字节之间的时间距离
    }
	if(U2DS.TBusy)  				  //;串口2发送1个字节以后的延时时间
    { 
        U2DS.TBusy-=10; 
    }
		
	if(U5DS.RecRun)   //;串口5正在接收字节
	{ 
			U5DS.RBet2Byte+=10;      //;记录串口5接收两个字节之间的时间距离
	}
	if(U5DS.TBusy)  				  //;串口5发送1个字节以后的延时时间
    { 
        U5DS.TBusy-=10; 
    }	
}
//定时器2的回调函数
//用于计时
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
			Sensors.Status|=0x8008;//开始进行试验启动触摸屏的定时器
		else if((Sensors.Status&0x8002)==0x0002)
			Sensors.Status|=0x8002;//正在进行气路平衡，启动触摸屏的定时器
		else if((Sensors.Status&0x8020)==0x0020)
			Sensors.Status|=0x8020;//开启清洗定时器
		else if((Sensors.Status&0x8100)==0x0100)
			Sensors.Status|=0x8100;//开启消毒液杀毒定时器
		else if((Sensors.Status&0x8200)==0x0200)
			Sensors.Status|=0x8200;//开启蒸馏水杀毒定时器
		else if((Sensors.Status&0x8400)==0x0400)
			Sensors.Status|=0x8400;//开启空气杀毒定时器
		
	}
	else if(((Sensors.Status&0x8008)==0x8008)&&Sec_count==Run_Setting.Liquid_Time)
	{
		Sec_count=0;
		Sensors.Status&=0x0;
		Sensors.Status|=0x8018;//正式试验中，供液时间到		
	}
}
//串口1的数据发送控制 1S发送一条数据
//PLC_Add:器件地址
//Flag:标志 0：读取气体质量流量控制器的流量 1：设置控制器的流量
//控制两个气体质量流量控制器和1个喷雾器的驱动器
//地址8：左路流量控制器Left_FlowCont_ADDR
//地址9：右路流量控制器Right_FlowCont_ADDR
//地址10：喷雾泵的驱动控制器
//气体质量流量控制器的响应时间为1S，速度不能太快
void tmr3_callback(void *p_tmr, void *p_arg)
{
	if(Start_Off_Flag==1)//启动状态
	{
		//设置各个控制器流量
		if(U1DS.RecData_Process==0)//设置左路流量控制器
			Uart1_SendDataMsg(Left_FlowCont_ADDR,1,28.3f);
		else if(U1DS.RecData_Process==1)//设置右路流量控制器流量
			Uart1_SendDataMsg(Right_FlowCont_ADDR,1,28.3f);
		else if(U1DS.RecData_Process==2)//设置喷雾驱动器转速
			Uart1_SendDataMsg(Aerosol_AQMD3610,0,AQMD3610_FLOW);//80% 对应10L/MIN
		//读取两个质量控制器的流量
		else if(U1DS.RecData_Process==3)
			Uart1_SendDataMsg(Left_FlowCont_ADDR,0,0);//读取左路流量控制器的流量
		else if(U1DS.RecData_Process==4)
			Uart1_SendDataMsg(Right_FlowCont_ADDR,0,0);//读取右路流量控制器的流量
		else if(U1DS.RecData_Process==5)
			Uart1_SendDataMsg(Left_FlowCont_ADDR,0,0);//读取左路流量控制器的流量
	}
 if((Start_Off_Flag==0&&U1DS.RecData_Process>0)||((Sensors.Status&0x0040)==0x0040))//试验停止或者试验完成，需要关闭流量控制器和喷雾驱动器
	{
			if(U1DS.RecData_Process>3)//设置左路流量控制器
				Uart1_SendDataMsg(Left_FlowCont_ADDR,1,0);//关闭左路流量控制器
			else if(U1DS.RecData_Process==1)//关闭右路流量控制器
				Uart1_SendDataMsg(Right_FlowCont_ADDR,1,0);
			else if(U1DS.RecData_Process==2)//设置喷雾驱动器转速
				Uart1_SendDataMsg(Aerosol_AQMD3610,0,0);//0 stop
			else if(U1DS.RecData_Process==3)
			{
				U1DS.RecData_Process=0;
				Out5=1;//气路平衡电磁阀关
				Out6=1;//采样电磁阀关
				Out7=1;//采样风机关	
				Out8=1;//采样风机关
				Sensors.Left_Flow=0;
				Sensors.Right_Flow=0;
				Sensors.Aerosol_Flow=0;	
			}
	}
}
//串口5的数据发送控制
//PLC_Add:器件地址
//控制三个压力传感器和2个压差传感器和1个流量计
//地址2：柜体压差传感器	Box_Press_ADDR		      200ms
//地址3：气雾室压差传感器 Aerosol_Press_ADDR		200ms
//地址4：喷雾口流量计Aerosol_Flow_ADDR					10ms
//地址5：雾化器气路压力 Atomizer_Press_ADDR			1s					
//地址6：左路压力传感器 Left_Press_ADDR					1s
//地址7：右路压力传感器 Right_Press_ADDR				1s

void tmr4_callback(void *p_tmr, void *p_arg)
{	
	if(U5DS.RecData_Process==0)
		Uart5_SendDataMsg(Box_Press_ADDR);//读取柜体差压变送器数据
	else if(U5DS.RecData_Process==1)
		Uart5_SendDataMsg(Aerosol_Press_ADDR);//读取气雾室差压变送器数据
	else if(U5DS.RecData_Process==2)
		Uart5_SendDataMsg(Aerosol_Flow_ADDR);//读取喷雾口流量计数据	
	else if(U5DS.RecData_Process==6)	
		Uart5_SendDataMsg(Box_Press_ADDR);//读取柜体差压变送器数据	
}
void tmr5_callback(void *p_tmr, void *p_arg)
{	
	if(U5DS.RecData_Process==3)	
		Uart5_SendDataMsg(Atomizer_Press_ADDR);//读取喷雾口压力传感器数据
	else if(U5DS.RecData_Process==4)	
		Uart5_SendDataMsg(Left_Press_ADDR);//读取左路压力传感器数据
	else if(U5DS.RecData_Process==5)	
		Uart5_SendDataMsg(Right_Press_ADDR);//读取右路压力传感器数据	
}
//led0任务函数
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
//串口数据发送数据
void UartSenddata_task(void *p_arg)
{
	OS_ERR err;
	
	CPU_SR_ALLOC();
	p_arg = p_arg;	
	while(1)
	{	
		OS_CRITICAL_ENTER();	//进入临界区
		USART_Com();		
		OS_CRITICAL_EXIT();	//退出临界区		
		OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_HMSM_STRICT,&err); 
		
	}
}
//串口接收数据处理
void UartRecdata_task(void *p_arg)
{
	OS_ERR err;
	
	CPU_SR_ALLOC();
	p_arg = p_arg;	
	while(1)
	{	
		OS_CRITICAL_ENTER();	//进入临界区
		ZD_Port();	
		DataProcess();
		OS_CRITICAL_EXIT();	//退出临界区		
		OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_HMSM_STRICT,&err);		
	}
}


void Read_savedata(void)
{
	u8 save_data[20]={0};
	//读取保存的参数20个字节
	AT24CXX_Read(save_data,0,20);
	if(save_data[0]==0xff)//还没有保存过参数，选择默认参数
	{
		Run_Setting.Peristaltic_Flow=5;
		Run_Setting.Aerosol_Flow		=100;
		Run_Setting.Sampling_Time		=120;
		Run_Setting.Liquid_Time			=60;
		Run_Setting.Clear_Time			=60;
	}
	else//保存过参数，将参数传递给结构体
	{
		Run_Setting.Peristaltic_Flow=(save_data[0]<<8)+save_data[1];
		Run_Setting.Aerosol_Flow		=(save_data[2]<<8)+save_data[3];
		Run_Setting.Sampling_Time		=(save_data[4]<<8)+save_data[5];
		Run_Setting.Liquid_Time			=(save_data[6]<<8)+save_data[7];
		Run_Setting.Clear_Time			=(save_data[8]<<8)+save_data[9];
	}
	if(save_data[10]==0xff)//还没有保存过参数，选择默认参数
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
	//设置系统初始状态
	Sensors.Status=1;//默认为空闲状态
}

//压力和状态监测
void PressOrstatus_Check(void)
{	

	//各个压力和流量的检测
	//箱体压力50~200pa
	//气雾室压力-50~-200pa
	//采样气路流量27.8-28.8
	//喷雾流量0-12
	if(Start_Off_Flag==1&&((Sensors.Status&0x0002)==0x0002)&&\
					(Sensors.Box_Press>50&&Sensors.Box_Press<200)&&\
				  (Sensors.Aerosol_Press>50&&Sensors.Aerosol_Press<200)&&\
				  (Sensors.Left_Flow>275&&Sensors.Left_Flow<290)&&\
				  (Sensors.Right_Flow>275&&Sensors.Right_Flow<290)&&\
				  (Sensors.Aerosol_Flow<120))//气路平衡正常		
			{
				Sensors.Status&=0x0;
				Sensors.Status|=0x0004;//气路平衡完成				
			}
			
// 	if(Start_Off_Flag==1&&((Sensors.Status&0x0002)==0x0002))//test 气路平衡正常		
// 			{
// 				Sensors.Status&=0x0;
// 				Sensors.Status|=0x0004;//气路平衡完成					
// 			}
			

}
