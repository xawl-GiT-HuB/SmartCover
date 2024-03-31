#include "stm32f10x.h"                  // Device header
#include "Delay.h"


#define CAL_PPM  10  // 校准环境中PPM值
#define RL	     10  // RL阻值
#define R0	     26  // R0阻值
#define SMOG_READ_TIMES 30  //读取ADC的次数


/**
  * 函    数：MQ_AD_Init
  * 参    数：无
  * 返 回 值：无
  * 功    能：Mq-2 初始化
  */
void MQ_AD_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;						//定义结构体变量	
	
	/*引脚和ADC的时钟使能*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//开启ADC1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
	
	/*设置ADC时钟*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);					//选择时钟6分频，ADCCLK = 72MHz / 6 = 12MHz
	
	/*GPIO初始化，配置引脚为模拟功能模式*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				   //Mq-2 AOUT端接系统板A0口
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		   //模拟功能模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  //GPIO速度设置为50Mhz
	GPIO_Init(GPIOA, &GPIO_InitStructure);					   //将PA0引脚初始化为模拟输入
	
	/*规则组通道配置*/
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5);		//配置ADC1的采样通道0放入规则组序列1的位置
	
	/*ADC1的初始化*/
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;		  //模式，选择独立模式，即单独使用ADC1
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//数据对齐，选择右对齐
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//使用软件触发，不需要外部触发
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;			//连续转换，失能，连续转换规则组序列
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;						//扫描模式，失能，只转换规则组的序列1这一个位置
	ADC_InitStructure.ADC_NbrOfChannel = 1;									//通道数，为1，仅在扫描模式下，才需要指定大于1的数，在非扫描模式下，只能是1
	ADC_Init(ADC1, &ADC_InitStructure);											//将结构体变量交给ADC_Init，配置ADC1
	
	ADC_ITConfig(ADC1,ADC_IT_EOC,ENABLE);
	
	/*ADC使能*/
	ADC_Cmd(ADC1, ENABLE);									//使能ADC1，ADC开始运行
	
	/*ADC校准*/
	ADC_ResetCalibration(ADC1);							//参考手册固定流程，内部有电路会自动执行校准
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
}


/**
  * 函    数：MQ_AD_Read
  * 参    数：无
  * 返 回 值：16位无符号短整型
	* 功    能：读取初始ADC值30次取平均值adcval_average
  */
uint16_t MQ_AD_Read(void)
{
	uint16_t adc_value=0;
	uint8_t  t=0;
	uint16_t adcval_average=0;

	for(t=0;t<SMOG_READ_TIMES;t++)
	{
		ADC_SoftwareStartConvCmd(ADC1,ENABLE);       //软件触发AD检测
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);	//等待EOC标志位，即等待AD转换结束
	  adc_value =adc_value+ADC_GetConversionValue(ADC1);					//读数据寄存器，得到AD转换的结果	
	}
	adcval_average=adc_value/30;
	return adcval_average;
}



/**
  * 函    数：MQ_Get_Vol
  * 参    数：无
  * 返 回 值：单精度浮点数的模拟电压值
	* 功    能：等效Mq2传感器的电压值voltage
  */
float MQ_Get_Vol(void)
{
	uint16_t adc = 0;    //从MQ-6传感器模块电压输出的ADC转换中获得的原始数字值，该值的范围为0到4095，将模拟电压表示为数字值
	float voltage = 0;   //定义MQ-6传感器模块的电压输出值，与气体浓度成正比
	
	adc = MQ_AD_Read();//#define SMOG_ADC_CHX	ADC_Channel_0	定义烟雾传感器所在的ADC通道编号
	delay_ms(5);
	voltage  = (3.3/4096.0)*(adc);
	return voltage;
}


/**
  * 函    数：MQ_AD_GetPPM
  * 参    数：无
  * 返 回 值：单精度浮点数的ppm值
	* 功    能：计算Mq2传感器的ppm值
  */
//float MQ_AD_GetPPM(void)
//{
//	float RS = (3.3f - MQ_Get_Vol()) / MQ_Get_Vol() * RL;
//	float ppm = 98.322f * pow(RS/R0, -1.458f);
//	return  ppm;
//}

