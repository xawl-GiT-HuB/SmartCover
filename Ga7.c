#include "stm32f10x.h"                  // Device header
#include "usart.h"
#include "usart2.h"
#include <stdio.h>
#include <string.h>
#include "sys.h"
#include "delay.h"
#include "OLED.h"
#include "Ga7.h"
#include "MQTT.h"

//�ڶ���

/*
**�ѽ��ո�Ϊ��ѭ����������buf_posָ��дλ�ã�read_posָ���λ�ã����������ͬʱ�����ó�0
**read_pos == buf_pos��������Ϊ��
**read_pos����ָ����Ҫ����λ��
**buf_pos����ָ����һ��Ҫд���λ�ã������һ��Ҫд��λ�õ���read_pos��ô����Ϊ��������ʱ���˷�һ���ֽ�
*/
static char uart2_buf[Buf2_Max] = {'\0'};//����2���ջ���
static int buf_pos = 0;
static int read_pos = 0;
static paser_buf_t line_buf;

void clear_debug_buf(void)
{
	memset((char *)&line_buf, 0, sizeof(line_buf));
}

void add_char_to_debug_buf(unsigned char c)
{
	line_buf.buf[line_buf.pos] = c;
	line_buf.pos++;
	if (line_buf.pos >= (BUF_LEN - 1))
	{
		line_buf.pos = 0;
	}
}

char *get_debug_buf(void)
{
	return line_buf.buf;
}

void add_char_to_uart2_buf(unsigned char c)
{    
    int tmp;
    tmp = (buf_pos + 1) % Buf2_Max;
	/* �������޷��ٷ��� */
    if (tmp == read_pos)
    {
        return;
    }
    uart2_buf[buf_pos] = c;
    buf_pos = tmp;
}

int get_uart2_buf_pos(void)
{
	return buf_pos;
}

int uart2_buf_find_crcn(void)
{
	if (buf_pos < 2)
	{
		return 0;
	}
	if (uart2_buf[buf_pos - 2] == '\r' && uart2_buf[buf_pos - 1] == '\n')
	{
		return 1;
	}
	return 0;
}

int find_str_from_uart2_buf(char *str, char *str2)
{
  char read_buf[32] = {'\0'};
    
	if (str == NULL)
	{
		return 0;
	}
    while (1)
    {
        if (try_read_line(read_buf, 32) == 0)
        {
            printf("read_buf:%s\r\n", read_buf);
            if (strstr(read_buf, str) || (str2 != NULL && strstr(read_buf, str2)))
            {
                printf("find str:%s\r\n", str);
                return 1;
            }
        }
        else
        {
            printf("not find str:%s\r\n", str);
            return 0;
        }
    }	
}

void clear_uart2_buf(void)
{
	memset((char *)uart2_buf, 0, Buf2_Max);
  buf_pos = 0;              //�����ַ�������ʼ�洢λ��
	read_pos = 0;
}

void clear_uart2_buf_accord_pos(void)
{
	if (buf_pos > 0 && buf_pos < Buf2_Max)
	{
		memset((char *)uart2_buf, 0, buf_pos);
		buf_pos = 0;
		return;
	}
	if (buf_pos > 0)
	{
		clear_uart2_buf();
	}
}

/*
**���Դ�buf�ж�ȡһ�У���ȡ�ɹ�����0����ȡʧ�ܷ��ظ�ֵ��
**��ȡ��һ��֮�󣬻��buf���һ���ֽ����ó�\0�������ַ�������
*/
void show_read_buf_pos(void)
{
    printf("read_pos:%d, buf_pos:%d\r\n", read_pos, buf_pos);
}

