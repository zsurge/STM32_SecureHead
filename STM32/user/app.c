#include <stdlib.h>
#include <stdio.h>
#include "app.h"

#include "math.h"
#include "usb_type.h"
#include "ini.h"
#include "stm32f10x.h"
#include "dev_uart.h"
#include "utilities.h"
#include "dukpt.h"
#include "usb_endp.h"
#include "dev_msread.h"

extern AES_KEY key;

extern uint8_t IDTUSB_Reviceflag,IDTUSB_SendBuff[idte_datalen],usbd_sendlen,sendcount;
extern char /*USB_Buff[8],*/usb_reviceflag;
extern void Transfe_Mode_Switch(void);
//extern uint8_t keyflag;
union __ENC_KEY ENC_KEY;					//��Կ�������ݽṹ
uint8_t CryptoKey[16];						//DUKPT������Ĺ�����Կ
								  	
uint8_t /*random[8],*/LOADKEY_AuthenticateRadomBuf[8]={0},LOADKEY_Authenticate_flag=0xAA;							//�����
//uint8_t macdata[8];							//��������
uint8_t FixKey_AuthenBuff[8];//External Authenticate Command (Fixed Key Only)  ������֤FIX�����ܵ�8bit�����
const char SHID[] = "PF SecureHeadReader V 1.1.2";
//#ifdef FLASHKEY_SUPPORT
//const char APPVERSION[] = "PAYFULL-M301";
//#else
const char APPVERSION[] = "PAYFULL-M301";
//#endif

volatile uint8_t WriteFlag = 0,USB_Updataflag=0xAD;				//������Կ��־��0�������棬1��ʹ�ܱ���
volatile uint8_t GetNextFlag = 0;			//��ȡ��һ��KSN��־��0������ȡ��һ����1��ʹ�ܻ�ȡ��һ�� 

volatile uint8_t macstate = 0x00,FIX_Authenticate_Value=0x30;			//����״̬��0x30��δ���յ��������ݻ��������ʧ�ܣ�0x31����������OK
//volatile uint8_t encryptdatastatus = 0x00;	//bit 1 ==1 ��ʾ1�ŵ��������ݴ���
											//bit 2 ==1 ��ʾ2�ŵ��������ݴ���
											//bit 3 ==1 ��ʾ3�ŵ��������ݴ���
											//bit 4 ==1 ��ʾ1�ŵ���ϣ���ݴ���
											//bit 5 ==1 ��ʾ2�ŵ���ϣ���ݴ���
											//bit 6 ==1 ��ʾ3�ŵ���ϣ���ݴ���
											//bit 7 ==1 ��ʾ�ỰID���ݴ���
											//bit 8 ==1 ��ʾKSN����

volatile uint8_t RxdStatus = 0;			 		//����״̬
volatile uint8_t SynHead = 0;				 	//���հ�ͷ����
volatile uint8_t RxdFrameStatus = 0;			//���հ�״̬
volatile uint8_t NowLen = 0;				 	//�����ֽ�ָ��
volatile uint8_t RxdTotalLen = 0;			 	//���հ����ݳ���

#define RxdBuf ((uint8_t *)TempTrackDirtData.Value)			//���հ����ݻ���
//volatile uint8_t RxdBuf[100]={0};
volatile uint8_t Bcc;						//У���

//char firstflag=0,secondflag=0,countflag=0,firstkeybuff[16],secondkeybuff[16];

extern void delay_ms(uint16_t nms);



void init_serial_boot(void)
{
	RxdStatus = 0;
	SynHead = 0;
	RxdFrameStatus = SPACE;
}


void Default_Settings(void)
{
	uint8_t flag=0,msn_buff[10],ksnbuff[10]={0};

	if(ENC_KEY.temp.writesnflag==0x31 ) ENC_KEY.temp.writesnflag = 0x31;
	else if(ENC_KEY.temp.writesnflag==0x30 ) ENC_KEY.temp.writesnflag = 0x30;
	else ENC_KEY.temp.writesnflag = 0x30;
	
	memcpy(ENC_KEY.temp.APP_RunFlagBuf,"\x55\xAA\x55\xAA",4);
	flag=ENC_KEY.temp.writesnflag;
	memcpy(msn_buff,ENC_KEY.temp.sn,10);
	memcpy(ksnbuff,ENC_KEY.temp.ksn,10);
	
	memset(ENC_KEY.key,0x00,200/*sizeof(ENC_KEY.key)*/);
	
	memcpy(ENC_KEY.temp.ksn,ksnbuff,10);
	memcpy(ENC_KEY.temp.sn,msn_buff,10);
	ENC_KEY.temp.writesnflag=flag;
	
	ENC_KEY.temp.zhanwei = 0xAA; //���á���ֹ��־λ
	ENC_KEY.temp.beepmode = 0x31; //���á���ֹ��־λ
	ENC_KEY.temp.enabledevice = 0x31; //���á���ֹ��־λ
	
	if(ENC_KEY.temp.writesnflag == 0x30){
	  ENC_KEY.temp.writesnflag = 0x30;  //дsn��־λ
		memcpy(ENC_KEY.temp.ksn,"\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00",10);//  ksnֵ���
		
	}
	
//	DBG_H("ENC_KEY.temp.ksn",ENC_KEY.temp.ksn,10);
//	  printf("ENC_KEY.temp.writesnflagddes %x\r\n",ENC_KEY.temp.writesnflag); 
	
	ENC_KEY.temp.Terminator = 0x0D;     //�ŵ�������  ---�����ĵ�
	
	ENC_KEY.temp.selecttrack = 0x30;    //ѡ��ŵ�����
	
	ENC_KEY.temp.setseparator = 0x0D;   //д�����ݷָ��	

	ENC_KEY.temp.settrack2format = 0x31;//���2���ݹ���
	ENC_KEY.temp.status = 0;			//ʧ�ܼ���
	ENC_KEY.temp.level = 0x31;//���ܵȼ�Ϊ0x31
	
	ENC_KEY.temp.encmode = 0x31;			//Ĭ��DUKPT����
	ENC_KEY.temp.encway = 0x31;			//Ĭ��DES����
	memcpy(ENC_KEY.temp.fix,"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\xaa\xab\xac\xad\xae\xaf",16);// fix key�ܳ״��ֵ
	memcpy(ENC_KEY.temp.key,"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\xaa\xbb\xcc\xdd\xee\xff",16);//  key�ܳ״��ֵ
//	IDT_UpdateKSN(10,ENC_KEY.temp.ksn);
	
	memcpy(ENC_KEY.temp.sid,"\x01\x02\x03\x04\x05\x06\x07\x08",8);//  sidֵ���
//	memcpy(ENC_KEY.temp.sn,"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x00",10);//  snֵ���
	
	memset(ENC_KEY.temp.Preamble,0x00,15 );     /*sizeof(ENC_KEY.temp.Preamble)*///��ͷ��ʼ��  15bit
	memset(ENC_KEY.temp.Postamble,0x00,15 );//15bit   /*sizeof(ENC_KEY.temp.Postamble)*/
	
	memset(ENC_KEY.temp.track1prefix,0x00,6 );//��ͷǰ׺ 6bit /*sizeof(ENC_KEY.temp.track1prefix)*/
	memset(ENC_KEY.temp.track2prefix,0x00,6 );//6bit /*sizeof(ENC_KEY.temp.track2prefix)*/
	memset(ENC_KEY.temp.track3prefix,0x00,6 );//6bit /*sizeof(ENC_KEY.temp.track3prefix)*/
	
	memset(ENC_KEY.temp.track1suffix,0x00,6 );//��ͷ��׺  6bit /*sizeof(ENC_KEY.temp.track1suffix)*/
	memset(ENC_KEY.temp.track2suffix,0x00,6 );//6bit /*sizeof(ENC_KEY.temp.track2suffix)*/
	memset(ENC_KEY.temp.track3suffix,0x00,6 );//6bit /*sizeof(ENC_KEY.temp.track3suffix)*/
	
	ENC_KEY.temp.Enhancedstatue=0x30;//ǿ���ܺ���ͨ����״̬
	
	ENC_KEY.temp.Enhancedoption=0x08;//
	
	ENC_KEY.temp.HASHSET=0x37;//��ϣֵ�ķ���״̬
	ENC_KEY.temp.MaskSetting=0x07;//����������

  ENC_KEY.temp.tp1len=0;
	ENC_KEY.temp.tp2len=0;
	ENC_KEY.temp.tp3len=0;
	ENC_KEY.temp.ts1len=0;
	ENC_KEY.temp.ts2len=0;
	ENC_KEY.temp.ts3len=0;
	ENC_KEY.temp.Activa_Challenge_Reply=0x30;
	ENC_KEY.temp.Exit_Authenticated=0x30;
	ENC_KEY.temp.decoded_directions=0x31;
	ENC_KEY.temp.Encryption_OptionSetting=0x37;
//	ENC_KEY.temp.Tranmodeselect = save_trans_flag;

   if(ENC_KEY.temp.Tranmodeselect ==0x38) ENC_KEY.temp.Tranmodeselect =0x38;
   else if(ENC_KEY.temp.Tranmodeselect ==0x30) ENC_KEY.temp.Tranmodeselect =0x30;
   else ENC_KEY.temp.Tranmodeselect =0x30;
//   printf("ENC_KEY.temp.Tranmodeselect =%d\r\n",ENC_KEY.temp.Tranmodeselect);
	ENC_KEY.temp.MaskChar=0x2a;
	ENC_KEY.temp.PrePANID=0x06;
	ENC_KEY.temp.PostPANID=0x04;
	ENC_KEY.temp.DispDataflag=0x30;
	memset(ENC_KEY.temp.Authenticationkey_Buff,0x00,16);  
	memset(ENC_KEY.temp.Challenge1_Buff,0x00,8);
  memset(ENC_KEY.temp.Challenge2_Buff,0x00,8); 	
	

	WriteFlag = 1;
	WriteENCKEY();	
//	WriteFlag = 0;
}

