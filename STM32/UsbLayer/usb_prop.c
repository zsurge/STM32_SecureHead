/**
  ******************************************************************************
  * @file    usb_prop.c
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    29-June-2012
  * @brief   All processings related to Custom HID Demo
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
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_prop_hid.h"
#include "usb_endp.h"
#include "app.h"
extern u8 *usbdata_Request(void);
extern u8 *My_Data_Request(u16 length);
//extern char usb_reviceflag;
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t ProtocolValue;
__IO uint8_t EXTI_Enable;

uint8_t IDTUSB_Reviceflag=0xac,IDTUSB_SendBuff[idte_datalen]={0x00},usbd_sendlen=0,sendcount=0;

u8 PF_Buffer[55]={0};
/* -------------------------------------------------------------------------- */
/*  Structures initializations */
/* -------------------------------------------------------------------------- */

DEVICE Device_Table =
  {
    EP_NUM,
    1
  };

DEVICE_PROP Device_Property_Custom =
  {
    CustomHID_init,
    CustomHID_Reset,
    CustomHID_Status_In,
    CustomHID_Status_Out,
    CustomHID_Data_Setup,
    CustomHID_NoData_Setup,
    CustomHID_Get_Interface_Setting,
    CustomHID_GetDeviceDescriptor,
    CustomHID_GetConfigDescriptor,
    CustomHID_GetStringDescriptor,
    0,
    0x40 /*MAX PACKET SIZE*/
  };
USER_STANDARD_REQUESTS User_Standard_Requests_Custom =
  {
    CustomHID_GetConfiguration,
    CustomHID_SetConfiguration,
    CustomHID_GetInterface,
    CustomHID_SetInterface,
    CustomHID_GetStatus,
    CustomHID_ClearFeature,
    CustomHID_SetEndPointFeature,
    CustomHID_SetDeviceFeature,
    CustomHID_SetDeviceAddress
  };

ONE_DESCRIPTOR Device_Descriptor_Custom =
  {
    (uint8_t*)CustomHID_DeviceDescriptor,
    CUSTOMHID_SIZ_DEVICE_DESC
  };

ONE_DESCRIPTOR Config_Descriptor_Custom =
  {
    (uint8_t*)CustomHID_ConfigDescriptor,
    CUSTOMHID_SIZ_CONFIG_DESC
  };

ONE_DESCRIPTOR CustomHID_Report_Descriptor =
  {
    (uint8_t *)CustomHID_ReportDescriptor,
    CUSTOMHID_SIZ_REPORT_DESC
  };

ONE_DESCRIPTOR CustomHID_Hid_Descriptor =
  {
    (uint8_t*)CustomHID_ConfigDescriptor + CUSTOMHID_OFF_HID_DESC,
    CUSTOMHID_SIZ_HID_DESC
  };

ONE_DESCRIPTOR String_Descriptor_Custom[4] =
  {
    {(uint8_t*)CustomHID_StringLangID, CUSTOMHID_SIZ_STRING_LANGID},
    {(uint8_t*)CustomHID_StringVendor, CUSTOMHID_SIZ_STRING_VENDOR},
    {(uint8_t*)CustomHID_StringProduct, CUSTOMHID_SIZ_STRING_PRODUCT},
    {(uint8_t*)CustomHID_StringSerial, CUSTOMHID_SIZ_STRING_SERIAL}
  };

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

	
void Custom_Init(void)
{
	Device_Property=Device_Property_Custom;
  	User_Standard_Requests=User_Standard_Requests_Custom;
	
	
}
/*******************************************************************************
* Function Name  : CustomHID_init.
* Description    : Custom HID init routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void CustomHID_init(void)
{
  /* Update the serial number string descriptor with the data from the unique 
  ID*/
  Get_SerialNum();
    
  pInformation->Current_Configuration = 0;
  /* Connect the device */
  PowerOn();

  /* Perform basic device initialization operations */
  USB_SIL_Init();

  bDeviceState = UNCONNECTED;
}

/*******************************************************************************
* Function Name  : CustomHID_Reset.
* Description    : Custom HID reset routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void CustomHID_Reset(void)
{
  /* Set Joystick_DEVICE as not configured */
  pInformation->Current_Configuration = 0;
  pInformation->Current_Interface = 0;/*the default Interface*/
  
  /* Current Feature initialization */
  pInformation->Current_Feature = CustomHID_ConfigDescriptor[7];
  