int try_read_line(char *buf, int len)
{
    int i, end_pos, found = 0;
    
    /* ������Ϊ�� */
    if (read_pos == buf_pos || (read_pos + 1) == buf_pos)
    {
        return -1;
    }
    /* read_pos��buf_pos����end_pos�ǻ�����������λ�� */
    if (read_pos < buf_pos)
    {
        end_pos = buf_pos;
    }
    else /* read_pos��buf_posǰ�� ˵��������ѭ�� */
    {
        end_pos = Buf2_Max + buf_pos;
    }
    /* i < end_pos����ΪҪ�õ�i+1���λ�� */
    for (i = read_pos; i < end_pos; i++)
    {
        /* ˵���ҵ���һ�� */
        if (uart2_buf[i % Buf2_Max] == '\r' && uart2_buf[(i + 1)% Buf2_Max] == '\n')
        {
            found = 1;
        }
        if ((i - read_pos) >= len)
        {
            printf("one line len %d bytes out of %d bytes, please modify it.\r\n", i - read_pos, len);
            clear_uart2_buf();
            return -2;
        }
        if (read_pos == (i % Buf2_Max))
        {
            found = 0; //˵��Ҫ����λ�úͺ��ҵ���λ����ͬ��Ҳ����read_posֱ��ָ����\r\n����û�������ַ���
            continue;
        }
        if (found)
        {
            /* 1.�ȿ�����һ������ */
            /* ˵��\n��λ����read_posǰ�棬����ֱ�ӿ��� */
            if (read_pos < (i % Buf2_Max))
            {
                memcpy(buf, &uart2_buf[read_pos], i - read_pos + 1); //��\r�ַ�Ҳ����������
                buf[i - read_pos] = '\0'; /* ��\r�޸ĳ�'\0' */
                /* 2.������������� */
                memset((char *)&uart2_buf[read_pos], 0, i - read_pos + 2); //��\n�ַ�Ҳ����
            }
            else /* �϶�������� read_pos == (i % Buf2_Max) �����*/
            {
                memcpy(buf, &uart2_buf[read_pos], Buf2_Max - read_pos);
                memset(&uart2_buf[read_pos], 0, Buf2_Max - read_pos);
                memcpy(&buf[Buf2_Max - read_pos], uart2_buf, (i % Buf2_Max) + 1);
                memset((char *)uart2_buf, 0, (i % Buf2_Max) + 1 + 1); //(i % Buf2_Max) + 1��\r��λ��
                buf[Buf2_Max - read_pos + (i % Buf2_Max)] = '\0'; /* ��\r�޸ĳ�'\0' */
            }
            /* 3.�޸Ķ�ָ���λ�� */
            read_pos = (i + 1 + 1) % Buf2_Max; //i+1��λ����\n�� i+1+1����һ��Ҫ����λ��
            show_read_buf_pos();
            return 0;
        }
    }
    
    return -3;
}

/*******************************************************************************
* ������ : find_str
* ����   : �жϻ������Ƿ���ָ�����ַ���
* ����   : 
* ���   : 
* ����   : 1 �ҵ�ָ���ַ���0 δ�ҵ�ָ���ַ� 
* ע��   : 
*******************************************************************************/

int find_str(char *a)
{ 
	if(strstr(uart2_buf, a)!=NULL)
	{
		return 1;
	}	
	else
	{
		return 0;
	}
}

/*******************************************************************************
* ������ : test_communicate_pass
* ����   : �жϴ����Ƿ��ܹ�ͨѶ
* ����   : 
* ���   : 
* ����   : 
* ע��   : 
*******************************************************************************/
int test_communicate_pass(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("AT","OK",3,999);//����ͨ���Ƿ�ɹ�
	return ret;
}

/*******************************************************************************
* ������ : close_ate
* ����   : �ر��������
* ����   : 
* ���   : 
* ����   : 
* ע��   : 
*******************************************************************************/
int close_ate(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("ATE0","OK",3,999);
	return ret;
}

/*******************************************************************************
* ������ : wait_creg
* ����   : �ȴ�ģ��ע��ɹ�
* ����   : 
* ���   : 
* ����   : 
* ע��   : 
*******************************************************************************/
int wait_creg(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command_Ext("AT+CREG?","+CREG: 0,6","+CREG: 0,7",1,1999);//����ͨ���Ƿ�ɹ�
	return ret;
}


int mtcfg(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("AT+ECMTCFG=\"keepalive\",0,60","OK",1,1999);
	return ret;
}

int mtopen(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("AT+ECMTOPEN=0,\"39.101.73.149\",1883","OK",1,2999);
	return ret;//\r\n\r\n+ECMTOPEN: 0,0
}

