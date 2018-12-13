/**
  ******************************************************************************
  * @file    usb_desc.c
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    29-June-2012
  * @brief   Descriptors for Keyboard Demo
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
#include "usb_desc_hid.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* USB Standard Device Descriptor */
const uint8_t Keyboard_DeviceDescriptor[KEYBOARD_SIZ_DEVICE_DESC] =
  {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    0x40,                       /*bMaxPacketSize 64*/
    0x52,                       /*idVendor (0x0483)*/   // UIC
    0x63,
    0x1A,                       /*idProduct = 0x5710*/
    0x20,
		
//		0xCD,                       /*idVendor (0x0483)*/
//    0x0A,
//    0x10,                       /*idProduct = 0x5710*/
//    0x20,
    0x00,                       /*bcdDevice rel. 2.00*/
    0x02,
    1,                          /*Index of string descriptor describing
                                                  manufacturer */
    2,                          /*Index of string descriptor describing
                                                 product*/
    3,                          /*Index of string descriptor describing the
                                                 device serial number */
    0x01                        /*bNumConfigurations*/
  }
  ; /* Keyboard_DeviceDescriptor */


/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
const uint8_t Keyboard_ConfigDescriptor[KEYBOARD_SIZ_CONFIG_DESC] =
  {
    0x09, /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
    KEYBOARD_SIZ_CONFIG_DESC,
    /* wTotalLength: Bytes returned */
    0x00,
    0x01,         /*bNumInterfaces: 1 interface*/
    0x01,         /*bConfigurationValue: Configuration value*/
    0x00,         /*iConfiguration: Index of string descriptor describing
                                     the configuration*/
    0xa0,         /*bmAttributes: bus powered */
    0x32,         /*MaxPower 100 mA: this current is used for detecting Vbus*/

    /************** Descriptor of Keyboard interface ****************/
    /* 09 */
    0x09,         /*bLength: Interface Descriptor size*/
    USB_INTERFACE_DESCRIPTOR_TYPE,/*bDescriptorType: Interface descriptor type*/
    0x00,         /*bInterfaceNumber: Number of Interface*/
    0x00,         /*bAlternateSetting: Alternate setting*/
    0x02,         /*bNumEndpoints*/
    0x03,         /*bInterfaceClass: HID*/
    0x01,         /*bInterfaceSubClass : 1=BOOT, 0=no boot*/
    0x01,         /*nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse*/
    0,            /*iInterface: Index of string descriptor*/
    /******************** Descriptor of Keyboard HID ********************/
    /* 18 */
    0x09,         /*bLength: HID Descriptor size*/
    HID_DESCRIPTOR_TYPE, /*bDescriptorType: HID*/
    0x11,         /*bcdHID: HID Class Spec release number*/
    0x01,
    0x00,         /*bCountryCode: Hardware target country*/
    0x01,         /*bNumDescriptors: Number of HID class descriptors to follow*/
    0x22,         /*bDescriptorType*/
    KEYBOARD_SIZ_REPORT_DESC,/*wItemLength: Total length of Report descriptor*/
    0x00,
    /******************** Descriptor of Keyboard endpoint ********************/
    /* 27 */
    0x07,          /*bLength: Endpoint Descriptor size*/
    USB_ENDPOINT_DESCRIPTOR_TYPE, /*bDescriptorType:*/

    0x81,          /*bEndpointAddress: Endpoint Address (IN)*/
    0x03,          /*bmAttributes: Interrupt endpoint*/
    0x08,          /*wMaxPacketSize: 8 Byte max */
    0x00,
    0x02,//0x20,          /*bInterval: Polling Interval (32 ms)*/
		
		/******************** Descriptor of Keyboard endpoint ********************/
    /* 34 */
    0x07,          /*bLength: Endpoint Descriptor size*/
    USB_ENDPOINT_DESCRIPTOR_TYPE, /*bDescriptorType:*/

    0x01,          /*bEndpointAddress: Endpoint Address (OUT)*/
    0x03,          /*bmAttributes: Interrupt endpoint*/
    0x08,          /*wMaxPacketSize: 8 Byte max */
    0x00,
    0x01//0x20,          /*bInterval: Polling Interval (32 ms)*/
    /* 41 */
  }
  ; /* Keyboard_ConfigDescriptor */

  const uint8_t Keyboard_ReportDescriptor[KEYBOARD_SIZ_REPORT_DESC] =
  {

    0x05, 0x01,// Usage Page (Generic Desktop)   
		0x09, 0x06 ,// Usage (Keyboard)
		0xA1, 0x01,//  Collection (Application) 
		0x05, 0x07,//      Usage Page (Keyboard/Keypad)
		0x19 ,0xE0 ,   //  Usage Minimum (Keyboard Left Control) 
		0x29 ,0xE7 ,  //   Usage Maximum (Keyboard Right GUI) 
		0x15, 0x00,    //  Logical Minimum (0) 
		0x25 ,0x01,   //   Logical Maximum (1)
		0x75 ,0x01,    //  Report Size (1) 
		0x95 ,0x08,     // Report Count (8) 
		0x81, 0x02,     // Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit) 
		0x95 ,0x01,    //  Report Count (1) 
		0x75 ,0x08,      //Report Size (8) 
		0x81, 0x01,   //   Input (Cnst,Ary,Abs) 
		0x95 ,0x05,   //   Report Count (5) 
		0x75 ,0x01,   //   Report Size (1) 
		0x05 ,0x08 ,  //   Usage Page (LEDs) 
		0x19 ,0x01,    //  Usage Minimum (Num Lock) 
		0x29 ,0x05 ,   //  Usage Maximum (Kana) 
		0x91 ,0x02,    //  Output (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit) 
		0x95, 0x01 ,  //   Report Count (1)
		0x75, 0x03,   //   Report Size (3) 
		0x91 ,0x01,   //   Output (Cnst,Ary,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
		0x95 ,0x06 ,   //  Report Count (6) 
		0x75, 0x08 ,   //  Report Size (8) 
		0x15, 0x00,     // Logical Minimum (0) 
		0x25 ,0x66 ,   //  Logical Maximum (102) 
		0x05 ,0x07,    //  Usage Page (Keyboard/Keypad) 
		0x19 ,0x00  ,  //  Usage Minimum (Undefined) 
		0x29, 0x66  ,  //  Usage Maximum (Keyboard Power)
		0x81 ,0x00,   //   Input (Data,Ary,Abs) 
		0x06 ,0x2D, 0xFF,    //  Usage Page (Vendor-Defined 46) 
		0x95 ,0x01,    //  Report Count (1) 
		0x26 ,0xFF, 0x00,    //  Logical Maximum (255) 
		0x15, 0x01,    //  Logical Minimum (1) 
		0x75  ,0x08, //Report Size (8) 
		0x09 ,0x20 ,   //  Usage (Vendor-Defined 32) 
		0x95, 0x08 ,    // Report Count (8) 
		0xB2 ,0x02, 0x01,//      Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Buf)
		0xC0 // End Collection
  };
