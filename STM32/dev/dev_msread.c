#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "app.h"
#include "usb_endp.h"
#include "usb_type.h"

#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_pwr.h"

SHA1Context foo;
AES_KEY key;

uint8_t BCD_ASCII_Buf[1024]={0},ch[1024]={0};
//uint8_t *sha;
uint8_t sha[60];
#define MAX 128
/**************************************************************************************
//	*** 函 数 名:	void MSR_SendData(void)
//	*** 功能描述： 磁道数据加密上送函数
//	*** 参    数: 
//	*** 返 回 值: 解码的长度  	 	
//	*** 模块信息: Ouyangweiquan 2012.04.24创建
***************************************************************************************/

void DeleteChars(uint8_t *str1,char *str2)
{
	if(str1==NULL || str2==NULL)
		return;

	bool hashtable[MAX];
	memset(hashtable,0,sizeof(hashtable));

	//将str2中字符对应的hashtable数组中的位置上的值设为ture
	while(*str2 != '\0')
	{
		//ASCII值在128-255之间的的字符，
		//用char保存，转化为int型,在-128--1之间
		int index;
		if(*str2 >= 0)
			index = *str2;
		else
			index = *str2 + 256;

		hashtable[index] = 1;
		++str2;
	}

  uint8_t *pFast = str1;
	uint8_t *pSlow = str1;
	while(*pFast != '\0')
	{
		int index;
		if(*pFast >= 0)
			index = *pFast;
		else
			index = *pFast + 256;

		//无论是否碰到要删除的字符，pFast都后移，
		//只有没碰到要删除的字符时，pSlow才后移
		if(!hashtable[index])
			*pSlow++ = *pFast;	
		++pFast;
	}
	*pSlow = '\0';
}

//void Delete_Chars(char* src, char* sub,char *dst)
//{
//	int i = 0;
//	if (src == NULL || sub == NULL)
//	{
//		return;
//	}

//	bool hashtable[256];
//	memset(hashtable, 0, sizeof(hashtable));

//	
//	while (*sub != '\0')
//	{

//		int index;
//		if (*sub >= 0)
//		{
//			index = *sub;
//		}
//		else
//		{
//			index = *sub + 256;
//		}

//		hashtable[index] = 1;
//		++sub;
//	}

//	char* pFast = src;
//	char pSlow[256] = {0};
//	while (*pFast != '\0')
//	{
//		int index;
//		if (*pFast > 0)
//		{
//			index = *pFast;
//		}
//		else
//		{
//			index = *pFast + 256;
//		}


//		if (!hashtable[index])
//		{
//			
//			pSlow[i++] = *pFast;
//		}
//		++pFast;
//	}
//	pSlow[i] = '\0';

//	memcpy(dst, pSlow, i);
//	
//}

//void GetAccount(char *data,char *account)
//{
//	char *p1,*p2;
//	
//	p1 = strstr ( (const char *) data, ";");
//	p2 = strstr ( (const char *) data, "=");

//	if(p1 == NULL || p2 == NULL)
//	{
//	    return;
//	}
//	
//	
//	strncpy (account, (const char *) p1+1, p2-p1-1);

//}

int getbit(int index,int num)
{
    return num&(0x01<<index);
}


