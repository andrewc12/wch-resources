/*; CH375/CH372/CH451 EVT
; U2(AT89C51) Program
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.12
;
;****************************************************************************
*/

/* MCS-51��Ƭ��C���Ե�ʾ������ */

#include <reg52.h>
#include <string.h>
#include "..\..\MCU_IF1\MCS51C\CH375INC.H"

typedef	struct	_COMMAND_PACKET	{	/* �Զ����������ṹ */
	unsigned char	mCommandCode;		/* ����������,������Ķ��� */
	unsigned char	mCommandCodeNot;	/* ������ķ���,����У������� */
	union	{
		unsigned char	mParameter[5];	/* ���� */
		struct {
			unsigned char	mBufferID;  /* ������ʶ����,���������MCS51��Ƭ������: 1-ר�ù��ܼĴ���SFR, 2-�ڲ�RAM, 3-�ⲿRAM, ����������ʵ��ֻ��ʾ�ڲ�RAM */
			unsigned int	mBufferAddr;	/* ��д��������ʼ��ַ,Ѱַ��Χ��0000H-0FFFFH,���ֽ���ǰ */
			unsigned int	mLength;	/* ���ݿ��ܳ���,���ֽ���ǰ */
		} buf;
	} u;
}	mCOMMAND_PACKET,	*mpCOMMAND_PACKET;

#define		CONST_CMD_LEN		0x07	/* �����ĳ��� */
/* �������������ݶ���ͨ�������´��ܵ�(USB�˵�2��OUT)�´�, Ϊ�˷�ֹ���߻���,
 ���ǿ����ڼ����Ӧ�ó����뵥Ƭ������֮��Լ��, �����ĳ�������7, �����ݿ�ĳ��ȿ϶�����7, ����64,32��
 ����, ����Լ��, ���������ֽ���������, �ȵ�
 ������Լ��������: 80H-0FFH��ͨ������,�����ڸ���Ӧ��
                   00H-7FH��ר������,��Ը���Ӧ���ر��� */
/* ͨ������ */
#define		DEF_CMD_GET_INFORM		0x90	/* ��ȡ��λ����˵����Ϣ,���Ȳ�����64���ַ�,�ַ�����00H���� */
#define		DEF_CMD_TEST_DATA		0x91	/* ��������,��λ����PC�����������������������ȡ���󷵻� */
#define		DEF_CMD_CLEAR_UP		0xA0	/* ���ϴ����ݿ�֮ǰ����ͬ��,ʵ��������λ������ϴ����������������� */
#define		DEF_CMD_UP_DATA			0xA1	/* ����λ����ָ����ַ�Ļ������ж�ȡ���ݿ�(�ϴ����ݿ�) */
#define		DEF_CMD_DOWN_DATA		0xA2	/* ����λ����ָ����ַ�Ļ�������д�����ݿ�(�´����ݿ�) */
/* ר������ */
#define		DEMO_CH451_CMD			0x56	/* PC���������CH451,������ʾCH451�Ĺ��� */
/* ����MCS51��Ƭ����ʹ��ͨ������ʱ,����Ҫָ��������ʶ���� */
#define		ACCESS_MCS51_SFR		1		/* ��д51��Ƭ����SFR */
#define		ACCESS_MCS51_IRAM		2		/* ��д51��Ƭ�����ڲ�RAM */
#define		ACCESS_MCS51_XRAM		3		/* ��д51��Ƭ�����ⲿRAM */

unsigned char volatile xdata CH375_CMD_PORT _at_ 0xBDF1;		/* CH375����˿ڵ�I/O��ַ */
unsigned char volatile xdata CH375_DAT_PORT _at_ 0xBCF0;		/* CH375���ݶ˿ڵ�I/O��ַ */

/* �й�CH451�Ķ���,��ʾ������ӷ�ʽ */
sbit   CH451_dclk=P1^7;      /* ��������ʱ�������Ӽ��� */
sbit   CH451_din=P1^6;	     /* �����������,��CH451���������� */
sbit   CH451_load=P1^5;      /* �����������,�����Ӽ��� */
sbit   CH451_dout=P3^3;      /* INT1,�����жϺͼ�ֵ��������,��CH451��������� */
unsigned char CH451_KEY;				/* ��ż����ж��ж�ȡ�ļ�ֵ */
unsigned char LAST_KEY;					/* �����ϴεļ�ֵ */

