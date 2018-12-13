/**
  ******************************************************************************
  * @file    usb_endp.c
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    29-June-2012
  * @brief   Endpoint routines
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_prop.h"
#include "app.h"
#include "usb_endp.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern __IO uint8_t PrevXferComplete;


extern uint8_t Usb_hid_key[];






/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : EP1_OUT_Callback.
* Description    : EP1 OUT Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
//u8 Release_Key_Flag;
//void EP1_IN_Callback(void)
//{
//  u8 Key_Buffer[8] = {0,0,0,0,0,0,0,0}/*,i*/;
////  printf("Release_Key_Flag=%d\r\n",Release_Key_Flag);
//	printf("+++++Release_Key_Flag=%d\r\n",Release_Key_Flag);
//  if(Release_Key_Flag == 0)
//	{
//	if(Send_DataBuf.len > 0)
//	{
//	  EP1BUSY = 1;
//	  Release_Key_Flag = 1;
//	
//	  Key_Buffer[0] = Key_Table[Send_DataBuf.data[Send_DataBuf.read]][0];
//	  Key_Buffer[2] =	Key_Table[Send_DataBuf.data[Send_DataBuf.read]][1];

//	
//	  Send_DataBuf.len --;
//	  Send_DataBuf.read++;
//  if(Send_DataBuf.read == SEND_BUF_LENGTH)
//   {
//	   Send_DataBuf.read = 0;
//	 }
//	  USB_SIL_Write(EP1_IN, Key_Buffer, 8); 
//	  SetEPTxValid(ENDP1);
//	}
//	else
//	{
//	  EP1BUSY = 0;
//	}
//	}
//	else
//	{
//	  Release_Key_Flag = 0;
//	  EP1BUSY = 1;
//	  USB_SIL_Write(EP1_IN, Key_Buffer, 8); 
//	  SetEPTxValid(ENDP1);
//	}
//}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

