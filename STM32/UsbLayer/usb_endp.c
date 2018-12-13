
/* Includes ------------------------------------------------------------------*/

#include "hw_config.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_endp.h"
#include "usb_pwr.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "usb_prop_hid.h"
#include <app.h>

//SEND_BUF Send_DataBuf;

__IO unsigned char RecvBuff[64+1];
__IO unsigned char SendBuff[64+1];

__IO char gUsbConnFlag = 0;
uint8_t CRC_8bits_format; 

extern volatile uint8_t RxdFrameStatus;

#define USBMAXBUFFSIZE 1536   //512//

__IO unsigned char USBRecvBuf[USBMAXBUFFSIZE] = {0};
__IO unsigned int USBRecvTop = 0, USBRecvEnd = 0;

extern unsigned int g1msTimer6;

__IO char gUsbRespFlag = DISABLE; 

//ASC2对应HID键值转换
static char   Key_Table[][2] = 
{
 {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},		  //0~7
 {0x00,0x2A},    //backspace   8 
 {0,0},{0,0},{0,0},{0,0},  // 9~12
 {0x00,0x28},	  // CR (carriage return)  13
 {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},   //14~31
 {0x00,0x2C},       //(space) 空格  32
 {0x20,0x1E},		   //  !   33
 {0x20,0x34},			  // "  34
 {0x20,0x20},			 // #   35
 {0x20,0x21},			 // $   36
 {0x20,0x22},			 // %   37
 {0x20,0x24},			 // &   38
 {0x00,0x34},			//  '   39
 {0x20,0x26},			//  (   40
 {0x20,0x27},		    //  )   41
 {0x20,0x25},	        //  *   42
 {0x20,0x2E},         //   +   43
 {0x00,0x36},		    //	,  	44	
 {0x00,0x2D},		    //	-	45
 {0x00,0x37},		   //	.	46
 {0x00,0x38},		   //	/	47
 {0x00,0x27},{0x00,0x1E},{0x00,0x1F},{0x00,0x20},{0x00,0x21},{0x00,0x22},{0x00,0x23},{0x00,0x24},
 {0x00,0x25},{0x00,0x26},  //0~9    48~57
 {0x20,0x33},	       //   :   58
 {0x00,0x33},		   //	;	59
 {0x20,0x36},		   //	<	60
 {0x00,0x2E},		   //	=	61
 {0x20,0x37},		   //	>	62
 {0x20,0x38},		   //	?	63
 {0x20,0x1F},		      //   @	 64
 {0x20,0x04},{0x20,0x05},{0x20,0x06},{0x20,0x07},{0x20,0x08},{0x20,0x09},{0x20,0x0A},{0x20,0x0B},
 {0x20,0x0C},{0x20,0x0D},{0x20,0x0E},{0x20,0x0F},{0x20,0x10},{0x20,0x11},{0x20,0x12},{0x20,0x13},
 {0x20,0x14},{0x20,0x15},{0x20,0x16},{0x20,0x17},{0x20,0x18},{0x20,0x19},{0x20,0x1A},{0x20,0x1B},
 {0x20,0x1C},{0x20,0x1D},		  //	A~Z 	65~90		  
 {0x00,0x2F},		  //	[	91
 {0x00,0x31},		  //	\	92
 {0x00,0x30},		  //	]	93
 {0x20,0x23},		 //	    ^	94
 {0x20,0x2D},		     //	    _	95
 {0x00,0x32},		 //	    `	96
 {0x00,0x04},{0x00,0x05},{0x00,0x06},{0x00,0x07},{0x00,0x08},{0x00,0x09},{0x00,0x0A},{0x00,0x0B},
 {0x00,0x0C},{0x00,0x0D},{0x00,0x0E},{0x00,0x0F},{0x00,0x10},{0x00,0x11},{0x00,0x12},{0x00,0x13},
 {0x00,0x14},{0x00,0x15},{0x00,0x16},{0x00,0x17},{0x00,0x18},{0x00,0x19},{0x00,0x1A},{0x00,0x1B},
 {0x00,0x1C},{0x00,0x1D},		  //	a~z 	97~122
 {0x20,0x2F},      //	  {		123
 {0x20,0x31},      //	  |		124
 {0x20,0x30},      //	  }		125
 {0x20,0x32},      //	  ~		126
 {0x00,0x00},      //	  DEL (delete)		127
};


u8 Keyboard_Send(u8 *Key)
{
     
	 while(_GetEPTxStatus(ENDP1)!=(0x02<<4));
	 /*copy mouse position info in ENDP1 Tx Packet Memory Area*/
	 UserToPMABufferCopy(Key, GetEPTxAddr(ENDP1), 8);
	 /* enable endpoint for transmission */
	 SetEPTxStatus(ENDP1,EP_TX_VALID);
	 SetEPTxValid(ENDP1);

	
	 memset(Key,0x00,8);

	 while(_GetEPTxStatus(ENDP1)!=(0x02<<4));
	 /*copy mouse position info in ENDP1 Tx Packet Memory Area*/
	 UserToPMABufferCopy(Key, GetEPTxAddr(ENDP1), 8);
//	 /* enable endpoint for transmission */
	 SetEPTxValid(ENDP1);
	 SetEPTxStatus(ENDP1,EP_TX_VALID);

	 return 0;
}