mCOMMAND_PACKET	CMD_PKT;				/* ������ṹ������ */
unsigned char data *CurrentRamAddr;		/* �������ݿ鴫��ʱ���汻��д�Ļ���������ʼ��ַ */
unsigned char CurrentRamLen;			/* �������ݿ鴫��ʱ����ʣ�೤�� */
bit		FLAG_INT_WAIT;		/* �жϵȴ���־,1ָʾ���ж���������CH375�еȴ����� */
unsigned char CH451_CMD_H;	/* PC������CH451�ĸ�4λ����,Ϊ0FFH��������Ч */
unsigned char CH451_CMD_L;  /* PC������CH451�ĵ�8λ���� */
unsigned char code InformString[16] = "CH375/CH451\x0";	/* ��Ϣ�ַ��� */

/* ��ʱ2΢��,����ȷ */
void	Delay2us( )
{
	unsigned char i;
#define DELAY_START_VALUE	1  /* ���ݵ�Ƭ����ʱ��ѡ���ֵ,20MHz����Ϊ0,30MHz����Ϊ2 */
	for ( i=DELAY_START_VALUE; i!=0; i-- );
}

/* ��ʱ50����,����ȷ */
void	Delay50ms( )
{
	unsigned char i, j;
	for ( i=200; i!=0; i-- ) for ( j=250; j!=0; j-- );
}

/* ��PC���ĵ��ֽ���ǰ��16λ������ת��ΪC51�ĸ��ֽ���ǰ������ */
unsigned int	BIG_ENDIAN( unsigned int value )
{
	unsigned int  in, out;
	in = value;
	((unsigned char *)&out)[1] = ((unsigned char *)&in)[0];
	((unsigned char *)&out)[0] = ((unsigned char *)&in)[1];
	return( out );
}

/* CH375��ʼ���ӳ��� */
void	CH375_Init( )
{
	unsigned char i;
	FLAG_INT_WAIT = 0;  /* �巢���жϵȴ���־ */
/* ����CH375�Ƿ���������,��ѡ����,ͨ������Ҫ */
#ifdef TEST_CH375_FIRST
	CH375_CMD_PORT = CMD_CHECK_EXIST;  /* ����CH375�Ƿ��������� */
	Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
	CH375_DAT_PORT = 0x55;  /* д��������� */
	Delay2us( );
	i = ~ 0x55;  /* ��������Ӧ���ǲ�������ȡ�� */
	if ( CH375_DAT_PORT != i ) {  /* CH375������ */
		for ( i=80; i!=0; i-- ) {
			CH375_CMD_PORT = CMD_RESET_ALL;  /* ����ظ�������,ִ��Ӳ����λ */
			Delay2us( );
		}
		CH375_CMD_PORT = 0;
		Delay50ms( );  /* ��ʱ50ms */
	}
#endif
#ifdef USE_MY_USB_ID
/* �����ⲿ�Զ����USB�豸VID��PID,��ѡ����,��ִ�и�������ʹ��Ĭ�ϵ�VID��PID */
	CH375_CMD_PORT = CMD_SET_USB_ID;  /* �����ⲿ�Զ����USB�豸VID��PID,��ѡ���� */
	Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
	CH375_DAT_PORT = (unsigned char)MY_USB_VENDOR_ID;  /* д�볧��ID�ĵ��ֽ� */
	CH375_DAT_PORT = (unsigned char)(MY_USB_VENDOR_ID>>8);  /* д�볧��ID�ĸ��ֽ� */
	CH375_DAT_PORT = (unsigned char)MY_USB_DEVICE_ID;  /* д���豸ID�ĵ��ֽ� */
	CH375_DAT_PORT = (unsigned char)(MY_USB_DEVICE_ID>>8);  /* д���豸ID�ĸ��ֽ� */
	Delay2us( );
#endif
/* ����USB����ģʽ, ��Ҫ���� */
	CH375_CMD_PORT = CMD_SET_USB_MODE;
	Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
	CH375_DAT_PORT = 2;  /* ����Ϊʹ�����ù̼���USB�豸��ʽ */
	for ( i=100; i!=0; i-- ) {  /* �ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS */
		if ( CH375_DAT_PORT==CMD_RET_SUCCESS ) break;
	}
/*	if ( i==0 ) { CH372/CH375����Ӳ������ }; */
/* ���������ж�,�ٶ�CH375������INT0 */
	IT0 = 0;  /* ���ⲿ�ź�Ϊ�͵�ƽ���� */
	IE0 = 0;  /* ���жϱ�־ */
	EX0 = 1;  /* ����CH375�ж� */
}

