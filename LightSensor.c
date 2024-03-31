#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Beep.h"
#include "OLED.h"

//_Bool WARNNING_FLAG = 0;

/**
  * 函    数：LightSensor_Init
  * 参    数：无
  * 返 回 值：无
  * 功    能：光敏传感器初始化
  */
void LightSensor_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
	
	/*GPIO初始化*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB13引脚初始化为上拉输入（1）
}

/**
  * 函    数：LightSensor_Get
  * 参    数：无
  * 返 回 值：光敏传感器输出的高低电平，范围：0/1
  * 功    能：获取当前光敏传感器输出的高低电平
  */
uint8_t LightSensor_Get(void)
{
	return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13);			//返回PB13输入寄存器的状态
}


/**（可删主函数内完成或可主函数调用)
  * 函    数：LightSensor_Beep
  * 参    数：无
  * 返 回 值：无
  * 功    能：光敏传感器控制蜂鸣器，有光照时蜂鸣器鸣叫
  */
void LightSensor_Beep(void)
{
	/*模块初始化*/
	//Beep_Init();			//蜂鸣器初始化
	//LightSensor_Init();		//光敏传感器初始化
	//while (1)
	//{
		if (LightSensor_Get() == 0 )		//如果当前光敏输出0（有光照、DO口输出0）
		{
			Beep_On();				//蜂鸣器开启
			//WARNNING_FLAG = 1;
		 }
		else							
		{
			Beep_Off();				//蜂鸣器关闭
			//WARNNING_FLAG = 0;
			//GPIO_ResetBits(GPIOB, GPIO_Pin_13);
		 }
	//}
}
