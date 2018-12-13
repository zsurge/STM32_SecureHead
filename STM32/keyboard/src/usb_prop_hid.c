/**
  ******************************************************************************
  * @file    usb_prop.c
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    29-June-2012
  * @brief   All processing related to Keyboard Mouse Demo
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
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop_hid.h"
#include "usb_desc_hid.h"
#include "usb_pwr.h"
#include "hw_config.h"
#include "usb_prop.h"
#include "usb_desc_hid.h"
#include "usb_core.h"
#include "string.h"
#include "app.h"
#include "usb_prop.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern uint32_t ProtocolValue/*,keyboard_idlestate*/;
char /*USB_Buff[8]={0},*/usb_reviceflag=0;

extern uint8_t IDTUSB_Reviceflag,IDTUSB_SendBuff[idte_datalen],usbd_sendlen,sendcount;
u8 My_Buffer[82];
//void DataStageIn(void);
//extern uint8_t Keyboard_StringSerial[38];
/* -------------------------------------------------------------------------- */
/*  Structures initializations */
/* -------------------------------------------------------------------------- */
/*
DEVICE Device_Table =
  {
    EP_NUM,
    1
  };  */

DEVICE_PROP Device_Property_Hid =
  {
    Keyboard_init,
    Keyboard_Reset,
    Keyboard_Status_In,
    Keyboard_Status_Out,
    Keyboard_Data_Setup,
    Keyboard_NoData_Setup,
    Keyboard_Get_Interface_Setting,
    Keyboard_GetDeviceDescriptor,
    Keyboard_GetConfigDescriptor,
    Keyboard_GetStringDescriptor,
    0,
    0x40 /*MAX PACKET SIZE*/
  };
USER_STANDARD_REQUESTS User_Standard_Requests_Hid =
  {
    Keyboard_GetConfiguration,
    Keyboard_SetConfiguration,
    Keyboard_GetInterface,
    Keyboard_SetInterface,
    Keyboard_GetStatus,
    Keyboard_ClearFeature,
    Keyboard_SetEndPointFeature,
    Keyboard_SetDeviceFeature,
    Keyboard_SetDeviceAddress,
//		Keyboard_SetReport
  };

ONE_DESCRIPTOR Device_Descriptor_Hid =
  {
    (uint8_t*)Keyboard_DeviceDescriptor,
    KEYBOARD_SIZ_DEVICE_DESC
  };

ONE_DESCRIPTOR Config_Descriptor_Hid =
  {
    (uint8_t*)Keyboard_ConfigDescriptor,
    KEYBOARD_SIZ_CONFIG_DESC
  };

ONE_DESCRIPTOR Keyboard_Report_Descriptor =
  {
    (uint8_t *)Keyboard_ReportDescriptor,
    KEYBOARD_SIZ_REPORT_DESC
  };

ONE_DESCRIPTOR Mouse_Hid_Descriptor =
  {
    (uint8_t*)Keyboard_ConfigDescriptor + KEYBOARD_OFF_HID_DESC,
    KEYBOARD_SIZ_HID_DESC
  };

ONE_DESCRIPTOR String_Descriptor_Hid[4] =
  {
    {(uint8_t*)Keyboard_StringLangID, KEYBOARD_SIZ_STRING_LANGID},
    {(uint8_t*)Keyboard_StringVendor, KEYBOARD_SIZ_STRING_VENDOR},
    {(uint8_t*)Keyboard_StringProduct, KEYBOARD_SIZ_STRING_PRODUCT},
    {(uint8_t*)Keyboard_StringSerial, KEYBOARD_SIZ_STRING_SERIAL}
  };

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : USB_Hid_init.
* Description    : Device_Property init routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Keyboard_hid_init(void)
{
	Device_Property=Device_Property_Hid;
  	User_Standard_Requests=User_Standard_Requests_Hid;


}
/*******************************************************************************
* Function Name  : Keyboard_init.
* Description    : Keyboard Mouse init routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Keyboard_init(void)
{

  /* Update the serial number string descriptor with the data from the unique
  ID*/
  Get_SerialNum();

  pInformation->Current_Configuration = 0;
  /* Connect the device */
  PowerOn();
#if 0
  /* Perform basic device initialization operations */
  USB_SIL_Init();
#else
  /* USB interrupts initialization */
  /* clear pending interrupts */
  _SetISTR(0);
  wInterrupt_Mask =CNTR_CTRM |CNTR_RESETM| CNTR_SOFM|CNTR_WKUPM;//|CNTR_SUSPM;//|CNTR_RESUME;
  //CNTR_CTRM | CNTR_RESETM;// IMR_MSK; (CNTR_CTRM  | CNTR_SOFM  | CNTR_RESETM )
  /* set interrupts mask */
  _SetCNTR(wInterrupt_Mask);
#endif
  bDeviceState = UNCONNECTED;
//  USB_Tx_State = 0;PrevXferComplete=0;
}