/* �����ϴ����� */
void	LoadUpData( unsigned char data *Buf, unsigned char Len )
{
	unsigned char i;
	CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* ��USB�˵�2�ķ��ͻ�����д�����ݿ� */
	Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
	CH375_DAT_PORT = Len;  /* ����д��������ݳ��� */
	for ( i=0; i<Len; i++ ) CH375_DAT_PORT = Buf[i];  /* �������� */
}

/* CH375�жϷ������INT0,ʹ�üĴ�����1 */
void	mCH375Interrupt( ) interrupt 0 using 1
{
	unsigned char InterruptStatus;
	unsigned char length, c1, len1, len2, i;
#define		cmd_buf		((unsigned char data *)(&CMD_PKT))
	CH375_CMD_PORT = CMD_GET_STATUS;  /* ��ȡ�ж�״̬��ȡ���ж����� */
	Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
	InterruptStatus = CH375_DAT_PORT;  /* ��ȡ�ж�״̬ */
	IE0 = 0;  /* ���жϱ�־,��Ӧ��INT0�ж� */
	if ( InterruptStatus == USB_INT_EP2_OUT ) {  /* �����˵��´��ɹ� */
		CH375_CMD_PORT = CMD_RD_USB_DATA;  /* �ӵ�ǰUSB�жϵĶ˵㻺������ȡ���ݿ�,���ͷŻ����� */
		Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
		length = CH375_DAT_PORT;  /* ���ȶ�ȡ�������ݳ��� */
		if ( length == CONST_CMD_LEN ) {  /* ����鳤������CONST_CMD_LEN,�������������� */
/* ����ͨ��USB���յ��������,��������CONST_CMD_LEN,���ֽ�Ϊ����,����Ϊ��ѡ�Ĳ���,��������ṹ���ɵ�Ƭ���ͼ����Ӧ�ò�֮�����ж���� */
			for ( i=0; i<CONST_CMD_LEN; i++ ) cmd_buf[i] = CH375_DAT_PORT;  /* ��������������� */
			if ( CMD_PKT.mCommandCode != (unsigned char)( ~ CMD_PKT.mCommandCodeNot ) ) return;  /* ���������У����� */
			switch ( CMD_PKT.mCommandCode ) {  /* ����������,switch�����ö��if/else���� */
				case DEF_CMD_GET_INFORM:  /* ��ȡ��λ����˵����Ϣ,���Ȳ�����64���ַ�,�ַ�����00H���� */
					CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* ��USB�˵�2�ķ��ͻ�����д�����ݿ� */
					Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
					CH375_DAT_PORT = 16;  /* ����д��������ݳ��� */
					for ( i=0; i<16; i++ ) CH375_DAT_PORT = InformString[i];  /* �������� */
					break;
				case DEF_CMD_TEST_DATA:  /* ��������,��λ����PC�����������������������ȡ���󷵻� */
					CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* ��USB�˵�2�ķ��ͻ�����д�����ݿ� */
					Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
					CH375_DAT_PORT = CONST_CMD_LEN;  /* ����д��������ݳ��� */
					for ( i=0; i<CONST_CMD_LEN; i++ ) CH375_DAT_PORT = ~ cmd_buf[i];  /* ��������,����ȡ���󷵻�,�ɼ����Ӧ�ó�����������Ƿ���ȷ */
					break;
				case DEF_CMD_CLEAR_UP:  /* ���ϴ����ݿ�֮ǰ����ͬ��,ʵ��������λ������ϴ����������������� */
/* �����ϴ����ݿ�֮ǰ����ͬ��,ʵ�����õ�Ƭ������ϴ�����������������
; �����һ�ν��������ϴ�ʱ,�������ǰ�����ϴ�,��ô�п������ϴ�������������������,�����ڵڶ����ϴ�ǰ��Ҫ����ϴ������� */
					CH375_CMD_PORT = CMD_SET_ENDP7;  /* ����USB�˵�2��IN,Ҳ���������ϴ��˵� */
					Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
					CH375_DAT_PORT = 0x0e;  /* ͬ������λ����,����USB�˵�2��IN��æ,����NAK */
					break;
				case DEF_CMD_UP_DATA:  /* ����λ����ָ����ַ�Ļ������ж�ȡ���ݿ�(�ϴ����ݿ�) */
/* �����ϴ����ݿ�, ������ʵ��ֻ��ʾ�ڲ�RAM */
/*					switch ( CMD_PKT.u.buf.mBufferID ) {
						case ACCESS_MCS51_SFR:  ��д51��Ƭ����SFR
						case ACCESS_MCS51_IRAM:  ��д51��Ƭ�����ڲ�RAM
						case ACCESS_MCS51_XRAM:  ��д51��Ƭ�����ⲿRAM
					} */
					CurrentRamAddr = (unsigned char)BIG_ENDIAN( CMD_PKT.u.buf.mBufferAddr );  /* ��ʼ��ַ,�����ڲ�RAMֻ�õ�8λ��ַ */
					CurrentRamLen = (unsigned char)BIG_ENDIAN( CMD_PKT.u.buf.mLength );  /* ���ݿ鳤��,�����ڲ�RAM�ܳ��Ȳ����ܳ���256 */
					len1 = CurrentRamLen >= CH375_MAX_DATA_LEN ? CH375_MAX_DATA_LEN : CurrentRamLen;  /* �����ϴ�,׼����һ������ */
					LoadUpData( CurrentRamAddr, len1);  /* �����ϴ����� */
					CurrentRamLen -= len1;
					CurrentRamAddr += len1;
					break;
				case DEF_CMD_DOWN_DATA:  /* ����λ����ָ����ַ�Ļ�������д�����ݿ�(�´����ݿ�) */
/* �����´����ݿ�, ������ʵ��ֻ��ʾ�ⲿRAM */
					CurrentRamAddr = BIG_ENDIAN( CMD_PKT.u.buf.mBufferAddr );  /* ��ʼ��ַ */
					CurrentRamLen = BIG_ENDIAN( CMD_PKT.u.buf.mLength );  /* ���ݿ鳤�� */
					break;
				case DEMO_CH451_CMD:  /* PC���������CH451,������ʾCH451�Ĺ��� */
/*; Ϊ�˷�ֹ��CH375�жϷ���������������е�CH451_READ��ִ��CH451_WRITE��������
; �����ڴ˱���CH451�����������������ڿ���ʱ����CH451 */
					CH451_CMD_L = CMD_PKT.u.mParameter[1];  /* ��8λ���� */
					CH451_CMD_H = CMD_PKT.u.mParameter[2];  /* ��4λ���� */
					break;
				default:
					break;
			}
		}
		else if ( length == 0 ) return;  /* ����Ϊ0,û��������ֱ���˳�,��ĳЩӦ����Ҳ���Խ�����0����Ϊһ���������� */
		else {  /* ��������� */
/* �����´������ݿ�,ÿ�����ݵĳ��Ȳ�����64�ֽ�,�����ܳ���150���ֽ�,���1��͵�2�����64�ֽ�,��3����ʣ�೤��22�ֽ�
; Ϊ�˽����ݿ�����������ֿ���,ʣ�೤�Ȳ��ܵ���CONST_CMD_LEN,������������ݵİ취������ͬ */
			if ( CMD_PKT.mCommandCode == DEF_CMD_DOWN_DATA ) {  /* ����λ����ָ����ַ�Ļ�������д�����ݿ�(�´����ݿ�) */
/* �����´������ݿ�,ÿ�����ݵĳ��Ȳ�����64�ֽ�,������ʾ����û���ⲿRAM,�����ⲿRAM��Ч�ʺܵ�,�������ڲ�RAMʾ�� */
				CurrentRamLen -= length;
				while ( length-- ) {
					*CurrentRamAddr = CH375_DAT_PORT;
					CurrentRamAddr ++;
				}
			}
			else {  /* δ��������� */
				while ( length -- ) c1 = CH375_DAT_PORT;  /* �������� */
			}
		}
	}
	else if ( InterruptStatus == USB_INT_EP2_IN ) {  /* �������ݷ��ͳɹ� */
		if ( CMD_PKT.mCommandCode == DEF_CMD_UP_DATA ) {  /* ����λ����ָ����ַ�Ļ������ж�ȡ���ݿ�(�ϴ����ݿ�) */
			len2 = CurrentRamLen >= CH375_MAX_DATA_LEN ? CH375_MAX_DATA_LEN : CurrentRamLen;  /* �����ϴ�,׼������ */
			LoadUpData( CurrentRamAddr, len2 );  /* �����ϴ����� */
			CurrentRamLen -= len2;
			CurrentRamAddr += len2;
		}
/* �յ��ϴ��ɹ��жϺ�,���˳�֮ǰ�������USB������,�Ա�����շ����� */
		CH375_CMD_PORT = CMD_UNLOCK_USB;  /* �ͷŵ�ǰUSB������ */
	}
	else if ( InterruptStatus == USB_INT_EP1_IN ) {  /* �ж����ݷ��ͳɹ� */
		CH375_CMD_PORT = CMD_UNLOCK_USB;  /* �ͷŵ�ǰUSB������ */
		FLAG_INT_WAIT = 0;  /* �巢���жϵȴ���־,֪ͨӦ�ó�����Լ��������ж����� */
	}
	else {  /* ���ù̼���USB��ʽ�²�Ӧ�ó��������ж�״̬ */
	}
}

