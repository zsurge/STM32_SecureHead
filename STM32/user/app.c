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
union __ENC_KEY ENC_KEY;					//密钥保存数据结构
uint8_t CryptoKey[16];						//DUKPT计算出的过程密钥
								  	
uint8_t /*random[8],*/LOADKEY_AuthenticateRadomBuf[8]={0},LOADKEY_Authenticate_flag=0xAA;							//随机数
//uint8_t macdata[8];							//鉴别数据
uint8_t FixKey_AuthenBuff[8];//External Authenticate Command (Fixed Key Only)  储存验证FIX不加密的8bit随机数
const char SHID[] = "PF SecureHeadReader V 1.1.2";
//#ifdef FLASHKEY_SUPPORT
//const char APPVERSION[] = "PAYFULL-M301";
//#else
const char APPVERSION[] = "PAYFULL-M301";
//#endif

volatile uint8_t WriteFlag = 0,USB_Updataflag=0xAD;				//保存密钥标志：0：不保存，1：使能保存
volatile uint8_t GetNextFlag = 0;			//获取下一个KSN标志：0：不获取下一个，1：使能获取下一个 

volatile uint8_t macstate = 0x00,FIX_Authenticate_Value=0x30;			//鉴定状态：0x30：未接收到鉴定数据或鉴定数据失败；0x31：鉴定数据OK
//volatile uint8_t encryptdatastatus = 0x00;	//bit 1 ==1 表示1磁道加密数据存在
											//bit 2 ==1 表示2磁道加密数据存在
											//bit 3 ==1 表示3磁道加密数据存在
											//bit 4 ==1 表示1磁道哈希数据存在
											//bit 5 ==1 表示2磁道哈希数据存在
											//bit 6 ==1 表示3磁道哈希数据存在
											//bit 7 ==1 表示会话ID数据存在
											//bit 8 ==1 表示KSN存在

volatile uint8_t RxdStatus = 0;			 		//接收状态
volatile uint8_t SynHead = 0;				 	//接收包头个数
volatile uint8_t RxdFrameStatus = 0;			//接收包状态
volatile uint8_t NowLen = 0;				 	//接收字节指针
volatile uint8_t RxdTotalLen = 0;			 	//接收包数据长度

#define RxdBuf ((uint8_t *)TempTrackDirtData.Value)			//接收包数据缓存
//volatile uint8_t RxdBuf[100]={0};
volatile uint8_t Bcc;						//校验和

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
	
	ENC_KEY.temp.zhanwei = 0xAA; //启用、禁止标志位
	ENC_KEY.temp.beepmode = 0x31; //启用、禁止标志位
	ENC_KEY.temp.enabledevice = 0x31; //启用、禁止标志位
	
	if(ENC_KEY.temp.writesnflag == 0x30){
	  ENC_KEY.temp.writesnflag = 0x30;  //写sn标志位
		memcpy(ENC_KEY.temp.ksn,"\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00",10);//  ksn值存放
		
	}
	
//	DBG_H("ENC_KEY.temp.ksn",ENC_KEY.temp.ksn,10);
//	  printf("ENC_KEY.temp.writesnflagddes %x\r\n",ENC_KEY.temp.writesnflag); 
	
	ENC_KEY.temp.Terminator = 0x0D;     //磁道结束符  ---对照文档
	
	ENC_KEY.temp.selecttrack = 0x30;    //选择磁道参数
	
	ENC_KEY.temp.setseparator = 0x0D;   //写入数据分割符	

	ENC_KEY.temp.settrack2format = 0x31;//轨道2数据管理
	ENC_KEY.temp.status = 0;			//失能加密
	ENC_KEY.temp.level = 0x31;//加密等级为0x31
	
	ENC_KEY.temp.encmode = 0x31;			//默认DUKPT加密
	ENC_KEY.temp.encway = 0x31;			//默认DES加密
	memcpy(ENC_KEY.temp.fix,"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\xaa\xab\xac\xad\xae\xaf",16);// fix key密匙存放值
	memcpy(ENC_KEY.temp.key,"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\xaa\xbb\xcc\xdd\xee\xff",16);//  key密匙存放值
//	IDT_UpdateKSN(10,ENC_KEY.temp.ksn);
	
	memcpy(ENC_KEY.temp.sid,"\x01\x02\x03\x04\x05\x06\x07\x08",8);//  sid值存放
