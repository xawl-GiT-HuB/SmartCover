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

#define STABLE_TIMES  5  //�ȴ�ϵͳ�ϵ����ȶ�

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
  char connect_status = 0;     /* 0-δ���� 1-���ӳɹ� 2-���ֳɹ� */
	uint8_t string[10] = {0};	
	uint32_t dhtdata[5];  				//DHT11��������
	
/*��������������������ϵͳӲ����ʼ����������������������test����������������������*/		

	SysTick_Init_Config();//ϵͳ�δ�ʱ�ӳ�ʼ��
	delay_Init();					//��ʱ��ʼ��
	Usart1_Init(115200);	//USART1��ʼ��
	Usart2_Init(115200);	//USART2��ʼ��
	Beep_Init();			    //Buzzer��������ʼ��
	LightSensor_Init();	 	//LightSensor������������ʼ��
	OLED_Init();					//OLED��ʾ����ʼ��
	DHT11_Init();					//DHT11��ʪ�ȴ�������ʼ��
	MQ_AD_Init();			  	//MQ-2������������ʼ��
	Hcsr04_Init();				//HC-SR04��������������ʼ��
	IIC_Init();						//
	MPU6050_Init();       //=====MPU6050��ʼ��	
	DMP_Init();						//

	delay_ms(999);				//��ʱ1s

	clear_debug_buf();    //��յ�����Ϣ������
	clear_uart2_buf();    //��ʼ�����ݻ�����
	delay_ms(1999);				//��ʱ2s
	
	printf("SYSTEM STARTING , PLEASE WAITING............(%d)s\r\n", STABLE_TIMES);
	for(i = 1;i <= STABLE_TIMES;i++)
	{
		delay_ms(999);
		printf("%d ", i);
	}
	printf("\r\n");
	OLED_ShowString(2,2,"SYSTEM INIT OK");

/*������������NBģ�����á�MQTT���ӡ��ͻ��˴��������ⶩ�ġ���������test������������*/	
	
	while(test_communicate_pass() == 0)
	{
		printf("AT ERROR.\r\n");
		delay_ms(2999);
	}
	printf("AT OK.\r\n");
	delay_ms(999);
	
	OLED_Clear();											//OLED����
	
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
  clear_uart2_buf(); //�ٴ����
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


/*������������������OLED��Ļ�̶�չʾģ�桪������������test����������������������*/

	if(connect_status ==1){
	OLED_Clear();											//OLED����
	OLED_ShowString(1,1,"Length:");		//ˮλ����
	OLED_ShowString(1,13,"cm");				
	OLED_ShowString(2,1,"PPM:");		  //����Ũ��չʾ
	OLED_ShowString(3,1,"TEMP:");			//�¶�Temperatureչʾ
	OLED_ShowString(3,9,"RH:");				//ʪ��Relative Humidityչʾ
	OLED_ShowString(4,1,"P:");		    //��б��Pitch����x��
	OLED_ShowString(4,9,"R:");		    //������Roll����y��
	
	TIM2_Getsample_Int(3999,719);		  //40ms��ʱ�жϻ�ȡ�Ƕ�ֵ,==1����ԣ����Ƿ����ϣ����޸Ķ�ʱʱ��
	}

	if(connect_status ==0){
	OLED_Clear();
	OLED_ShowString(2,2,"MQTT LINK ERROR");
	}
	
	
//��ѭ��
	while(1)
	{	
		WARNNING_FLAG = 0;						 //������־λ��ʼ��0
			
/*���������������������������ݻ�ȡ��������������������test����������������������*/		
		
		if(connect_status ==1){
			
		leng =Hcsr04_GetLength();      //�����ȡ
		length = leng*100;						 //ԭʼ��������*100
		
		mqv=MQ_AD_Read();							 //����Ũ��ģ��ֵ��ȡ
		
		DHT11_Read_Data(&temp,&humi);  //��ʪ�Ⱥ������û�ȡ��ֵ
		dhtdata[0]=temp;
		dhtdata[1]=humi;							 //��ʪ�����鸳ֵ
		
/*������������������OLED��Ļ����չʾģ�顪��������������test����������������������*/
		
		//����չʾ
		OLED_ShowNum(1,8,length/10000%10,1);			//��λ  length/10000
		OLED_ShowNum(1,9,length/1000%10,1);				//ʮλ  length/1000
		OLED_ShowNum(1,10,length/100%10,1);				//��λ  length%100
		OLED_ShowString(1,11,".");							
		OLED_ShowNum(1,12,length/10%10,1);		    //С��λ  length%100/10
		
		//����Ũ��չʾ
		OLED_ShowNum(2,5,mqv,4);
		//OLED_ShowNum(2,10,V,4);
		
		//��ʪ��չʾ
		OLED_ShowNum(3,6,dhtdata[0],2);
		//OLED_ShowString(3,9,"��");  //���ֿⲻȫ����ʱ����
		OLED_ShowNum(3,12,dhtdata[1],2);
		OLED_ShowString(3,14,"%");
		
		//��б�����Ƕ�չʾ
		sprintf((char *)string,"Pitch:%.2f",Pitch);
		OLED_ShowSignedNum(4,3,Pitch,3);
		sprintf((char *)string,"Roll :%.2f",Roll);
		OLED_ShowSignedNum(4,11,Roll,3);

/*���������������������������ն˱���ģ�顪������������������������test��������������������*/

//		LightSensor_Beep();  					 //�������������Ʒ���������
		
		if ( Pitch >= 45 || Roll >= 45 || Pitch <=-45  || Roll <=-45 || leng <= 50 || temp >=40 || LightSensor_Get() == 0 ) {
				Beep_On();
				WARNNING_FLAG = 1;
		} else {
				  Beep_Off();
				  WARNNING_FLAG = 0;
		    }
    }

/*���������������������������ڷ��͡������ϱ�ģ�顪����������������test��������������������*/
		
		if (connect_status == 1 && (ticks++ <= 19) == 1)
		{
		  printf("\r\nDistance=%.2f   PPM=%.2f ",leng,mqv);   //���ڷ��;��롢����Ũ��ֵ       
		  printf(" TEMP=%d   RH=%d  ",temp,humi); 						//���ڷ����¶ȡ�ʪ��ֵ
		  printf(" Pitch=%.2f   Roll=%.2f ",Pitch,Roll); 		  //���ڷ��͸����ǡ�����Ƕ�ֵ
	    printf(" WARNNING=%d\r\n",WARNNING_FLAG); 				  //���ڷ��ͱ��������־
		}	
		
		if (connect_status == 1 && (ticks == 20) == 1)
      {
					sprintf(SEND_BUF,"{\"Distance\":%.2f,\"PPM\":%.2f,\"Pitch\":%.2f,\"Roll\":%.2f,\"TEMP\":%d,\"RH\":%d,\"WARNNING\":%d}",leng,mqv,Pitch,Roll,temp,humi,WARNNING_FLAG);
					ret = send_mqtt_message(SEND_BUF);
				  printf("Trying To Send Message��ret=%d\r\n", ret);
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
