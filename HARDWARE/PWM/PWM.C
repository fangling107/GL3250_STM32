#include "PWM.h"


//��ʱ��3 PWM���
void TIM3_PWM_Int_Init(u32 arr)
{
	u32 period;
	GPIO_InitTypeDef GPIO_InitStructure;//����ṹ��
	TIM_OCInitTypeDef  TIM_OCInitStructure;//����ṹ��
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;//�����ʼ���ṹ��

	/*������ʱ��TIM3 ��ʱ��*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʹ�ܶ�ʱ��3ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB  | RCC_APB2Periph_AFIO, ENABLE);  //ʹ��GPIO�����AFIO���ù���ģ��ʱ��
	//TIM3 PB5
	GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //ѡ��Timer3������ӳ��
	//ѡ��ʱ��3��ͨ��2��ΪPWM���������TIM3_CH2->PB5    GPIOB.5
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //TIM_CH2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //�������칦��
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ������

	//��ʼ��TIM3��ʱ������
	period = SystemCoreClock / ((arr-1)*72) ;
	TIM_TimeBaseStructure.TIM_Period = period; //�Զ���װ�ؼĴ�����ֵ
	TIM_TimeBaseStructure.TIM_Prescaler =72-1; //TIMXԤ��Ƶ��ֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //ʱ�ӷָ�
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //���ϼ���
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //�������Ϲ��ܶԶ�ʱ�����г�ʼ��

	//PWM����
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//ѡ��ʱ��ģʽ��TIM������ȵ���ģʽ2  PWMģʽ2:CNT>CCRʱ�����Ч
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;//����Ƚϼ��Ը�
	TIM_OCInitStructure.TIM_Pulse = period / 5; //���ô�װ�벶��ȽϼĴ���������ֵ
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);//���ݽṹ����Ϣ���г�ʼ��
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);  //ʹ�ܶ�ʱ��TIM2��CCR2�ϵ�Ԥװ��ֵ
	//����PWM��ռ�ձ�
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	//ʹ�ܶ�ʱ��
	//TIM_Cmd(TIM3, ENABLE);  //ʹ�ܶ�ʱ��TIM3
}