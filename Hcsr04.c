#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "sys.h"
#include "Hcsr04.h"

//超声波硬件接口定义
#define HCSR04_PORT     GPIOB
#define HCSR04_CLK      RCC_APB2Periph_GPIOB
#define HCSR04_TRIG     GPIO_Pin_11
#define HCSR04_ECHO     GPIO_Pin_10

#define TRIG_Send  PBout(11)
#define ECHO_Reci  PBin(10)

uint16_t msHcCount = 0;

/**
  * 函    数：Hcsr04_Init
  * 参    数：无
  * 返 回 值：无
  * 功    能：GPIO、定时器初始化及NVIC初始化
  */
void Hcsr04_Init(void)
{  
	//定义结构体
    GPIO_InitTypeDef GPIO_InitStructure;
		TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
		NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//APB2使能
	
    //PB11口初始化
    GPIO_InitStructure.GPIO_Pin =HCSR04_TRIG;      
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50Mhz
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    //PB11设置为推挽输出
    GPIO_Init(HCSR04_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(HCSR04_PORT,HCSR04_TRIG);			//清除当前输出状态
    //PB10口初始化 
    GPIO_InitStructure.GPIO_Pin = HCSR04_ECHO;     
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//PB10设置为浮空输入
    GPIO_Init(HCSR04_PORT, &GPIO_InitStructure);  
    GPIO_ResetBits(HCSR04_PORT,HCSR04_ECHO);    
     
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);   //定时器时钟使能
    
	  //定时器4初始化设置
    TIM_TimeBaseStructure.TIM_Period = (1000-1);				//设置自动重装载寄存器的值
    TIM_TimeBaseStructure.TIM_Prescaler =(72-1); 				//设置时钟预分频器的值-1M
    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;		
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数模式
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);          	
        
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);     		//清除标志位
    TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);  		//使能TIM4  
    
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);		//配置中断优先级分组
		NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;     //中断通道为 TIM4 的中断       
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //设置中断的抢占优先级,设置为 1，次高优先级
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;         //设置中断的子优先级,1表示次高优先级。 
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;     //使能中断通道
		NVIC_Init(&NVIC_InitStructure);		
	
    TIM_Cmd(TIM4,DISABLE);     //失能TIM4
}

/**
  * 函    数：OpenTimer4_Hc
  * 参    数：无
  * 返 回 值：无
  * 功    能：打开定时器4
  */
static void OpenTimer4_Hc(void)  
{
   TIM_SetCounter(TIM4,0);
   msHcCount = 0;
   TIM_Cmd(TIM4, ENABLE); 
}


/**
  * 函    数：CloseTimer4_Hc
  * 参    数：无
  * 返 回 值：无
  * 功    能：关闭定时器4
  */
static void CloseTimer4_Hc(void)    
{
   TIM_Cmd(TIM4, DISABLE); 
}


/**
  * 函    数：TIM4_IRQHandler
  * 参    数：无
  * 返 回 值：无
  * 功    能：定时器TIM4中断服务程序
  */
void TIM4_IRQHandler(void)  
{
   if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  
   {
       TIM_ClearITPendingBit(TIM4, TIM_IT_Update); 
       msHcCount++;
   }
}

 
/**
  * 函    数：GetEchoTimer
  * 参    数：无
  * 返 回 值：int类型的计数器值t
  * 功    能：通过定时器4获取计数器值
  */
uint32_t GetEchoTimer(void)
{
   uint32_t t = 0;
   t = msHcCount*1000;          //时间转换为微秒
   t += TIM_GetCounter(TIM4);   //获取TIM4的计数器值
   TIM4->CNT = 0;  				//清空TIM4的CNT的值，便于再次调用
   delay_ms(50);				//延时50ms
   return t;					//返回计数值t
}
 

/**
  * 函    数：Hcsr04_GetLength
  * 参    数：无
  * 返 回 值：float类型的距离值LengthTemp
  * 功    能：通过定时器4计数器值推算距离
  */
float Hcsr04_GetLength(void)
{
   uint32_t t = 0;
   int i = 0;
   float LengthTemp = 0;
   float sum = 0;
   while(i!=5)
   {
      TRIG_Send = 1;    		//IO口PB11输出一个高电平  
      delay_us(20);					//高电平持续20us
      TRIG_Send = 0;			  //IO口PB11输出一个低电平
      while(ECHO_Reci == 0);      
      OpenTimer4_Hc();        
      i = i + 1;
      while(ECHO_Reci == 1);
      CloseTimer4_Hc();        
      t = GetEchoTimer();        
      LengthTemp = ((float)t/58.0);//计算单次的距离
      sum = LengthTemp + sum ;
    }
    LengthTemp = sum/5.0;    //求5次的平均值
    return LengthTemp;
}

