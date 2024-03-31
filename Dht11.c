#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Dht11.h"


/**
  * 函    数：DHT11_Rst
  * 参    数：无
  * 返 回 值：无
  * 功    能：DHT11复位
  */     
void DHT11_Rst(void)	   
{                 
  DHT11_IO_OUT(); 	//SET OUTPUT
  DHT11_DQ_OUT=0; 	//拉低DQ
  delay_ms(20);    	//拉低至少18ms
  DHT11_DQ_OUT=1; 	//DQ=1 
  delay_us(30);     	//主机拉高20~40us
}


/**
  * 函    数：DHT11_Check
  * 参    数：无
  * 返 回 值：无符号字节型0或1
  * 功    能：检查DHT11，等待DHT11的回应，返回1:未检测到DHT11存在;返回0:DHT11存在
  */
uint8_t DHT11_Check(void) 	   
{   
	uint8_t retry=0;
	DHT11_IO_IN();		//设置为输入	 
  while (DHT11_DQ_IN&&retry<100)	//等待DHT11传感器拉低数据线,DHT11会拉低40~80us，当数据线变低或者重试计数达到100时，退出循环。
	{
		retry++;			//自增
		delay_us(1);		//延时1us
	};	 
	if(retry>=100)return 1;			//重试大于100us，检测DHT11失败
	else retry=0;            //执行第一个循环成功，返回0，为下一拉高阶段重置重试计数。
    while (!DHT11_DQ_IN&&retry<100)//DHT11拉低后会再次拉高数据线40~80us，当数据线拉高或者重试计数达到100时，退出循环
	{
		retry++;
		delay_us(1);
	};
	if(retry>=100)return 1;	  //若重试计数达到100，即DHT11传感器没有在预期时间内拉高数据线，函数返回1，表示错误。  
	return 0;				//预期时间内DHT11成功完成低电平和高电平转换，函数返回0，表示DHT11传感器准备好进行通信。
}


/**
  * 函    数：DHT11_Read_Bit
  * 参    数：无
  * 返 回 值：无符号字节型0或1
  * 功    能：从DHT11读取一个位数据,高电平1;低电平0
  */
uint8_t DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
	while(DHT11_DQ_IN&&retry<100)	//等待DHT11传感器的数据线变为低电平
	{
		retry++;
		delay_us(1);
	}
	retry=0;
	while(!DHT11_DQ_IN&&retry<100)	//等待DHT11传感器的数据线变为高电平
	{
		retry++;
		delay_us(1);
	}
	delay_us(40);//等待40us
	if(DHT11_DQ_IN)return 1;		//数据线在延迟后仍为高电平，函数返回1
	else return 0;		   
}


/**
  * 函    数：DHT11_Read_Byte
  * 参    数：无
  * 返 回 值：无符号字节型数据
  * 功    能：从DHT11读取一个字节的数据
  */
uint8_t DHT11_Read_Byte(void)    
{        
    uint8_t i,data;
    data=0;
	for (i=0;i<8;i++)  			 //循环，迭代8次，每次读取一个位（bit）的数据组成字节（byte）
	{
   		data<<=1; 				 //左移一位
	    data|=DHT11_Read_Bit();  //调用DHT11_Read_Bit函数读取一个位的数据，将该位的数据设置到data的最低位
    }						    
    return data;
}

/**
  * 函    数：DHT11_Read_Data
  * 参    数：两个指针temp和humi，存储温度和湿度
  * 返 回 值：无符号字节型0或1，0：正常;1：读取失败
  * 功    能：从DHT11传感器读取温湿度数据，temp:温度值(范围:0~50°);humi:湿度值(范围:20%~90%);
  */
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi)    
{        
 	uint8_t buf[5];     //存储从DHT11传感器读取的5个字节的数据
	uint8_t i;          //循环计数器i
	DHT11_Rst();		//复位DHT11传感器
	if(DHT11_Check()==0)//DHT11通行正常
	{
		for(i=0;i<5;i++)//读取5轮共40位的数据
		{
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])//数据校验是否通过
		{
			*humi=buf[0];	//湿度数据存储在指针"humi"所指向的位置	
			*temp=buf[2];	//温度数据存储在指针"temp"所指向的位置
		}
	}else return 1;			//读取失败
	return 0;	    		//读取成功
}


/**
  * 函    数：DHT11_Init
  * 参    数：无
  * 返 回 值：无符号字节型0或1
  * 功    能：初始化DHT11的IO口，同时检测DHT11的存在，返回0:存在;返回1:不存在
  */   	 
uint8_t DHT11_Init(void)
{	 
 	GPIO_InitTypeDef GPIO_InitStructure;
 	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 //使能PA端口时钟
	
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;				 //PA11端口配置
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOA, &GPIO_InitStructure);				 	 //初始化IO口
 	GPIO_SetBits(GPIOA,GPIO_Pin_11);						 //PA11 输出高
			    
	DHT11_Rst();  //复位DHT11
	return DHT11_Check();//等待DHT11的回应
} 