//	memcpy(ENC_KEY.temp.sn,"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x00",10);//  sn值存放
	
	memset(ENC_KEY.temp.Preamble,0x00,15 );     /*sizeof(ENC_KEY.temp.Preamble)*///磁头起始符  15bit
	memset(ENC_KEY.temp.Postamble,0x00,15 );//15bit   /*sizeof(ENC_KEY.temp.Postamble)*/
	
	memset(ENC_KEY.temp.track1prefix,0x00,6 );//磁头前缀 6bit /*sizeof(ENC_KEY.temp.track1prefix)*/
	memset(ENC_KEY.temp.track2prefix,0x00,6 );//6bit /*sizeof(ENC_KEY.temp.track2prefix)*/
	memset(ENC_KEY.temp.track3prefix,0x00,6 );//6bit /*sizeof(ENC_KEY.temp.track3prefix)*/
	
	memset(ENC_KEY.temp.track1suffix,0x00,6 );//磁头后缀  6bit /*sizeof(ENC_KEY.temp.track1suffix)*/
	memset(ENC_KEY.temp.track2suffix,0x00,6 );//6bit /*sizeof(ENC_KEY.temp.track2suffix)*/
	memset(ENC_KEY.temp.track3suffix,0x00,6 );//6bit /*sizeof(ENC_KEY.temp.track3suffix)*/
	
	ENC_KEY.temp.Enhancedstatue=0x30;//强加密和普通加密状态
	
	ENC_KEY.temp.Enhancedoption=0x08;//
	
	ENC_KEY.temp.HASHSET=0x37;//哈希值的发送状态
	ENC_KEY.temp.MaskSetting=0x07;//掩码轨道发送

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
	*** 函 数 名:	void WriteENCKEY(void)
	*** 功能描述：	讲ENCKEY保存到FLASH中
	*** 参    数:  	输入: 全局变量ENC_KEY
					输入: ENC_KEY保存地址
				    输出：NULL 
	*** 返 回 值:	NULL  	 	
	*** 模块信息: 	Ouyangweiquan 2011.09.20创建
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
// 			tri_des(tempdata+i*8,(unsigned char *)SHID,(unsigned char *)SHID+8,0);	   //加密要存储的密钥值
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
	*** 函 数 名:	void ReadENCKEY(void)
	*** 功能描述：	读取保存的ENCKEY到RAM中
	*** 参    数:  	输入: ENC_KEY保存地址
				    输出：全局变量ENC_KEY 
	*** 返 回 值:	TRUE/FALSE   	 	
	*** 模块信息: 	Ouyangweiquan 2011.09.20创建
***************************************************************************************/
uint8_t ReadENCKEY(void)
{
//	#ifdef FLASHKEY_SUPPORT
	uint8_t tempdata[200],i=0,j=0,lrc=0,bcc=0;
	memset(tempdata ,0, 200);
	ReadBlockFlash(KEYADD ,tempdata, 200);     			//读出之前保存的密钥值

//	DBG_H("tempdata0024",tempdata,200);
	//判定值是XX 那就是第一次，执行设置默认值
	//值是ZZ，读取，然后赋值
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
	if(FIX_Authenticate_Value==0x30) ENC_KEY.temp.encmode=0x30;//查看3.9.2小节 每次启用fix加密都需要认证一次

	return TRUE;
	
//	#endif
	
	}

