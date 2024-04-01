
# SmartCover

基于STM32和NB-IoT的智能井盖监测系统设计

以STM32为主控，NB模块接入网络，使用MQTT协议，连接挂载于阿里云服务器的MQTT服务器EMQX进行数据上报，使用微信小程序接收展示数据

接线说明：
1.BUZZER
			1.1连线
				I/O  -->  GPIOB.12
				VCC 3.3V
				GND 
		
		2.LightSensor
			2.1连线
				DO  -->  GPIOB.13
				VCC 3.3V
				GND 
		
		3.OLED
			3.1连线
				SCK  -->  GPIOB.8
				SDA  -->  GPIOB.9
				VCC 3.3V
				GND
		
		4.MQ2
			4.1连线
				AO   -->  GPIOA.0
				VCC 3.3V
				GND 
	
		5.DHT11
			5.1连线
				DATA  -->  GPIOA.11
				VCC 3.3V
				GND

		6.HC-SR04
			6.1连线
				TRIG  -->  GPIOB.11
				ECHO  -->  GPIOB.10
				VCC 3.3V
				GND

		7.MPU6050
			7.1连线
				SCL  -->  GPIOB.6
				SDA  -->  GPIOB.7
				INT  -->  GPIOB.5
				VCC 3.3V
				GND
	
		8.USART1串口
			8.1连线		
				RX  -->  USART1_TX   GPIOA.9
				TX  -->  USART1_RX	GPIOA.10
				GND  -->  GND
				VCC 3.3V

    		9.GA7
			9.1连线
				RX  -->  USART2_TX  GPIOA.2
				TX  -->  USART2_RX	GPIOA.3
				GND  -->  GND 
				VCC 5V
    
深感开源伟大 开源精神不朽
