#include "stm32f10x.h"                
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "string.h"
#include "MQTT.h"
#include "TIME.h"
#include "LightSensor.h"
#include "OLED.h"
#include "DHT11.h"
#include "Beep.h"
#include "Ga7.h"
#include "Mq2.h"
#include "Hcsr04.h"
#include "MPU6050.h"

#define STABLE_TIMES  5  //等待系统上电后的稳定

uint8_t temp,humi;
uint32_t length;
float leng;
float mqv;
float Pitch,Roll,Yaw;
_Bool WARNNING_FLAG;

char SEND_BUF[512];

int main(void)
{	
	uint8_t i;
	uint32_t  ret, ticks = 0;
  char connect_status = 0;     /* 0-未连接 1-连接成功 2-发现成功 */
	uint8_t string[10] = {0};	
	uint32_t dhtdata[5];  				//DHT11数据数组
	
/*——————————系统硬件初始化——————————test———————————*/		

	SysTick_Init_Config();//系统滴答时钟初始化
	delay_Init();					//延时初始化
	Usart1_Init(115200);	//USART1初始化
	Usart2_Init(115200);	//USART2初始化
	Beep_Init();			    //Buzzer蜂鸣器初始化
	LightSensor_Init();	 	//LightSensor光敏传感器初始化
	OLED_Init();					//OLED显示屏初始化
	DHT11_Init();					//DHT11温湿度传感器初始化
	MQ_AD_Init();			  	//MQ-2气敏传感器初始化
	Hcsr04_Init();				//HC-SR04超声波传感器初始化
	IIC_Init();						//
	MPU6050_Init();       //=====MPU6050初始化	
	DMP_Init();						//

	delay_ms(999);				//延时1s

	clear_debug_buf();    //清空调试信息缓冲区
	clear_uart2_buf();    //初始化数据缓冲区
	delay_ms(1999);				//延时2s
	
	printf("SYSTEM STARTING , PLEASE WAITING............(%d)s\r\n", STABLE_TIMES);
	for(i = 1;i <= STABLE_TIMES;i++)
	{
		delay_ms(999);
		printf("%d ", i);
	}
	printf("\r\n");
	OLED_ShowString(2,2,"SYSTEM INIT OK");

/*——————NB模块配置、MQTT连接、客户端创建、话题订阅—————test——————*/	
	
	while(test_communicate_pass() == 0)
	{
		printf("AT ERROR.\r\n");
		delay_ms(2999);
	}
	printf("AT OK.\r\n");
	delay_ms(999);
	
	OLED_Clear();											//OLED清屏
	
  while(close_ate() == 0)
	{
		printf("ATE0 ERROR.\r\n");
		delay_ms(2999);
	}
	printf("ATE0 OK.\r\n");
	delay_ms(999);
	while(wait_creg() == 0)
	{
		printf("AT+CREG? ERROR.\r\n");
		delay_ms(2999);
	}
	printf("AT+CREG? OK.\r\n");
  clear_uart2_buf(); //再次清空
	delay_ms(999);
	  while(mtcfg() == 0)
	{
		printf("MTCFG ERROR.\r\n");
		delay_ms(2999);
	}
	printf("MTCFG OK.\r\n");
	delay_ms(999);
		while(mtopen() == 0)
	{
		printf("MTOPEN ERROR.\r\n");
		delay_ms(2999);
	}
	printf("MTOPEN OK.\r\n");
	
	OLED_ShowString(2,2,"MQTT LINK OK");
	
	delay_ms(999);
			while(mtconn() == 0)
	{
		printf("MTCONN ERROR.\r\n");
		delay_ms(2999);
	}
	printf("MTCONN OK.\r\n");
	connect_status = 1;
	delay_ms(999);
			while(mtsub() == 0)
	{
		printf("MTSUB ERROR.\r\n");
		delay_ms(2999);
	}
	printf("MTSUB OK.\r\n");
	delay_ms(999);


/*—————————OLED屏幕固定展示模版———————test———————————*/

	if(connect_status ==1){
	OLED_Clear();											//OLED清屏
	OLED_ShowString(1,1,"Length:");		//水位距离
	OLED_ShowString(1,13,"cm");				
	OLED_ShowString(2,1,"PPM:");		  //气体浓度展示
	OLED_ShowString(3,1,"TEMP:");			//温度Temperature展示
	OLED_ShowString(3,9,"RH:");				//湿度Relative Humidity展示
	OLED_ShowString(4,1,"P:");		    //倾斜角Pitch测试x轴
	OLED_ShowString(4,9,"R:");		    //俯仰角Roll测试y轴
	
	TIM2_Getsample_Int(3999,719);		  //40ms定时中断获取角度值,==1里测试，看是否抢断，可修改定时时间
	}

	if(connect_status ==0){
	OLED_Clear();
	OLED_ShowString(2,2,"MQTT LINK ERROR");
	}
	
	
//主循环
	while(1)
	{	
		WARNNING_FLAG = 0;						 //报警标志位初始置0
			
/*————————————数据获取——————————test———————————*/		
		
		if(connect_status ==1){
			
		leng =Hcsr04_GetLength();      //距离获取
		length = leng*100;						 //原始距离数据*100
		
		mqv=MQ_AD_Read();							 //气体浓度模拟值获取
		
		DHT11_Read_Data(&temp,&humi);  //温湿度函数调用获取数值
		dhtdata[0]=temp;
		dhtdata[1]=humi;							 //温湿度数组赋值
		
/*—————————OLED屏幕数据展示模块————————test———————————*/
		
		//距离展示
		OLED_ShowNum(1,8,length/10000%10,1);			//百位  length/10000
		OLED_ShowNum(1,9,length/1000%10,1);				//十位  length/1000
		OLED_ShowNum(1,10,length/100%10,1);				//个位  length%100
		OLED_ShowString(1,11,".");							
		OLED_ShowNum(1,12,length/10%10,1);		    //小数位  length%100/10
		
		//气体浓度展示
		OLED_ShowNum(2,5,mqv,4);
		//OLED_ShowNum(2,10,V,4);
		
		//温湿度展示
		OLED_ShowNum(3,6,dhtdata[0],2);
		//OLED_ShowString(3,9,"℃");  //℃字库不全，暂时舍弃
		OLED_ShowNum(3,12,dhtdata[1],2);
		OLED_ShowString(3,14,"%");
		
		//倾斜俯仰角度展示
		sprintf((char *)string,"Pitch:%.2f",Pitch);
		OLED_ShowSignedNum(4,3,Pitch,3);
		sprintf((char *)string,"Roll :%.2f",Roll);
		OLED_ShowSignedNum(4,11,Roll,3);

/*—————————————终端报警模块—————————————test——————————*/

//		LightSensor_Beep();  					 //光敏传感器控制蜂鸣器鸣响
		
		if ( Pitch >= 45 || Roll >= 45 || Pitch <=-45  || Roll <=-45 || leng <= 50 || temp >=40 || LightSensor_Get() == 0 ) {
				Beep_On();
				WARNNING_FLAG = 1;
		} else {
				  Beep_Off();
				  WARNNING_FLAG = 0;
		    }
    }

/*————————————串口发送、数据上报模块—————————test——————————*/
		
		if (connect_status == 1 && (ticks++ <= 19) == 1)
		{
		  printf("\r\nDistance=%.2f   PPM=%.2f ",leng,mqv);   //串口发送距离、气体浓度值       
		  printf(" TEMP=%d   RH=%d  ",temp,humi); 						//串口发送温度、湿度值
		  printf(" Pitch=%.2f   Roll=%.2f ",Pitch,Roll); 		  //串口发送俯仰角、横滚角度值
	    printf(" WARNNING=%d\r\n",WARNNING_FLAG); 				  //串口发送报警警告标志
		}	
		
		if (connect_status == 1 && (ticks == 20) == 1)
      {
					sprintf(SEND_BUF,"{\"Distance\":%.2f,\"PPM\":%.2f,\"Pitch\":%.2f,\"Roll\":%.2f,\"TEMP\":%d,\"RH\":%d,\"WARNNING\":%d}",leng,mqv,Pitch,Roll,temp,humi,WARNNING_FLAG);
					ret = send_mqtt_message(SEND_BUF);
				  printf("Trying To Send Message，ret=%d\r\n", ret);
			    if(ret == 1)
			    {
				     printf("Send Message To MQTT Success.\r\n");
			    }
			    else
			    {
				    printf("Send Message To MQTT Error.\r\n");
            disconnect_mqtt_server();
            connect_status = 0;
			     }
					ticks = 0;
       }
	}
}