/*******************************************************************************
* Function Name  : Keyboard_Reset.
* Description    : Keyboard Mouse reset routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Keyboard_Reset(void)
{
  /* Set Keyboard_DEVICE as not configured */
  pInformation->Current_Configuration = 0;
  pInformation->Current_Interface = 0;/*the default Interface*/

  /* Current Feature initialization */
  pInformation->Current_Feature = Keyboard_ConfigDescriptor[7];

#ifdef STM32F10X_CL   
  /* EP0 is already configured in DFU_Init() by USB_SIL_Init() function */
  
  /* Init EP1 IN as Interrupt endpoint */
  OTG_DEV_EP_Init(EP4_IN, OTG_DEV_EP_TYPE_INT, 4);
#else 

  SetBTABLE(BTABLE_ADDRESS);

  /* Initialize Endpoint 0 */
//  SetEPType(ENDP0, EP_CONTROL);
//  SetEPTxStatus(ENDP0, /*EP_TX_STALL*/EP_TX_VALID);//原函数为  SetEPTxStatus(ENDP0, EP_TX_STALL);/
//	SetEPRxStatus(ENDP0, /*EP_TX_STALL*/EP_RX_VALID);//新添加-18-06-12
////	SetEPTxStatus(ENDP0, EP_RX_STALL);//--新添加-18-06-05
//  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
//  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
//  Clear_Status_Out(ENDP0);
//  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
//  SetEPRxValid(ENDP0);
//	
//	SetEPTxCount(ENDP0, Device_Property.MaxPacketSize);//-----新添加18-06-12
//  SetEPTxValid(ENDP0);//-----新添加18-06-12

/* Initialize Endpoint 0 */
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);

  /* Initialize Endpoint 1 */
  SetEPType(ENDP1, EP_INTERRUPT);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPTxCount(ENDP1, 0x08);
  SetEPRxStatus(ENDP1, EP_RX_DIS);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
	
//	/* Initialize Endpoint 1 */
//  SetEPType(ENDP1, EP_INTERRUPT);
//  SetEPTxAddr(ENDP1, ENDP1_RXADDR);
//  SetEPTxCount(ENDP1, 0x08);
//  SetEPRxStatus(ENDP1, EP_RX_VALID);
//  SetEPTxStatus(ENDP1, EP_TX_DIS);

  /* Set this device to response on default address */
  SetDeviceAddress(0);
#endif /* STM32F10X_CL */

  bDeviceState = ATTACHED;
//  USB_Tx_State = 0;PrevXferComplete=0;
}
/*******************************************************************************
* Function Name  : Keyboard_SetConfiguration.
* Description    : Update the device state to configured.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Keyboard_SetConfiguration(void)
{
  DEVICE_INFO *pInfo = &Device_Info;

  if (pInfo->Current_Configuration != 0)
  {
    /* Device configured */
    bDeviceState = CONFIGURED;
  }
}
/*******************************************************************************
* Function Name  : Keyboard_SetConfiguration.
* Description    : Update the device state to addressed.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Keyboard_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}


u8 *My_Data_Request(u16 length)
{
	
    if (length == 0)
        return (u8*)82;    // 假定你的REPORT长度和Buffer长度为10

     return &My_Buffer[pInformation->Ctrl_Info.Usb_wOffset];
}

/*******************************************************************************
* Function Name  : Keyboard_Status_In.
* Description    : Keyboard status IN routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Keyboard_Status_In(void)
{


}

/*******************************************************************************
* Function Name  : Keyboard_Status_Out
* Description    : Keyboard status OUT routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Keyboard_Status_Out (void)
{
 
}




/*******************************************************************************
* Function Name  : Keyboard_Data_Setup
* Description    : Handle the data class specific requests.
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT Keyboard_Data_Setup(uint8_t RequestNo)
{

	uint8_t buff[8]={0},sbuffer[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};
  uint8_t *(*CopyRoutine)(uint16_t);

  CopyRoutine = NULL;
  if ((RequestNo == GET_DESCRIPTOR)
      && (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
      && (pInformation->USBwIndex0 == 0))
  {
    if (pInformation->USBwValue1 == REPORT_DESCRIPTOR)
    {
      CopyRoutine = Keyboard_GetReportDescriptor;
    }
    else if (pInformation->USBwValue1 == HID_DESCRIPTOR_TYPE)
    {
      CopyRoutine = Keyboard_GetHIDDescriptor;
    }

  } /* End of GET_DESCRIPTOR */

  /*** GET_PROTOCOL ***/
  else if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
           && RequestNo == GET_PROTOCOL)
  {
    CopyRoutine = Keyboard_GetProtocolValue;
  }


	/*** SET_REPORT ***/ //新添加-18-06-05
  else if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
           && RequestNo == SET_REPORT) // SET_REPORT =9
  {
		 CopyRoutine = My_Data_Request;//mKeyboard_GetReportDescriptor;//Keyboard_GetReportDescriptor;  //

		pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
		pInformation->Ctrl_Info.Usb_wLength =82;

		return USB_SUCCESS;//0x4B;//0x5A;//U//0x4B;//
  }
	
	/*** GET_REPORT ***/ //新添加-18-06-05
	else if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
           && RequestNo == GET_REPORT) //GET_REPORT = 1
  {				
		if(IDTUSB_Reviceflag==0xab)
		{
			while(_GetEPTxStatus(ENDP0)!=(0x02<<4));
			memcpy(buff,IDTUSB_SendBuff+sendcount*8,8);
      USB_SIL_Write(EP0_IN, (unsigned char*)buff, 8); 
			SetEPTxValid(ENDP0);

		  sendcount++;
//			DBG_H("buff111",buff,8);
			while(_GetEPTxStatus(ENDP0)!=(0x02<<4));
		if(sendcount == usbd_sendlen )
		{			
			sendcount=0;
			usbd_sendlen=0xdd;
			memset(IDTUSB_SendBuff,0x00,idte_datalen);
			IDTUSB_Reviceflag=0xff;
		}			
		}
		else
		{
			USB_SIL_Write(EP0_IN, (unsigned char*)sbuffer , 8);  
		  SetEPTxValid(ENDP0);

			
		}while(_GetEPTxStatus(ENDP0)!=(0x02<<4));

			pInformation->Ctrl_Info.Usb_wLength =8;
		  return USB_SUCCESS;//0x5A;//0x4B;//
  }
  if (CopyRoutine == NULL)
  {	
    return  USB_UNSUPPORT;
		
  }
  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);
  return  USB_SUCCESS;  
}