/* �ϴ��ж������ӳ���(ͨ���ж϶˵��ϴ�),��ѡ�ӳ��� */
void	LoadIntData( unsigned char c1, unsigned char c2 )
{
	unsigned int i;
	for ( i=1000; i!=0; i-- ) {  /* �ȴ������ж��������,��ΪPC��ÿ��1����Ͷ�ȡ�ж�����,�������ȴ�1���� */
		if ( FLAG_INT_WAIT == 0 ) break;  /* ǰ���ж������Ѿ���PC��ȡ�� */
	}
/* ���δ����������,���߼�����е�Ӧ�ó���δ����,�����ֳ�ʱ,���򲻳���1�����ж����ݾ�Ӧ�ñ�PC��ȡ�� */
	EX0 = 0;  /* Ϊ�˷�ֹ��;���ж϶�����˳��,�����Ƚ�ֹ�ж� */
	CH375_CMD_PORT = CMD_WR_USB_DATA5;  /* ��USB�˵�1�ķ��ͻ�����д�����ݿ� */
	Delay2us( );
	CH375_DAT_PORT = 2;  /* �������ݳ��� */
/* ��Ȼÿ�ο����ϴ�8���ֽ�, ���Ǳ����������ڼ����ж�, ����ÿ��ֻ��Ҫ���������ֽ� */
	FLAG_INT_WAIT = 1;  /* ���жϵȴ���־ */
	CH375_DAT_PORT = c1;  /* �����ж�����1 */
	CH375_DAT_PORT = c2;  /* �����ж�����2 */
	EX0 = 1;  /* �����ж� */
}

