/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "dma.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
//#include "usb_device.h"
//#include "usb_otg_hs.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "rl_fs.h"                      // Keil.MDK-Plus::File System:CORE
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE

//#include "usbd_cdc_if.h"

#define BT_port GPIOE
#define rd(k)  HAL_GPIO_ReadPin(BT_port, k)

#define BT_SET  GPIO_PIN_2
#define BT_UP   GPIO_PIN_3
#define BT_DW   GPIO_PIN_4	

void toggle_led1 (void const *arg);
void toggle_led2 (void const *arg);
void show7segment (void const *arg);
void readswitch (void const *arg);

osThreadDef (toggle_led1, osPriorityNormal, 1, 0); 	 		
osThreadDef (toggle_led2, osPriorityNormal, 1, 0); 
osThreadDef (show7segment, osPriorityNormal, 1, 0); 
osThreadDef (readswitch, osPriorityNormal, 1, 0); 

osThreadId ID_th1;
osThreadId ID_th2;
osThreadId ID_th3;
osThreadId ID_th4;

osSemaphoreDef(accessdata);
osSemaphoreId (accessdata_id);

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

FILE *fl;
RTC_TimeTypeDef Timeupdate = {0} ;
RTC_DateTypeDef Dateupdate = {0};
RTC_TimeTypeDef Timeset = {0} ;
RTC_DateTypeDef Dateset = {0};


uint8_t clockdata[4] = {0};
uint8_t datedata[4] = {0};
uint8_t timedata[8] = {0};
uint8_t menusetup;

//const char digits[]= {128,64,32,16,8,4,2,1 };
const char digits[]= {1,2,4,8,16,32,64,128 };
						//digit   1 2 3 4 not
//const char digits[]= {1,2,4,8,0};

//character set (0-9)0-9
  //            (10-19)0.-9.
  //            (20-45)A-Z
  //            (46-71)a-z
  //            (72)- (73) space
const char characters[]= {
	//0  1    2     3   4    5    6    7    8    9    10
 0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xFF,
}; //72-73
//        0           1          2        3					4					5			     6			7
//enum{standby, setMinutes, setHours ,setDate, setMonth, setWeekday, setYear ,save};
enum{standby, setMinutes, setHours ,setDate, setMonth,save};
uint8_t modesetup = standby;
		
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

uint16_t spitxcount;
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi->Instance == SPI1){
		spitxcount++;
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET); 
		//osDelay(1);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET); 
	}
  
}



/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

int32_t udp_sock; 

// Notify the user application about UDP socket events.
uint32_t udp_cb_func (int32_t socket, const NET_ADDR *addr, const uint8_t *buf, uint32_t len) {

	return 0;
}


// Send UDP data to destination client.
void test_send_udp_data (void) {

  if (udp_sock >= 0) {
    // IPv4 address: 192.168.0.1
    NET_ADDR addr = { NET_ADDR_IP4, 20001, 192, 168, 1, 10 };
  
    uint8_t *sendbuf;
		const char my_name[] = "P'Boat.";
		uint16_t len = sizeof(my_name);	
 
    sendbuf = netUDP_GetBuffer ( len );
		
		memcpy ( sendbuf , my_name , len );
	
		netUDP_Send (udp_sock, &addr, sendbuf, len );
		
  }
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all
	peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
	
	static fsStatus SDCard_State;
	
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	
	osKernelInitialize();
	netInitialize();
	
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_CRC_Init();
  MX_RTC_Init();
//  MX_USB_DEVICE_Init();
//  MX_IWDG_Init();
  MX_SPI2_Init();
  MX_USART6_UART_Init();
//  MX_USB_OTG_HS_PCD_Init();
  /* USER CODE BEGIN 2 */
	
	for(uint8_t i =0; i<255; i++)
  {
		HAL_IWDG_Refresh(&hiwdg);
		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_6);
		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_7);
		osDelay(20);
  }
	
	SDCard_State = finit ("M0:");
	SDCard_State = fmount ("M0:");
	osDelay(5);
	fl = fopen ("M0:Logging.CSV","a");     /* Open a file from current drive. */
	
	osKernelStart();
	accessdata_id = osSemaphoreCreate(osSemaphore(accessdata), 1);
	ID_th1 = osThreadCreate(osThread(toggle_led1), NULL);
	ID_th2 = osThreadCreate(osThread(toggle_led2), NULL);
	ID_th3 = osThreadCreate(osThread(show7segment), NULL);
	ID_th4 = osThreadCreate(osThread(readswitch), NULL);
	
	udp_sock = netUDP_GetSocket (udp_cb_func);
	if (udp_sock >= 0) {
		netUDP_Open (udp_sock,1138);
	}
	

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		
		//HAL_IWDG_Refresh(&hiwdg);
		osDelay(200);
		
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

