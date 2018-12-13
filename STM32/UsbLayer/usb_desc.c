/**
  ******************************************************************************
  * @file    usb_desc.c
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    29-June-2012
  * @brief   Descriptors for Custom HID Demo
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
#include "usb_desc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* USB Standard Device Descriptor */
const uint8_t CustomHID_DeviceDescriptor[CUSTOMHID_SIZ_DEVICE_DESC] =
  {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    0x40,                       /*bMaxPacketSize40*/
    0x52,                       /*idVendor (0x0483)*/  //UIC
    0x63,
    0x1B,                       /*idProduct = 0x5750*/
    0x20,
		
//		0xCD,                       /*idVendor (0x0483)*/  //IDTECH
//    0x0A,
//    0x30,                       /*idProduct = 0x5750*/
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
  ; /* CustomHID_DeviceDescriptor */


/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
const uint8_t CustomHID_ConfigDescriptor[CUSTOMHID_SIZ_CONFIG_DESC] =
  {
    0x09, /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
    CUSTOMHID_SIZ_CONFIG_DESC,
    /* wTotalLength: Bytes returned */
    0x00,
    0x01,         /* bNumInterfaces: 1 interface */
    0x01,         /* bConfigurationValue: Configuration value */
    0x00,         /* iConfiguration: Index of string descriptor describing
                                 the configuration*/
    0xA0,         /* bmAttributes: Bus powered */
    0x32,         /* MaxPower 100 mA: this current is used for detecting Vbus */

    /************** Descriptor of Custom HID interface ****************/
    /* 09 */
    0x09,         /* bLength: Interface Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,/* bDescriptorType: Interface descriptor type */
    0x00,         /* bInterfaceNumber: Number of Interface */
    0x00,         /* bAlternateSetting: Alternate setting */
    0x02,         /* bNumEndpoints */
    0x03,         /* bInterfaceClass: HID */
    0x01,         /* bInterfaceSubClass : 1=BOOT, 0=no boot */
    0x01,         /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
    0,            /* iInterface: Index of string descriptor */
    /******************** Descriptor of Custom HID HID ********************/
    /* 18 */
    0x09,         /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE, /* bDescriptorType: HID */
    0x11,         /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,         /* bCountryCode: Hardware target country */
    0x01,         /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,         /* bDescriptorType */
    CUSTOMHID_SIZ_REPORT_DESC,/* wItemLength: Total length of Report descriptor */
    0x00,
    /******************** Descriptor of Custom HID endpoints ******************/
    /* 27 */
    0x07,          /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: */

    0x81,          /* bEndpointAddress: Endpoint Address (IN) */
    0x03,          /* bmAttributes: Interrupt endpoint */
    PACKAGE_SIZE,  /* wMaxPacketSize: 2 Bytes max */	//TWO
    0x00,
    0x02,          /* bInterval: Polling Interval (32 ms) */
    /* 34 */
    	
    0x07,	/* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,	/* bDescriptorType: */
			/*	Endpoint descriptor type */
    0x01,	/* bEndpointAddress: */
			/*	Endpoint Address (OUT) */
    0x03,	/* bmAttributes: Interrupt endpoint */
    PACKAGE_SIZE,	/* wMaxPacketSize: 2 Bytes max  */
    0x00,
    0x02	/* bInterval: Polling Interval (32 ms) */
//    /* 41 */
  }; /* CustomHID_ConfigDescriptor */

const uint8_t CustomHID_ReportDescriptor[CUSTOMHID_SIZ_REPORT_DESC] =
  {  
    0x06 ,0x00, 0xFF , //Usage Page (Vendor-Defined 1)
		0x09, 0x01,  //Usage (Vendor-Defined 1)
		0xA1 ,0x01,  //Collection (Application)
		0x15 ,0x00,  //    Logical Minimum (0)
		0x26 ,0xFF ,0x00 ,//    Logical Maximum (255) 
		0x75 ,0x08,    // Report Size (8) 
		0x09 ,0x20 ,    // Usage (Vendor-Defined 32) 
		0x09 ,0x21,      //Usage (Vendor-Defined 33) 
		0x09 ,0x22,      //Usage (Vendor-Defined 34) 
		0x09 ,0x28,      //Usage (Vendor-Defined 40) 
		0x09 ,0x29 ,    // Usage (Vendor-Defined 41) 
		0x09 ,0x2A ,    // Usage (Vendor-Defined 42)
		0x09 ,0x38 ,    // Usage (Vendor-Defined 56) 
		0x95 ,0x07 ,    // Report Count (7) 
		0x81 ,0x02 ,    // Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit) 
		0x09 ,0x30  ,   // Usage (Vendor-Defined 48) 
		0x95 ,0x02 ,    // Report Count (2) 
		0x82 ,0x02 ,0x01 ,     //Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Buf)
		0x09 ,0x31 ,     //Usage (Vendor-Defined 49) 
		0x96 ,0x27, 0x02,    //  Report Count (551) 
		0x82 ,0x02, 0x01,    //  Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Buf) 
		0x09 ,0x20,     // Usage (Vendor-Defined 32) 
		0x95 ,0x08 ,     //Report Count (8) 
		0xB2 ,0x02 ,0x01,    //  Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Buf)
		0xC0  //End Collection 
		
		
  }; /* CustomHID_ReportDescriptor */

/* USB String Descriptors (optional) */
const uint8_t CustomHID_StringLangID[CUSTOMHID_SIZ_STRING_LANGID] =
  {
    CUSTOMHID_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04
  }
  ; /* LangID = 0x0409: U.S. English */

const uint8_t CustomHID_StringVendor[CUSTOMHID_SIZ_STRING_VENDOR] =
  {
    CUSTOMHID_SIZ_STRING_VENDOR, /* Size of Vendor string */
    USB_STRING_DESCRIPTOR_TYPE,  /* bDescriptorType*/
    /* Manufacturer: "STMicroelectronics" */
		'U', 0, 'I', 0, 'C', 0//, 'T', 0, 'E', 0, 'C', 0, 'H', 0
  };

const uint8_t CustomHID_StringProduct[CUSTOMHID_SIZ_STRING_PRODUCT] =
  {
    CUSTOMHID_SIZ_STRING_PRODUCT,          /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
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
uint8_t CustomHID_StringSerial[CUSTOMHID_SIZ_STRING_SERIAL] =
  {
    CUSTOMHID_SIZ_STRING_SERIAL,           /* bLength */
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

  };

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