int mtconn(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("AT+ECMTCONN=0,\"Hardware_Client\"","OK",1,6000);
	return ret;//\r\n\r\n+ECMTCONN: 0,0,0
}

int mtsub(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("AT+ECMTSUB=0,1,\"/Pc_Control\",2","OK",1,3000);
	return ret;//\r\n\r\n+ECMTSUB: 0,1,0,1
}

/*******************************************************************************
* ������ : active_pdp
* ����   : ����Pdp
* ����   : 
* ���   : 
* ����   : 
* ע��   : ���pdp�Ѿ������ô�Ͳ��ü�����
*******************************************************************************/
int active_pdp(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("AT+QIACT?","+QIACT:1,1,1",1,1999);
	if (ret == 1)
	{
		return 1;
	}
	ret = UART2_Send_AT_Command("AT+QIACT=1","OK",1,1999);
	return ret;
}









//��һ�� ����
//int GA7Init_Flag =1;         //ȫ�ֱ���,�ж�ga7�Ƿ�����,0 ���ӳɹ�,1 ����ʧ��,��ʼ��Ϊʧ��
//int GA7ATE0_Flag =1;				 //ȫ�ֱ���,�ر�ga7�������
//int GA7Internet_Flag =1;		 //ȫ�ֱ���,�ж�ga7�Ƿ�����
//int GA7MQTT_Flag =1;		 		 //ȫ�ֱ���,�ж�ga7�Ƿ�����MQTT������
//int GA7Client_Flag =1;			 //ȫ�ֱ���,�ж�ga7�Ƿ�ע��ͻ���
//int GA7Topic_Flag =1;				 //ȫ�ֱ���,�ж�ga7�Ƿ�������

///*-------------------------------------------------*/
///*��������GA7��������ָ��                          */
///*��  ����cmd��ָ��                                */
///*��  ����timeout����ʱʱ�䣨1000ms�ı�����        */
///*����ֵ��0����ȷ   ����������                     */
///*-------------------------------------------------*/
//char GA7_Send_AT(char *cmd, int timeout)
//{
//	GA7_RxCounter=0;                          //GA7������������������                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);     //���GA7���ջ����� 
//	GA7_printf("%s\r\n",cmd);                 //����ָ��
//	while(timeout--){                         //�ȴ���ʱʱ�䵽0
//		delay_ms(1000);                         //��ʱ1000ms
//		if(strstr(GA7_RX_BUF,"OK"))            	//������յ�OK��ʾָ��ɹ�
//			break;       													//��������whileѭ��
////		printf("AT ERROR! TIMEOUT is %d ",timeout);               //���������Ϣ����ʱʱ��
//	}
////	printf("GA7 AT SUCCESS\r\n");                          			//���������Ϣ
//	if(timeout<=0)return 1;                   //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�OK������1
//	else return 0;		         				  			//��֮����ʾAT������ȷ��˵���յ�OK��ͨ��break��������while
//}

///*-------------------------------------------------*/
///*��������GA7��ʼ�������ж�                            */
///*��  ����timeout����ʱʱ�䣨100ms�ı�����            */
///*����ֵ��0����ȷ   ����������                     */
///*-------------------------------------------------*/
//char GA7_Init(int timeout)
//{		
//	GA7_RxCounter=0;                             //������������������                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //��ս��ջ����� 
//	GA7_printf("AT\r\n"); 											   //����ָ��"AT",�ж�ģ���Ƿ�����
////	printf("AT CODE");
//		OLED_ShowString(1,1,USART2_TxBuff);
//	while(timeout--){                            //�ȴ���ʱʱ�䵽0
//		delay_ms(1000);                            //��ʱ1000ms�ȴ����صĻ�Ӧ����
//		OLED_ShowString(2,1,GA7_RX_BUF);
//		if(strstr(GA7_RX_BUF,"OK")) 							 //������յ�OK����ʾ�ɹ�
////			OLED_ShowString(2,1,GA7_RX_BUF);
//			break;       						    						 //��������whileѭ��
//		printf("GA7 Init ERROR! TIMEOUT is %d ",timeout);           //���������Ϣ����ʱʱ��
//	}
////	printf("%s",GA7_RX_BUF);
//	printf("GA7 Init Success!");                              //���������Ϣ
//	if(timeout<=0){
//		GA7Init_Flag=1;
//		return 1;
//	}                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�"OK"������1
//	GA7Init_Flag=0;
//	return 0;                                       //��ȷ������0
//}