/* CH451��ʼ���ӳ��� */
void	CH451_Init( )
{
	CH451_din=0;         /* �ȵͺ��,ѡ��4������ */
	CH451_din=1;
	IT1 =0;  /* ���ⲿ�ź�Ϊ�͵�ƽ���� */
	IE1 =0;  /* ���жϱ�־ */
	EX1 =1;  /* ���������ж� */
}

/* CH451��������ӳ��� */
/* ����һ�޷������ͱ����洢12�ֽڵ������� */
void	CH451_Write( unsigned int command )
{
  unsigned char i;
  EX1 = 0;  /* ��ֹ�����ж� */
  CH451_load=0;  /* ���ʼ */
  for( i=0; i<12; i++ ) {  /* ����12λ����,��λ��ǰ */
    CH451_din = command & 1;
    CH451_dclk = 0;
    command >>= 1;
    CH451_dclk = 1;  /* ��������Ч,����ʱ����������֪ͨCH451����λ���� */
  }
  CH451_load = 1;  /* ��������,��������������֪ͨCH451������������ */
  EX1 = 1;  /* ���������ж� */
}

/* ��ȡCH451��ֵ�ӳ��� */
unsigned char CH451_Read( )
{
  unsigned char i;
  unsigned char command, keycode;
  EX1 = 0;  /* ��ֹ�����ж� */
  command=0x07;  /* �����451������,ֻ��Ҫ��4λ,��8λ��ʡȥ */
  CH451_load=0;  /* ���ʼ */
  for( i=0; i<4; i++ ){  /* ����4λ����,��λ��ǰ */
    CH451_din = command & 1;
    CH451_dclk = 0;
    command >>= 1;
    CH451_dclk = 1;  /* ��������Ч,����ʱ����������֪ͨCH451����λ���� */
  }
  CH451_load = 1;  /* ��������,��������������֪ͨCH451������������ */
  keycode=0;
  for( i=0; i<7; i++ ){  /* �����ֵ,7λ */
    keycode<<=1;  /* ��������keycode,��λ��ǰ,��λ�ں� */
    keycode|=CH451_dout;  /* �Ӹߵ��Ͷ���451������ */
    CH451_dclk=0; /* ����ʱ��������֪ͨCH451�����һλ */
    CH451_dclk=1;
  }
  IE1=0;  /* ���жϱ�־,�Ƕ�ȡʱDOUT����͵�ƽ������ */
  EX1=1;
  return( keycode );  /* ���ؼ�ֵ */
}