#ifdef STM32F10X_CL   
  /* EP0 is already configured in DFU_Init() by USB_SIL_Init() function */
  
  /* Init EP1 IN as Interrupt endpoint */
  OTG_DEV_EP_Init(EP1_IN, OTG_DEV_EP_TYPE_INT, 2);
  
  /* Init EP1 OUT as Interrupt endpoint */
  OTG_DEV_EP_Init(EP1_OUT, OTG_DEV_EP_TYPE_INT, 2);
#else 
  SetBTABLE(BTABLE_ADDRESS);

/* Initialize Endpoint 0 */
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, /*EP_TX_STALL*/EP_TX_VALID);//原函数为  SetEPTxStatus(ENDP0, EP_TX_STALL);/
	SetEPRxStatus(ENDP0, /*EP_TX_STALL*/EP_RX_VALID);//新添加-18-06-12
//	SetEPTxStatus(ENDP0, EP_RX_STALL);//--新添加-18-06-05
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);
	
	SetEPTxCount(ENDP0, Device_Property.MaxPacketSize);//-----新添加18-06-12
  SetEPTxValid(ENDP0);//-----新添加18-06-12

  /* Initialize Endpoint 1 */
  SetEPType(ENDP1, EP_INTERRUPT);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPTxCount(ENDP1, 0x40);
  SetEPRxStatus(ENDP1, EP_RX_DIS);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
//  /* Initialize Endpoint 0 */
//  SetEPType(ENDP0, EP_CONTROL);
//  SetEPTxStatus(ENDP0, EP_TX_STALL);
//  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
//  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
//  Clear_Status_Out(ENDP0);
//  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
//  SetEPRxValid(ENDP0);


//  /* Initialize Endpoint 1 */
//  SetEPType(ENDP1, EP_INTERRUPT);
//  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
//  SetEPRxAddr(ENDP1, ENDP1_RXADDR);
//  SetEPTxCount(ENDP1, PACKAGE_SIZE);	//ONE
//  SetEPRxCount(ENDP1, PACKAGE_SIZE);	//
//  SetEPRxStatus(ENDP1, EP_RX_VALID);
//  SetEPTxStatus(ENDP1, EP_TX_NAK);

  /* Set this device to response on default address */
  SetDeviceAddress(0);
#endif /* STM32F10X_CL */

  bDeviceState = ATTACHED;

}
/*******************************************************************************
* Function Name  : CustomHID_SetConfiguration.
* Description    : Update the device state to configured and command the ADC 
*                  conversion.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void CustomHID_SetConfiguration(void)
{
  if (pInformation->Current_Configuration != 0)
  {
    /* Device configured */
    bDeviceState = CONFIGURED;
    
  }
}