/**************************************************************************************
	*** �� �� ��:	void WriteENCKEY(void)
	*** ����������	��ENCKEY���浽FLASH��
	*** ��    ��:  	����: ȫ�ֱ���ENC_KEY
					����: ENC_KEY�����ַ
				    �����NULL 
	*** �� �� ֵ:	NULL  	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.09.20����
***************************************************************************************/
void WriteENCKEY(void)
{
	#ifdef FLASHKEY_SUPPORT
	uint8_t j=0;
	uint8_t tempdata[200];
	memset(tempdata ,0, sizeof(tempdata));
	if(WriteFlag==1)
	{
    WriteFlag=0;  
	  memcpy(tempdata,ENC_KEY.key,200);
		
		for(j=0;j<159;j++)
		{
			tempdata[160] ^= tempdata[j];
			tempdata[161] += tempdata[j];			
		}

// 		for(i=0;i<8;i++)
// 			tri_des(tempdata+i*8,(unsigned char *)SHID,(unsigned char *)SHID+8,0);	   //����Ҫ�洢����Կֵ
		__disable_irq();
		EarseBlockFlash(KEYADD);
		WriteBlockFlash(KEYADD ,tempdata, 200);
		__enable_irq();
		return ;
	}
	else
	{
		return ;
	}
	#endif
	
}

/**************************************************************************************
	*** �� �� ��:	void ReadENCKEY(void)
	*** ����������	��ȡ�����ENCKEY��RAM��
	*** ��    ��:  	����: ENC_KEY�����ַ
				    �����ȫ�ֱ���ENC_KEY 
	*** �� �� ֵ:	TRUE/FALSE   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.09.20����
***************************************************************************************/
uint8_t ReadENCKEY(void)
{
//	#ifdef FLASHKEY_SUPPORT
	uint8_t tempdata[200],i=0,j=0,lrc=0,bcc=0;
	memset(tempdata ,0, 200);
	ReadBlockFlash(KEYADD ,tempdata, 200);     			//����֮ǰ�������Կֵ

//	DBG_H("tempdata0024",tempdata,200);
	//�ж�ֵ��XX �Ǿ��ǵ�һ�Σ�ִ������Ĭ��ֵ
	//ֵ��ZZ����ȡ��Ȼ��ֵ
//	if(tempdata[0] != 0xAA)
	if(tempdata[4]!=0xAA)
	{

	    Default_Settings();
//       
	}

	else
	{	
		memcpy(ENC_KEY.key,tempdata,200);
//		DBG_H("ENC_KEY.key",ENC_KEY.key,200);
//		memcpy(ENC_KEY.temp.key,tempdata,200);
      i=164;
        for(j = 0; j < i-2; j++)
        {
            lrc ^= tempdata[j];
            bcc += tempdata[j];
        }  

        if(ENC_KEY.temp.lrc == lrc && ENC_KEY.temp.bcc == bcc)
        {
            return TRUE;  
        }
        else
        {
            return FALSE;
        }        
		  
	}
	if(FIX_Authenticate_Value==0x30) ENC_KEY.temp.encmode=0x30;//�鿴3.9.2С�� ÿ������fix���ܶ���Ҫ��֤һ��

	return TRUE;
	
//	#endif
	
	}

/**************************************************************************************
	*** �� �� ��:	uint8_t AsciiToHex(uint8_t * pAscii, uint8_t * pHex, int nLen)
	*** ����������	��һ�� ASSC ���ʾ��ʮ�������ַ���ת����ʮ�����Ƶ��ֽڴ���
	*** ��    ��:  	����: pAscii -- Ҫת����ASCII���ʾ��ʮ�������ַ������׵�ַ��
						  nLen	 -- Ҫת����ASCII���ʾ��ʮ�������ַ����ĳ��ȣ��ֽ�������
				    �����pHex	 -- ת�����ʮ���������ֽڴ��׵�ַ��
	*** �� �� ֵ:	TRUE/FALSE   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.09.20����
***************************************************************************************/
//uint8_t AsciiToHex(uint8_t *pAscii, uint8_t *pHex, int nLen)
//{
//	unsigned char Nibble[2];
//	int nHexLen,i,j;
//	if (nLen%2)
//		return FALSE;
//	nHexLen = nLen/2;
//	for (i = 0; i < nHexLen; i ++)
//	{
//		
//		Nibble[0] = *pAscii++;
//		Nibble[1] = *pAscii++;
//		for (j = 0; j < 2; j++)
//		{
//			if (Nibble[j] <= 'F' && Nibble[j] >= 'A')
//				Nibble[j] = Nibble[j] - 'A' + 10;
//			else if (Nibble[j] <= 'f' && Nibble[j] >= 'a')
//					Nibble[j] = Nibble[j] - 'a' + 10;
//			else if (Nibble[j] >= '0' && Nibble[j] <= '9')
//					Nibble [j] = Nibble[j] - '0';
//			else
//				return FALSE;
//		}	// for (int j = ...)
//		pHex[i] = Nibble[0] << 4;	// Set the high nibble
//		pHex[i] |= Nibble[1];		//Set the low nibble
//	}	// for (int i = ...)
//	return TRUE;
//}

/**************************************************************************************
	*** �� �� ��:	void HexToAscii(unsigned char * pHex, unsigned char * pASSCHex, int nLen)
	*** ����������	��һ��ʮ�������ֽڴ�ת���� ASCII ���ʾ���ַ�����
	*** ��    ��:  	����: pHex 	   -- Ҫת����ʮ���������ֽڴ��׵�ַ��
						  nLen	   -- Ҫת����ʮ���������ֽڴ��ĳ��ȣ��ֽ�������
				    �����pASSCHex -- ת����� ASCII ���ʾ���ַ������׵�ַ��
	*** �� �� ֵ:	NULL   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.09.20����
***************************************************************************************/
//void HexToAscii(uint8_t *pHex, uint8_t *pASSCHex, int nLen)
//{
//	uint8_t Nibble[2];
//	int i,j;
//	for ( i = 0; i < nLen; i ++)
//	{
//		Nibble[0] = (pHex[i] & 0xF0) >> 4;	
//		Nibble[1] = pHex[i] & 0x0F;
//		for ( j = 0; j < 2; j ++)
//		{
//			if (Nibble[j] < 10)
//				Nibble[j] += 0x30;
//			else
//			{
//				if (Nibble[j] < 16)
//					Nibble[j] = Nibble[j] - 10 + 'A';
//			}
//			*pASSCHex ++ = Nibble[j];
//		}	// for (int j = ...)
//	}	// for (int i = ...)
//}


uint16_t Check_Sum(uint8_t *Buf,int plen,int toplen)
{
	uint16_t Slen,Sum=0;
	
	for(Slen=toplen;Slen<plen;Slen++)
	{
		Sum+=Buf[Slen];
//		printf("Buf[Slen]=%x\r\n",Buf[Slen]);
	}
	
	return Sum;
}

void asc2bcd(unsigned char *bcd_buf, unsigned char *ascii_buf, int conv_len, unsigned char type )
{
    int    cnt;
    char   ch, ch1;

    if (conv_len&0x01 && type ) ch1=0;
    else ch1=0x55;
    for (cnt=0; cnt<conv_len; ascii_buf++, cnt++)
    {
        if (*ascii_buf >= 'a' ) ch = *ascii_buf-'a' + 10;
        else if ( *ascii_buf >= 'A' ) ch =*ascii_buf- 'A' + 10;
        else if ( *ascii_buf >= '0' ) ch =*ascii_buf-'0';
        else ch = 0;
        if (ch1==0x55) ch1=ch;
        else {
            *bcd_buf++=( ch1<<4 )| ch;
            ch1=0x55;
        }
    }
    if (ch1!=0x55) *bcd_buf=ch1<<4;
}

void bcd2asc(unsigned char *ascii_buf,unsigned char * bcd_buf, int conv_len, unsigned char type)
{
    int cnt=0;

    if (conv_len&0x01 && type) {cnt=1; conv_len++;}
    else cnt=0;
    for (; cnt<conv_len; cnt++, ascii_buf++){
        *ascii_buf = ((cnt&0x01) ? (*bcd_buf++&0x0f) : (*bcd_buf>>4));
        *ascii_buf += ((*ascii_buf>9) ? ('A'-10) : '0');
    }
}
/**************************************************************************************
	*** �� �� ��:	void Random(void) 
	*** ����������	ȡ8���ֽڵ�16���Ƶ��������
	*** ��    ��:  	����: NULL��
				    �����8��ʮ�������������
	*** �� �� ֵ:	NULL   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.09.20����
***************************************************************************************/
//void Random(void) 
//{
//	uint8_t i;
//	for(i = 0; i < 8; i++)
//	{
//		random[i] = rand() % 255;
//	}
//}

/**************************************************************************************
	*** �� �� ��:	uint8_t GetNextKSN(uint8_t* pBUserKSN) 
	*** ����������	ʹDUKPT�㷨����KSN�Լ�1��
	*** ��    ��:  	����: KSN��
				    �������һ��KSN��
	*** �� �� ֵ:	FALSE / TRUE   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.11.18����
***************************************************************************************/
uint8_t GetNextKSN(void)
{

	uint8_t KsnBuf[5]={0};
//	uint8_t

  do
	{
		( * ( ENC_KEY.temp.ksn + 9 ) )++;

		if (	* ( ENC_KEY.temp.ksn + 9 ) == 0x00 )
		{
			( * ( ENC_KEY.temp.ksn + 8 ) )++;
			if (	* ( ENC_KEY.temp.ksn + 8 ) == 0x00 )
			{
				( * ( ENC_KEY.temp.ksn + 7 ) )++;
				if (	( * ( ENC_KEY.temp.ksn + 7 ) ) == 0x00 )
				{
					( * ( ENC_KEY.temp.ksn + 6 ) )++;
					if (	* ( ENC_KEY.temp.ksn + 6 ) == 0x00 )
					{
						( * ( ENC_KEY.temp.ksn + 5 ) )++;
						if (	* ( ENC_KEY.temp.ksn + 5 ) >= 0xFF )
						{
							return FALSE;
						}
					}
				}
			}
		}
		memcpy(KsnBuf,ENC_KEY.temp.ksn+5,5);
	}
	while ( calcsum (KsnBuf));
	
	WriteFlag = 1;
	WriteENCKEY();							  //������º��KSN

//	WriteFlag = 0;
	return TRUE;
}

void Dukpt_KeyManagement(void)
{
	memcpy(CryptoKey,ENC_KEY.temp.key,16);
//	CalcCryptoKeyfc();
//	GetIK(ENC_KEY.temp.ksn,ENC_KEY.temp.key,CryptoKey);
//	DBG_H("IK___toKey",CryptoKey,16);
	CalcCryptoKey();
	
//	DBG_H("CryptoKey",CryptoKey,16);
//			DBG_H("ENC_KEY.temp.ksnasn",ENC_KEY.temp.ksn,10);
//			DBG_H("ENC_KEY.temp.keykey",ENC_KEY.temp.key,16);
}

