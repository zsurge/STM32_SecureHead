#ifndef __APP_H
#define __APP_H

#include "magdecode.h"
#include "dev_eprom.h"	
#include "dev_uart.h"
#include "dev_timer.h"
#include "dev_msread.h"
#include "des.h"
#include "sha1.h"
#include "aes.h"
#include "dukpt.h"
#include "utilities.h"

#define idte_datalen 128
#define	FLASHKEY_SUPPORT		//ouyangweiquan	2012.08.20 add ������Կд��Flash�ĵ���
//#undef	FLASHKEY_SUPPORT


typedef void (*function)(void);

#define STX								    0x02	/* frame head */
#define ETX		    					  0x03	/* text end */
#define CMDSUCC	    					0x06	/* command success */
#define CMDERRO	    					0x15	/* command error */
#define DATAFMTID             0x15
#define DATAFMTID1            0x16
#define RESET_STM32           0x6A
#define REQUEST_DOWN					0x55	/* request download */
//#define READ_VERSION					0x28 	/* read application ersion */

#define RESETHEAD						  0x38	/* reset secureheadreader */
//#define	SETKEY							0x46	/* setting key commands */
#define READCMD							  0x52	/* read status commands */
#define SETTINGCMD						0x53	/* setting commands */

//#define	SETKEYHEAD					  	0xFF	/* setting Dukpt key commands head */
#define	SETKEYVALUE						0x0A	/* setting Dukpt key value commands */
//#define	SETKEYCOMMU						0x13	/* setting Dukpt key communications commands */

#define SETBEEP               0x11    /* Beep Setting */
#define READID							  0x22	/* read secureheadreader ID */
#define SETDECODEWAY					0x1D	/* Decoding Method Settings */
#define MSRSETTINGS						0x1A	/* MSR Reading Settings */
#define SETDEFAULT						0x18	/* set default configuration */
#define SETENCMODE						0x58	/* set encryption mode */
#define GETCHALLENGE					0x74	/* get encrypt challenge */
#define SENDAUTHDATA					0x74	/* send authentication data */
#define GETSECURITYLEVEL			0x7E	/* get security level */
#define SETDEVICEKEY					0x76	/* set device key command */	

#define SETENCRYPTIONOPTIONSET				0x84	/* Encryption Option Setting */
#define SETENCRYPTIONFORMAT				    0x85	/* Encryption Output Format Setting*/

#define REVIEWKSN						0x51	/* review KSN (DUKPT key management only) */
#define REVIEWSN						0x4E	/* review serial number */
#define SETSID							0x54	/* set Session ID */
#define SETENCWAY						0x4C	/* set encryption way */
#define HASHOPTION					0x5C	/* Hash Option 
Setting */
#define MASKOPTION          0x86

//#define DDATAOK							0x20	//�ش���֡��ȷ

#define SPACE		        		0x00
#define FINISH		       	 	0x55

//add for zd
#define SETTERMINATOR                    0x21   /*���ý�����*/
#define SETPREAMBLE                      0xD2   /*������ʼ�ַ���*/
#define SETPOSTAMBLE                     0xD3   /*���ý����ַ���*/

#define SETTRACK1PREFIX                  0X34   /*���ôŵ�1��ͷ����,���6�ֽ�*/
#define SETTRACK2PREFIX                  0X35   /*���ôŵ�2��ͷ����,���6�ֽ�*/
#define SETTRACK3PREFIX                  0X36   /*���ôŵ�3��ͷ����,���6�ֽ�*/
#define SETTRACK1SUFFIX                  0X37   /*���ôŵ�1�������ݣ����6�ֽ�*/
#define SETTRACK2SUFFIX                  0X38   /*���ôŵ�2�������ݣ����6�ֽ�*/
#define SETTRACK3SUFFIX                  0X39   /*���ôŵ�3�������ݣ����6�ֽ�*/
#define SETSELECTTRACK                   0X13   /*����ѡ����Ĺ��*/