///*-------------------------------------------------*/
///*��������GA7AT����رջ���                        */
///*��  ����timeout����ʱʱ�䣨100ms�ı�����         */
///*����ֵ��0����ȷ   ����������                     */
///*-------------------------------------------------*/
//char GA7_ATE0(int timeout)
//{		
//	GA7_RxCounter=0;                             //������������������                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //��ս��ջ����� 
//	GA7_printf("ATE0\r\n"); 										 //����ָ��"ATE0",�ر�ģ��at�������	
//	while(timeout--){                            //�ȴ���ʱʱ�䵽0
//		delay_ms(100);                             //��ʱ100ms
//		if(strstr(GA7_RX_BUF,"OK")) 							 //������յ�OK����ʾ�ɹ�
//			break;       						    						 //��������whileѭ��
//		printf("GA7 ATE0 ERROR! TIMEOUT is %d ",timeout);           //���������Ϣ����ʱʱ��
//	}
//	printf("GA7 ATE0 Success!\r\n");                              //���������Ϣ
//	if(timeout<=0){
//		GA7ATE0_Flag=1;
//		return 1;
//	}                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�"OK"������1
//	GA7ATE0_Flag=0;
//	return 0;                                       //��ȷ������0
//}

///*-------------------------------------------------*/
///*��������GA7���������ж�                          */
///*��  ����timeout����ʱʱ�䣨500ms�ı�����         */
///*����ֵ��0����ȷ   ����������                     */
///*-------------------------------------------------*/
//char GA7_Internet(int timeout)
//{		
//	GA7_RxCounter=0;                             //������������������                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //��ս��ջ����� 
//	GA7_printf("AT+CREG?\r\n"); 								 //����ָ��"AT+CREG?",�ж�ģ���Ƿ�פ��	
//	while(timeout--){                            //�ȴ���ʱʱ�䵽0
//		delay_ms(500);                             //��ʱ500ms
//		if(strstr(GA7_RX_BUF,"+CREG: 0,6\r\n\r\nOK")) 	 //������յ�+CREG: 0,6\r\n\r\nOK����ʾ�ɹ�פ��
//			break;       						    						 //��������whileѭ��
//		printf("GA7 Internet ERROR! TIMEOUT is %d ",timeout);           //���������Ϣ����ʱʱ��
//	}
//	printf("Internet Connect Success!\r\n");                              //���������Ϣ
//	if(timeout<=0){
//		GA7Internet_Flag=1;
//		return 1;
//	}                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�"OK"������1
//	GA7Internet_Flag=0;
//	return 0;                                       //��ȷ������0
//}

///*-------------------------------------------------*/
///*��������GA7����������                            */
///*��  ����timeout����ʱʱ�䣨100ms�ı�����         */
///*����ֵ��0����ȷ   ����������                     */
///*-------------------------------------------------*/
//char GA7_KeepAlive(int timeout)
//{		
//	GA7_RxCounter=0;                             //������������������                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //��ս��ջ����� 
//	GA7_printf("AT+ECMTCFG=\"keepalive\",0,60\r\n"); //����ATָ��"AT+ECMTCFG=\"keepalive\",0,60",����ģ������
//	while(timeout--){                            //�ȴ���ʱʱ�䵽0
//		delay_ms(100);                             //��ʱ500ms
//		if(strstr(GA7_RX_BUF,"OK")) 	 						 //������յ�OK����ʾ���óɹ�
//			break;       						    						 //��������whileѭ��
//	}
//	if(timeout<=0){
//		return 1;
//	}                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�"OK"������1
//	return 0;                                     //��ȷ������0
//}