//void CalcCryptoKeyfc(void) 
//{
//   uint8_t ksnbuff[16]={0},keybuff[16]={0},i=0,CheckSumbuff[16]={"\xc0\xc0\xc0\xc0\x00\x00\x00\x00\xc0\xc0\xc0\xc0\x00\x00\x00\x00"};
//	 
//	 memcpy(ksnbuff,ENC_KEY.temp.ksn,8);//KSN���Ѿ���ֵ�˵�
//	 memcpy(keybuff,ENC_KEY.temp.key,16);//BDK��ԭʼ��Կ
//	 

//	 ksnbuff[8] &=0xe0;
//	 

//	 tri_des(ksnbuff,ENC_KEY.temp.key,ENC_KEY.temp.key+8,0);

//	 
//	 memcpy(CryptoKey,ksnbuff,8);

//	 for(i=0;i<16;i++)
//	   keybuff[i] ^= CheckSumbuff[i];
//	 
//	 memcpy(ksnbuff,ENC_KEY.temp.ksn,8);
//	 ksnbuff[8] &=0xe0;

//	 tri_des(ksnbuff,keybuff,keybuff+8,0);

//	 
//	 memcpy(CryptoKey+8,ksnbuff,8);
////   DBG_H("CryptoKey345",CryptoKey,16);
//	
//}
/**************************************************************************************
	*** �� �� ��:	void CalcCryptoKey(void) 
	*** ����������	���ݵ�ǰ���º��KSN��KEY�����������Կ��
	*** ��    ��:  	����: KSN��KEY��
				    �����������Կ��
	*** �� �� ֵ:	NULL   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.11.18����
***************************************************************************************/
void CalcCryptoKey(void) 
{
	int counter;
	uint8_t k=1;
	//GetNextKSN();
	GetNextFlag = 1;
	IDT_LoadInitKey(1,CryptoKey,ENC_KEY.temp.ksn);
		
	counter = ENC_KEY.temp.ksn[9] | (ENC_KEY.temp.ksn[8] << 8) | ((ENC_KEY.temp.ksn[7] & 0x1f) << 16);
	do
	{	       
		IDT_EncryptPIN(1,IDT_ChangeCounter(counter, k),ENC_KEY.temp.ksn,CryptoKey,0); 
		k++;
	}while(k <= IDT_GetNumOfOnes(counter));
	
}

/*  


�����


*/

 int CalcXOR(uint8_t *datas, int len,int sl)
{
		int i = 0;
		int CheckSum = 0x02;

		for (i = sl; i < len; i++)
		{ 
//			printf("datas %x\r\n",datas[i]);
				CheckSum ^= datas[i];
		}

		return CheckSum;
}

 int CalcXOR_none(uint8_t *datas, int len,int sl)
{
		int i = 0;
		int CheckSum = 0x00;

		for (i = sl; i < len; i++)
		{ 
//			printf("datas %x\r\n",datas[i]);
				CheckSum ^= datas[i];
		}

		return CheckSum;
}

/******************************************************
*��������:CheckSum8
*��   ��:buf ҪУ������� 
        len У�����ݵĳ�
*��   ��:У���
*��   ��:У���-8
*******************************************************/
char CheckSum8(char *buf,int len)
{
	  int    i=0;
	  char Sum=0;

	  for (i=0;i<len;i++)
	  {
		  Sum+=*buf++;
	  }

	  return Sum;
}