#define SETTRACKSEPARATOR                0x17   /*���ù���ָ��*/
#define SETTRACK2                        0x19   /*���ù��2�������͸�ʽ*/


#define MAX_RXD_BUF_LEN        			100
#define MAX_TXD_BUF_LEN					    0xFF

#define REVIEWSET					                     0x1F   /*��Ҫ�Ǽ�����������  */
#define ENCRYPTEXTERNAL					               0x41   /*��Ҫ�Ǽ�����������  */
#define ACTIVATEAUTHEN                         0x80  //���ܵȼ�4��ʱ����֤ģʽ
#define EXITACTIVATEAUTHEN                     0x81  //�˳�������֤  ֻ���ڼ��ܵȼ�4��ʱ����Ч
#define ACTIVATION_CHALLENGE_REPLY_COMMAND     0x82   /*���ܵȼ�4ʱ�� ���ж�����֤*/
#define GETREADER_STATUSCOMMAND                0x83   /*������״̬�Ķ�ȡ*/
#define TRANSMODE                    0x23   /*keyboard   and custom*/

#define SETMASKCHAR                  0x4B   /*�����ַ�����*/
#define SETPrePANID                  0x49   /*��ʼPANID*/
#define SETPostPANID                 0x4A   /*����PANID*/
#define DispExpDateID                0x50   /*��������������*/

#define LOADKEY                      0xF3  //������Կ key
#define UIC_LOADKEY                  0xFC   /*���ù��2�������͸�ʽ*/
#define LOADKEYAUTHENTICATE          0xF4  //��Կ key ��֤

#define KEYADD							0x800FC00		  //��Կ�洢��ַ
//#define KEYADDBAK						0x800E400		  //������Կ�洢��ַ   63906


union __ENC_KEY
{
	//uint8_t key[64];
	uint8_t key[200];                        //��Ӵŵ�ǰ��׺
	struct{
	uint8_t APP_RunFlagBuf[4];						//�̶���Կ��ֵ	
	uint8_t zhanwei;                       //վλ
	uint8_t beepmode;                       //beep mode
	uint8_t enabledevice;                   //ʹ�ô�ͷ for zd
	uint8_t writesnflag;                    //�Ƿ��Ѿ�д��SN for zd
	uint8_t Terminator;                     //������ for zd
	uint8_t selecttrack;                    //Ҫѡ��Ĺ��  for zd
	uint8_t setseparator;                   //���ù���ָ�� for zd
	uint8_t settrack2format;                //���ù��2�������͸�ʽ for zd
	uint8_t status;							//����״̬��0��ʧ�ܼ��ܣ�1��ʹ�ܼ���
	uint8_t level;							//���ܼ���0x30����ԿʧЧ��0x31�����ģ�0x32���Ѿ�ע����Կ��0x33��ʹ��DUPUT���ܣ�0x34��δʹ��
	uint8_t encmode;						//���ܷ�ʽ��0x30��FIX���ܣ�0x31��DUPUT����
	uint8_t encway;							//�������ͣ�0x30��DES���ܣ�0x31��3DES���ܣ�0x32��AES����
		//12bit
	uint8_t fix[16];						//�̶���Կ��ֵ
	uint8_t key[16];						//������Կ��ֵ
	uint8_t ksn[10];						//KSN��ֵ
	uint8_t sid[8];							//�ỰID
	uint8_t sn[10];							//��ͷ���к�
	uint8_t Preamble[15];                   //��ʼ�ַ��������15�ֽ� for zd
	uint8_t Postamble[15];                  //�����ַ��������15�ֽ� for zd
	uint8_t track1prefix[6];                //�ŵ�1ǰ׺�����6�ֽ� for zd
	uint8_t track2prefix[6];                //�ŵ�2ǰ׺�����6�ֽ� for zd
	uint8_t track3prefix[6];                //�ŵ�3ǰ׺�����6�ֽ� for zd
	uint8_t track1suffix[6];                //�ŵ�1��׺�����6�ֽ� for zd
	uint8_t track2suffix[6];                //�ŵ�1��׺�����6�ֽ� for zd
	uint8_t track3suffix[6];                //�ŵ�1��׺�����6�ֽ� for zd
  