/**************************************************************************************
	*** 函 数 名:	uint8_t AsciiToHex(uint8_t * pAscii, uint8_t * pHex, int nLen)
	*** 功能描述：	将一个 ASSC 码表示的十六进制字符串转换成十六进制的字节串；
	*** 参    数:  	输入: pAscii -- 要转换的ASCII码表示的十六进制字符串的首地址；
						  nLen	 -- 要转换的ASCII码表示的十六进制字符串的长度（字节数）；
				    输出：pHex	 -- 转换后的十六进制数字节串首地址；
	*** 返 回 值:	TRUE/FALSE   	 	
	*** 模块信息: 	Ouyangweiquan 2011.09.20创建
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
	*** 函 数 名:	void HexToAscii(unsigned char * pHex, unsigned char * pASSCHex, int nLen)
	*** 功能描述：	将一个十六进制字节串转换成 ASCII 码表示的字符串；
	*** 参    数:  	输入: pHex 	   -- 要转换的十六进制数字节串首地址；
						  nLen	   -- 要转换的十六进制数字节串的长度（字节数）；
				    输出：pASSCHex -- 转换后的 ASCII 码表示的字符串的首地址；
	*** 返 回 值:	NULL   	 	
	*** 模块信息: 	Ouyangweiquan 2011.09.20创建
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
	*** 函 数 名:	void Random(void) 
	*** 功能描述：	取8个字节的16进制的随机数；
	*** 参    数:  	输入: NULL；
				    输出：8个十六进制随机数；
	*** 返 回 值:	NULL   	 	
	*** 模块信息: 	Ouyangweiquan 2011.09.20创建
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
	*** 函 数 名:	uint8_t GetNextKSN(uint8_t* pBUserKSN) 
	*** 功能描述：	使DUKPT算法因子KSN自加1；
	*** 参    数:  	输入: KSN；
				    输出：下一个KSN；
	*** 返 回 值:	FALSE / TRUE   	 	
	*** 模块信息: 	Ouyangweiquan 2011.11.18创建
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
	WriteENCKEY();							  //保存更新后的KSN

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
//	 memcpy(ksnbuff,ENC_KEY.temp.ksn,8);//KSN是已经赋值了的
//	 memcpy(keybuff,ENC_KEY.temp.key,16);//BDK是原始密钥
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
	*** 函 数 名:	void CalcCryptoKey(void) 
	*** 功能描述：	根据当前更新后的KSN和KEY计算出过程密钥；
	*** 参    数:  	输入: KSN和KEY；
				    输出：过程密钥；
	*** 返 回 值:	NULL   	 	
	*** 模块信息: 	Ouyangweiquan 2011.11.18创建
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


异或函数


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
*函数名称:CheckSum8
*输   入:buf 要校验的数据 
        len 校验数据的长
*输   出:校验和
*功   能:校验和-8
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
	*** 函 数 名:	void send_frame(uint8_t frame_type)
	*** 功能描述：	返回对命令的响应；
	*** 参    数:  	输入: frame_type -- 响应命令的参数；
	*** 返 回 值:	NULL   	 	
	*** 模块信息: 	Ouyangweiquan 2011.09.20创建
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
		  case SETENCRYPTIONFORMAT: //响应强加密/普通加密
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
		  case ACTIVATEAUTHEN://加密等级4格式响应 10bit KSN+ 20bits随机数
				Dukpt_KeyManagement();
			                                                  //当前dukpt密钥异或上 16bits的0XF0
			  GetXORKey("\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0",CryptoKey,ivec,16);
			  
			  memcpy(ENC_KEY.temp.Authenticationkey_Buff,ivec,16);//保存认证1的加密密钥
				TxdBuf[i++]=0x06;
			  TxdBuf[i++]=0x02;
			
			  memcpy(TxdBuf+i,ENC_KEY.temp.ksn,10);
			  i+=10;
			
			  memset(TempBuf,0x00,sizeof(TempBuf));			  
			  for(j=0;j<6;j++)  //产生challenge 1
			    TempBuf[j]=rand()%127;
			  
			  TempBuf[j++]=ENC_KEY.temp.ksn[8];
			  TempBuf[j++]=ENC_KEY.temp.ksn[9];			
					
				tri_des(TempBuf,ivec,ivec+8,0);//加密challenge 1 随机数
				memcpy(TxdBuf+i,TempBuf,8);
				i+=8;		

				memcpy(ENC_KEY.temp.Challenge1_Buff,TempBuf,8);//保存8bits加密随机数	
        
				memset(TempBuf,0x00,sizeof(TempBuf));
        for(j=0;j<8;j++)TempBuf[j]=rand()%127;// 产生challenge 2  8bits
			    		  
			  memcpy(ENC_KEY.temp.Challenge2_Buff,TempBuf,8); //ENC_KEY.temp.Challenge2_Buff 用于退出加密等级4的时候
									
				tri_des(TempBuf,ivec,ivec+8,0);//加密challenge 2 随机数
				memcpy(TxdBuf+i,TempBuf,8);
				memcpy(ENC_KEY.temp.Challenge2_Buff,TempBuf,8);//保存8bits加密随机数	
				i+=8;	
					
//			  DBG_H("TxdBuf",TxdBuf,i);	
//				TxdBuf[i++]=ETX;

				ENC_KEY.temp.Activa_Challenge_Reply=0x35;//加密等级4的时候 第一次认证
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
			case SETENCWAY://读取加密状态  aes或3des

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
			
//		case LOADKEYAUTHENTICATE:///  验证导入密钥检测  验证通过  则允许导入密钥    

//			if(ENC_KEY.temp.KeyAuthenticateFlag==0x31)
//			{
//				TxdBuf[i++]=0x01; TxdBuf[i++]=0x00; TxdBuf[i++]=0x02; TxdBuf[i++]=0x01; TxdBuf[i++]=0x00;
//			}
//			else { TxdBuf[i++]=0x01; TxdBuf[i++]=0x00; TxdBuf[i++]=0x02; TxdBuf[i++]=0x01; TxdBuf[i++]=0x01;  }
//		  bcc=0xAA;
//			break;
			
//		case SETKEYVALUE://产生8Bit 随机数给上位机,上位机进行解密这8bit随机数。若正确，则允许用于密钥导入  IDTECH上导入 暂时屏蔽

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

//		if (0x31 == ENC_KEY.temp.encmode)				   		//DUKPT加密
//		{

//			Dukpt_KeyManagement(); //计算DUKPT过程密钥  ENC_KEY.temp.key
//			
//		}
//		else if (0x30 == ENC_KEY.temp.encmode)			 		//FIX加密
//		{
//			memcpy(CryptoKey,ENC_KEY.temp.fix,16);				//拷贝FIX密钥
//		}

//		if(0x31 == ENC_KEY.temp.encway)					//three DES加密 
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

		  TxdBuf[i++]=0x06;//   其实帧包头
		  TxdBuf[i++]=0x02;
		  TxdBuf[i++]=0x83;
		  TxdBuf[i++]=0x02;
		  TxdBuf[i++]=0x00;
		  TxdBuf[i++]=0x00;
		
//		  TxdBuf[i++]=ETX;
//		  TxdBuf[i++]=0x80;
		  
		break;
		
		case REVIEWSET:

		TxdBuf[i++]=0x06;//   其实帧包头
		TxdBuf[i++]=0x02;
//		
    TxdBuf[i++]=GETSECURITYLEVEL;//加密等级
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
		
		TxdBuf[i++]=TRANSMODE;//传输模式
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Tranmodeselect;//传输模式

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
		TxdBuf[i++]=ENC_KEY.temp.Enhancedstatue;//强制加密 或 普通加密
		
		
		TxdBuf[i++]=SETBEEP;//				0x11	/* beep set*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.beepmode;//蜂鸣器模式
		
//		TxdBuf[i++]=0x12;//				0x12	/* 字符传输间隔时间*/
//		TxdBuf[i++]=0x01;// 41
//		TxdBuf[i++]=0x30;//
		
		TxdBuf[i++]=SETSELECTTRACK;//				/* 轨道选择  0 三轨道 1-轨道1 2-轨道2 */
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
		
		TxdBuf[i++]=SETTRACKSEPARATOR;//				0x17	/*  分隔符设定*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.setseparator;//
		
		TxdBuf[i++]=SETTRACK2;//				0x19	/*设置轨道2数据上送格式*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.settrack2format;//
		
		TxdBuf[i++]=MSRSETTINGS;//				0x1A	/*刷卡器  enable or  disable*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.enabledevice;//
		
