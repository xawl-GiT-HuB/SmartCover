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

//第二版

/*
**把接收改为满循环缓冲区，buf_pos指向写位置，read_pos指向读位置，读完的数据同时被设置成0
**read_pos == buf_pos：缓冲区为空
**read_pos总是指向需要读的位置
**buf_pos总是指向下一个要写入的位置，如果下一个要写的位置等于read_pos那么就认为是满，此时会浪费一个字节
*/
static char uart2_buf[Buf2_Max] = {'\0'};//串口2接收缓存
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
	/* 已满，无法再放入 */
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
  buf_pos = 0;              //接收字符串的起始存储位置
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
**尝试从buf中读取一行，读取成功返回0，读取失败返回负值。
**读取完一行之后，会把buf最后一个字节设置成\0，方便字符串查找
*/
void show_read_buf_pos(void)
{
    printf("read_pos:%d, buf_pos:%d\r\n", read_pos, buf_pos);
}

int try_read_line(char *buf, int len)
{
    int i, end_pos, found = 0;
    
    /* 缓冲区为空 */
    if (read_pos == buf_pos || (read_pos + 1) == buf_pos)
    {
        return -1;
    }
    /* read_pos在buf_pos后面end_pos是缓冲区结束的位置 */
    if (read_pos < buf_pos)
    {
        end_pos = buf_pos;
    }
    else /* read_pos在buf_pos前面 说明发生了循环 */
    {
        end_pos = Buf2_Max + buf_pos;
    }
    /* i < end_pos是因为要用到i+1这个位置 */
    for (i = read_pos; i < end_pos; i++)
    {
        /* 说明找到了一行 */
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
            found = 0; //说明要读出位置和和找到的位置相同，也就是read_pos直接指向了\r\n，并没有其他字符串
            continue;
        }
        if (found)
        {
            /* 1.先拷贝出一行数据 */
            /* 说明\n的位置在read_pos前面，可以直接拷贝 */
            if (read_pos < (i % Buf2_Max))
            {
                memcpy(buf, &uart2_buf[read_pos], i - read_pos + 1); //把\r字符也拷贝进来了
                buf[i - read_pos] = '\0'; /* 把\r修改成'\0' */
                /* 2.清除缓冲区数据 */
                memset((char *)&uart2_buf[read_pos], 0, i - read_pos + 2); //把\n字符也置零
            }
            else /* 肯定不会出现 read_pos == (i % Buf2_Max) 的情况*/
            {
                memcpy(buf, &uart2_buf[read_pos], Buf2_Max - read_pos);
                memset(&uart2_buf[read_pos], 0, Buf2_Max - read_pos);
                memcpy(&buf[Buf2_Max - read_pos], uart2_buf, (i % Buf2_Max) + 1);
                memset((char *)uart2_buf, 0, (i % Buf2_Max) + 1 + 1); //(i % Buf2_Max) + 1是\r的位置
                buf[Buf2_Max - read_pos + (i % Buf2_Max)] = '\0'; /* 把\r修改成'\0' */
            }
            /* 3.修改读指针的位置 */
            read_pos = (i + 1 + 1) % Buf2_Max; //i+1的位置是\n， i+1+1是下一个要读的位置
            show_read_buf_pos();
            return 0;
        }
    }
    
    return -3;
}

/*******************************************************************************
* 函数名 : find_str
* 描述   : 判断缓存中是否含有指定的字符串
* 输入   : 
* 输出   : 
* 返回   : 1 找到指定字符，0 未找到指定字符 
* 注意   : 
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
* 函数名 : test_communicate_pass
* 描述   : 判断串口是否能够通讯
* 输入   : 
* 输出   : 
* 返回   : 
* 注意   : 
*******************************************************************************/
int test_communicate_pass(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("AT","OK",3,999);//测试通信是否成功
	return ret;
}

/*******************************************************************************
* 函数名 : close_ate
* 描述   : 关闭命令回显
* 输入   : 
* 输出   : 
* 返回   : 
* 注意   : 
*******************************************************************************/
int close_ate(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command("ATE0","OK",3,999);
	return ret;
}