void MSR_SendData(void)
{
	uint8_t /**ch =(uint8_t *)TrackBitFlow.Value,*/maskbuff[256]={0}, ivec[16],randbuff[8],s=0,lp,pflag;
	uint16_t len = 0x10,j;
	uint8_t lrc,bcc,block1,block2,block3;
	uint8_t /*extralength,tp1,ts1,tp2,ts2,tp3,ts3,Preamblelen,Postamblelen,*/track1=0,track2=0,track3=0;
	uint16_t templen = 0;

//	uint8_t account[20] = {0};
  memset(ch,0x00,sizeof(ch));
	
//	TK1len=66;TK2len=41; TK3len=105;
//	memcpy(TK1buf,"a%6226200202135845^ANONYMITY((((^160210118003((((999900970000007?",66);
//	memcpy(TK2buf,"a;6226200202135845=49125200097173800000?",41);
//	memcpy(TK3buf,"a;996226200202135845=1561560000000000001003000000040404049120=000000000000=000000000000=097173800020009?",105);
//	

//	ENC_KEY.temp.PrePANID=0x06;
//	ENC_KEY.temp.PostPANID=0x04;
//	printf("ENC_KEY.temp.PrePANID=%d\r\n",ENC_KEY.temp.PrePANID);
//	printf("ENC_KEY.temp.PostPANID=%d\r\n",ENC_KEY.temp.PostPANID);
	if(TK1len)
	{
		TK1len-=2;

		for(j=0;j<TK1len;j++) TK1buf[j]=TK1buf[j+1];
		  

	}
	
	if(TK2len)	
	{
		TK2len-=2;
	  
		for(j=0;j<TK2len;j++) TK2buf[j]=TK2buf[j+1];

	}
	
	if(TK3len)
	{
		lp=TK3len-=2;		
		
		for(j=0;j<TK3len;j++) TK3buf[j]=TK3buf[j+1];

		TK3len=lp;
	}
//	DBG_H("TK1buf",TK1buf,TK1len);
//	DBG_H("TK2buf",TK2buf,TK2len);
//	DBG_H("TK3buf",TK3buf,TK3len);
	
//	memset(account,0x00,sizeof(account));
	memset(sha,0x00,sizeof(sha));



	if ( ENC_KEY.temp.level < 0x30 && ENC_KEY.temp.level > 0x34 )	//加密级别失效
	{
		goto ERRORDATA;											//密钥失效返回错误
	}

	
	if(ENC_KEY.temp.level==0x34)  //等级4
		if(ENC_KEY.temp.Activa_Challenge_Reply==0x30)
			return;

	if (1 == ENC_KEY.temp.status)								//密文回送磁道数据
	{

		templen = 0;
		/*长度加84 = 8个数据包头 + 60 个哈希值 + 10个KSN +16个密钥+ 2个校验码 + 1个包结束 + extralength */
		if(0x32 == ENC_KEY.temp.encway)							//如果AES加密则每块16字节 
		{
			block1 = ceil((TK1len-1)/16.0);
			block2 = ceil((TK2len-1)/16.0);
			block3 = ceil((TK3len-1)/16.0);         //84

		}
		else													//如果DES加密则每块8字节
		{
			block1 = ceil((TK1len-1)/8.0);
			block2 = ceil((TK2len-1)/8.0);
			block3 = ceil((TK3len-1)/8.0);       //84

		}
		
		if(!block1 && !block2 && !block3)						//如果所有磁道数为空，刷卡失败
			goto ERRORDATA;

		if(TK1len)
		{
			SHA1Init (&foo);
			SHA1Update (&foo, &TK1buf[0], TK1len);			//计算第一磁道数据哈希值
			SHA1Final (&foo, sha);
		}

		if(TK2len)
		{
			SHA1Init (&foo);
			SHA1Update (&foo, &TK2buf[0], TK2len);			//计算第二磁道数据哈希值
			SHA1Final (&foo, sha+20);
		}

		if(TK3len)
		{
			SHA1Init (&foo);
			SHA1Update (&foo, &TK3buf[0], TK3len);			//计算第三磁道数据哈希值
			SHA1Final (&foo, sha+40);
			
		}		
    		
		//            0  1   2   3  4    5   6   7   8  9						
		//memcpy(ch,"\x02\x00\x00\x80\x3F\x00\x00\x00\x00\x01",10);//10个字节数据包头
		memset(ch,0x00,sizeof(ch));
		ch[0]=0x02;
//		ch[1] = (uint8_t)(len-7);								 //磁卡数据总保存长度
//		ch[2] = ((len-7)>>8);
    	
			ch[3]=0x00;	//card encoding type (0: ISO/ABA, 4: for Raw Mode)
	//		//占三位
			if(TK1len) track1=0x3f;else track1=0x3e;			  
			
			if(TK2len) track2=0x3f;else track2=0x3d;  		  		
			
			if(TK3len) track3=0x3f;else track3=	0x3b;
        
			
      track1=track1&track2&track3;
			
			ch[4]=track1;

      if(TK1len){ch[5] = TK1len;}//1磁头数据长度

			if(TK2len){ch[6] = TK2len;}//2磁头数据长度
			if(TK3len){ch[7] = TK3len;}//3磁头数据长度

			if(ENC_KEY.temp.Enhancedstatue==0x30) //在加密等级==3的时候输出的 数据方式
				{  
					templen+=8;
					if(TK1len)//判断轨道1是否为空
						{
							memset(maskbuff,0x00,sizeof(maskbuff));
							memcpy(maskbuff,TK1buf,TK1len);
							pflag=0xAC;
							
							for(lp=ENC_KEY.temp.PrePANID+1;lp<TK1len;lp++)//加一的原因是% ;这些字符不算掩码长度
							{
								if(maskbuff[lp]=='^' &&pflag==0xAC)
								{										
									pflag=0xAF;
									
									track1=lp-1;
									memset(maskbuff+ENC_KEY.temp.PrePANID+1,ENC_KEY.temp.MaskChar ,track1-ENC_KEY.temp.PostPANID-ENC_KEY.temp.PrePANID);
								}

								else if(maskbuff[lp]!='?' && pflag==0xAF) maskbuff[lp]=ENC_KEY.temp.MaskChar;
							}

								if(maskbuff[1]=='B')maskbuff[1] = ENC_KEY.temp.MaskChar;
							
								memcpy(ch+templen,maskbuff,TK1len);//copy轨道1的数据明文
								templen+=TK1len;
						 }
						
						if(TK2len)//判断轨道1是否为空
						{
							memset(maskbuff,0x00,sizeof(maskbuff));
							memcpy(maskbuff,TK2buf,TK2len);
							 
							for(lp=ENC_KEY.temp.PrePANID+1;lp<TK2len;lp++){
								if(maskbuff[lp]=='=' && pflag==0xDF){
									pflag=0xAF;										
									track1=lp-1;
									memset(maskbuff+ENC_KEY.temp.PrePANID+1,ENC_KEY.temp.MaskChar ,track1-ENC_KEY.temp.PostPANID-ENC_KEY.temp.PrePANID);
								}
								else if(maskbuff[lp]!='?' && pflag==0xAF) maskbuff[lp]=ENC_KEY.temp.MaskChar;
							}
							 if(maskbuff[1]=='B')maskbuff[1] = ENC_KEY.temp.MaskChar;
							 memcpy(ch+templen,maskbuff,TK2len);//copy轨道2的数据明文
							 templen+=TK2len;
						 }
								 
						 memcpy(ch+templen,TK3buf,TK3len);//copy轨道3的数据明文
								 
						 templen += TK3len; 
				}
				else if(ENC_KEY.temp.Enhancedstatue==0x31) // 强加密模式下的输出格式
					{
					   if(getbit(0,ENC_KEY.temp.MaskSetting) == 0x01) track1=0x07;//掩码设置
					     else track1=0x06;
						 if(getbit(1,ENC_KEY.temp.MaskSetting) == 0x02) track2=0x07;
					     else track2=0x05;
						 if(getbit(2,ENC_KEY.temp.MaskSetting) == 0x04) track3=0x07;
					     else track3=0x03;
						
						ch[8]=track1=track1&track2&track3;
						
						if(ENC_KEY.temp.encmode==0x31) track1=0xff; else track1=0x7f;//7bit
						if(ENC_KEY.temp.level==0x34)   track2=0xff; else track2=0xbf;//6bit
						if(getbit(0,ENC_KEY.temp.HASHSET) == 0x04) track3=0xff; else track3=0xdf;//5bit
						
						ch[9]=lp=track1&track2&track3;
						if(getbit(1,ENC_KEY.temp.HASHSET) == 0x02) track2=0xff; else track2=0xef;//4bit
						if(getbit(2,ENC_KEY.temp.HASHSET) == 0x01) track1=0xff; else track1=0xf7;//3bit
						if(getbit(0,ENC_KEY.temp.Encryption_OptionSetting) == 0x04) track3=0xff; else track3=0xfb;//2bit
						ch[9]=lp=lp&track1&track2&track3;
						
						if(getbit(1,ENC_KEY.temp.Encryption_OptionSetting) == 0x02) track2=0xff; else track2=0xfd;//1bit
						if(getbit(2,ENC_KEY.temp.Encryption_OptionSetting) == 0x01) track1=0xff; else track1=0xfe;//0bit
						
						ch[9]=lp=lp&track1&track2;

						templen += 10;
						
//						printf("000getbit(0,ENC_KEY.temp.Encryption_OptionSetting=%d\r\n",getbit(0,ENC_KEY.temp.Encryption_OptionSetting));
//						printf("000getbit(0,ENC_KEY.temp.MaskSetting) =%d\r\n",getbit(0,ENC_KEY.temp.MaskSetting) );
//						
//						printf("111getbit(1,ENC_KEY.temp.Encryption_OptionSetting=%d\r\n",getbit(1,ENC_KEY.temp.Encryption_OptionSetting));
//						printf("111getbit(1,ENC_KEY.temp.MaskSetting) =%d\r\n",getbit(1,ENC_KEY.temp.MaskSetting) );
//						
//						printf("222getbit(2,ENC_KEY.temp.Encryption_OptionSetting=%d\r\n",getbit(2,ENC_KEY.temp.Encryption_OptionSetting));
//						printf("222getbit(2,ENC_KEY.temp.MaskSetting) =%d\r\n",getbit(2,ENC_KEY.temp.MaskSetting) );
						

						if(getbit(0,ENC_KEY.temp.Encryption_OptionSetting) == 0x01)
						{
							if(getbit(0,ENC_KEY.temp.MaskSetting) == 0x01){
								if(TK1len)//判断轨道1是否为空
								{
									memset(maskbuff,0x00,sizeof(maskbuff));
									memcpy(maskbuff,TK1buf,TK1len);
									pflag=0xAC;
									
									for(lp=ENC_KEY.temp.PrePANID+1;lp<TK1len;lp++)//加一的原因是% ;这些字符不算掩码长度
									{
										if(maskbuff[lp]=='^' &&pflag==0xAC)
										{										
											pflag=0xAF;
											
											track1=lp-1;
											memset(maskbuff+ENC_KEY.temp.PrePANID+1,ENC_KEY.temp.MaskChar ,track1-ENC_KEY.temp.PostPANID-ENC_KEY.temp.PrePANID);
										}

										else if(maskbuff[lp]!='?' && pflag==0xAF) maskbuff[lp]=ENC_KEY.temp.MaskChar;
									}

							      if(maskbuff[1]=='B')maskbuff[1] = ENC_KEY.temp.MaskChar;
									
								    memcpy(ch+templen,maskbuff,TK1len);//copy轨道1的数据明文
								    templen+=TK1len;
								 }
						 } 
						}

						if(getbit(1,ENC_KEY.temp.Encryption_OptionSetting) == 0x02)
						{
							pflag=0xDF;
							
							if(getbit(1,ENC_KEY.temp.MaskSetting) == 0x02){
								if(TK2len)//判断轨道1是否为空
								{
									memset(maskbuff,0x00,sizeof(maskbuff));
									memcpy(maskbuff,TK2buf,TK2len);
									 
									for(lp=ENC_KEY.temp.PrePANID+1;lp<TK2len;lp++){
										if(maskbuff[lp]=='=' && pflag==0xDF){
											pflag=0xAF;										
											track1=lp-1;
											memset(maskbuff+ENC_KEY.temp.PrePANID+1,ENC_KEY.temp.MaskChar ,track1-ENC_KEY.temp.PostPANID-ENC_KEY.temp.PrePANID);
										}
										else if(maskbuff[lp]!='?' && pflag==0xAF) maskbuff[lp]=ENC_KEY.temp.MaskChar;
									}
									
									if(maskbuff[1]=='B')maskbuff[1] = ENC_KEY.temp.MaskChar;

							     memcpy(ch+templen,maskbuff,TK2len);//copy轨道2的数据明文
							     templen+=TK2len;
								 }
						 }
						}
						if(getbit(2,ENC_KEY.temp.Encryption_OptionSetting) == 0x04)
						{
							if(getbit(2,ENC_KEY.temp.MaskSetting) == 0x04){							
								if(TK3len)//判断轨道1是否为空
								{
									memset(maskbuff,0x00,sizeof(maskbuff));
									memcpy(maskbuff,TK3buf,TK3len);
									pflag=0xA1;
									for(lp=ENC_KEY.temp.PrePANID+1;lp<TK3len;lp++){
										if(maskbuff[lp]=='=' && pflag==0xA1){
											pflag=0xAF;
											
											track1=lp-1;
	//										printf("lp=%d\r\n",lp);
											memset(maskbuff+ENC_KEY.temp.PrePANID+1,ENC_KEY.temp.MaskChar ,track1-ENC_KEY.temp.PostPANID-ENC_KEY.temp.PrePANID);
										}
										else if(maskbuff[lp]!='?' && pflag==0xAF) maskbuff[lp]=ENC_KEY.temp.MaskChar;
									}
									  memcpy(ch+templen,maskbuff,TK3len);//copy轨道2的数据明文
									  templen+=TK3len;
								 }
						 }
				  }
				
		}

//		DBG_H("3333ch",ch,templen);
		if (0x31 == ENC_KEY.temp.encmode)				   		//DUKPT加密
		{
			
			Dukpt_KeyManagement(); //计算DUKPT过程密钥  ENC_KEY.temp.key
			
		}
		else if (0x30 == ENC_KEY.temp.encmode)			 		//FIX加密
		{
			memcpy(CryptoKey,ENC_KEY.temp.fix,16);				//拷贝FIX密钥
//			DBG_H("ENC_KEY.temp.fix",CryptoKey,16);
//			DBG_H("ENC_KEY.temp.ksnasn",ENC_KEY.temp.ksn,10);
		}
		if( 0x30 == ENC_KEY.temp.encway)						//DES加密
		{
			goto ERRORDATA;
		}
		else if(0x31 == ENC_KEY.temp.encway)					//three DES加密 
		{
			if(ENC_KEY.temp.Enhancedstatue==0x31)//强制加密情况下  需要把每个轨道数据分开加密
			{
			if(block1)	
			{
				tri_des(TK1buf,CryptoKey,CryptoKey+8,0);
				for(j = 1;j< block1;j++)							//加密第一磁道数据
				{			
					xor(TK1buf+j*8,TK1buf+j*8-8,8);
					tri_des(TK1buf+j*8,CryptoKey,CryptoKey+8,0);
				}

					//保存1磁道信息
					memcpy(ch+templen,TK1buf,(block1*8));						  
					templen += block1*8;			
			 }
				if(block2)
				{
					tri_des(TK2buf,CryptoKey,CryptoKey+8,0);
					for(j = 1;j< block2;j++)							//加密第二磁道数据
				 {
					 xor(TK2buf+j*8,TK2buf+j*8-8,8);
					 tri_des(TK2buf+j*8,CryptoKey,CryptoKey+8,0);
				 }      
				memcpy(ch+templen,TK2buf,block2*8);//memcpy(ch+templen,TK2buf+1,block2*8);原函数  若是这样会把起始符给去掉  请注意
				templen += block2*8;

			  }		

			
				if(block3)
				{
					tri_des(TK3buf,CryptoKey,CryptoKey+8,0);
					for(j = 1;j< block3;j++)							//加密第三磁道数据
					{
						xor(TK3buf+j*8,TK3buf+j*8-8,8);
						tri_des(TK3buf+j*8,CryptoKey,CryptoKey+8,0);
					}			
					memcpy(ch+templen,TK3buf,block3*8);
					templen += block3*8; 
		
			  }
			}
			else if(ENC_KEY.temp.Enhancedstatue==0x30)
			{
				
				memcpy(maskbuff,TK1buf,TK1len);
				memcpy(maskbuff+TK1len,TK2buf,TK2len);
				
				tri_des(maskbuff,CryptoKey,CryptoKey+8,0);
				for(j = 1;j< block1+block2;j++)							//加密第三磁道数据
				{
					xor(maskbuff+j*8,maskbuff+j*8-8,8);
					tri_des(maskbuff+j*8,CryptoKey,CryptoKey+8,0);
				}
				j=block1+block2;
				memcpy(ch+templen,maskbuff,j*8);
				templen += j*8; 

			}
							
		}		
		else if(0x32 == ENC_KEY.temp.encway)					//AES加密
		{
			AES_set_encrypt_key(CryptoKey,128,&key);
			/* 加密第一磁道数据 */
			if(ENC_KEY.temp.Enhancedstatue==0x31)
			{
				if(block1)
				{
					memset(ivec,0,sizeof(ivec));			    
					memcpy(ch+templen,TK1buf,block1*16);                

					AES_cbc_encrypt(ch+templen,ch+templen,block1*16,&key,ivec,1);
					templen += block1*16;
			
				}
			/* 加密第二磁道数据 */
				if(block2)
				{
					memset(ivec,0,sizeof(ivec));				    
					memcpy(ch+templen,TK2buf,block2*16);

					AES_cbc_encrypt(ch+templen,ch+templen,block2*16,&key,ivec,1);
					templen += block2*16;
					
				}
			/* 加密第三磁道数据 */
				if(block3)
				{
					memset(ivec,0,sizeof(ivec));			    
					memcpy(ch+templen,TK3buf,block3*16);
					AES_cbc_encrypt(ch+templen,ch+templen,block3*16,&key,ivec,1);
					templen += block3*16;  

				}
			}
			else if(ENC_KEY.temp.Enhancedstatue==0x30)
			{
				memcpy(maskbuff,TK1buf,TK1len);
				memcpy(maskbuff+TK1len,TK2buf,TK2len);
				j=block1+block2;
				AES_cbc_encrypt(maskbuff,maskbuff,j*16,&key,ivec,1);
				
				memcpy(ch+templen,maskbuff,j*16);
			  templen += j*16; 
				
			}

	}
	 	else
		{
			goto ERRORDATA;
		}
				
			if(ENC_KEY.temp.level==0x33)//强制加密等级3的时候	 强制加密的DUKPT和FIX加密是把KSN和SN添加  DUKPT添加---KSN    FIX加密添加SN
			{
				if(getbit(0,ENC_KEY.temp.HASHSET) == 0x01 && ENC_KEY.temp.Enhancedstatue==0x31)//若是加密 确定轨道1哈希值是否发送
					{
					  if(block1)
					 {
						 memcpy(ch+templen,sha,20);					//保存第1磁道数据哈希值
						 templen+=20;

					 }						
				  }
          else if(ENC_KEY.temp.Enhancedstatue==0x30)
					{
						 if(block1)
					 {
						 memcpy(ch+templen,sha,20);					//保存第1磁道数据哈希值
						 templen+=20;

					 }

					}						
					if(getbit(1,ENC_KEY.temp.HASHSET) == 0x02 && ENC_KEY.temp.Enhancedstatue==0x31)//if(ENC_KEY.temp.HASHSET==0x32) //若是加密 确定轨道2哈希值是否发送
					{
						if(block2)
						{
              memcpy(ch+templen,sha+20,20);					//保存第2磁道数据哈希值
						  templen+=20;

						}							            						
					}
					else if(ENC_KEY.temp.Enhancedstatue==0x30)//不是强加密设置下  就把哈希值1和2发送
					{
						if(block2)
						{
              memcpy(ch+templen,sha+20,20);					//保存第2磁道数据哈希值
						  templen+=20;

						}
						
					}
					if(getbit(2,ENC_KEY.temp.HASHSET) == 0x04 && ENC_KEY.temp.Enhancedstatue==0x31)//if(ENC_KEY.temp.HASHSET==0x34) //若是加密 确定轨道3哈希值是否发送
					{
						if(block3)
						{
              memcpy(ch+templen,sha+40,20);					//保存第3磁道数据哈希值
						  templen+=20;

						}							            						
					}	   					
					if(0x31 == ENC_KEY.temp.encmode)//是dukpt加密方式  则返回ksn
					{
				    memcpy(ch+templen,ENC_KEY.temp.ksn,10);
						templen+=10;

					}
			
					if(0x30 == ENC_KEY.temp.encmode)//是fix加密方式  则返回sn
					{
						memcpy(ch+templen,ENC_KEY.temp.sn,10);       
            templen+=10;
            ch[templen++]=0x00;
            ch[templen++]=0x00;					
						
					}

		  }
			
			if(ENC_KEY.temp.level==0x34) //  DUKPT加密就是加密等级4    强制加密等级4的时候
			{
				memcpy(randbuff,ENC_KEY.temp.sid,8);
				tri_des(randbuff,CryptoKey,CryptoKey+8,0);
				memcpy(ch+templen,randbuff,8);//添加SSID  只有在加密等级是4的时候 才添加SSID
				templen+=8;

				if(getbit(0,ENC_KEY.temp.HASHSET) == 0x01 && ENC_KEY.temp.Enhancedstatue==0x31)//若是加密 确定轨道1哈希值是否发送
					{
					  if(block1)
					 {
						 memcpy(ch+templen,sha,20);					//保存第1磁道数据哈希值
						 templen+=20;

					 }						
				  }
					else if(ENC_KEY.temp.Enhancedstatue==0x30)
					{
						 if(block1)
						 {
							 memcpy(ch+templen,sha,20);					//保存第1磁道数据哈希值
							 templen+=20;

						 }
						
					}
				if(getbit(1,ENC_KEY.temp.HASHSET) == 0x02  && ENC_KEY.temp.Enhancedstatue==0x31) //若是加密 确定轨道2哈希值是否发送
					{
						if(block2)
						{
							memcpy(ch+templen,sha+20,20);					//保存第1磁道数据哈希值
						  templen+=20;						
						}            						
					}
					else if( ENC_KEY.temp.Enhancedstatue==0x30)
					{
						if(block2)
						{
							memcpy(ch+templen,sha+20,20);					//保存第1磁道数据哈希值
						  templen+=20;
						
						}
					}
					if(getbit(2,ENC_KEY.temp.HASHSET) == 0x04  && ENC_KEY.temp.Enhancedstatue==0x31)//if(ENC_KEY.temp.HASHSET==0x34) //若是加密 确定轨道3哈希值是否发送
					{
						if(block3)
						{
							memcpy(ch+templen,sha+40,20);					//保存第3磁道数据哈希值
						  templen+=20;
					
						}					
					}
				   					
					if(0x31 == ENC_KEY.temp.encmode)//是dukpt加密方式  则返回ksn
					{
				    memcpy(ch+templen,ENC_KEY.temp.ksn,10);
						templen+=10;
					}
			
					if(0x30 == ENC_KEY.temp.encmode)//是fix加密方式  则返回sn
					{
						memcpy(ch+templen,ENC_KEY.temp.sn,10);
						templen+=10;
						ch[templen++]=0x00;
						ch[templen++]=0x00;
						
					}
			
				
			}
			
      len=templen-3;
			ch[1] = len&0xFF;  //数据长度低位
		  ch[2] = (len>>8)&0xFF; //数据长度高位


		lrc=CalcXOR_none(ch,templen,3);
		bcc=CheckSum8((char*)&ch[3],templen-3);

		templen+=3;
		ch[templen-3] = lrc;//计算校验码
		ch[templen-2] = bcc;//计算校验码
		ch[templen-1] = 0x03;
		
		
	}
	else if(0 == ENC_KEY.temp.status)			  					//明文回送数据
	{		
			templen = 0;
      len=0;

			if(ENC_KEY.temp.Tranmodeselect==0x30)
			{
				ch[0]=0x00;	
				ch[1]=0x00;	
				ch[2]=0x00;					
			  ch[6]=0x04;				
				templen += 9;
			}
			  memcpy(ch+templen,ENC_KEY.temp.Preamble,ENC_KEY.temp.Preamblelen_m);				
				templen += ENC_KEY.temp.Preamblelen_m;

			if(TK1len)
			{
				
				if( ENC_KEY.temp.settrack2format==0x30 || ENC_KEY.temp.settrack2format==0x32){ //当轨道1设置去掉开始符和结束符的时候 输出格式
				  for(s=0;s<TK1len;s++){					
					  if(TK1buf[s]=='?' || TK1buf[s]=='%' || TK1buf[s]==';')TK1len=TK1len-1;						
					}					
					 DeleteChars( TK1buf ,"%+,-;?");          					
				}
				if(ENC_KEY.temp.Tranmodeselect==0x30){
					len+=TK1len;
				  ch[3]=(u16)TK1len;
				}			
				memcpy(ch+templen,ENC_KEY.temp.track1prefix,ENC_KEY.temp.tp1len);//添加轨道1前缀数据
    		templen += ENC_KEY.temp.tp1len;
				
				memcpy(ch+templen,TK1buf,TK1len);//添加轨道1数据
        templen += TK1len;
				
				memcpy(ch+templen,ENC_KEY.temp.track1suffix,ENC_KEY.temp.ts1len);//添加轨道1后缀数据
    		templen += ENC_KEY.temp.ts1len;
				
				if(!TK2len){    //如果只有一轨道有数据  就直接添加结束符
					memcpy(ch+templen,ENC_KEY.temp.Postamble,ENC_KEY.temp.Postamblelen_m);//添加数据 posta
    	    templen += ENC_KEY.temp.Postamblelen_m;
					ch[templen++]=ENC_KEY.temp.Terminator;//添加结束符  默认CR
				}
				
				else ch[templen++]=ENC_KEY.temp.setseparator;

			}
			
			if(TK2len)
			{
				if( ENC_KEY.temp.settrack2format==0x30 || ENC_KEY.temp.settrack2format==0x32){ //当轨道2设置去掉开始符和结束符的时候 输出格式
				  for(s=0;s<TK2len;s++){					
					  if(TK2buf[s]=='?' || TK2buf[s]=='%' || TK2buf[s]==';')TK2len=TK2len-1;					
					}
          DeleteChars( TK2buf ,"%+,-;?");															
				}
				//轨道2的数据会在等于号后面不给与输出  在下条件
				if(ENC_KEY.temp.settrack2format==0x33 || ENC_KEY.temp.settrack2format==0x32){
					DeleteChars( TK2buf ,"%+,-;?");	
					for(s=0;s<TK2len;s++){
						if(TK2buf[s]=='='){TK2len=s-1; break;}
					}
					
				}
				if(ENC_KEY.temp.Tranmodeselect==0x30){
					len+=TK2len;
				  ch[4]=(u16)TK2len;
				}
				
				memcpy(ch+templen,ENC_KEY.temp.track2prefix,ENC_KEY.temp.tp2len);//添加轨道前缀数据
    		templen += ENC_KEY.temp.tp2len;
				
				memcpy(ch+templen,TK2buf,TK2len);//添加轨道数据
        templen += TK2len;
				
				memcpy(ch+templen,ENC_KEY.temp.track2suffix,ENC_KEY.temp.ts2len);//添加轨道后缀数据
    		templen += ENC_KEY.temp.ts2len;

				if(!TK3len){			//如果只有轨道2有数据		
					memcpy(ch+templen,ENC_KEY.temp.Postamble,ENC_KEY.temp.Postamblelen_m);//添加数据 posta
    	    templen += ENC_KEY.temp.Postamblelen_m;
					ch[templen++]=ENC_KEY.temp.Terminator;// 如果只有轨道2有数据 添加结束符  默认CR
				}
				
				else ch[templen++]=ENC_KEY.temp.setseparator;//添加分隔符  默认CR

			}
			
			if(TK3len)
			{
         //当轨道3设置去掉开始符和结束符的时候 输出格式
				if( ENC_KEY.temp.settrack2format==0x30 || ENC_KEY.temp.settrack2format==0x32){ 
				  for(s=0;s<TK3len;s++){					
					  if(TK3buf[s]=='?' || TK3buf[s]=='%' || TK3buf[s]==';')TK3len=s-1;				 						 
					}	
					DeleteChars( TK3buf ,"%+,-;?");										
				}
				if(ENC_KEY.temp.Tranmodeselect==0x30){
					len+=TK3len;
				  ch[5]=(u16)TK3len;
				}
								         
			
				memcpy(ch+templen,ENC_KEY.temp.track3prefix,ENC_KEY.temp.tp3len);//添加轨道1前缀数据
    		templen += ENC_KEY.temp.tp3len;
				
				memcpy(ch+templen,TK3buf,TK3len);//添加轨道1数据
        templen += TK3len;
				
				memcpy(ch+templen,ENC_KEY.temp.track3suffix,ENC_KEY.temp.ts3len);//添加轨道1后缀数据
    		templen += ENC_KEY.temp.ts3len;
				memcpy(ch+templen,ENC_KEY.temp.Postamble,ENC_KEY.temp.Postamblelen_m);
    	  templen += ENC_KEY.temp.Postamblelen_m;
		
			  ch[templen++]=ENC_KEY.temp.Terminator;//添加结束符  默认CR

			}

    		
			
			  
//	    }
		if(ENC_KEY.temp.Tranmodeselect==0x30)
		{
			
		  len += 3;//三位数据长度  轨道1、2、3
			ch[8] = len&0xFF;  //数据长度低位
		  ch[7] = (len>>8)&0xFF; //数据长度高位
			
		}

		
	}
	else
	{
ERRORDATA:
    	len = 10;
    	memset(ch, 0xFF, templen);	
	}

	if(0 == ENC_KEY.temp.status)
	{
		if(ENC_KEY.temp.Tranmodeselect==0x38)DataSend_USBHID(ch,templen); 
			
		else Custom_Data_Send(ch,560,200);
			
  }
	else
	{
		if(ENC_KEY.temp.Tranmodeselect==0x38)
		{
			bcd2asc(BCD_ASCII_Buf,ch,templen*2,0);
  	  DataSend_USBHID(BCD_ASCII_Buf,templen*2);
			
		}
    else
		{
			templen=560;
		  Custom_Data_Send(ch,templen,200);
		}
	}
//	DBG_H("ch",ch,templen);
//	delay_ms(100);
//  while(_GetEPTxStatus(ENDP1)!=(0x02<<4));
	return;
}