/*

    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
	  pInformation->Ctrl_Info.PacketSize = pProperty->MaxPacketSize;
    (*CopyRoutine)(0);

*/
/*******************************************************************************
* Function Name  : Keyboard_NoData_Setup
* Description    : handle the no data class specific requests
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT Keyboard_NoData_Setup(uint8_t RequestNo)
{
  if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
      && (RequestNo == SET_PROTOCOL))
  {
    return Keyboard_SetProtocol();
  }
	
	else if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))//新添加2018-06-05
      && (RequestNo == SET_IDLE))
  {
    return USB_SUCCESS;
  }

//  else
//  {
    return USB_UNSUPPORT;
//  }
}

/*******************************************************************************
* Function Name  : Keyboard_GetDeviceDescriptor.
* Description    : Gets the device descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the device descriptor.
*******************************************************************************/
uint8_t *Keyboard_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor_Hid);
}

/*******************************************************************************
* Function Name  : Keyboard_GetConfigDescriptor.
* Description    : Gets the configuration descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *Keyboard_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor_Hid);
}

/*******************************************************************************
* Function Name  : Keyboard_GetStringDescriptor
* Description    : Gets the string descriptors according to the needed index
* Input          : Length
* Output         : None.
* Return         : The address of the string descriptors.
*******************************************************************************/
uint8_t *Keyboard_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  if (wValue0 > 4)
  {
    return NULL;
  }
  else
  {
    return Standard_GetDescriptorData(Length, &String_Descriptor_Hid[wValue0]);
  }
}

/*******************************************************************************
* Function Name  : Keyboard_GetReportDescriptor.
* Description    : Gets the HID report descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *Keyboard_GetReportDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Keyboard_Report_Descriptor);
}

/*******************************************************************************
* Function Name  : Keyboard_GetHIDDescriptor.
* Description    : Gets the HID descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *Keyboard_GetHIDDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Mouse_Hid_Descriptor);
}

/*******************************************************************************
* Function Name  : Keyboard_Get_Interface_Setting.
* Description    : tests the interface and the alternate setting according to the
*                  supported one.
* Input          : - Interface : interface number.
*                  - AlternateSetting : Alternate Setting number.
* Output         : None.
* Return         : USB_SUCCESS or USB_UNSUPPORT.
*******************************************************************************/
RESULT Keyboard_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
  if (AlternateSetting > 0)
  {
    return USB_UNSUPPORT;
  }
  else if (Interface > 0)
  {
    return USB_UNSUPPORT;
  }
  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : Keyboard_SetProtocol
* Description    : Keyboard Set Protocol request routine.
* Input          : None.
* Output         : None.
* Return         : USB SUCCESS.
*******************************************************************************/
RESULT Keyboard_SetProtocol(void)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  ProtocolValue = wValue0;
  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : Keyboard_GetProtocolValue
* Description    : get the protocol value
* Input          : Length.
* Output         : None.
* Return         : address of the protocol value.
*******************************************************************************/
uint8_t *Keyboard_GetProtocolValue(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = 1;
    return NULL;
  }
  else
  {
    return (uint8_t *)(&ProtocolValue);
  }
}

uint8_t *Keyboard_SET_IDLE(uint16_t Length)
{
	return 0;
  
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