uint16_t fcount=1 ;
void toggle_led1(void const * arg )
{

	while (1)
  {
		
		if(fl != NULL){
			if(fcount % 10 == 0){			
				fprintf(fl, "Index(%d)%d:%d:%d %d/%d/%d\n",fcount,Timeupdate.Hours,Timeupdate.Minutes,Timeupdate.Seconds,Dateupdate.Date,Dateupdate.Month,Dateupdate.Year);
				fclose(fl);
				fl = fopen ("M0:Logging.CSV","a"); 
				
			}
			else {
				
				fprintf(fl, "Index(%d)%d:%d:%d %d/%d/%d,",fcount,Timeupdate.Hours,Timeupdate.Minutes,Timeupdate.Seconds,Dateupdate.Date,Dateupdate.Month,Dateupdate.Year);
				
			}
			
			fcount++;
		}
		
		test_send_udp_data ();
		
		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_6);
		osDelay(1000);		
	}
		 
}

void toggle_led2(void const * arg )
{
	while (1)
  {
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_SET); //debug time
		
		switch (modesetup)
		{
			case standby :
				HAL_RTC_GetTime(&hrtc, &Timeupdate, FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &Dateupdate, FORMAT_BIN);
				
				//for 7segment 4 digit		
//				osSemaphoreWait(accessdata_id, osWaitForever);
//				// Date
//				datedata[3] = characters[Dateupdate.Date / 10];
//				datedata[2] = characters[Dateupdate.Date % 10];
//				datedata[2] = datedata[2] &= ~(0x80); 
//				datedata[1] = characters[Dateupdate.Month / 10];
//				datedata[0] = characters[Dateupdate.Month % 10];
//				
//				// Time
//				clockdata[3] = characters[Timeupdate.Hours / 10];
//				
//				if(Timeupdate.Seconds % 2){
//					clockdata[2] = characters[(Timeupdate.Hours % 10)]; 
//				}
//				else{
//					clockdata[2] = characters[(Timeupdate.Hours % 10)]; 
//					clockdata[2] = clockdata[2] &= ~(0x80);  
//				}
//				clockdata[1] = characters[Timeupdate.Minutes / 10] ;
//				clockdata[0] = characters[Timeupdate.Minutes % 10] ;
//				osSemaphoreRelease(accessdata_id);

			   // for 7segment 8 digit
				osSemaphoreWait(accessdata_id, osWaitForever);
				// Date
				timedata[7] = characters[Dateupdate.Date / 10];
				if(timedata[7] == characters[0])timedata[7] = characters[10]; //show blank
				timedata[6] = characters[Dateupdate.Date % 10];
				timedata[6] = timedata[6] &= ~(0x80); 
				timedata[5] = characters[Dateupdate.Month / 10];
				timedata[4] = characters[Dateupdate.Month % 10];
				
				// Time
				timedata[3] = characters[Timeupdate.Hours / 10];
				if(timedata[3] == characters[0])timedata[3] = characters[10]; //show blank
				if(Timeupdate.Seconds % 2){
					timedata[2] = characters[(Timeupdate.Hours % 10)]; 
				}
				else{
					timedata[2] = characters[(Timeupdate.Hours % 10)]; 
					timedata[2] = timedata[2] &= ~(0x80);  //add dot
				}
				
				timedata[1] = characters[Timeupdate.Minutes / 10] ;
				timedata[0] = characters[Timeupdate.Minutes % 10] ;
				osSemaphoreRelease(accessdata_id);

				break;
				
			case setMinutes : 
				osSemaphoreWait(accessdata_id, osWaitForever);
				// Date
				timedata[7] = characters[10]; // show Blank
				timedata[6] = characters[10]; // show Blank

				timedata[5] = characters[10]; // show Blank
				timedata[4] = characters[10]; // show Blank
				
				// Time
				timedata[3] = characters[10]; // show Blank
				timedata[2] = characters[10]; // show Blank
				
				timedata[1] = characters[Timeset.Minutes / 10] ;
				timedata[0] = characters[Timeset.Minutes % 10] ;
				timedata[0] = timedata[0] &= ~(0x80); 
				osSemaphoreRelease(accessdata_id);
				
				break;
			case setHours :
				osSemaphoreWait(accessdata_id, osWaitForever);
				// Date
				timedata[7] = characters[10]; // show Blank
				timedata[6] = characters[10]; // show Blank
	
				timedata[5] = characters[10]; // show Blank
				timedata[4] = characters[10]; // show Blank
				
				// Time
				timedata[3] = characters[Timeset.Hours / 10];
				timedata[2] = characters[(Timeset.Hours % 10)]; 
				timedata[2] = timedata[2] &= ~(0x80); 

				timedata[1] = characters[10] ; // show Blank
				timedata[0] = characters[10] ; // show Blank
				
				osSemaphoreRelease(accessdata_id);
				
				break;
			case setDate : 
				osSemaphoreWait(accessdata_id, osWaitForever);
				// Date
				timedata[7] = characters[Dateset.Date / 10];
				timedata[6] = characters[Dateset.Date % 10];
				timedata[6] = timedata[6] &= ~(0x80);

				timedata[5] = characters[10]; // show Blank
				timedata[4] = characters[10]; // show Blank
				
				// Time
				timedata[3] = characters[10]; // show Blank
				timedata[2] = characters[10]; // show Blank
				 
				timedata[1] = characters[10] ; // show Blank
				timedata[0] = characters[10] ; // show Blank
				
				osSemaphoreRelease(accessdata_id);
				break;
			case setMonth :
				osSemaphoreWait(accessdata_id, osWaitForever);
				// Date
				timedata[7] = characters[10]; // show Blank
				timedata[6] = characters[10]; // show Blank
				
				timedata[5] = characters[Dateset.Month / 10];
				timedata[4] = characters[Dateset.Month % 10];
				timedata[4] = timedata[4] &= ~(0x80);
			
				// Time 
				timedata[3] = characters[10]; // show Blank
				timedata[2] = characters[10]; // show Blank
				 
				timedata[1] = characters[10]; // show Blank
				timedata[0] = characters[10]; // show Blank
				
				osSemaphoreRelease(accessdata_id);
				break;
			
			default:
				break;
		}
				
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_RESET); //debug time
		osDelay(500);
  }
}