	uint8_t Enhancedstatue;
	uint8_t Enhancedoption;
	uint8_t HASHSET;
	uint8_t MaskSetting;
	
	uint8_t tp1len;
	uint8_t tp2len;
	uint8_t tp3len;
	
	uint8_t ts1len;
	uint8_t ts2len;
	uint8_t ts3len;
	
	uint8_t Preamblelen_m;
	uint8_t Postamblelen_m;
	uint8_t Activa_Challenge_Reply   ;//�ڵȼ�4��ʱ��  ������֤����Ч
	uint8_t Exit_Authenticated   ;//�ڵȼ�4��ʱ��  ������֤����Ч
	uint8_t decoded_directions;//�ſ����뷽������  ��ˢ ��ˢ ����ˢ
	uint8_t Encryption_OptionSetting;
	uint8_t Tranmodeselect;
	uint8_t MaskChar;
	uint8_t PrePANID;
	uint8_t PostPANID;
	uint8_t DispDataflag;
	uint8_t KeyAuthenticateFlag;
	uint8_t Authenticationkey_Buff[16];
	uint8_t Challenge1_Buff[10];
	uint8_t Challenge2_Buff[10];
	uint8_t FixKey_AuthenFlag;
	
	uint8_t lrc;							//����
	uint8_t bcc;							//�ۼӺ�
	}temp;
};

extern union __ENC_KEY ENC_KEY;				//��Կ�������ݽṹ
extern uint8_t CryptoKey[16];				//DUKPT������Ĺ�����Կ 
//extern uint8_t random[8];					//�����
//extern uint8_t macdata[8];					//��������

extern volatile uint8_t WriteFlag;			//������Կ��־��0�������棬1��ʹ�ܱ��� 
extern volatile uint8_t GetNextFlag;		//��ȡ��һ��KSN��־��0������ȡ��һ����1��ʹ�ܻ�ȡ��һ�� 

extern volatile uint8_t macstate;			//����״̬��0x30��δ���յ��������ݻ��������ʧ�ܣ�0x31����������OK
//extern volatile uint8_t encryptdatastatus;	//bit 1 ==1 ��ʾ1�ŵ��������ݴ���
                							//bit 2 ==1 ��ʾ2�ŵ��������ݴ���
                							//bit 3 ==1 ��ʾ3�ŵ��������ݴ���
											//bit 4 ==1 ��ʾ1�ŵ���ϣ���ݴ���
		                					//bit 5 ==1 ��ʾ2�ŵ���ϣ���ݴ���
		                					//bit 6 ==1 ��ʾ3�ŵ���ϣ���ݴ���
											//bit 7 ==1 ��ʾ�ỰID���ݴ���
		                					//bit 8 ==1 ��ʾKSN���ݴ���

void init_serial_boot(void);
void DealRxData(void);
void DealSerialParse(void);
uint8_t ReadENCKEY(void);
void WriteENCKEY(void);
uint8_t AsciiToHex(uint8_t * pAscii, uint8_t * pHex, int nLen);
uint8_t GetNextKSN(void);
void CalcCryptoKey(void);
//void ResetSetting(void);

void Default_Settings(void);//�ָ��������
//void CalcCryptoKeyfc(void);
void Dukpt_KeyManagement(void);

void NVIC_Clear_all(void);
void NVIC_Open_all(void);
void asc2bcd(unsigned char *bcd_buf, unsigned char *ascii_buf, int conv_len, unsigned char type );
void bcd2asc(unsigned char *ascii_buf,unsigned char * bcd_buf, int conv_len, unsigned char type);
 int CalcXOR_none(uint8_t *datas, int len,int sl);
 int CalcXOR(uint8_t *datas, int len,int sl);
char CheckSum8(char *buf,int len);
#endif /* __APP_H */