//		TxdBuf[i++]=0x1B;//				0x1B	/*  Data Editing Control */
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
		
		TxdBuf[i++]=SETDECODEWAY;//				0x1D	/*  Decoding in bothdirection; ‘0’ Raw data ‘2’forward ‘3’ reverse */
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
//		TxdBuf[i++]=0x41;//				0x41	/*  9600 bps, ‘2’ is 1200, ‘7’is 38,400 bps; ‘9’ is 115.2kbps* */
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
		
		TxdBuf[i++]=DispExpDateID;//				0x50	/* ‘1’ don’t mask expirationdate */
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.DispDataflag;//
		
//		TxdBuf[i++]=0x55;//				0x50	/* don’t include mod10, ‘1’display mod10, ‘2’ displaywrong mod10*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x60;//				0x60	/* Without LRC in output*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
//		
//		TxdBuf[i++]=0x61;//				0x61	/* ‘%’ as Track 1 7 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x62;//				0x62	/* ‘%’ as Track 1 6 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x63;//				0x63	/* ‘;’ as Track 1 5 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3B;//
//		
//		TxdBuf[i++]=0x64;//				0x64	/* ‘%’ as Track 2 7 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x65;//				0x65	/* ‘%’ as Track 2 7 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3B;//  131
//		
//		TxdBuf[i++]=0x66;//				0x66	/* ‘%’ as Track 3 7 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x67;//				0x67	/* ‘!’ as Track 3 6 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x21;//
//		
//		TxdBuf[i++]=0x68;//				0x68	/* ‘;’ as Track 3 5 Bit StartSentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3B;//
//		
//		TxdBuf[i++]=0x69;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3F;//
//		
//		TxdBuf[i++]=0x6A;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3F;//
//		
//		TxdBuf[i++]=0x6B;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3F;//
//		
//		TxdBuf[i++]=0x6C;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x25;//
//		
//		TxdBuf[i++]=0x6D;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x3B;//
//		
//		TxdBuf[i++]=0x6E;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x2B;//
//		
//		TxdBuf[i++]=0x6F;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x31;//   161
//		
//		TxdBuf[i++]=0x72;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x00;//
//		
//		TxdBuf[i++]=0x73;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x00;//
		
//		TxdBuf[i++]=0x7A;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=ENC_KEY.temp.beepmode;//
		
//		TxdBuf[i++]=0x7B;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x30;//
		
//		TxdBuf[i++]=0x7A;//				0x69	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=ENC_KEY.temp.beepmode;//
		
		
		//OK
		TxdBuf[i++]=SETPREAMBLE;//				0xd2	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Preamblelen_m;//
		
		TxdBuf[i++]=SETPREAMBLE;//				0xd2	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Preamblelen_m;//
		
		TxdBuf[i++]=SETPOSTAMBLE;//				0xd3	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.Postamblelen_m;//
		
		TxdBuf[i++]=SETTRACK1PREFIX;//				0x34	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.tp1len;//
		
		TxdBuf[i++]=SETTRACK1SUFFIX;//				0x37	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.ts1len;//  191
		
		TxdBuf[i++]=SETTRACK2PREFIX;//				0x35	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.tp2len;//
		
		TxdBuf[i++]=SETTRACK2SUFFIX;//				0x38	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.ts2len;//
		
		TxdBuf[i++]=SETTRACK3PREFIX;//				0x36	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.tp3len;//
		
		TxdBuf[i++]=SETTRACK3SUFFIX;//				0x39	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.ts3len;//
		
		TxdBuf[i++]=HASHOPTION;//				0x5C	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.HASHSET;//
		
		TxdBuf[i++]=MASKOPTION;//				0x86	/* ‘?’ as End Sentinel*/
		TxdBuf[i++]=0x01;
		TxdBuf[i++]=ENC_KEY.temp.MaskSetting;//
		