//  #endif
/* USB String Descriptors (optional) */
const uint8_t Keyboard_StringLangID[KEYBOARD_SIZ_STRING_LANGID] =
  {
    KEYBOARD_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04
  }
  ; /* LangID = 0x0409: U.S. English */

const uint8_t Keyboard_StringVendor[KEYBOARD_SIZ_STRING_VENDOR] =
  {
    KEYBOARD_SIZ_STRING_VENDOR, /* Size of Vendor string */
    USB_STRING_DESCRIPTOR_TYPE,  /* bDescriptorType*/
    /* Manufacturer: "Postech" */
    'U', 0, 'I', 0, 'C', 0//, 'T', 0, 'E', 0, 'C', 0, 'H', 0
  };

const uint8_t Keyboard_StringProduct[KEYBOARD_SIZ_STRING_PRODUCT] =
  {
    KEYBOARD_SIZ_STRING_PRODUCT,          /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
	/*"Postech Keyboard" */
		'M', 0, 'C', 0, 'H', 0, '2', 0, '0', 0, '0', 0, '-', 0, 'D', 0,
	  'U', 0, 'K', 0, 'P', 0, 'T', 0 /*, 'e', 0, ' ', 0, 'U', 0, 'S', 0,
		'B', 0, '-', 0, 'H', 0, 'I', 0, 'D', 0, ' ', 0, 'K', 0, 'e', 0,
	  'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'R', 0,
		'e', 0, 'a', 0, 'd', 0, 'e', 0, 'r', */
//    'T', 0, 'M', 0, '3', 0, ' ', 0, 'M', 0, 'a', 0, 'g', 0, 's', 0,
//	  't', 0, 'r', 0, 'i', 0, 'p', 0, 'e', 0, ' ', 0, 'U', 0, 'S', 0,
//		'B', 0, '-', 0, 'H', 0, 'I', 0, 'D', 0, ' ', 0, 'K', 0, 'e', 0,
//	  'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'R', 0,
//		'e', 0, 'a', 0, 'd', 0, 'e', 0, 'r', 
  };
uint8_t Keyboard_StringSerial[KEYBOARD_SIZ_STRING_SERIAL] =
  {
    KEYBOARD_SIZ_STRING_SERIAL,           /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
    'U', 0, 
		'I', 0, 
		'C', 0,
		'-', 0,
		'M', 0, 
		'C', 0, 
		'H', 0,
		'2', 0,
		'0', 0,
		'0', 0,
//		'0', 0,
//		'1', 0
  };

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

