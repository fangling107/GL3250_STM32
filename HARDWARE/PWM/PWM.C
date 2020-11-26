#include "PWM.h"


//定时器3 PWM输出
void TIM3_PWM_Int_Init(u32 arr)
{
	u32 period;
	GPIO_InitTypeDef GPIO_InitStructure;//定义结构体
	TIM_OCInitTypeDef  TIM_OCInitStructure;//定义结构体
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;//定义初始化结构体

	/*开启定时器TIM3 的时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //使能定时器3时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB  | RCC_APB2Periph_AFIO, ENABLE);  //使能GPIO外设和AFIO复用功能模块时钟
	//TIM3 PB5
	GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //选择Timer3部分重映像
	//选择定时器3的通道2作为PWM的输出引脚TIM3_CH2->PB5    GPIOB.5
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //TIM_CH2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化引脚

	//初始化TIM3定时器设置
	period = SystemCoreClock / ((arr-1)*72) ;
	TIM_TimeBaseStructure.TIM_Period = period; //自动重装载寄存器的值
	TIM_TimeBaseStructure.TIM_Prescaler =72-1; //TIMX预分频的值
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //时钟分割
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //向上计数
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据以上功能对定时器进行初始化

	//PWM设置
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//选择定时器模式，TIM脉冲宽度调制模式2  PWM模式2:CNT>CCR时输出有效
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;//输出比较极性高
	TIM_OCInitStructure.TIM_Pulse = period / 5; //设置待装入捕获比较寄存器的脉冲值
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);//根据结构体信息进行初始化
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能定时器TIM2在CCR2上的预装载值
	//调节PWM的占空比
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	//使能定时器
	//TIM_Cmd(TIM3, ENABLE);  //使能定时器TIM3
}