//		TxdBuf[i++]=0x89;//				0x89	/* ‘?’ as End Sentinel*/
//		TxdBuf[i++]=0x01;
//		TxdBuf[i++]=0x34;//
		
		TxdBuf[i++]=SETENCRYPTIONOPTIONSET;//				0x86	/* ‘?’ as End Sentinel*/
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
		
		
		case ENCRYPTEXTERNAL://外部认证模式  3.12小节 此小节不是很清楚IDTECH很实际的应用

		  TxdBuf[i++]=0x06;
		  TxdBuf[i++]=0x02;
		  j=RxdBuf[2]|RxdBuf[3]<<8;
//		  loop=RxdBuf[2]|RxdBuf[3]<<8;//计算长度
		  memcpy(TxdBuf+i,RxdBuf+4,j);
		  
		  
		if (0x31 == ENC_KEY.temp.encmode)				   		//DUKPT加密
		{			
			Dukpt_KeyManagement(); //计算DUKPT过程密钥  ENC_KEY.temp.key			
		}
		else if (0x30 == ENC_KEY.temp.encmode)			 		//FIX加密
		{
			memcpy(CryptoKey,ENC_KEY.temp.fix,16);				//拷贝FIX密钥
		}
		
		if(0x31 == ENC_KEY.temp.encway)					//three DES加密 
		{
			SUM_Vaule = ceil((j-1)/8.0);

			tri_des(TxdBuf+i,CryptoKey,CryptoKey+8,0);
			for(j = 1;j< SUM_Vaule;j++)							//加密第一磁道数据
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
		
		else if(0x32 == ENC_KEY.temp.encway)					//AES加密
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
		
		if (0x31 == ENC_KEY.temp.encmode)				   		//DUKPT加密
		{			
			memcpy(TxdBuf+i,ENC_KEY.temp.ksn,10);
			i+=10;
		}
		else if (0x30 == ENC_KEY.temp.encmode)			 		//FIX加密
		{
			memcpy(TxdBuf+i,ENC_KEY.temp.sn,10);
			i+=10;
			TxdBuf[i++]=0x00;
			TxdBuf[i++]=0x00;
		}
//		TxdBuf[i++]=ETX;

       break;
//    case LOADKEY://密钥导入成功指令返回  该指令是IDTECH指令

//		  TxdBuf[i++]=0x01;
//		  TxdBuf[i++]=0x00;
//		  TxdBuf[i++]=0x02;
//		  TxdBuf[i++]=0x01;
//		  TxdBuf[i++]=0x00;	
//		
//		break;
    case REQUEST_DOWN:   				/* request download */
						    //请求下载
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
		case CMDERRO:					   //加密磁头协议
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
		case GETCHALLENGE://用于fix加密认证  认证通过才可以进行fix加密
				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = 0x02; 		// 02
				
//			  产生8bit随机数
				for(j=0;j<8;j++)
				{
					FixKey_AuthenBuff[j]=rand()%127;
				}
				  memcpy(TempBuf,FixKey_AuthenBuff,8);

				  Dukpt_KeyManagement();

				  tri_des(TempBuf,CryptoKey,CryptoKey+8,0);///加密随机数
				  memcpy(TxdBuf+i,TempBuf,8);

				i+=8;
//				TxdBuf[i++] = ETX;
	
			break;			
		case READID:

				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = RxdBuf[0]; 		// 02

				for(j=0; j<strlen(SHID); j++)  //拷贝安全磁头ID、计算异或和
				{
					TxdBuf[i++] = SHID[j];

				}
//				TxdBuf[i++] = ETX;

			break;
		case REVIEWKSN://获取设备序列号KSN

				TxdBuf[i++] = 0x06; 			// 06
				TxdBuf[i++] = RxdBuf[0]; 		// 02

				TxdBuf[i++] = RxdBuf[2];		// 51

				TxdBuf[i++] = 0x0B;				// 0B

				TxdBuf[i++] = 0x0A;				// 0A

				
				for(j=0; j< 10; j++)  			//拷贝KSN、计算异或和
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
				
				for(j=0; j< 10; j++)  			//拷贝SN、计算异或和
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
				
				for(j=0; j< 8; j++)  			//拷贝SID、计算异或和
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
	  memset(IDTUSB_SendBuff,0x00,128);//清楚发送buff
		sendcount=0;
	  memcpy(IDTUSB_SendBuff,TxdBuf,i);
}