/**************************************************************************************
	*** �� �� ��:	void send_frame(uint8_t frame_type)
	*** ����������	���ض��������Ӧ��
	*** ��    ��:  	����: frame_type -- ��Ӧ����Ĳ�����
	*** �� �� ֵ:	NULL   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.09.20����
***************************************************************************************/
void send_frame(uint8_t frame_type)
{
  uint8_t  i = 0,BLRC,j,ivec[16]/*,fkey[16]*/;
	uint8_t TxdBuf[MAX_TXD_BUF_LEN]={0};
	uint8_t TempBuf[16];
	uint8_t pBKCV[6];
	uint16_t SUM_Vaule=0;
  
  i=0;      
  switch (frame_type) 
	{
		
		  case SETENCRYPTIONOPTIONSET:
			  TxdBuf[i++]=0x06;
			  TxdBuf[i++]=0x02;
			  TxdBuf[i++]=SETENCRYPTIONOPTIONSET;
			  TxdBuf[i++]=0x01;
			  TxdBuf[i++]=ENC_KEY.temp.Enhancedoption;
			  					
		    break;
		  case SETENCRYPTIONFORMAT: //��Ӧǿ����/��ͨ����
				TxdBuf[i++]=0x06;
			  TxdBuf[i++]=0x02;
			  TxdBuf[i++]=SETENCRYPTIONFORMAT;
			  TxdBuf[i++]=0x01;
			  TxdBuf[i++]=ENC_KEY.temp.Enhancedstatue;
			  			
				break;
			case SETENCMODE:
				TxdBuf[i++]=0x06;
			  TxdBuf[i++]=0x02;
			  TxdBuf[i++]=SETENCMODE;
			  TxdBuf[i++]=0x01;
			  TxdBuf[i++]=ENC_KEY.temp.encmode;
			  break;
		  case ACTIVATEAUTHEN://���ܵȼ�4��ʽ��Ӧ 10bit KSN+ 20bits�����
				Dukpt_KeyManagement();
			                                                  //��ǰdukpt��Կ����� 16bits��0XF0
			  GetXORKey("\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0",CryptoKey,ivec,16);
			  
			  memcpy(ENC_KEY.temp.Authenticationkey_Buff,ivec,16);//������֤1�ļ�����Կ
				TxdBuf[i++]=0x06;
			  TxdBuf[i++]=0x02;
			
			  memcpy(TxdBuf+i,ENC_KEY.temp.ksn,10);
			  i+=10;
			
			  memset(TempBuf,0x00,sizeof(TempBuf));			  
			  for(j=0;j<6;j++)  //����challenge 1
			    TempBuf[j]=rand()%127;
			  
			  TempBuf[j++]=ENC_KEY.temp.ksn[8];
			  TempBuf[j++]=ENC_KEY.temp.ksn[9];			
					
				tri_des(TempBuf,ivec,ivec+8,0);//����challenge 1 �����
				memcpy(TxdBuf+i,TempBuf,8);
				i+=8;		

				memcpy(ENC_KEY.temp.Challenge1_Buff,TempBuf,8);//����8bits���������	
        
				memset(TempBuf,0x00,sizeof(TempBuf));
        for(j=0;j<8;j++)TempBuf[j]=rand()%127;// ����challenge 2  8bits
			    		  
			  memcpy(ENC_KEY.temp.Challenge2_Buff,TempBuf,8); //ENC_KEY.temp.Challenge2_Buff �����˳����ܵȼ�4��ʱ��
									
				tri_des(TempBuf,ivec,ivec+8,0);//����challenge 2 �����
				memcpy(TxdBuf+i,TempBuf,8);
				memcpy(ENC_KEY.temp.Challenge2_Buff,TempBuf,8);//����8bits���������	
				i+=8;	
					
//			  DBG_H("TxdBuf",TxdBuf,i);	
//				TxdBuf[i++]=ETX;

				ENC_KEY.temp.Activa_Challenge_Reply=0x35;//���ܵȼ�4��ʱ�� ��һ����֤
				break;
		  case UIC_LOADKEY:  //UIC LOADKEY Response 
				
        			
					if(sendcount==0xF1 && macstate==0xF1)
					{
						memset(TempBuf,0x00,16);
						
						TxdBuf[i++]=0x02;
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x30;
				
						TxdBuf[i++]=0x30;					
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x30;
						
						tri_des(TempBuf,ENC_KEY.temp.key,ENC_KEY.temp.key+8,0);
						tri_des(TempBuf+8,ENC_KEY.temp.key,ENC_KEY.temp.key+8,0);

						bcd2asc(pBKCV,TempBuf,4,0);
					
						memcpy(TxdBuf+i,pBKCV,4);
						i+=4;
						ENC_KEY.temp.level=0x33;
						ENC_KEY.temp.status=1;
						WriteFlag=1;
						WriteENCKEY();
					}
				else	if(sendcount==0xFC && macstate==0xF1)
					{
						TxdBuf[i++]=0x02;
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x30;
				
						TxdBuf[i++]=0x30;					
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x33;
					}
					
				else	if(sendcount==0xF1 && macstate==0xFC)
					{
						TxdBuf[i++]=0x02;
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x30;
				
						TxdBuf[i++]=0x30;					
						TxdBuf[i++]=0x33;
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x30;
					}
				else if(sendcount==0xFC && macstate==0xFC)
					{
						TxdBuf[i++]=0x02;
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x30;
				
						TxdBuf[i++]=0x30;					
						TxdBuf[i++]=0x33;
						TxdBuf[i++]=0x30;
						TxdBuf[i++]=0x33;
					}
				else		if(macstate==0xFA)
					{
						TxdBuf[i++]=0x02;
				    TxdBuf[i++]=0x30;
				    TxdBuf[i++]=0x30;
					
					  TxdBuf[i++]=0x30;					
					  TxdBuf[i++]=0x32;
					  TxdBuf[i++]=0x30;
					  TxdBuf[i++]=0x32;

					}
				sendcount=0; macstate=0;
					
//				  TxdBuf[i++]=ETX;


			break;
			case SETDECODEWAY:

			  TxdBuf[i++]=0x06;
			  TxdBuf[i++]=0x02;
			  TxdBuf[i++]=0x1d;
			  TxdBuf[i++]=0x01;
			  TxdBuf[i++]=ENC_KEY.temp.decoded_directions;
			  TxdBuf[i++]=0x03;

			break;
			case SETENCWAY://��ȡ����״̬  aes��3des

			  TxdBuf[i++]=0x06;
			  TxdBuf[i++]=0x02;
			  TxdBuf[i++]=0x4c;
			  TxdBuf[i++]=0x01;
			  TxdBuf[i++]=ENC_KEY.temp.encway;
			
//			  TxdBuf[i++]=ETX;

			break;
			case DATAFMTID1:
		    TxdBuf[i++]=0x06;
			break;
			
//		case LOADKEYAUTHENTICATE:///  ��֤������Կ���  ��֤ͨ��  ����������Կ    

//			if(ENC_KEY.temp.KeyAuthenticateFlag==0x31)
//			{
//				TxdBuf[i++]=0x01; TxdBuf[i++]=0x00; TxdBuf[i++]=0x02; TxdBuf[i++]=0x01; TxdBuf[i++]=0x00;
//			}
//			else { TxdBuf[i++]=0x01; TxdBuf[i++]=0x00; TxdBuf[i++]=0x02; TxdBuf[i++]=0x01; TxdBuf[i++]=0x01;  }
//		  bcc=0xAA;
//			break;
			
//		case SETKEYVALUE://����8Bit ���������λ��,��λ�����н�����8bit�����������ȷ��������������Կ����  IDTECH�ϵ��� ��ʱ����

//			  TxdBuf[i++]=0x01;
//				TxdBuf[i++]=0x00;
//				TxdBuf[i++]=0x0A;
//				TxdBuf[i++]=0x01;
//				TxdBuf[i++]=0x00;
//		
//		for(j=0;j<8;j++)
//		{
//		  TempBuf[j]=rand()%127;
//			LOADKEY_AuthenticateRadomBuf[j]=TempBuf[j];
//			
//		}

//		if (0x31 == ENC_KEY.temp.encmode)				   		//DUKPT����
//		{

//			Dukpt_KeyManagement(); //����DUKPT������Կ  ENC_KEY.temp.key
//			
//		}
//		else if (0x30 == ENC_KEY.temp.encmode)			 		//FIX����
//		{
//			memcpy(CryptoKey,ENC_KEY.temp.fix,16);				//����FIX��Կ
//		}

//		if(0x31 == ENC_KEY.temp.encway)					//three DES���� 
//		{
//			tri_des(TempBuf,CryptoKey,CryptoKey+8,0);
//			memcpy(TxdBuf+i,TempBuf,8);
//			i+=8;
//		}
//		else
//		{
//			AES_set_encrypt_key(CryptoKey,128,&key);
//			AES_cbc_encrypt(TempBuf,TempBuf,16,&key,ivec,1);

//			memcpy(TxdBuf+i,TempBuf,16);
//			i+=16;
//			
//		}

////		TxdBuf[i++] = ETX;
//		
//			break;
		
		case GETREADER_STATUSCOMMAND:

		  TxdBuf[i++]=0x06;//   ��ʵ֡��ͷ
		  TxdBuf[i++]=0x02;
		  TxdBuf[i++]=0x83;
		  TxdBuf[i++]=0x02;
		  TxdBuf[i++]=0x00;
		  TxdBuf[i++]=0x00;
		
//		  TxdBuf[i++]=ETX;
//		  TxdBuf[i++]=0x80;
		  
		break;
		
		case REVIEWSET:

		TxdBuf[i++]=0x06;//   ��ʵ֡��ͷ
		TxdBuf[i++]=0x02;
//		
    TxdBuf[i++]=GETSECURITYLEVEL;//���ܵȼ�
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.level;
		
		TxdBuf[i++]=SETENCWAY	;//					0x4C	/* set encryption way */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.encway;//Encryption Settings
		
		TxdBuf[i++]=REVIEWSN	;//					0x4E	/* KSN */
		TxdBuf[i++]=0x0B;//
		TxdBuf[i++]=0x0A;//
		memcpy(TxdBuf+i,ENC_KEY.temp.ksn,10);
		i+=10;//  21
		
		TxdBuf[i++]=TRANSMODE;//����ģʽ
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Tranmodeselect;//����ģʽ

//    TxdBuf[i++]=0x77;//
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x10;//
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;
//		TxdBuf[i++]=0x3D;
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x00;
		
		TxdBuf[i++]=SETENCRYPTIONFORMAT;//				0x85	/* Encryption Output Format Setting*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Enhancedstatue;//ǿ�Ƽ��� �� ��ͨ����
		
		
		TxdBuf[i++]=SETBEEP;//				0x11	/* beep set*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.beepmode;//������ģʽ
		
//		TxdBuf[i++]=0x12;//				0x12	/* �ַ�������ʱ��*/
//		TxdBuf[i++]=0x01;// 41
//		TxdBuf[i++]=0x30;//
		
		TxdBuf[i++]=SETSELECTTRACK;//				/* ���ѡ��  0 ����� 1-���1 2-���2 */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.selecttrack;//
		
//		TxdBuf[i++]=0x14;//				0x14	/* USB HID Polling Interval*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x01;//

//    TxdBuf[i++]=0x15;//				0x15	/*  ID TECH Format;*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x16;//				0x16	/*  FmtOptionID*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x59;//
		
		TxdBuf[i++]=SETTRACKSEPARATOR;//				0x17	/*  �ָ����趨*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.setseparator;//
		
		TxdBuf[i++]=SETTRACK2;//				0x19	/*���ù��2�������͸�ʽ*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.settrack2format;//
		
		TxdBuf[i++]=MSRSETTINGS;//				0x1A	/*ˢ����  enable or  disable*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.enabledevice;//
		
//		TxdBuf[i++]=0x1B;//				0x1B	/*  Data Editing Control */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
		
		TxdBuf[i++]=SETDECODEWAY;//				0x1D	/*  Decoding in bothdirection; ��0�� Raw data ��2��forward ��3�� reverse */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.decoded_directions;//
		
		TxdBuf[i++]=SETTERMINATOR;//				0x21	/*  Terminator */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Terminator;////  71
		
//		TxdBuf[i++]=0x24;//				0x24	/*  Foreign Keyboard */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x30;//				0x30	/*  ArmtoReadID* */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x00;//
//		
//		TxdBuf[i++]=0x41;//				0x41	/*  9600 bps, ��2�� is 1200, ��7��is 38,400 bps; ��9�� is 115.2kbps* */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x35;//
//		
//		TxdBuf[i++]=0x42;//				0x42	/*  8 Bits required in securemode */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x43;//				0x43	/*Data Parity  none */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x44;//				0x44	/*  Software (Xon/Xoff) handshake */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x45;//				0x45	/*  Stop Bit */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x47;//				0x47	/*  0x11 as XOn */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x11;//
//		
//		TxdBuf[i++]=0x48;//				0x48	/* 0x13 as XOff */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x13;//
		
		TxdBuf[i++]=SETPrePANID;//				0x49	/* 0x13 as XOff */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.PrePANID;// 101
		
		TxdBuf[i++]=SETPostPANID;//				0x4A	/* 0x13 as XOff */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.PostPANID;//
		
		TxdBuf[i++]=SETMASKCHAR;//				0x4B	/* 0x13 as XOff */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.MaskChar;//
		
		TxdBuf[i++]=DispExpDateID;//				0x50	/* ��1�� don��t mask expirationdate */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.DispDataflag;//
		
//		TxdBuf[i++]=0x55;//				0x50	/* don��t include mod10, ��1��display mod10, ��2�� displaywrong mod10*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x60;//				0x60	/* Without LRC in output*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x61;//				0x61	/* ��%�� as Track 1 7 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x62;//				0x62	/* ��%�� as Track 1 6 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x63;//				0x63	/* ��;�� as Track 1 5 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3B;//
//		
//		TxdBuf[i++]=0x64;//				0x64	/* ��%�� as Track 2 7 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x65;//				0x65	/* ��%�� as Track 2 7 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3B;//  131
//		
//		TxdBuf[i++]=0x66;//				0x66	/* ��%�� as Track 3 7 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x67;//				0x67	/* ��!�� as Track 3 6 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x21;//
//		
//		TxdBuf[i++]=0x68;//				0x68	/* ��;�� as Track 3 5 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3B;//
//		
//		TxdBuf[i++]=0x69;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3F;//
//		
//		TxdBuf[i++]=0x6A;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3F;//
//		
//		TxdBuf[i++]=0x6B;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3F;//
//		
//		TxdBuf[i++]=0x6C;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x6D;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3B;//
//		
//		TxdBuf[i++]=0x6E;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x2B;//
//		
//		TxdBuf[i++]=0x6F;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x31;//   161
//		
//		TxdBuf[i++]=0x72;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x00;//
//		
//		TxdBuf[i++]=0x73;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x00;//
		
//		TxdBuf[i++]=0x7A;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=ENC_KEY.temp.beepmode;//
		
//		TxdBuf[i++]=0x7B;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
		
//		TxdBuf[i++]=0x7A;//				0x69	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=ENC_KEY.temp.beepmode;//
		
		
		//OK
		TxdBuf[i++]=SETPREAMBLE;//				0xd2	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Preamblelen_m;//
		
		TxdBuf[i++]=SETPREAMBLE;//				0xd2	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Preamblelen_m;//
		
		TxdBuf[i++]=SETPOSTAMBLE;//				0xd3	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Postamblelen_m;//
		
		TxdBuf[i++]=SETTRACK1PREFIX;//				0x34	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.tp1len;//
		
		TxdBuf[i++]=SETTRACK1SUFFIX;//				0x37	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.ts1len;//  191
		
		TxdBuf[i++]=SETTRACK2PREFIX;//				0x35	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.tp2len;//
		
		TxdBuf[i++]=SETTRACK2SUFFIX;//				0x38	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.ts2len;//
		
		TxdBuf[i++]=SETTRACK3PREFIX;//				0x36	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.tp3len;//
		
		TxdBuf[i++]=SETTRACK3SUFFIX;//				0x39	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.ts3len;//
		
		TxdBuf[i++]=HASHOPTION;//				0x5C	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.HASHSET;//
		
		TxdBuf[i++]=MASKOPTION;//				0x86	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.MaskSetting;//
		
//		TxdBuf[i++]=0x89;//				0x89	/* ��?�� as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x34;//
		
		TxdBuf[i++]=SETENCRYPTIONOPTIONSET;//				0x86	/* ��?�� as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Encryption_OptionSetting;//   
		
//		TxdBuf[i++]=0x5D;//				0x5D	/* HexCaseID*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x31;//  218
//		
//		TxdBuf[i++]=0xFA;//				0xFA	/* No Added Field*/
//		TxdBuf[i++]=0x00;//
//		
//		TxdBuf[i++]=0xFB;//				0xFB	/* No Added Field*/
//		TxdBuf[i++]=0x00;//
//		
//		TxdBuf[i++]=0xFC;//				0xFB	/* No Added Field*/
//		TxdBuf[i++]=0x08;//
//		TxdBuf[i++]=0x00;//		
//		
//		TxdBuf[i++]=0xFF;//
//		TxdBuf[i++]=0x00;//	
//		TxdBuf[i++]=0xFF;//
//		TxdBuf[i++]=0x00;//
//		TxdBuf[i++]=0xFF;//
//		TxdBuf[i++]=0x00;//
//		TxdBuf[i++]=0xFF;//

//		TxdBuf[i++]=ETX;//  218+15=233
		
		break;
		
		
		case ENCRYPTEXTERNAL://�ⲿ��֤ģʽ  3.12С�� ��С�ڲ��Ǻ����IDTECH��ʵ�ʵ�Ӧ��

		  TxdBuf[i++]=0x06;
		  TxdBuf[i++]=0x02;
		  j=RxdBuf[2]|RxdBuf[3]<<8;
//		  loop=RxdBuf[2]|RxdBuf[3]<<8;//���㳤��
		  memcpy(TxdBuf+i,RxdBuf+4,j);
		  
		  
		if (0x31 == ENC_KEY.temp.encmode)				   		//DUKPT����
		{			
			Dukpt_KeyManagement(); //����DUKPT������Կ  ENC_KEY.temp.key			
		}
		else if (0x30 == ENC_KEY.temp.encmode)			 		//FIX����
		{
			memcpy(CryptoKey,ENC_KEY.temp.fix,16);				//����FIX��Կ
		}
		
		if(0x31 == ENC_KEY.temp.encway)					//three DES���� 
		{
			SUM_Vaule = ceil((j-1)/8.0);

			tri_des(TxdBuf+i,CryptoKey,CryptoKey+8,0);
			for(j = 1;j< SUM_Vaule;j++)							//���ܵ�һ�ŵ�����
			{			
				xor(TxdBuf+j*8+i,TxdBuf+j*8-8+i,8);
				tri_des(TxdBuf+j*8+i,CryptoKey,CryptoKey+8,0);
			}

			i+=SUM_Vaule*8;

			if(ENC_KEY.temp.level==0x34)
			{
				memcpy(TempBuf,ENC_KEY.temp.sid,8);
				tri_des(TempBuf,CryptoKey,CryptoKey+8,0);
				memcpy(TxdBuf+i,TempBuf,8);
				i+=8;
			}
						
		}
		
		else if(0x32 == ENC_KEY.temp.encway)					//AES����
		{
			SUM_Vaule = ceil((j-1)/16.0);
			AES_set_encrypt_key(CryptoKey,128,&key);
			
			AES_cbc_encrypt(TxdBuf+i,TxdBuf+i,SUM_Vaule*16,&key,ivec,1);
			i+=SUM_Vaule*16;
			
			if(ENC_KEY.temp.level==0x34)
			{
				memcpy(TempBuf,ENC_KEY.temp.sid,8);
				AES_cbc_encrypt(TxdBuf+i,TxdBuf+i,SUM_Vaule*16,&key,ivec,1);
				memcpy(TxdBuf+i,TempBuf,16);
				i+=16;
			}
			
		}
		
		if(ENC_KEY.temp.level==0x33){
			memcpy(TxdBuf+i,ENC_KEY.temp.sid,8);
			i+=8;
		}
		
		if (0x31 == ENC_KEY.temp.encmode)				   		//DUKPT����
		{			
			memcpy(TxdBuf+i,ENC_KEY.temp.ksn,10);
			i+=10;
		}
		else if (0x30 == ENC_KEY.temp.encmode)			 		//FIX����
		{
			memcpy(TxdBuf+i,ENC_KEY.temp.sn,10);
			i+=10;
			TxdBuf[i++]=0x00;
			TxdBuf[i++]=0x00;
		}
//		TxdBuf[i++]=ETX;

       break;
//    case LOADKEY://��Կ����ɹ�ָ���  ��ָ����IDTECHָ��

//		  TxdBuf[i++]=0x01;
//		  TxdBuf[i++]=0x00;
//		  TxdBuf[i++]=0x02;
//		  TxdBuf[i++]=0x01;
//		  TxdBuf[i++]=0x00;	
//		
//		break;
    case REQUEST_DOWN:   				/* request download */
						    //��������
			TxdBuf[i++] = frame_type;
			break;

//		case READ_VERSION:					/* read application ersion */
////			i = 0;
//			TxdBuf[i++] = 0x06; 			// 06
//			TxdBuf[i++] = RxdBuf[0]; 		// 02
//			bcc = RxdBuf[0];
//			TxdBuf[i++] = strlen(APPVERSION);

//			for(j = 0; j < strlen(APPVERSION); j++)
//			{
//				TxdBuf[i++] = APPVERSION[j];

//			}
//			TxdBuf[i++] = ETX;

//			break;
        			 
		case CMDSUCC:
		case CMDERRO:					   //���ܴ�ͷЭ��
			TxdBuf[i++] = frame_type;

			break;
		case GETSECURITYLEVEL:					//0x7E	/* get security level */
			
//				i = 0;
				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = RxdBuf[0]; 		// 02

				TxdBuf[i++] = GETSECURITYLEVEL;
				TxdBuf[i++] = 0x01;

				TxdBuf[i++] = ENC_KEY.temp.level;
			
//				TxdBuf[i++] = ETX;

//			init_serial_boot();	
			break;
		case GETCHALLENGE://����fix������֤  ��֤ͨ���ſ��Խ���fix����
				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = 0x02; 		// 02
				
//			  ����8bit�����
				for(j=0;j<8;j++)
				{
					FixKey_AuthenBuff[j]=rand()%127;
				}
				  memcpy(TempBuf,FixKey_AuthenBuff,8);

				  Dukpt_KeyManagement();

				  tri_des(TempBuf,CryptoKey,CryptoKey+8,0);///���������
				  memcpy(TxdBuf+i,TempBuf,8);

				i+=8;
//				TxdBuf[i++] = ETX;
	
			break;			
		case READID:

				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = RxdBuf[0]; 		// 02

				for(j=0; j<strlen(SHID); j++)  //������ȫ��ͷID����������
				{
					TxdBuf[i++] = SHID[j];

				}
//				TxdBuf[i++] = ETX;

			break;
		case REVIEWKSN://��ȡ�豸���к�KSN

				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = RxdBuf[0]; 		// 02

				TxdBuf[i++] = RxdBuf[2];		// 51

				TxdBuf[i++] = 0x0B;				// 0B

				TxdBuf[i++] = 0x0A;				// 0A

				
				for(j=0; j< 10; j++)  			//����KSN����������
				{
					TxdBuf[i++] = ENC_KEY.temp.ksn[j];

				}
				TxdBuf[i++] = ETX;
			break;
		case REVIEWSN:							//0x4E	/* review serial number */

				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = RxdBuf[0]; 		// 02

				TxdBuf[i++] = RxdBuf[2];		// 4E

				TxdBuf[i++] = 0x0B;				// 0B
				TxdBuf[i++] = 0x0A;				// 0A
				
				for(j=0; j< 10; j++)  			//����SN����������
				{
					TxdBuf[i++] = ENC_KEY.temp.sn[j];
//					bcc ^= ENC_KEY.temp.sn[j];
				}
//				TxdBuf[i++] = ETX;

			break;
		case SETSID:							//0x54	/* set Session ID */

				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = RxdBuf[0]; 		// 02

				TxdBuf[i++] = RxdBuf[2];		// 54

				TxdBuf[i++] = 0x09;				// 09

				TxdBuf[i++] = 0x08;				// 08
				
				for(j=0; j< 8; j++)  			//����SID����������
				{
					TxdBuf[i++] = ENC_KEY.temp.sid[j];
				}
//				TxdBuf[i++] = ETX;
			
			break;

        default:			
        	init_serial_boot();
			return;
    }
	init_serial_boot();
	if(i!=0x01)
	{
		TxdBuf[i++]=ETX;
	  BLRC=CalcXOR_none(TxdBuf,i,0);		
		TxdBuf[i++]=BLRC;
		
	}
//	printf("i=%d\r\n",i);
//	DBG_H("TxdBuf",TxdBuf,i);
		
		IDTUSB_Reviceflag=0xab;
	
		usbd_sendlen=ceil(i/8.0);//i/8;
     
//		usbd_sendlen+=1;
	  memset(IDTUSB_SendBuff,0x00,128);//�������buff
		sendcount=0;
	  memcpy(IDTUSB_SendBuff,TxdBuf,i);
}