///*-------------------------------------------------*/
///*��������GA7����MQTT������							             */
///*��  ����timeout�� ��ʱʱ�䣨500ms�ı�����   		     */
///*����ֵ��0����ȷ  ����������                      */
///*-------------------------------------------------*/
//char GA7_Mqtt(int timeout)
//{		
//	GA7_RxCounter=0;                             //������������������                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //��ս��ջ����� 
//	GA7_printf("AT+ECMTOPEN=0,\"39.101.73.149\",1883\r\n"); //����ATָ��"AT+ECMTOPEN=0,\"39.101.73.149\",1883",����MQTT������
//	while(timeout--){                            //�ȴ���ʱʱ�䵽0
//		delay_ms(500);                             //��ʱ500ms
//		if(strstr(GA7_RX_BUF,"+ECMTOPEN: 0,0")) 	 //������յ�+ECMTOPEN: 0,0����ʾ���ӳɹ�
//			break;       						    						 //��������whileѭ��
//			printf("GA7 MQTT ERROR! TIMEOUT is %d ",timeout);           //���������Ϣ����ʱʱ��
//	}
//		printf("GA7 MQTT Success!\r\n");                              //���������Ϣ
//	if(timeout<=0){
//		GA7MQTT_Flag=1;
//		return 1;
//	}                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�"OK"������1
//	GA7MQTT_Flag=0;
//	return 0;                                     //��ȷ������0
//}

///*-------------------------------------------------*/
///*��������GA7ע��MQTT�ͻ���						             */
///*��  ����timeout�� ��ʱʱ�䣨500ms�ı�����        */
///*����ֵ��0����ȷ  ����������                      */
///*-------------------------------------------------*/
//char GA7_Client(int timeout)
//{		
//	GA7_RxCounter=0;                             //������������������                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //��ս��ջ����� 
//	GA7_printf("AT+ECMTCONN=0,\"Hardware_Client\"\r\n"); //����ATָ��"AT+ECMTCONN=0,"Hardware_Client",ע��ͻ���
//	while(timeout--){                            //�ȴ���ʱʱ�䵽0
//		delay_ms(500);                             //��ʱ500ms
//		if(strstr(GA7_RX_BUF,"+ECMTSTAT: 0,1\r\n\r\n+ECMTCONN: 0,0,0")) 	 //������յ�+ECMTSTAT: 0,1\r\n\r\n+ECMTCONN: 0,0,0����ʾע��ɹ�
//			break;       						    						 //��������whileѭ��
//			printf("GA7 Client ERROR! TIMEOUT is %d ",timeout);           //���������Ϣ����ʱʱ��
//	}
//		printf("GA7 Client Success!\r\n");                              //���������Ϣ
//	if(timeout<=0){
//		GA7Client_Flag=1;
//		return 1;
//	}                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�"OK"������1
//	GA7Client_Flag=0;
//	return 0;                                     //��ȷ������0
//}

///*-------------------------------------------------*/
///*��������GA7ע��MQTT�ͻ���						             */
///*��  ����timeout�� ��ʱʱ�䣨500ms�ı�����        */
///*����ֵ��0����ȷ  ����������                      */
///*-------------------------------------------------*/
//char GA7_Topic(int timeout)
//{		
//	GA7_RxCounter=0;                             //������������������                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //��ս��ջ����� 
//	GA7_printf("AT+ECMTSUB=0,1,\"/Pc_Control\",2\r\n"); //����ATָ��"AT+ECMTSUB=0,1,"/Pc_Control",2",���Ļ���
//	while(timeout--){                            //�ȴ���ʱʱ�䵽0
//		delay_ms(500);                             //��ʱ500ms
//		if(strstr(GA7_RX_BUF,"OK\r\n\r\n+ECMTSUB: 0,1,0,1")) 	 //������յ�OK\r\n\r\n+ECMTSUB: 0,1,0,1����ʾ���ĳɹ�
//			break;       						    						 //��������whileѭ��
//			printf("GA7 Topic ERROR! TIMEOUT is %d ",timeout);           //���������Ϣ����ʱʱ��
//	}
//		printf("GA7 Topic Success!\r\n");                              //���������Ϣ
//	if(timeout<=0){
//		GA7Topic_Flag=1;
//		return 1;
//	}                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�"OK"������1
//	GA7Topic_Flag=0;
//	GA7_RxCounter=0;                             //������������������
//	return 0;                                     //��ȷ������0
//}