/*******************************************************************************
* 函数名 : wait_creg
* 描述   : 等待模块注册成功
* 输入   : 
* 输出   : 
* 返回   : 
* 注意   : 
*******************************************************************************/
int wait_creg(void)
{
	int ret;
	
	ret = UART2_Send_AT_Command_Ext("AT+CREG?","+CREG: 0,6","+CREG: 0,7",1,1999);//测试通信是否成功
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
* 函数名 : active_pdp
* 描述   : 激活Pdp
* 输入   : 
* 输出   : 
* 返回   : 
* 注意   : 如果pdp已经激活，那么就不用激活了
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









//第一版 错误
//int GA7Init_Flag =1;         //全局变量,判断ga7是否连接,0 连接成功,1 连接失败,初始化为失败
//int GA7ATE0_Flag =1;				 //全局变量,关闭ga7命令回显
//int GA7Internet_Flag =1;		 //全局变量,判断ga7是否联网
//int GA7MQTT_Flag =1;		 		 //全局变量,判断ga7是否连接MQTT服务器
//int GA7Client_Flag =1;			 //全局变量,判断ga7是否注册客户端
//int GA7Topic_Flag =1;				 //全局变量,判断ga7是否订阅主题

///*-------------------------------------------------*/
///*函数名：GA7发送设置指令                          */
///*参  数：cmd：指令                                */
///*参  数：timeout：超时时间（1000ms的倍数）        */
///*返回值：0：正确   其他：错误                     */
///*-------------------------------------------------*/
//char GA7_Send_AT(char *cmd, int timeout)
//{
//	GA7_RxCounter=0;                          //GA7接收数据量变量清零                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);     //清空GA7接收缓冲区 
//	GA7_printf("%s\r\n",cmd);                 //发送指令
//	while(timeout--){                         //等待超时时间到0
//		delay_ms(1000);                         //延时1000ms
//		if(strstr(GA7_RX_BUF,"OK"))            	//如果接收到OK表示指令成功
//			break;       													//主动跳出while循环
////		printf("AT ERROR! TIMEOUT is %d ",timeout);               //串口输出信息及超时时间
//	}
////	printf("GA7 AT SUCCESS\r\n");                          			//串口输出信息
//	if(timeout<=0)return 1;                   //如果timeout<=0，说明超时时间到了，也没能收到OK，返回1
//	else return 0;		         				  			//反之，表示AT发送正确，说明收到OK，通过break主动跳出while
//}

///*-------------------------------------------------*/
///*函数名：GA7初始化连接判断                            */
///*参  数：timeout：超时时间（100ms的倍数）            */
///*返回值：0：正确   其他：错误                     */
///*-------------------------------------------------*/
//char GA7_Init(int timeout)
//{		
//	GA7_RxCounter=0;                             //接收数据量变量清零                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //清空接收缓冲区 
//	GA7_printf("AT\r\n"); 											   //发送指令"AT",判断模块是否连接
////	printf("AT CODE");
//		OLED_ShowString(1,1,USART2_TxBuff);
//	while(timeout--){                            //等待超时时间到0
//		delay_ms(1000);                            //延时1000ms等待返回的回应数据
//		OLED_ShowString(2,1,GA7_RX_BUF);
//		if(strstr(GA7_RX_BUF,"OK")) 							 //如果接收到OK，表示成功
////			OLED_ShowString(2,1,GA7_RX_BUF);
//			break;       						    						 //主动跳出while循环
//		printf("GA7 Init ERROR! TIMEOUT is %d ",timeout);           //串口输出信息及超时时间
//	}
////	printf("%s",GA7_RX_BUF);
//	printf("GA7 Init Success!");                              //串口输出信息
//	if(timeout<=0){
//		GA7Init_Flag=1;
//		return 1;
//	}                         //如果timeout<=0，说明超时时间到了，也没能收到"OK"，返回1
//	GA7Init_Flag=0;
//	return 0;                                       //正确，返回0
//}

///*-------------------------------------------------*/
///*函数名：GA7AT命令关闭回显                        */
///*参  数：timeout：超时时间（100ms的倍数）         */
///*返回值：0：正确   其他：错误                     */
///*-------------------------------------------------*/
//char GA7_ATE0(int timeout)
//{		
//	GA7_RxCounter=0;                             //接收数据量变量清零                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //清空接收缓冲区 
//	GA7_printf("ATE0\r\n"); 										 //发送指令"ATE0",关闭模块at命令回显	
//	while(timeout--){                            //等待超时时间到0
//		delay_ms(100);                             //延时100ms
//		if(strstr(GA7_RX_BUF,"OK")) 							 //如果接收到OK，表示成功
//			break;       						    						 //主动跳出while循环
//		printf("GA7 ATE0 ERROR! TIMEOUT is %d ",timeout);           //串口输出信息及超时时间
//	}
//	printf("GA7 ATE0 Success!\r\n");                              //串口输出信息
//	if(timeout<=0){
//		GA7ATE0_Flag=1;
//		return 1;
//	}                         //如果timeout<=0，说明超时时间到了，也没能收到"OK"，返回1
//	GA7ATE0_Flag=0;
//	return 0;                                       //正确，返回0
//}

///*-------------------------------------------------*/
///*函数名：GA7网络连接判断                          */
///*参  数：timeout：超时时间（500ms的倍数）         */
///*返回值：0：正确   其他：错误                     */
///*-------------------------------------------------*/
//char GA7_Internet(int timeout)
//{		
//	GA7_RxCounter=0;                             //接收数据量变量清零                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //清空接收缓冲区 
//	GA7_printf("AT+CREG?\r\n"); 								 //发送指令"AT+CREG?",判断模块是否驻网	
//	while(timeout--){                            //等待超时时间到0
//		delay_ms(500);                             //延时500ms
//		if(strstr(GA7_RX_BUF,"+CREG: 0,6\r\n\r\nOK")) 	 //如果接收到+CREG: 0,6\r\n\r\nOK，表示成功驻网
//			break;       						    						 //主动跳出while循环
//		printf("GA7 Internet ERROR! TIMEOUT is %d ",timeout);           //串口输出信息及超时时间
//	}
//	printf("Internet Connect Success!\r\n");                              //串口输出信息
//	if(timeout<=0){
//		GA7Internet_Flag=1;
//		return 1;
//	}                         //如果timeout<=0，说明超时时间到了，也没能收到"OK"，返回1
//	GA7Internet_Flag=0;
//	return 0;                                       //正确，返回0
//}

///*-------------------------------------------------*/
///*函数名：GA7心跳包设置                            */
///*参  数：timeout：超时时间（100ms的倍数）         */
///*返回值：0：正确   其他：错误                     */
///*-------------------------------------------------*/
//char GA7_KeepAlive(int timeout)
//{		
//	GA7_RxCounter=0;                             //接收数据量变量清零                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //清空接收缓冲区 
//	GA7_printf("AT+ECMTCFG=\"keepalive\",0,60\r\n"); //发送AT指令"AT+ECMTCFG=\"keepalive\",0,60",设置模块心跳
//	while(timeout--){                            //等待超时时间到0
//		delay_ms(100);                             //延时500ms
//		if(strstr(GA7_RX_BUF,"OK")) 	 						 //如果接收到OK，表示设置成功
//			break;       						    						 //主动跳出while循环
//	}
//	if(timeout<=0){
//		return 1;
//	}                         //如果timeout<=0，说明超时时间到了，也没能收到"OK"，返回1
//	return 0;                                     //正确，返回0
//}

///*-------------------------------------------------*/
///*函数名：GA7连接MQTT服务器							             */
///*参  数：timeout： 超时时间（500ms的倍数）   		     */
///*返回值：0：正确  其他：错误                      */
///*-------------------------------------------------*/
//char GA7_Mqtt(int timeout)
//{		
//	GA7_RxCounter=0;                             //接收数据量变量清零                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //清空接收缓冲区 
//	GA7_printf("AT+ECMTOPEN=0,\"39.101.73.149\",1883\r\n"); //发送AT指令"AT+ECMTOPEN=0,\"39.101.73.149\",1883",连接MQTT服务器
//	while(timeout--){                            //等待超时时间到0
//		delay_ms(500);                             //延时500ms
//		if(strstr(GA7_RX_BUF,"+ECMTOPEN: 0,0")) 	 //如果接收到+ECMTOPEN: 0,0，表示连接成功
//			break;       						    						 //主动跳出while循环
//			printf("GA7 MQTT ERROR! TIMEOUT is %d ",timeout);           //串口输出信息及超时时间
//	}
//		printf("GA7 MQTT Success!\r\n");                              //串口输出信息
//	if(timeout<=0){
//		GA7MQTT_Flag=1;
//		return 1;
//	}                         //如果timeout<=0，说明超时时间到了，也没能收到"OK"，返回1
//	GA7MQTT_Flag=0;
//	return 0;                                     //正确，返回0
//}

///*-------------------------------------------------*/
///*函数名：GA7注册MQTT客户端						             */
///*参  数：timeout： 超时时间（500ms的倍数）        */
///*返回值：0：正确  其他：错误                      */
///*-------------------------------------------------*/
//char GA7_Client(int timeout)
//{		
//	GA7_RxCounter=0;                             //接收数据量变量清零                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //清空接收缓冲区 
//	GA7_printf("AT+ECMTCONN=0,\"Hardware_Client\"\r\n"); //发送AT指令"AT+ECMTCONN=0,"Hardware_Client",注册客户端
//	while(timeout--){                            //等待超时时间到0
//		delay_ms(500);                             //延时500ms
//		if(strstr(GA7_RX_BUF,"+ECMTSTAT: 0,1\r\n\r\n+ECMTCONN: 0,0,0")) 	 //如果接收到+ECMTSTAT: 0,1\r\n\r\n+ECMTCONN: 0,0,0，表示注册成功
//			break;       						    						 //主动跳出while循环
//			printf("GA7 Client ERROR! TIMEOUT is %d ",timeout);           //串口输出信息及超时时间
//	}
//		printf("GA7 Client Success!\r\n");                              //串口输出信息
//	if(timeout<=0){
//		GA7Client_Flag=1;
//		return 1;
//	}                         //如果timeout<=0，说明超时时间到了，也没能收到"OK"，返回1
//	GA7Client_Flag=0;
//	return 0;                                     //正确，返回0
//}

///*-------------------------------------------------*/
///*函数名：GA7注册MQTT客户端						             */
///*参  数：timeout： 超时时间（500ms的倍数）        */
///*返回值：0：正确  其他：错误                      */
///*-------------------------------------------------*/
//char GA7_Topic(int timeout)
//{		
//	GA7_RxCounter=0;                             //接收数据量变量清零                        
//	memset(GA7_RX_BUF,0,GA7_RXBUFF_SIZE);        //清空接收缓冲区 
//	GA7_printf("AT+ECMTSUB=0,1,\"/Pc_Control\",2\r\n"); //发送AT指令"AT+ECMTSUB=0,1,"/Pc_Control",2",订阅话题
//	while(timeout--){                            //等待超时时间到0
//		delay_ms(500);                             //延时500ms
//		if(strstr(GA7_RX_BUF,"OK\r\n\r\n+ECMTSUB: 0,1,0,1")) 	 //如果接收到OK\r\n\r\n+ECMTSUB: 0,1,0,1，表示订阅成功
//			break;       						    						 //主动跳出while循环
//			printf("GA7 Topic ERROR! TIMEOUT is %d ",timeout);           //串口输出信息及超时时间
//	}
//		printf("GA7 Topic Success!\r\n");                              //串口输出信息
//	if(timeout<=0){
//		GA7Topic_Flag=1;
//		return 1;
//	}                         //如果timeout<=0，说明超时时间到了，也没能收到"OK"，返回1
//	GA7Topic_Flag=0;
//	GA7_RxCounter=0;                             //接收数据量变量清零
//	return 0;                                     //正确，返回0
//}