/**************************************************************************************
	*** �� �� ��:	void DealSerialParse(void)
	*** ����������	���յ������ݽ��������ݰ���
	*** ��    ��:  	NULL
	*** �� �� ֵ:	NULL   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.09.20����
***************************************************************************************/
void DealSerialParse(void)
{
	uint8_t i = 0;
	while(1)
	{	
		if(UsbRecvOne(&i) != 1)  //��ȡUSB����
		{
		    return;
		}	
		
		switch (RxdStatus)
		{ /*��������״̬*/
				
			case 0:
				if(STX == i)
				{
//					printf("012\r\n");
					RxdBuf[0] = i;
					Bcc = i;	
					NowLen = 1;
					RxdStatus = 25;
				}
				else 
				{
					SynHead = 0;
//					RxdFrameStatus = FINISH;	
				}
				break;				/*���հ�ͷ*/
			case 25:
				if (i == ETX) 
				{
//					printf("034\r\n");
					RxdStatus = 26;
				}
				RxdBuf[NowLen++] = i;
				Bcc ^= i;

				break;
			case 26:
				if(i == Bcc || (i ^ 0x02) == Bcc)
				{
					RxdBuf[NowLen++] = i;
					RxdTotalLen = NowLen;
					RxdFrameStatus = FINISH;					
					RxdStatus = 0;
					SynHead = 0;

					break;
				}
				else
				{
				  RxdFrameStatus = FINISH;			
					RxdBuf[NowLen++] = i;
					Bcc ^= i;
					RxdTotalLen = NowLen;

					RxdStatus = 25;
				}
				break;
			default:
				
				if (RxdFrameStatus == SPACE) 
				{
					RxdFrameStatus = FINISH;

					RxdStatus = 0;
					SynHead = 0;
//					RxdTotalLen = NowLen;
				}
			break;
		 }
	}
}

/**************************************************************************************
	*** �� �� ��:	void DealRxData(void)
	*** ����������	�����յ����������ݰ�������Ӧ���ݰ���
	*** ��    ��:  	NULL
	*** �� �� ֵ:	NULL   	 	
	*** ģ����Ϣ: 	Ouyangweiquan 2011.09.20����
***************************************************************************************/

