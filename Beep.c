#include"stm32f10x.h"
#include "Delay.h"
//Buzzer
/**
  * 函    数：Beep_Init
  * 参    数：无
  * 返 回 值：无
  * 功    能：初始化蜂鸣器
  */
void Beep_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;					//定义结构体变量
	
		/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//开启GPIOB的时钟
															//使用各个外设前必须开启时钟，否则对外设的操作无效
	
	/*GPIO初始化*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//GPIO模式，赋值为推挽输出模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;				  //GPIO引脚，赋值为第12号引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//GPIO速度，赋值为50MHz
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将赋值后的结构体变量传递给GPIO_Init函数
															//函数内部会自动根据结构体的参数配置相应寄存器
															//实现GPIOB的初始化
	GPIO_SetBits(GPIOB, GPIO_Pin_12);								//将PB12引脚初始化为高电平

}

/**
  * 函    数：Beep_On
  * 参    数：无
  * 返 回 值：无
  * 功    能：初始化模块，使蜂鸣器发声
  */

void Beep_On()
{
	/*模块初始化*/
		//Beep_Init();			//BEEP蜂鸣器初始化
	  //while (1)
			//{
				GPIO_ResetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为低电平，蜂鸣器鸣叫
				delay_ms(500);							//延时500ms
				GPIO_SetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为高电平，蜂鸣器停止
				delay_ms(300);							//延时300ms
				GPIO_ResetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为低电平，蜂鸣器鸣叫
				delay_ms(500);							//延时500ms
				GPIO_SetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为高电平，蜂鸣器停止
				delay_ms(300);							//延时300ms
			//}
}


/**
  * 函    数：Beep_Off
  * 参    数：无
  * 返 回 值：无
  * 功    能：初始化模块，蜂鸣器不发声
  */

void Beep_Off()
{
	/*模块初始化*/
		//Beep_Init();			//BEEP蜂鸣器初始化
	  //while (1)
		//	{
				GPIO_SetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为高电平，蜂鸣器停止
				//Delay_ms(700);							//延时700ms
		//}
}