/**************************************************************************************
	*** 函 数 名:	void DealSerialParse(void)
	*** 功能描述：	将收到的数据解析成数据包；
	*** 参    数:  	NULL
	*** 返 回 值:	NULL   	 	
	*** 模块信息: 	Ouyangweiquan 2011.09.20创建
***************************************************************************************/
void DealSerialParse(void)
{
	uint8_t i = 0;
	while(1)
	{	
		if(UsbRecvOne(&i) != 1)  //读取USB数据
		{
		    return;
		}	
		
		switch (RxdStatus)
		{ /*接收数据状态*/
				
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
				break;				/*接收包头*/
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
	*** 函 数 名:	void DealRxData(void)
	*** 功能描述：	解析收到的完整数据包，并响应数据包；
	*** 参    数:  	NULL
	*** 返 回 值:	NULL   	 	
	*** 模块信息: 	Ouyangweiquan 2011.09.20创建
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
		
		BQLRC=RxdBuf[RxdTotalLen-1]; //		RxdTotalLen=接收长度  BQLRC=整个CMD的LRC 
    
//	printf("BQLRC=%d\r\n",BQLRC);
		if(RxdBuf[0] == STX)						//解析02数据包
		{

//			usb_reviceflag=0x1B;  //有用处  用于响应无效指令
	/*		if(0x01 == RxdBuf[1])	   //更改为UIC  load key的方式  暂时把IDTECH的load key 屏蔽
			{
				if(0x03 == RxdBuf[3])	  //产生密钥中获取8bit随机数 用于验证密钥导入  
				{
//					printf("009\r\n");
					send_frame(SETKEYVALUE);
					init_serial_boot();						
					
				}
				else if(0x06 == RxdBuf[3])	 //  用于导入密钥认证  验证通过则允许导入密钥
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
							firstflag = 0X55; //标志位为0X55 
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
								
									memset(ENC_KEY.temp.key,0x00,sizeof(ENC_KEY.temp.key));//清理key数组                
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
	 if (0x5A != RxdBuf[1] && RxdBuf[2]!=0x5A && RxdBuf[3]!=0xA5 && RxdBuf[4]!=0xA5 )	// 当中间有其他操作 则需要重新操作3次进入IAP0x55	/* request download */
		{
			USB_Updataflag=0xAD;
		}
		//UIC的load key 
		if(0x30 == RxdBuf[1] && 0x30 == RxdBuf[2] && 0x30 == RxdBuf[3] && 0x30 == RxdBuf[4]  )//缓存KSN 20Bits  接收之后再合并成10bits
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

				
				if(CalcXOR(&RxdBuf[1],loop,0)==RxdBuf[loop+1]) //判断etx后面是否等于 lrc
				{

				if(loop-5==52)
				{
					memcpy(EnyBuff,RxdBuf+5,20);
					
					for(i=0;i<20;i++)//比较KSN的ASCII  20bits
					{
						if(EnyBuff[i] >=0x30 && EnyBuff[i] <=0x7A)
						{//删除"; %"*/
						  DeleteChars(EnyBuff,":;<=>?@[]^_`");
						}
					}
					if(memcmp(EnyBuff,RxdBuf+5,20)==0)
						macstate=0xF1;
					
					else macstate=0xFC;
					
//					DBG_H("EnyBuff",EnyBuff,20);
					memcpy(EnyBuff,RxdBuf+25,32);
					
					for(i=0;i<32;i++)//比较KEY的ASCII   32bits
					{
						if(EnyBuff[i] >=0x30 && EnyBuff[i] <=0x7A)
						{
						  DeleteChars(EnyBuff,":;<=>?@[]^_`"); //删除"; %"
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
				//三次相同指令进入IAP模式
			else if (0x5A == RxdBuf[1] && RxdBuf[2]==0x5A && RxdBuf[3]==0xA5 && RxdBuf[4]==0xA5 )							//0x55	/* request download */
			{												//重启
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
			else if(ENCRYPTEXTERNAL == RxdBuf[1])//  && ETX==RxdBuf[5] 0x41 3.12小节 该命令只有在level 3 4状态下有效  加密外部数据  暂未发现有什么功能
			{
				if(ENC_KEY.temp.level==0x33 ||ENC_KEY.temp.level==0x34)
				{
					loop=RxdBuf[2]|RxdBuf[3]<<8;//计算长度

					if(RxdBuf[loop+5]==CalcXOR_none(RxdBuf,loop+5,0))//计算ETX后面是不是LRC  如果相符  长度正确  否则返回错误
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
		
			else if (READCMD == RxdBuf[1])					//0x52	/* read status commands */  读取状态命令
			{
				switch (RxdBuf[2])
				{
					case SETENCMODE:
						send_frame(SETENCMODE);   
					          break;
					case SETDECODEWAY:                 //设置磁卡解码方向  正刷 反刷 正反刷  0x1d
						send_frame(SETDECODEWAY);
                		break;
					case SETENCWAY:                    //设置加密方式    ‘1’ 3DES ‘2’ AES  0x4c
						send_frame(SETENCWAY);
                		break;
						
					case GETREADER_STATUSCOMMAND:                     /*读卡器状态的读取  0x83 */
						send_frame(GETREADER_STATUSCOMMAND);
                		break;
					case REVIEWSET:                                  /*读卡器状态信息读取 0x1f*/
						send_frame(REVIEWSET);
                		break;
					case SETENCRYPTIONOPTIONSET:                                  /*读卡器状态信息 轨道加密 0x84*/
						send_frame(SETENCRYPTIONOPTIONSET);
                		break;
						
					case ACTIVATEAUTHEN:// Activate Authentication Mode Command   0x80 外部认证指令  在等级4进行数据传输的时候					
						  send_frame(ACTIVATEAUTHEN);										
					break;
					case READID:						    //0x22	/* read secureheadreader ID */
						send_frame(READID);
                		break;
					case GETCHALLENGE:						//0x74	/* get encrypt challenge */
//						Random();							//产生随机数
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
					case SETENCRYPTIONFORMAT:							//0x85	/* 查询普通加密还是强制加密 */
						send_frame(SETENCRYPTIONFORMAT);
					
                		break;					
	           		default:
    	           		init_serial_boot();
								    send_frame(CMDERRO);
        	    		break;
        		}
				RxdFrameStatus = SPACE;
			}
			
/*                设置读卡器指令集                                      */
/*                根据IDTECH开发文档设置指令集                                     */
			else if (SETTINGCMD == RxdBuf[1])				//0x53	/* setting commands */
			{
				switch (RxdBuf[2])
				{
					
					case DATAFMTID:// 因为跟IDTECH上位机通信有此功能  但实际上不了解该功能  
					
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
					
					case TRANSMODE://数据传输模式  23   HID或是KEYBOARD

							if((RxdBuf[4]==0x38  || RxdBuf[4]==0x30) && RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)/// keyboard
							{
								
							  if(RxdBuf[4] != ENC_KEY.temp.Tranmodeselect)//模式不相等  则改变
								{
									
                  ENC_KEY.temp.Tranmodeselect=RxdBuf[4];
								  delay_ms(1000);
									send_frame(CMDSUCC);
                  WriteFlag = 1;
									WriteENCKEY();					//保存更新后的KEY									
								  NVIC_SystemReset();
									
								}
								else send_frame(CMDSUCC);
			        
							}
							else send_frame(CMDERRO);
									
						break;
					
					case SETMASKCHAR://掩码字符设置  4B

							if((RxdBuf[4]>=0x20 && RxdBuf[4]<=0x7E)&& RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
							    ENC_KEY.temp.MaskChar=RxdBuf[4];

								  send_frame(CMDSUCC);																								
							}
							else send_frame(CMDERRO);

						break;
							
						
					case SETPrePANID://掩码起始字符设置  从第几位开始植入掩码
              
							if(RxdBuf[4]<=0x06&& RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
							    ENC_KEY.temp.PrePANID=RxdBuf[4];

								  send_frame(CMDSUCC);																								
							}
							else send_frame(CMDERRO);

						break;	
					
						case SETPostPANID://掩码结束字符设置  从第几位开始植入掩码

							if(RxdBuf[4]<=0x04 && RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)
							{
							    ENC_KEY.temp.PostPANID=RxdBuf[4];
									
								  send_frame(CMDSUCC);																								
							}
							else send_frame(CMDERRO);

						break;	
					
						
					case EXITACTIVATEAUTHEN://退出认证模式
						if((RxdBuf[3]==0x08)||(RxdBuf[3]==0x10))
						{
							for(i=0;i<50;i++){
								if(RxdBuf[i]==0x03)
									break;
							}

						if(CalcXOR(&RxdBuf[1],i,0)==RxdBuf[i+1]) //判断ETX后面是否等于 LRC
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
					
					case ACTIVATION_CHALLENGE_REPLY_COMMAND://数据二次认证  在等级4的时候	
						
						if((RxdBuf[3]==0x10) /*&&(ENC_KEY.temp.Activa_Challenge_Reply==0x35) */)							
						{													
							for(i=0;i<23;i++){
								if(RxdBuf[i]==0x03)
									break;
							}
							
							if(CalcXOR(&RxdBuf[1],i,0)==RxdBuf[i+1]) //判断ETX后面是否等于 LRC
							{
							  memcpy(TKMbuff,RxdBuf+4,8); //Challenge 1数据
//								
								Dukpt_KeyManagement();
								GetXORKey("\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C\x3C",CryptoKey,hbuff,16);

								tri_des(TKMbuff,hbuff,hbuff+8,1);

								if(memcmp(TKMbuff,ENC_KEY.temp.Challenge1_Buff,6)==0){
									 								 
									 memcpy(TKMbuff,RxdBuf+4+8,8);
									 tri_des(TKMbuff,hbuff,hbuff+8,1);
									 memcpy(ENC_KEY.temp.sid,TKMbuff,8);//改变SID

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
					
					case MASKOPTION: //MaskSetting  轨道数据掩码设置  86

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
							macstate = 0x00;					//未接收到鉴定数据
							Default_Settings();					
							send_frame(CMDSUCC);

            break;
					case SETBEEP:
							if((0x30 <= RxdBuf[4] && 0x34 >= RxdBuf[4]) && RxdBuf[3] == 0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)		 //匹配命令的参数
							{
								ENC_KEY.temp.beepmode = RxdBuf[4];	//读取设置的加密方式														
								send_frame(CMDSUCC);
							}
							else
							{
								send_frame(CMDERRO);
							}

                		break;
        case SETSELECTTRACK :  /*设置选择读的轨道*/

							if((0x30 <= RxdBuf[4] && 0x39 >= RxdBuf[4])&&RxdBuf[3] == 0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)		 //匹配命令的参数
							{
								ENC_KEY.temp.selecttrack = RxdBuf[4];	//读取设置的加密方式								                								
							
								send_frame(CMDSUCC);
							}
							else
							{
								send_frame(CMDERRO);
							}

                	    break;
						
         case SETTRACKSEPARATOR:   /*设置轨道分割符*/ // 17

							if(0x7F >= RxdBuf[4] && RxdBuf[3] == 0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)		 //匹配命令的参数
							{

								ENC_KEY.temp.setseparator = RxdBuf[4];	//写入分割符	
							
								send_frame(CMDSUCC);
							}
							else
							{
								send_frame(CMDERRO);
							}
         	    
            	    break;
           case SETTRACK2:   /*设置轨道2数据上送格式*/

							if((0x30 <= RxdBuf[4] && 0x39 >= RxdBuf[4])&&RxdBuf[3] == 0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)		 //匹配命令的参数
							{
								ENC_KEY.temp.settrack2format = RxdBuf[4];	//读取设置的加密方式							
								
								send_frame(CMDSUCC);
							}
							else
							{
								send_frame(CMDERRO);
							}

          	    
            	        break;
					case SETDECODEWAY:						//0x1D	/* set decoding in both directions */	
					    //这里还未完成，待补充
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

							if((0x30==RxdBuf[4]|| 0x31==RxdBuf[4])&&RxdBuf[3]==0x01 && RxdBuf[5] == ETX && RxdBuf[6]==BQLRC)//30 fix加密  31=DUKPT加密 
							{ 
								//fix加密   必须要进行外部认证OK  才可以  否则还是DUKPT
                if(FIX_Authenticate_Value==0x31 && RxdBuf[4]==0x30){								
								  ENC_KEY.temp.encmode = RxdBuf[4];	//读取设置的加密方式
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
					case SENDAUTHDATA:						//0x74	/* send authentication data  参考3.9.2小节*/
            if(	RxdBuf[3]==0x08 &&RxdBuf[1+RxdBuf[3]]==ETX && RxdBuf[RxdBuf[3]+5]== BQLRC ){							
							if(memcmp(FixKey_AuthenBuff,&RxdBuf[4],RxdBuf[3]) == 0)
							{
//								ENC_KEY.temp.FixKey_AuthenFlag = 0x31;				//鉴定数据OK
								FIX_Authenticate_Value=0x31;

								send_frame(CMDSUCC);	
							}
							else
							{
//								ENC_KEY.temp.FixKey_AuthenFlag = 0x30;				//鉴定数据失败
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

								ENC_KEY.temp.encway = RxdBuf[4];//读取设置的加密类型   
								ENC_KEY.temp.status = 1;	//使能加密
								send_frame(CMDSUCC);
							}
							else if(0x30 == RxdBuf[4])
							{
								ENC_KEY.temp.encway = RxdBuf[4];//读取设置的加密类型   
								ENC_KEY.temp.status = 0;	//使能加密
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
							  send_frame(CMDERRO);  //只能写一次  printf("&& RxdBuf[RxdBuf[3]+4]== ETX=%d\r\n",RxdBuf[RxdBuf[3]+4]);
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
						
        case SETTERMINATOR: /*设置结束符*/

							if(RxdBuf[4]<=0x7F && RxdBuf[3]==0x01 && RxdBuf[5]==ETX && RxdBuf[6]==BQLRC)
							{
								ENC_KEY.temp.Terminator = RxdBuf[4];

								send_frame(CMDSUCC);
							}
								else send_frame(CMDERRO);

              break;
				case SETPREAMBLE:   //d2  <STX><S><n><Len><Prefix><ETX><CheckSum>
						if(RxdBuf[3] <= 0x0F && RxdBuf[RxdBuf[3]+4]== ETX && RxdBuf[RxdBuf[3]+5]== BQLRC)  //判断 长度后面的数据  是否等于ETX  有类似的判断都是按照这样的格式
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
				WriteENCKEY();					//保存更新后的KEY

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