void DealRxData(void)
{
	uint16_t i,loop,BQLRC;
	uint8_t EnyBuff[100]={0},ivec[16],hbuff[16]={0},TKMbuff[32]={0};
   
	if (FINISH == RxdFrameStatus)
	{    
//		  DBG_H("CMD", RxdBuf, RxdTotalLen);		
//			BQLRC=CalcXOR_none(RxdBuf,RxdTotalLen-1,0);

//   printf("RxdTotalLen=%d\r\n",RxdTotalLen);
		
		BQLRC=RxdBuf[RxdTotalLen-1]; //		RxdTotalLen=���ճ���  BQLRC=����CMD��LRC 
    
//	printf("BQLRC=%d\r\n",BQLRC);
		if(RxdBuf[0] == STX)						//����02���ݰ�
		{

//			usb_reviceflag=0x1B;  //���ô�  ������Ӧ��Чָ��
	/*		if(0x01 == RxdBuf[1])	   //����ΪUIC  load key�ķ�ʽ  ��ʱ��IDTECH��load key ����
			{
				if(0x03 == RxdBuf[3])	  //������Կ�л�ȡ8bit����� ������֤��Կ����  
				{
//					printf("009\r\n");
					send_frame(SETKEYVALUE);
					init_serial_boot();						
					
				}
				else if(0x06 == RxdBuf[3])	 //  ���ڵ�����Կ��֤  ��֤ͨ������������Կ
				{
					if(RxdBuf[5]==0x05)
					{
//						memset(EnyBuff,0x00,sizeof(EnyBuff));
						memcpy(EnyBuff,RxdBuf+5,4);
						
						if(memcmp(EnyBuff,LOADKEY_AuthenticateRadomBuf,4)==0)
							ENC_KEY.temp.KeyAuthenticateFlag=0x31;
						
						else ENC_KEY.temp.KeyAuthenticateFlag=0x30;
						
//						printf("ENC_KEY.temp.KeyAuthenticateFlag=%d\r\n",ENC_KEY.temp.KeyAuthenticateFlag);

//						DBG_H("EnyBuff",EnyBuff,4);
//						DBG_H("random",random,4);
						send_frame(LOADKEYAUTHENTICATE);
						init_serial_boot();
					}
					else
					{
						send_frame(CMDERRO);
						init_serial_boot();
					}									
				}
				else if(0x13== RxdBuf[3])
				{
					if(RxdBuf[5] == 0x04)
					{
						if(ENC_KEY.temp.KeyAuthenticateFlag==0x31)
						{
						if(RxdBuf[6]==0x01)
						{    
							firstflag = 0X55; //��־λΪ0X55 
							memset(firstkeybuff,0x00,sizeof(firstkeybuff));
							memcpy(firstkeybuff,&RxdBuf[7],16);	
							send_frame(CMDSUCC);
							init_serial_boot();	
			//				DBG_H("firstkeybuff",firstkeybuff,16);
						}
								
						else if(RxdBuf[6]==0x02 )
						{
							secondflag=0x55;
							memset(secondkeybuff,0x00,sizeof(secondkeybuff));
							memcpy(secondkeybuff,&RxdBuf[7],16);
			//        DBG_H("secondkeybuff",secondkeybuff,16);
							send_frame(CMDSUCC);
							init_serial_boot();	
								 
						}
						
						if(firstflag == 0x55  && secondflag==0x55)
							{
								  firstflag=0;
								  secondflag=0;
									xor(firstkeybuff,secondkeybuff,16);	
								
									memset(ENC_KEY.temp.key,0x00,sizeof(ENC_KEY.temp.key));//����key����                
									memcpy(ENC_KEY.temp.key,firstkeybuff,16);
			//					DBG_H("ENC_KEY.temp.key",ENC_KEY.temp.key,16);
			//						send_frame(CMDSUCC);
								  memset(firstkeybuff,0x00,sizeof(firstkeybuff));
									memset(secondkeybuff,0x00,sizeof(secondkeybuff));
								  send_frame(LOADKEY);
								  init_serial_boot();
							}
						}
            else
						{
							send_frame(CMDERRO);
							init_serial_boot();	
						}							
					
					}
					else
						{
							send_frame(CMDERRO);
							init_serial_boot();	
						}					
				}
				
			}*/
	 if (0x5A != RxdBuf[1] && RxdBuf[2]!=0x5A && RxdBuf[3]!=0xA5 && RxdBuf[4]!=0xA5 )	// ���м����������� ����Ҫ���²���3�ν���IAP0x55	/* request download */
		{
			USB_Updataflag=0xAD;
		}
		//UIC��load key 
		if(0x30 == RxdBuf[1] && 0x30 == RxdBuf[2] && 0x30 == RxdBuf[3] && 0x30 == RxdBuf[4]  )//����KSN 20Bits  ����֮���ٺϲ���10bits
			{
				memset(EnyBuff,0x00,100);
				for(i=0;i<100;i++)
				{
					if(RxdBuf[i]==0x03)
					{
						loop=i;
						break;
					}						
				}

				
				if(CalcXOR(&RxdBuf[1],loop,0)==RxdBuf[loop+1]) //�ж�etx�����Ƿ���� lrc
				{

				if(loop-5==52)
				{
					memcpy(EnyBuff,RxdBuf+5,20);
					
					for(i=0;i<20;i++)//�Ƚ�KSN��ASCII  20bits
					{
						if(EnyBuff[i] >=0x30 && EnyBuff[i] <=0x7A)
						{//ɾ��"; %"*/
						  DeleteChars(EnyBuff,":;<=>?@[]^_`");
						}
					}
					if(memcmp(EnyBuff,RxdBuf+5,20)==0)
						macstate=0xF1;
					
					else macstate=0xFC;
					
//					DBG_H("EnyBuff",EnyBuff,20);
					memcpy(EnyBuff,RxdBuf+25,32);
					
					for(i=0;i<32;i++)//�Ƚ�KEY��ASCII   32bits
					{
						if(EnyBuff[i] >=0x30 && EnyBuff[i] <=0x7A)
						{
						  DeleteChars(EnyBuff,":;<=>?@[]^_`"); //ɾ��"; %"
						}
					}
					
					if(memcmp(EnyBuff,RxdBuf+25,32)==0)
						sendcount=0xF1;
					
					else sendcount=0xFC;
					
						
					if(macstate==0xF1)
					{
						memcpy(EnyBuff,RxdBuf+5,20);										
						asc2bcd(TKMbuff,EnyBuff,20,0);
						memcpy(ENC_KEY.temp.ksn,TKMbuff,10);
					}

					if(sendcount==0xF1)
					{						
						memcpy(EnyBuff,RxdBuf+25,32);
					
						asc2bcd(TKMbuff,EnyBuff,32,0);
						
						memcpy(ENC_KEY.temp.key,TKMbuff,16);
					
					}
				}
				else
				{
					macstate=0xFA;
				}
				}
				else
				{
					macstate=0xFA;
				}

				send_frame(UIC_LOADKEY);		
			}
				//������ָͬ�����IAPģʽ
			else if (0x5A == RxdBuf[1] && RxdBuf[2]==0x5A && RxdBuf[3]==0xA5 && RxdBuf[4]==0xA5 )							//0x55	/* request download */
			{												//����
				USB_Updataflag++;

				if(USB_Updataflag==0xb0)
				{
				  WriteUpgradeFlag();

				  send_frame(CMDSUCC);

					delay_ms(1000);
				  NVIC_SystemReset();
				}
				
			    
			}
			else if (RESET_STM32 == RxdBuf[1] && RxdBuf[2]==0x03  )							//0x6A/* request STM32 */
			{
				send_frame(CMDSUCC);
				delay_ms(1000);
				NVIC_SystemReset();
				
			}
			else if(ENCRYPTEXTERNAL == RxdBuf[1])//  && ETX==RxdBuf[5] 0x41 3.12С�� ������ֻ����level 3 4״̬����Ч  �����ⲿ����  ��δ������ʲô����
			{
				if(ENC_KEY.temp.level==0x33 ||ENC_KEY.temp.level==0x34)
				{
					loop=RxdBuf[2]|RxdBuf[3]<<8;//���㳤��

					if(RxdBuf[loop+5]==CalcXOR_none(RxdBuf,loop+5,0))//����ETX�����ǲ���LRC  ������  ������ȷ  ���򷵻ش���
					{
						send_frame(ENCRYPTEXTERNAL);										
					}
					else
					{
						send_frame(CMDERRO);
					}
			  }
				else send_frame(CMDERRO);
			}
//			else if (READ_VERSION == RxdBuf[1])						//0x28	/* read application ersion */
//			{
//				send_frame(READ_VERSION);
//				init_serial_boot();
//			}
//			else if (RESETHEAD == RxdBuf[1])						//0xF8	/* reset secureheadreader */
//			{
//				Default_Settings();
////				ResetSetting();
//				send_frame(CMDSUCC);
//				init_serial_boot();
//			}
		
			else if (READCMD == RxdBuf[1])					//0x52	/* read status commands */  ��ȡ״̬����
			{
				switch (RxdBuf[2])
				{
					case SETENCMODE:
						send_frame(SETENCMODE);   
					          break;
					case SETDECODEWAY:                 //���ôſ����뷽��  ��ˢ ��ˢ ����ˢ  0x1d
						send_frame(SETDECODEWAY);
                		break;
					case SETENCWAY:                    //���ü��ܷ�ʽ    ��1�� 3DES ��2�� AES  0x4c
						send_frame(SETENCWAY);
                		break;
						
					case GETREADER_STATUSCOMMAND:                     /*������״̬�Ķ�ȡ  0x83 */
						send_frame(GETREADER_STATUSCOMMAND);
                		break;
					case REVIEWSET:                                  /*������״̬��Ϣ��ȡ 0x1f*/
						send_frame(REVIEWSET);
                		break;
					case SETENCRYPTIONOPTIONSET:                                  /*������״̬��Ϣ ������� 0x84*/
						send_frame(SETENCRYPTIONOPTIONSET);
                		break;
						
					case ACTIVATEAUTHEN:// Activate Authentication Mode Command   0x80 �ⲿ��ָ֤��  �ڵȼ�4�������ݴ����ʱ��					
						  send_frame(ACTIVATEAUTHEN);										
					break;
					case READID:						    //0x22	/* read secureheadreader ID */
						send_frame(READID);
                		break;
					case GETCHALLENGE:						//0x74	/* get encrypt challenge */
//						Random();							//���������
						send_frame(GETCHALLENGE);

                		break;
					case GETSECURITYLEVEL:					//0x7E	/* get security level */
						send_frame(GETSECURITYLEVEL);
                		break;
					case REVIEWKSN:							//0x51	/* review KSN (DUKPT key management only) */
						send_frame(REVIEWKSN);
                		break;
					case REVIEWSN:							//0x4E	/* review serial number */
						send_frame(REVIEWSN);
                		break;
					case SETSID:							//0x54	/* set Session ID */
						send_frame(SETSID);
					  break;
					case SETENCRYPTIONFORMAT:							//0x85	/* ��ѯ��ͨ���ܻ���ǿ�Ƽ��� */
						send_frame(SETENCRYPTIONFORMAT);
					
                		break;					
	           		default:
    	           		init_serial_boot();
								    send_frame(CMDERRO);
        	    		break;
        		}
				RxdFrameStatus = SPACE;
			}
			
/*                ���ö�����ָ�                                      */
/*                ����IDTECH�����ĵ�����ָ�                                     */
			else if (SETTINGCMD == RxdBuf[1])				//0x53	/* setting commands */
			{
				switch (RxdBuf[2])
				{
					
					case DATAFMTID:// ��Ϊ��IDTECH��λ��ͨ���д˹���  ��ʵ���ϲ��˽�ù���  
					
						if(RxdBuf[3]==0x01)
						{
							
							send_frame(DATAFMTID1);		
							
						}
						else send_frame(CMDERRO);		
						
					break;
					case DispExpDateID:       //  50

							if((RxdBuf[4]==0x30  || RxdBuf[4]==0x31) && RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)/// keyboard
							{								
							  ENC_KEY.temp.DispDataflag=RxdBuf[4];
						
				        send_frame(CMDSUCC);						
							}
							else send_frame(CMDERRO);							

						
						break;
					
					case TRANSMODE://���ݴ���ģʽ  23   HID����KEYBOARD

							if((RxdBuf[4]==0x38  || RxdBuf[4]==0x30) && RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)/// keyboard
							{
								
							  if(RxdBuf[4] != ENC_KEY.temp.Tranmodeselect)//ģʽ�����  ��ı�
								{
									
                  ENC_KEY.temp.Tranmodeselect=RxdBuf[4];
								  delay_ms(1000);
									send_frame(CMDSUCC);
                  WriteFlag = 1;
									WriteENCKEY();					//������º��KEY									
								  NVIC_SystemReset();
									
								}
								else send_frame(CMDSUCC);
			        
							}
							else send_frame(CMDERRO);
									
						break;
					
					case SETMASKCHAR://�����ַ�����  4B

							if((RxdBuf[4]>=0x20 && RxdBuf[4]<=0x7E)&& RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
							    ENC_KEY.temp.MaskChar=RxdBuf[4];

								  send_frame(CMDSUCC);																								
							}
							else send_frame(CMDERRO);

						break;
							
						
					case SETPrePANID://������ʼ�ַ�����  �ӵڼ�λ��ʼֲ������
              
							if(RxdBuf[4]<=0x06&& RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
							    ENC_KEY.temp.PrePANID=RxdBuf[4];

								  send_frame(CMDSUCC);																								
							}
							else send_frame(CMDERRO);

						break;	
					
						case SETPostPANID://��������ַ�����  �ӵڼ�λ��ʼֲ������

							if(RxdBuf[4]<=0x04 && RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
							    ENC_KEY.temp.PostPANID=RxdBuf[4];
									
								  send_frame(CMDSUCC);																								
							}
							else send_frame(CMDERRO);

						break;	
					
						
					case EXITACTIVATEAUTHEN://�˳���֤ģʽ
						if((RxdBuf[3]==0x08)||(RxdBuf[3]==0x10))
						{
							for(i=0;i<50;i++){
								if(RxdBuf[i]==0x03)
									break;
							}

						if(CalcXOR(&RxdBuf[1],i,0)==RxdBuf[i+1]) //�ж�ETX�����Ƿ���� LRC
						{	
							Dukpt_KeyManagement();
							GetXORKey("\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C",CryptoKey,hbuff,16);	
					
							if(RxdBuf[3]==0x08){
															
								memcpy(TKMbuff,RxdBuf+4,8);
								tri_des(TKMbuff,hbuff,hbuff+8,1);
								
								if(memcmp(TKMbuff,ENC_KEY.temp.Challenge2_Buff,7)==0){
									if(TKMbuff[7]==0x01) GetNextKSN();
									
									ENC_KEY.temp.Activa_Challenge_Reply=0x30;
									ENC_KEY.temp.level=0x31;

									send_frame(CMDSUCC);	
								}
							}
							else if(RxdBuf[3]==0x10){
								AES_set_decrypt_key(hbuff,128,&key);
								memcpy(TKMbuff,RxdBuf+4,16);

								AES_decrypt(ivec,TKMbuff,&key);

								if(memcmp(TKMbuff,ENC_KEY.temp.Challenge2_Buff,7)==0){
									if(TKMbuff[7]==0x01) GetNextKSN();
									
									ENC_KEY.temp.Activa_Challenge_Reply=0x30;
									ENC_KEY.temp.level=0x31;
									send_frame(CMDSUCC);
								
								}
								
							}
							else send_frame(CMDERRO);
						}
						else send_frame(CMDERRO);
						}
						else 
						{
							send_frame(CMDERRO);
						}

						break;
					
					case ACTIVATION_CHALLENGE_REPLY_COMMAND://���ݶ�����֤  �ڵȼ�4��ʱ��	
						
						if((RxdBuf[3]==0x10) /*&&(ENC_KEY.temp.Activa_Challenge_Reply==0x35) */)							
						{													
							for(i=0;i<23;i++){
								if(RxdBuf[i]==0x03)
									break;
							}
							
							if(CalcXOR(&RxdBuf[1],i,0)==RxdBuf[i+1]) //�ж�ETX�����Ƿ���� LRC
							{
							  memcpy(TKMbuff,RxdBuf+4,8); //Challenge 1����
//								
								Dukpt_KeyManagement();
								GetXORKey("\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C",CryptoKey,hbuff,16);

								tri_des(TKMbuff,hbuff,hbuff+8,1);

								if(memcmp(TKMbuff,ENC_KEY.temp.Challenge1_Buff,6)==0){
									 								 
									 memcpy(TKMbuff,RxdBuf+4+8,8);
									 tri_des(TKMbuff,hbuff,hbuff+8,1);
									 memcpy(ENC_KEY.temp.sid,TKMbuff,8);//�ı�SID

									 ENC_KEY.temp.Activa_Challenge_Reply=0x31;
									 ENC_KEY.temp.level=0x34;
									 send_frame(CMDSUCC);

								}
								else 
								{ 
								send_frame(CMDERRO); 
								GetNextKSN();
								}								
								
							}
							else
							{
                                GetNextKSN();
								send_frame(CMDERRO);
								ENC_KEY.temp.Activa_Challenge_Reply=0x30;
								
							}
						}
					
					else
					{
						GetNextKSN();
						send_frame(CMDERRO);
						ENC_KEY.temp.Activa_Challenge_Reply=0x30;
					}

					break;
					
					case MASKOPTION: //MaskSetting  ���������������  86

							if(RxdBuf[4]<=0x07 && RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
								ENC_KEY.temp.MaskSetting=RxdBuf[4];

								send_frame(CMDSUCC);
							}
						  else send_frame(CMDERRO);

							break;
					case HASHOPTION:
							if((RxdBuf[4] >0x2f && RxdBuf[4] < 0x38) && RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
								ENC_KEY.temp.HASHSET=RxdBuf[4];
								send_frame(CMDSUCC);
							}
							else send_frame(CMDERRO);
							break;		
					case SETDEFAULT:						//0x18	/* set default configuration */
							macstate = 0x00;					//δ���յ���������
							Default_Settings();					
							send_frame(CMDSUCC);

            break;
					case SETBEEP:
							if((0x30 <= RxdBuf[4] && 0x34 >= RxdBuf[4]) && RxdBuf[3] == 0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)		 //ƥ������Ĳ���
							{
								ENC_KEY.temp.beepmode = RxdBuf[4];	//��ȡ���õļ��ܷ�ʽ														
								send_frame(CMDSUCC);
							}
							else
							{
								send_frame(CMDERRO);
							}

                		break;
        case SETSELECTTRACK :  /*����ѡ����Ĺ��*/

							if((0x30 <= RxdBuf[4] && 0x39 >= RxdBuf[4])&&RxdBuf[3] == 0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)		 //ƥ������Ĳ���
							{
								ENC_KEY.temp.selecttrack = RxdBuf[4];	//��ȡ���õļ��ܷ�ʽ								                								
							
								send_frame(CMDSUCC);
							}
							else
							{
								send_frame(CMDERRO);
							}

                	    break;
						
         case SETTRACKSEPARATOR:   /*���ù���ָ��*/ // 17

							if(0x7F >= RxdBuf[4] && RxdBuf[3] == 0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)		 //ƥ������Ĳ���
							{

								ENC_KEY.temp.setseparator = RxdBuf[4];	//д��ָ��	
							
								send_frame(CMDSUCC);
							}
							else
							{
								send_frame(CMDERRO);
							}
         	    
            	    break;
           case SETTRACK2:   /*���ù��2�������͸�ʽ*/

							if((0x30 <= RxdBuf[4] && 0x39 >= RxdBuf[4])&&RxdBuf[3] == 0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)		 //ƥ������Ĳ���
							{
								ENC_KEY.temp.settrack2format = RxdBuf[4];	//��ȡ���õļ��ܷ�ʽ							
								
								send_frame(CMDSUCC);
							}
							else
							{
								send_frame(CMDERRO);
							}

          	    
            	        break;
					case SETDECODEWAY:						//0x1D	/* set decoding in both directions */	
					    //���ﻹδ��ɣ�������
					  if(RxdBuf[3] == 0x01 && (RxdBuf[4]>=0x30 && RxdBuf[4]<=0x34) && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
						{
					    ENC_KEY.temp.decoded_directions=RxdBuf[4];
						  send_frame(CMDSUCC);
						}
						else
						{

							send_frame(CMDERRO);
						}

                		break;
					case SETENCMODE:						//0x58	/* set encryption mode */							

							if((0x30==RxdBuf[4]|| 0x31==RxdBuf[4])&&RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)//30 fix����  31=DUKPT���� 
							{ 
								//fix����   ����Ҫ�����ⲿ��֤OK  �ſ���  ������DUKPT
                if(FIX_Authenticate_Value==0x31 && RxdBuf[4]==0x30){								
								  ENC_KEY.temp.encmode = RxdBuf[4];	//��ȡ���õļ��ܷ�ʽ
									send_frame(CMDSUCC);
								}
								
								else if( 0x31==RxdBuf[4]){
									ENC_KEY.temp.encmode = RxdBuf[4];
									send_frame(CMDSUCC);
								}

								else send_frame(CMDERRO);								
							}
							else send_frame(CMDERRO);
								

                		break;
					case SENDAUTHDATA:						//0x74	/* send authentication data  �ο�3.9.2С��*/
            if(	RxdBuf[3]==0x08 &&RxdBuf[1+RxdBuf[3]]==ETX && RxdBuf[RxdBuf[3]+5]== BQLRC ){							
							if(memcmp(FixKey_AuthenBuff,&RxdBuf[4],RxdBuf[3]) == 0)
							{
//								ENC_KEY.temp.FixKey_AuthenFlag = 0x31;				//��������OK
								FIX_Authenticate_Value=0x31;

								send_frame(CMDSUCC);	
							}
							else
							{
//								ENC_KEY.temp.FixKey_AuthenFlag = 0x30;				//��������ʧ��
								FIX_Authenticate_Value=0x30;

								send_frame(CMDERRO);	
							}
						}
						else send_frame(CMDERRO);
						
                		break;
						
					case SETENCWAY:							//0x4C	/* set encryption way */
                        
							
							if( (0x31 == RxdBuf[4] || 0x32 == RxdBuf[4])&& RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)// 31=3des 32=aes
							{
								if(ENC_KEY.temp.level<0x34){
									ENC_KEY.temp.level=0x33;
								}

								ENC_KEY.temp.encway = RxdBuf[4];//��ȡ���õļ�������   
								ENC_KEY.temp.status = 1;	//ʹ�ܼ���
								send_frame(CMDSUCC);
							}
							else if(0x30 == RxdBuf[4])
							{
								ENC_KEY.temp.encway = RxdBuf[4];//��ȡ���õļ�������   
								ENC_KEY.temp.status = 0;	//ʹ�ܼ���
								send_frame(CMDSUCC);
							}
							else send_frame(CMDERRO);					
                		break;
						
						
					case SETENCRYPTIONOPTIONSET:					//0x84	/* Encryption Option Setting*/

							if(RxdBuf[4] <= 0x08 && RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)	
							{  						
							  ENC_KEY.temp.Enhancedoption=RxdBuf[4];
						    send_frame(CMDSUCC);
							}
							else send_frame(CMDERRO);
              break;
					case SETENCRYPTIONFORMAT:					 //0x85	/* Encryption Output Format Setting */

							if(RxdBuf[3] == 0x01&&( 0x31 == RxdBuf[4] || 0x30 == RxdBuf[4])&& RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
								ENC_KEY.temp.Enhancedstatue=RxdBuf[4];
								send_frame(CMDSUCC);
							}
							else send_frame(CMDERRO);

              break;

//					case SETDEVICEKEY:						//0x76	/* set device key command */
//						if(RxdBuf[3]==0x10 && 0x31 == macstate)
//						{
//							memcpy(ENC_KEY.temp.fix,&RxdBuf[4],RxdBuf[3]);
//							ENC_KEY.temp.level = 0x33;							
//							send_frame(CMDSUCC);
//						}
//						else send_frame(CMDERRO);
//								
//            break;
					case SETSID:							//0x54	/* set Session ID */
//						printf("RxdBuf[RxdBuf[3]+4]=%x\r\n",RxdBuf[RxdBuf[3]+4]);
//					     printf("5555RxdBuf[RxdBuf[3]+5]=%x\r\n",RxdBuf[RxdBuf[3]+5]);
						if(RxdBuf[3]==0x08 && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
						{
							memcpy(ENC_KEY.temp.sid,&RxdBuf[4],8);
							send_frame(CMDSUCC);
						}
						else send_frame(CMDERRO);

            break;
					case REVIEWSN:							//0x4E	/* review serial number */
//						   printf("RxdBuf[RxdBuf[3]+4]=%x\r\n",RxdBuf[RxdBuf[3]+4]);
//					     printf("5555RxdBuf[RxdBuf[3]+5]=%x\r\n",RxdBuf[RxdBuf[3]+5]);
							if(ENC_KEY.temp.writesnflag == 0x30 && RxdBuf[3]==0x0A && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
							{
								memcpy(ENC_KEY.temp.sn,&RxdBuf[4],10);
								memcpy(ENC_KEY.temp.ksn,&RxdBuf[4],7);
								
								ENC_KEY.temp.ksn[7]=0x10;
								ENC_KEY.temp.ksn[8]=0x00;
								ENC_KEY.temp.ksn[9]=0x00;

								ENC_KEY.temp.writesnflag = 0x31;
						
								send_frame(CMDSUCC);

							}
							else
							{
							  send_frame(CMDERRO);  //ֻ��дһ��  printf("&& RxdBuf[RxdBuf[3]+4]== ETX=%d\r\n",RxdBuf[RxdBuf[3]+4]);
							}
               break;
					case MSRSETTINGS:						//0x1A	/* MSR Reading Settings */
					
						 if(RxdBuf[3]==0x01 &&(RxdBuf[4]==0x30 ||RxdBuf[4]==0x31) && RxdBuf[5]==ETX && RxdBuf[6]==BQLRC) //add 20180205 level=0 dissable dev
							{							    
								ENC_KEY.temp.enabledevice = RxdBuf[4];						
								send_frame(CMDSUCC);    							
							}
							else
							{		
    						send_frame(CMDERRO);
							}

		            break;
						
        case SETTERMINATOR: /*���ý�����*/

							if(RxdBuf[4]<=0x7F && RxdBuf[3]==0x01 && RxdBuf[5]==ETX && RxdBuf[6]==BQLRC)
							{
								ENC_KEY.temp.Terminator = RxdBuf[4];

								send_frame(CMDSUCC);
							}
								else send_frame(CMDERRO);

              break;
				case SETPREAMBLE:   //d2  <STX><S><n><Len><Prefix><ETX><CheckSum>
						if(RxdBuf[3] <= 0x0F && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)  //�ж� ���Ⱥ��������  �Ƿ����ETX  �����Ƶ��ж϶��ǰ��������ĸ�ʽ
						{             						
							memset(ENC_KEY.temp.Preamble,0x00,15);
							memcpy(ENC_KEY.temp.Preamble,&RxdBuf[4],RxdBuf[3]);
							ENC_KEY.temp.Preamblelen_m=RxdBuf[3];
//					    DBG_H("ENC_KEY.temp.Preamble",ENC_KEY.temp.Preamble,15);
							send_frame(CMDSUCC);   
						}
						else
						{
							send_frame(CMDERRO);
						}
					    break;					
				case SETPOSTAMBLE: //d3
					
						if(RxdBuf[3] <= 0x0F && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
						{								
							memset(ENC_KEY.temp.Postamble,0x00,15);
														
							memcpy(ENC_KEY.temp.Postamble,&RxdBuf[4],RxdBuf[3]);
							ENC_KEY.temp.Postamblelen_m=RxdBuf[3];
						
							send_frame(CMDSUCC);   
						}
						else
						{
							send_frame(CMDERRO);
						}               
						break;
				case SETTRACK1PREFIX: //34
						if(RxdBuf[3] <= 0x06 && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
						{	
							memset(ENC_KEY.temp.track1prefix,0x00,6);
              ENC_KEY.temp.tp1len=RxdBuf[3];
							memcpy(ENC_KEY.temp.track1prefix,&RxdBuf[4],RxdBuf[3]);

							send_frame(CMDSUCC);   
						}
						else
						{
							send_frame(CMDERRO);
						}                 
						break;
				case SETTRACK2PREFIX:
						if(RxdBuf[3] <= 0x06 && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
						{	
							memset(ENC_KEY.temp.track2prefix,0x00,6);

							ENC_KEY.temp.tp2len=RxdBuf[3];
							memcpy(ENC_KEY.temp.track2prefix,&RxdBuf[4],RxdBuf[3]);

							send_frame(CMDSUCC);   
						}
						else
						{
							send_frame(CMDERRO);
						}

		            break;
        case SETTRACK3PREFIX:
            if(RxdBuf[3] <= 0x06 && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
						{	
							memset(ENC_KEY.temp.track3prefix,0x00,6);
							ENC_KEY.temp.tp3len=RxdBuf[3];
							memcpy(ENC_KEY.temp.track3prefix,&RxdBuf[4],RxdBuf[3]);
					    send_frame(CMDSUCC);   
						}
						else
						{
							send_frame(CMDERRO);
						}
						break;
				case SETTRACK1SUFFIX:
						if(RxdBuf[3] <= 0x06 && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
						{	
							memset(ENC_KEY.temp.track1suffix,0x00,6);
							memcpy(ENC_KEY.temp.track1suffix,&RxdBuf[4],RxdBuf[3]);
							ENC_KEY.temp.ts1len=RxdBuf[3];

							send_frame(CMDSUCC);   
						}
						else
						{
							send_frame(CMDERRO);
						}                   
						break;
				case SETTRACK2SUFFIX:
						if(RxdBuf[3] <= 0x06 && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
						{	
							ENC_KEY.temp.ts2len=RxdBuf[3];
							memset(ENC_KEY.temp.track2suffix,0x00,6);
							memcpy(ENC_KEY.temp.track2suffix,&RxdBuf[4],RxdBuf[3]);
				 		
							send_frame(CMDSUCC);   
						}
						else
						{
							send_frame(CMDERRO);
						}

						break;
            case SETTRACK3SUFFIX:
            if(RxdBuf[3] <= 0x06 && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)
						{					
              ENC_KEY.temp.ts3len=RxdBuf[3];							
					    memset(ENC_KEY.temp.track3suffix,0x00,6);
							memcpy(ENC_KEY.temp.track3suffix,&RxdBuf[4],RxdBuf[3]);

							send_frame(CMDSUCC);   
						}
						else
						{
							send_frame(CMDERRO);
						}

						break;

						
		            
	    	        default:
									 send_frame(CMDERRO);
	        	       init_serial_boot();
	            	break;
	            }
				init_serial_boot();
				WriteFlag = 1;
				WriteENCKEY();					//������º��KEY

			}
			else
			{
				init_serial_boot();
				send_frame(CMDERRO);
			}
		}
		else
		{
			init_serial_boot();
			send_frame(CMDERRO);
		}
	}
	
}