/* CH451�����жϷ������INT1,ʹ�üĴ�����1 */
void	mCH451Interrupt( ) interrupt 2 using 1
{
  unsigned char i, command;
  command=0x07;  /* �����451������,ֻ��Ҫ��4λ,��8λ��ʡȥ */
  CH451_load=0;  /* ���ʼ */
  for( i=0; i<4; i++ ){  /* ����4λ����,��λ��ǰ */
    CH451_din = command & 1;
    CH451_dclk = 0;
    command >>= 1;
    CH451_dclk = 1;  /* ��������Ч,����ʱ����������֪ͨCH451����λ���� */
  }
  CH451_load = 1;  /* ��������,��������������֪ͨCH451������������ */
  CH451_KEY=0;
  for( i=0; i<7; i++ ){  /* �����ֵ,7λ */
    CH451_KEY<<=1;  /* ��������,��λ��ǰ,��λ�ں� */
    CH451_KEY|=CH451_dout;  /* �Ӹߵ��Ͷ���451������ */
    CH451_dclk=0; /* ����ʱ��������֪ͨCH451�����һλ */
    CH451_dclk=1;
  }
  IE1=0;  /* ���жϱ�־ */
}

/* �ɵ�Ƭ��������ʾ,�ȴ�����,Ȼ��������ԭ��ʾ,�ټ��ϰ���ֵ */
void	DEMO_CH451_ONLY( )
{
	unsigned char key;
	CH451_Write( 0x0f00 | 0x17 );  /* ���1������ܼ�������'H' */
	CH451_Write( 0x0e00 | 0x0e );  /* ���1������ܼ�������'E' */
	CH451_Write( 0x0d00 | 0x18 );  /* ���1������ܼ�������'L' */
	CH451_Write( 0x0c00 | 0x19 );  /* ���1������ܼ�������'P' */
	CH451_Write( 0x0b00 | 0x10 );  /* ���1������ܼ�������' ' */
	CH451_Write( 0x0a00 | 0x14 );  /* ���1������ܼ�������'[' */
	CH451_Write( 0x0900 | 0x88 );  /* ���1������ܼ�������'8.' */
	CH451_Write( 0x0800 | 0x15 );  /* ���1������ܼ�������']' */
	CH451_Write( 0x0600 | 0x30 );  /* ��3���͵�4���������˸ */
	while ( 1 ) {  /* ������ʾΪ��ʾ��������,�������� */
		if ( CH451_KEY != 0xff ) {  /* ��⵽�µİ���,��������ʾ����ʾ��ֵ */
			key = CH451_KEY;
			CH451_KEY = 0xff;  /* ȡ����ֵ�����ԭֵ */
			CH451_Write( 0x0300 );  /* ����һλ */
			CH451_Write( 0x0300 );  /* ����һλ */
			key &= 0x3f;     /* ��ֵ0-63 */
			CH451_Write( 0x0900 | ( key / 10 ) );  /* ��7���������ʾ��ֵ��ʮλ�� */
			CH451_Write( 0x0800 | ( key % 10 ) );  /* ��8���������ʾ��ֵ�ĸ�λ�� */
		}
	}
}