void show7segment(void const * arg )
{
	static uint8_t digitcount;
	uint8_t spibuff[4] = {0};
	while (1)
  {
		
		osSemaphoreWait(accessdata_id, osWaitForever);
			//date 4 digit
//		spibuff[0] = (datedata[digitcount]);
//		spibuff[1] = (digits[digitcount]);
					//  jj
//		spibuff[2] = (clockdata[digitcount]);
//		spibuff[3] = (digits[digitcount]);
		
		//		8 digit
		spibuff[0] = (timedata[digitcount]);
		spibuff[1] = (digits[digitcount]);
		
		osSemaphoreRelease(accessdata_id);
		
		HAL_SPI_Transmit_IT(&hspi1, spibuff, 2);
		
		if(++digitcount > 7){
			digitcount = 0;
		}
		
		osDelay(2);
  }
}

uint16_t menutimecount;
void readswitch(void const * arg )
{

	while (1)
  {

		if(!rd(BT_SET))
		{
			osDelay(10);
			if(!rd(BT_SET))
			{
				menutimecount = 1500;
				if(modesetup == standby){
					Timeset = Timeupdate;
					Dateset = Dateupdate;
				}
				if(++modesetup >= save ){
					modesetup = standby;
					HAL_RTC_SetTime(&hrtc, &Timeset, RTC_FORMAT_BIN);
					HAL_RTC_SetDate(&hrtc, &Dateset, RTC_FORMAT_BIN);
					osDelay(300);
					
				}
				while (!rd(BT_SET))
        {
					osDelay(1);
        }
			}
		}
		else if((!rd(BT_UP)) && (modesetup > standby) )
		{
			osDelay(10);
			if(!rd(BT_UP))
			{
				menutimecount = 1500;
				switch (modesetup)
        {
        	case setMinutes : 
						if(++Timeset.Minutes >59) Timeset.Minutes = 0;
        		break;
        	case setHours :
						if(++Timeset.Hours > 23) Timeset.Hours = 0;
        		break;
					case setDate : 
						if(++Dateset.Date > 31 ) Dateset.Date = 1 ;
        		break;
        	case setMonth :
						if(++Dateset.Month > 12 ) Dateset.Month = 1 ;
        		break;
//					case setWeekday :
//						if(++Dateset.WeekDay > RTC_WEEKDAY_SUNDAY ) Dateset.WeekDay = RTC_WEEKDAY_MONDAY ;
//        		break;
//					case setYear :
//						if(++Dateset.Year > 99 ) Dateset.Year = 0 ;
//        		break;
				
        	default:
        		break;
        }
				while (!rd(BT_UP))
        {
					osDelay(1);
        }
			}
			
		}
		else if((!rd(BT_DW)) && (modesetup > standby) )
		{
			osDelay(10);
			if(!rd(BT_DW))
			{
				menutimecount = 1500;
				switch (modesetup)
        {
        	case setMinutes : 
						if(--Timeset.Minutes > 100) Timeset.Minutes = 59;
        		break;
        	case setHours :
						if(--Timeset.Hours > 100) Timeset.Hours = 23;
        		break;
					case setDate : 
						if(--Dateset.Date > 100 ) Dateset.Date = 31 ;
        		break;
        	case setMonth :
						if(--Dateset.Month > 100 ) Dateset.Month = 31 ;
        		break;
	
        	default:
        		break;
        }
				while (!rd(BT_DW))
        {
					osDelay(1);
        }
			}
		}
		
	 osDelay(10);
		
		if(menutimecount){
			if(--menutimecount==0)
			{
				modesetup = standby;
			}
		}
		
  }
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
