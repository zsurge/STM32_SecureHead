/**
  ******************************************************************************
  * @file    usb_prop.h
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    29-June-2012
  * @brief   All processing related to Keyboard Mouse demo
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


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_PROP_H
#define __USB_PROP_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum _HID_REQUESTS
{
  GET_REPORT = 1,
  GET_IDLE,
  GET_PROTOCOL,

  SET_REPORT = 9,
  SET_IDLE,
  SET_PROTOCOL
} HID_REQUESTS;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Keyboard_init(void);
void Keyboard_Reset(void);
void Keyboard_SetConfiguration(void);
void Keyboard_SetDeviceAddress (void);
void Keyboard_Status_In (void);
void Keyboard_Status_Out (void);

RESULT Keyboard_Data_Setup(uint8_t);
RESULT Keyboard_NoData_Setup(uint8_t);
RESULT Keyboard_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *Keyboard_GetDeviceDescriptor(uint16_t );
uint8_t *Keyboard_GetConfigDescriptor(uint16_t);
uint8_t *Keyboard_GetStringDescriptor(uint16_t);
RESULT Keyboard_SetProtocol(void);
uint8_t *Keyboard_GetProtocolValue(uint16_t Length);
RESULT Keyboard_SetProtocol(void);
uint8_t *Keyboard_GetReportDescriptor(uint16_t Length);
uint8_t *Keyboard_GetHIDDescriptor(uint16_t Length);

uint8_t *Keyboard_SET_IDLE(uint16_t Length);
u8 *usbdata_Request(void);
/* Exported define -----------------------------------------------------------*/
#define Keyboard_GetConfiguration          NOP_Process
//#define Keyboard_SetConfiguration          NOP_Process
#define Keyboard_GetInterface              NOP_Process
#define Keyboard_SetInterface              NOP_Process
#define Keyboard_GetStatus                 NOP_Process
#define Keyboard_ClearFeature              NOP_Process
#define Keyboard_SetEndPointFeature        NOP_Process
#define Keyboard_SetDeviceFeature          NOP_Process
//#define Keyboard_SetDeviceAddress          NOP_Process
#define Keyboard_SetReport          NOP_Process

#define REPORT_DESCRIPTOR                  0x22


void Keyboard_hid_init(void);

extern char /*USB_Buff[8],*/usb_reviceflag;
extern u8 *usbdata_Request(void);
extern u8 *My_Data_Request(u16 length);
#endif /* __USB_PROP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