u8 *PF_Data_Request(u16 length)
{
	
    if (length == 0)
        return (u8*)55;    // 假定你的REPORT长度和Buffer长度为10

     return &PF_Buffer[pInformation->Ctrl_Info.Usb_wOffset];;
}
/*******************************************************************************
* Function Name  : CustomHID_SetConfiguration.
* Description    : Update the device state to addressed.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void CustomHID_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}
/*******************************************************************************
* Function Name  : CustomHID_Status_In.
* Description    : Joystick status IN routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void CustomHID_Status_In(void)
{
	bDeviceState = CONFIGURED;
	
//	DBG_H("USB_Buff-12345",USB_Buff,8);
}

/*******************************************************************************
* Function Name  : CustomHID_Status_Out
* Description    : Joystick status OUT routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void CustomHID_Status_Out (void)
{
	bDeviceState = CONFIGURED;
	
//	DBG_H("USB_Buff-0987",USB_Buff,8);
}

/*******************************************************************************
* Function Name  : CustomHID_Data_Setup
* Description    : Handle the data class specific requests.
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT CustomHID_Data_Setup(uint8_t RequestNo)
{
	uint8_t *(*CopyRoutine)(uint16_t);

	uint8_t buff[8]={0};
  CopyRoutine = NULL;

  if ((RequestNo == GET_DESCRIPTOR)
      && (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
      && (pInformation->USBwIndex0 == 0))
  {

    if (pInformation->USBwValue1 == REPORT_DESCRIPTOR)
    {
      CopyRoutine = CustomHID_GetReportDescriptor;
    }
    else if (pInformation->USBwValue1 == HID_DESCRIPTOR_TYPE)
    {
      CopyRoutine = CustomHID_GetHIDDescriptor;
    }

  } /* End of GET_DESCRIPTOR */

  /*** GET_PROTOCOL ***/
  else if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
           && RequestNo == GET_PROTOCOL)
  {
    CopyRoutine = CustomHID_GetProtocolValue;
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
 
			UserToPMABufferCopy(buff, GetEPTxAddr(ENDP0), 8); 
			SetEPTxValid(ENDP0);
			SetEPTxCount(ENDP0, 8);
		  sendcount++;
			while(_GetEPTxStatus(ENDP0)!=(0x02<<4));//等待USB端点0发送完成
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
			UserToPMABufferCopy(buff, GetEPTxAddr(ENDP0), 8); 
		  SetEPTxValid(ENDP0);
			SetEPTxCount(ENDP0, 8);
		
		}while(_GetEPTxStatus(ENDP0)!=(0x02<<4));

			pInformation->Ctrl_Info.Usb_wLength =8;
		  return USB_SUCCESS;//0x5A;//0x4B;///
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

/*******************************************************************************
* Function Name  : CustomHID_NoData_Setup
* Description    : handle the no data class specific requests
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT CustomHID_NoData_Setup(uint8_t RequestNo)
{
  if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
      && (RequestNo == SET_PROTOCOL))
  {
    return CustomHID_SetProtocol();
  }
	
		else if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))//新添加2018-06-05
      && (RequestNo == SET_IDLE))
  {
    return USB_SUCCESS;
  }


  else
  {
    return USB_UNSUPPORT;
  }
	
}

/*******************************************************************************
* Function Name  : CustomHID_GetDeviceDescriptor.
* Description    : Gets the device descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the device descriptor.
*******************************************************************************/
uint8_t *CustomHID_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor_Custom);
}

/*******************************************************************************
* Function Name  : CustomHID_GetConfigDescriptor.
* Description    : Gets the configuration descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *CustomHID_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor_Custom);
}

/*******************************************************************************
* Function Name  : CustomHID_GetStringDescriptor
* Description    : Gets the string descriptors according to the needed index
* Input          : Length
* Output         : None.
* Return         : The address of the string descriptors.
*******************************************************************************/
uint8_t *CustomHID_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  if (wValue0 > 4)
  {
    return NULL;
  }
  else 
  {
    return Standard_GetDescriptorData(Length, &String_Descriptor_Custom[wValue0]);
  }
}

/*******************************************************************************
* Function Name  : CustomHID_GetReportDescriptor.
* Description    : Gets the HID report descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *CustomHID_GetReportDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &CustomHID_Report_Descriptor);
}

/*******************************************************************************
* Function Name  : CustomHID_GetHIDDescriptor.
* Description    : Gets the HID descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *CustomHID_GetHIDDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &CustomHID_Hid_Descriptor);
}

/*******************************************************************************
* Function Name  : CustomHID_Get_Interface_Setting.
* Description    : tests the interface and the alternate setting according to the
*                  supported one.
* Input          : - Interface : interface number.
*                  - AlternateSetting : Alternate Setting number.
* Output         : None.
* Return         : USB_SUCCESS or USB_UNSUPPORT.
*******************************************************************************/
RESULT CustomHID_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
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
* Function Name  : CustomHID_SetProtocol
* Description    : Joystick Set Protocol request routine.
* Input          : None.
* Output         : None.
* Return         : USB SUCCESS.
*******************************************************************************/
RESULT CustomHID_SetProtocol(void)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  ProtocolValue = wValue0;
  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : CustomHID_GetProtocolValue
* Description    : get the protocol value
* Input          : Length.
* Output         : None.
* Return         : address of the protocol value.
*******************************************************************************/
uint8_t *CustomHID_GetProtocolValue(uint16_t Length)
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