void DataSend_USBHID(uint8_t* ptrs,uint16_t data_len)
{
	uint8_t keyborad[8]={0,0,0,0,0,0,0,0},AsciiDataNum=0;
  uint16_t  loop=0;
  memset(keyborad,0x00,8);
	
   for(loop=0;loop<data_len;loop++)
  {

		memset(keyborad,0x00,8);
		AsciiDataNum=ptrs[loop];
//    printf("asciiflag=%d\r\n",AsciiDataNum);
		keyborad[0]=Key_Table[AsciiDataNum][0] ;
	  keyborad[2]=Key_Table[AsciiDataNum][1] ;

		Keyboard_Send(keyborad);
	
		}

	}


void UsbSetRespFlag(char flag)
{
	if(flag == DISABLE)
	{
		gUsbRespFlag = DISABLE;
	}
	else
	{
		gUsbRespFlag = ENABLE;
	}
}

char UsbCheckBuff(char *ExpectString)
{
	char *p = NULL;
	p = strstr((const char *)USBRecvBuf, ExpectString);
	
	if(p)
	{
	 	return 1;
	}
	else
	{
	 	return 0;
	}
}


//void PrintHexBuff_usb (unsigned char *title, unsigned char *buff, unsigned int len)
//{
//	#if 1
//	int i = 0;
//	printf ("%s|", title);

//	for (i = 0; i < len; i++)
//	{
//		printf ("%02x ", buff[i]);
//	}

//	printf ("\r\n");
//	printf ("\r\n");
//	#endif
//}

unsigned char CalcSum(unsigned char *buff, unsigned int len)
{
	unsigned int i = 0;
	unsigned char sum = 0;

	for(i=0; i<len; i++)
	{
		sum ^= buff[i];		
	}

	return sum;
}

/*******************************************************************************
* Function Name  : EP1_OUT_Callback.
* Description    : EP1 OUT Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_OUT_Callback(void)
{
//	printf("100145\r\n");
	
	
}


uint8_t Custom_Data_Send(const unsigned char *ptrs,uint16_t Sendlen,uint16_t delay_ms)
{
	unsigned int sent = 0;		//已发送的数据长度
	unsigned int rest = Sendlen;	//未发送的数据长度
	unsigned int PACKET_LEN = 64;

	g1msTimer6 = delay_ms;
	
//	DBG_H("ptrs",ptrs,Sendlen);
	//SendBuff
	//[0]    [1]    [2]    [3]    ...    [62]    [63]
	//05     len     d0    d1             d60     sum
	while(1)
	{
		if(g1msTimer6 == 0)return sent;

		if(rest == 0)return Sendlen;
		
		if (rest >= 64)PACKET_LEN = 64;
		else PACKET_LEN = rest;
		memset((unsigned char*)SendBuff, 0, sizeof(SendBuff));
		
		memcpy((unsigned char*)SendBuff, (ptrs+sent), PACKET_LEN);	//61 bytes
		
		while(_GetEPTxStatus(ENDP1)!=(0x02<<4));
//		DBG_H("SendBuff",SendBuff,PACKET_LEN);
		USB_SIL_Write(EP1_IN, (unsigned char*)SendBuff, PACKET_LEN);  
//	  SetEPTxCount(ENDP1, PACKET_LEN);
    SetEPTxValid(ENDP1);
		sent += PACKET_LEN;
		rest -= PACKET_LEN;
	}
	
	
//	u8 sendbuf[64];
	
}

void UsbRecvReset (void)
{
	USBRecvTop = 0;
	USBRecvEnd = 0;

	memset((void *)USBRecvBuf, 0, USBMAXBUFFSIZE); 
}

unsigned char UsbRecvOne (unsigned char *Str)
{
	if (USBRecvTop == USBRecvEnd) return 0;//read nothing

	*Str = USBRecvBuf[USBRecvTop];
	USBRecvTop++;
//  DBG_H("USBRecvBuf",USBRecvBuf,USBRecvTop);
	if (USBRecvTop >= USBMAXBUFFSIZE) USBRecvTop = 0;

	//printf("%s : USBRecvEnd is %d, USBRecvTop is %d\r\n", __func__, USBRecvEnd, USBRecvTop);

	return 1;//read one
}

unsigned int UsbRecvAtTime (unsigned char *Buff, unsigned int RecvSize, unsigned int ms)
{
	unsigned int RecvLen = 0;
	unsigned char tmp[1] = {0};

	g1msTimer6 = ms;

	if (RecvSize == 0) return 0;

	while (1)
	{
		if (g1msTimer6 == 0) return RecvLen;

		if (UsbRecvOne (tmp) == 1) 
		{
			Buff[RecvLen++] = tmp[0];
		}

		if (RecvLen >= RecvSize) return RecvLen;
	}
}


/*******************************************************************************
* Function Name  : EP1_IN_Callback.
* Description    : EP1 IN Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/




void Revice_IDTdata(uint8_t *pubuf,uint8_t len)
{
	uint8_t i=0;
//	USBRecvTop = 0;
//	USBRecvEnd = 0;	
	
	for(i=0; i<len; i++)
	{
		if ( (USBRecvEnd == (USBRecvTop - 1) ) || ( (USBRecvTop == 0) && (USBRecvEnd == (USBMAXBUFFSIZE - 1) ) ) )
		{
			//缓冲溢出
			USBRecvTop = 0;
			USBRecvEnd = 0;		
		}
		else
		{
			USBRecvBuf[USBRecvEnd] = pubuf[i];
			USBRecvEnd++;
			


			if (USBRecvEnd >= USBMAXBUFFSIZE) USBRecvEnd = 0;
		}
		
	}
  
}

void EP1_IN_Callback(void)
{
//	PrevXferComplete = 1;
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

