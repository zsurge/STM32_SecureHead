#ifndef __USB_ENDP_H
#define __USB_ENDP_H

#include "stm32f10x.h"

#define SEND_BUF_LENGTH 1024
typedef struct
{
unsigned char data[SEND_BUF_LENGTH];
volatile u16 read;
volatile u16 write;
volatile u16 len;
}SEND_BUF;

extern SEND_BUF Send_DataBuf;
extern __IO char gUsbConnFlag,EP1BUSY,Release_Key_Flag;

void UsbRecvReset (void);

void UsbSetRespFlag(char flag);

unsigned char UsbRecvOne (unsigned char *Str);

unsigned int UsbRecvAtTime (unsigned char *Buff, unsigned int RecvSize, unsigned int ms);

unsigned int UsbSendBuff(const unsigned char *buff, unsigned int len, unsigned int ms);

char UsbCheckBuff(char *ExpectString);

int UsbReadTo(unsigned char *Buff, unsigned int BuffSize, unsigned char *EndString);

//char UsbGetConn(void);

void keyboard_SendBuf(u8 *databuff,u16 datalen);

void Revice_IDTdata(uint8_t *pubuf,uint8_t len);
void DataSend_USBHID(uint8_t* ptrs,uint16_t data_len);
void USB_HIDSend(uint8_t *ptrs,uint16_t Sendlen);

uint8_t Custom_Data_Send(const unsigned char *ptrs,uint16_t Sendlen,uint16_t delay_ms);

extern uint8_t CRC_8bits_format;
#endif 