/* ��PC��ͨ��USB��ʾCH451�Ĺ���,��֤USBͨѶ */
void	DEMO_USB( )
{
	unsigned int  ch451cmd;
	TR0 = 1;  /* �ɶ�ʱ��0���ɼ���ֵ����α����� */
	CH451_CMD_H = 0xff;  /* ���CH451�������� */
	while ( 1 ) {  /* ����ָ�ʼ����ѭ��,�ȴ�PC��������в��� */
		if ( CH451_CMD_H == 0xff ) {  /* û��CH451������, ���¼�ⰴ��״̬ */
/*    LAST_KEY��CH451_KEY��������,����0FFH��û�м����� */
/*    ��CH451_KEYΪ��Ч��ֵ���Ǽ��հ���, ���߶�����Ч��ֵ������ͬ���Ѿ�֪ͨ������� */
			if ( LAST_KEY == 0xff ) {  /* ����δ���»����Ѱ��µ�����δ֪ͨ��PC�� */
				if ( CH451_KEY != 0xff && CH451_KEY >= 0x40 ) {  /* ��⵽�µļ���,��������Ч���ж���������,��֪ͨPC��Ӧ�ò� */
					LAST_KEY = CH451_KEY;
/* �趨�ж���������01H,֪ͨPC����Ӧ�ò�,�����Ѱ���, �ж����ݵĴ��ֽھ��Ǽ���,����PC��Ӧ�ò�ֱ�ӻ�ȡ��ֵ,�����ٶ�ȡ */
					LoadIntData( 1, LAST_KEY );  /* �ϴ��ж�����,���������ڼ����ж�,����ÿ��ֻ��Ҫ�������ֽ� */
				}
			}
			else {	/* �����Ѿ����²����Ѿ�֪ͨ��PC��,���Լ�ⰴ���ͷ� */
				CH451_KEY = CH451_Read( );  /* ���¶�ȡ���һ����Ч�����ļ�ֵ */
				if ( CH451_KEY != LAST_KEY ) {  /* ��ֵ��ͬ,�����Ѿ��ͷŻ����¼�����,���������ͬ,˵��������Ȼû���ͷ� */
/* �趨�ж���������02H,֪ͨPC����Ӧ�ò�,�������ͷ�, �ж����ݵĴ��ֽھ��Ǽ���,����PC��Ӧ�ò�ֱ�ӻ�ȡ��ֵ */
					LoadIntData( 2, LAST_KEY ); /* �ϴ��ж����� */
					LAST_KEY = 0xff;  /* �Ѿ������ͷ��źŸ�PC */
				}
			}
		}
		else {  /* ��CH375�жϷ�������н��յ�PC������CH451������ */
/* PC���������CH451,�����������е�CH451_READ���ܻᱻCH375�ĸ����ȼ��ж�,���Բ�����CH375�жϷ��������ִ�� */
			ch451cmd = ( (unsigned int)CH451_CMD_H << 8 ) + CH451_CMD_L;  /* ��4λ�����8λ���� */
			CH451_CMD_H = 0xff;  /* ���ԭ������,��ֹ�ط� */
			CH451_Write( ch451cmd );  /* ��CH451�������� */
		}
	}
}

main( ) {
	Delay50ms( );	/* ��ʱ�ȴ�CH375��ʼ�����,�����Ƭ����CH375�ṩ��λ�ź��򲻱���ʱ */
	CH375_Init( );  /* ��ʼ��CH375 */
	CH451_Init( );  /* ��ʼ��CH451 */
/* ����CH451����ʾ�����ͼ���ɨ�� */
	CH451_Write( 0x0403 );  /* ����ϵͳ��������,ʹ����ʾ����,ʹ�ܼ��̹��� */
	CH451_Write( 0x058C );  /* ������ʾ����,BCD���뷽ʽ,12������ */
/* ������CH451�������̹���ǰ�Ĵ�����������п��ܲ��������ж�,������Ҫ�ٴ�ȡ���ж� */
	IE1 = 0;
	CH451_KEY = 0xff;  /* �����������,��ʹ�ܼ���ǰ,CH451��DOUT����������� */
	LAST_KEY = 0xff;  /* �п��ܱ����������ж�����INT1,����Ҫ����� */
	EA = 1;  /* �����ж� */
/* ���P2�ĵ�5�ź͵�6��֮����϶�·�����ɵ�Ƭ��������ʾ,������PC��ͨ��USB���ƽ�����ʾ */
	if ( T1 ) DEMO_USB( );  /* T1,P3.5ΪĬ�ϵĸߵ�ƽ,��PC��ͨ��USB��ʾ */
	else DEMO_CH451_ONLY( );  /* �ɵ�Ƭ��������ʾ,�ȴ�����,Ȼ��������ԭ��ʾ,�ټ��ϰ���ֵ */
}