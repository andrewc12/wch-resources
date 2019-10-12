/* 2004.03.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB 1.1 Host Examples for CH375   **
**  KC7.0@MCS-51                      **
****************************************
*/
/* CH375��ΪUSB�����ӿڵĳ���ʾ��,�жϷ�ʽ */

/* MCS-51��Ƭ��C���Ե�ʾ������, U�����ݶ�д */

#include <reg51.h>
#include <string.h>
#include <stdio.h>

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif

/* ����CH375������뼰����״̬ */
#include "CH375INC.H"

/* ���¶���������MCS-51��Ƭ��,������Ƭ�������޸�,Ϊ���ṩC���Ե��ٶ���Ҫ�Ա���������Ż� */
#include <reg51.h>
unsigned char volatile xdata	CH375_CMD_PORT _at_ 0xBDF1;	/* CH375����˿ڵ�I/O��ַ */
unsigned char volatile xdata	CH375_DAT_PORT _at_ 0xBCF0;	/* CH375���ݶ˿ڵ�I/O��ַ */
unsigned char xdata				DATA_BUFFER[8192] _at_ 0x0000;	/* �ⲿRAM���ݻ���������ʼ��ַ,���Ȳ�����һ�ζ�д�����ݳ��� */

/* ��P1.4����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,��U�̲������ */
sbit P1_4  = P1^4;
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 �͵�ƽ����LED��ʾ */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 �͵�ƽ����LED��ʾ */

unsigned char volatile			UdiskStatus;	/* ��ǰU��״̬,�������� */
#define		STATUS_DISCONNECT		0	/* U����δ���ӻ����Ѿ��Ͽ� */
#define		STATUS_CONNECT			1	/* U�̸ո����� */
#define		STATUS_ERROR			2	/* U�̲���������߲�֧�� */
#define		STATUS_WAIT				3	/* U�����ڲ��� */
#define		STATUS_READY			4	/* U��׼���ý��ܲ��� */

/*
  ������USB�ƶ��洢�豸�ļ�Ӧ�òο����ڵ�Ƭ��Ӧ�ó����У�
  ���Զ���USB�洢�豸��4��״̬���ѶϿ��������ӡ���ʼ�����ɶ�д������"������"״̬��ѡ��
�� �ѶϿ���ָUSB�洢�豸�Ѿ��Ͽ�����USB�����аγ�������ȫ�����ã�
�� ��������ָUSB�洢�豸�Ѿ����ӣ�����USB�����У���������δ��ʼ����
�� ��ʼ����ָUSB�洢�豸���ڳ�ʼ�����߳�ʼ��ʧ�ܣ����Բ����Խ��ж�д��
�� �ɶ�д��ָUSB�洢�豸��ʼ���ɹ������Խ������ݶ�д��

��Ƭ������CH375�жϵ�һ�㲽�����£�
�� CH375��INT#��������Ϊ�͵�ƽ����Ƭ�������жϣ�
�� ��Ƭ�������жϷ����������ִ��GET_STATUS�����ȡ�ж�״̬��
�� CH375��GET_STATUS������ɺ�INT#���Żָ�Ϊ�ߵ�ƽ��ȡ���ж�����
�� ��Ƭ�����Բο������˳�����GET_STATUS�����ȡ���ж�״̬��
  �� ����ж�״̬��USB_INT_DISCONNECT����˵��USB�豸�Ѿ��Ͽ�����USB�洢�豸״̬Ϊ"�ѶϿ�"��
     ����ڴ�֮ǰUSB�洢�豸�ж�д������δ��ɣ������������������ʧ�ܴ�����Ȼ���˳��жϣ���Ҫʱ����֪ͨ��Ƭ��������
  �� ����ж�״̬��USB_INT_CONNECT����˵��USB�豸�Ѿ����ӣ���USB�洢�豸״̬Ϊ"������"��
     ���ŷ���DISK_INIT�������USB�洢�豸״̬Ϊ��ʼ����Ȼ���˳��жϣ���Ҫʱ����֪ͨ��Ƭ��������
  �� �����ǰ��USB�洢�豸״̬��"��ʼ��"��
    �� ����ж�״̬��USB_INT_SUCCESS����˵��USB�洢�豸��ʼ���ɹ�����USB�洢�豸״̬Ϊ"�ɶ�д"��
       Ȼ���˳��жϣ���Ҫʱ����֪ͨ��Ƭ��������
    �� ����ж�״̬������״̬����˵��USB�洢�豸��ʼ��ʧ�ܣ�Ӧ��֪ͨ��Ƭ��������
       ��ʾ��USB�豸���Ǵ洢�豸���߸�USB�豸��֧�֣�Ȼ���˳��жϡ����ߣ���Ƭ��ͨ�������������д�����USB�洢�豸��ͨѶЭ�顣
  �� �����ǰ��USB�洢�豸״̬��"�ɶ�д"������Ա����ж�״̬��֪ͨ��Ƭ������������
     Ȼ��ֱ���˳��жϣ����ߣ����жϷ�������м���������
    �� ����ж�״̬��USB_INT_DISK_READ����˵�����ڽ���USB�洢�豸�Ķ���������Ҫȡ��64���ֽڵ����ݣ�
       ���Է���RD_USB_DATA����ȡ�����ݣ������ٷ���DISK_RD_GO����ʹCH375��������Ȼ���˳��жϡ�
    �� ����ж�״̬��USB_INT_DISK_WRITE����˵�����ڽ���USB�洢�豸��д��������Ҫ�ṩ64���ֽڵ����ݣ�
       ���Է���WR_USB_DATA7�����ṩ���ݣ������ٷ���DISK_WR_GO����ʹCH375����д��Ȼ���˳��жϡ�
    �� ����ж�״̬��USB_INT_SUCCESS����˵����д�����ɹ���ֱ���˳��жϲ�֪ͨ������ò����ɹ���
    �� ����ж�״̬��USB_INT_DISK_ERR����˵����д����ʧ�ܣ�ֱ���˳��жϲ�֪ͨ������ò���ʧ�ܣ�
    �� ͨ�����᷵�������ж�״̬������У���˵�����ִ���
  �� ͨ������������USB�洢�豸״̬�·��������ж�״̬������У���˵�����ִ��󣬿��Բ���USB�豸�Ͽ���״̬������

����Ƭ����������Ҫ��USB�洢�豸�ж�д����ʱ�����Բ�ѯUSB�洢�豸״̬��
�����"�ɶ�д"״̬������Է���DISK_READ��������ݣ����߷���DISK_WRITE����д���ݡ�
���������ݶ�д���̿����ڵ�Ƭ�����жϷ����������ɣ�Ҳ�������жϳ����ñ�־֪ͨ�ȴ��е�������
������������ж�״̬��������ݶ�д���̡�
*/

unsigned char *mBufferPoint;

/* ��ʱ2΢��,����ȷ */
void	delay2us( )
{
	unsigned char i;
	for ( i = 2; i != 0; i -- );
}

/* ��ʱ1΢��,����ȷ */
void	delay1us( )
{
	unsigned char i;
	for ( i = 1; i != 0; i -- );
}

/* ��ʱ100����,����ȷ */
void	mDelay100mS( )
{
	unsigned char	i, j, c;
	for ( i = 200; i != 0; i -- ) for ( j = 200; j != 0; j -- ) c+=3;
}

/* �������� */

void CH375_WR_CMD_PORT( unsigned char cmd ) {  /* ��CH375������˿�д������,���ڲ�С��4uS,�����Ƭ���Ͽ�����ʱ */
	delay2us();
	CH375_CMD_PORT=cmd;
	delay2us();
}

void CH375_WR_DAT_PORT( unsigned char dat ) {  /* ��CH375�����ݶ˿�д������,���ڲ�С��1.5uS,�����Ƭ���Ͽ�����ʱ */
	CH375_DAT_PORT=dat;
	delay1us();  /* ��ΪMCS51��Ƭ����������ʵ����������ʱ */
}

unsigned char CH375_RD_DAT_PORT() {  /* ��CH375�����ݶ˿ڶ�������,���ڲ�С��1.5uS,�����Ƭ���Ͽ�����ʱ */
	delay1us();  /* ��ΪMCS51��Ƭ����������ʵ����������ʱ */
	return( CH375_DAT_PORT );
}

/* CH375��INT#��������51��Ƭ����INT0����, �����жϷ�ʽ */
/* CH375�жϷ������,ʹ�üĴ�����1 */
void	CH375Interrupt( ) interrupt 0 using 1
{
	unsigned char i, s, len;
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* ��ȡ�ж�״̬��ȡ���ж����� */
	for ( i = 2; i != 0; i -- );  /* ������ʱ2uS */
	s = CH375_RD_DAT_PORT( );  /* ��ȡ�ж�״̬ */
	if ( s == USB_INT_SUCCESS ) UdiskStatus = STATUS_READY;  /* �����ɹ� */
	else if ( s == USB_INT_DISCONNECT ) {
		UdiskStatus = STATUS_DISCONNECT;  /* ��⵽USB�豸�Ͽ��¼� */
		LED_OUT_INACT( );
	}
	else if ( s == USB_INT_CONNECT ) {
		UdiskStatus = STATUS_CONNECT;  /* ��⵽USB�豸�����¼� */
		LED_OUT_ACT( );
	}
	else if ( s == USB_INT_DISK_READ ) {  /* USB�洢�������ݿ�,�������ݶ��� */
		CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* ��CH375��������ȡ���ݿ� */
		for ( i = 2; i != 0; i -- );  /* ������ʱ2uS */
		len = CH375_RD_DAT_PORT( );  /* �������ݵĳ��� */
		while ( len ) {  /* ���ݳ��ȶ�ȡ���� */
			*mBufferPoint = CH375_RD_DAT_PORT( );  /* �������ݲ����� */
			mBufferPoint ++;
			len --;
		}
		CH375_WR_CMD_PORT( CMD_DISK_RD_GO );  /* ����ִ��USB�洢���Ķ����� */
	}
	else if ( s == USB_INT_DISK_WRITE ) {  /* USB�洢��д���ݿ�,��������д�� */
		CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* ��CH375������д�����ݿ� */
		for ( i = 2; i != 0; i -- );  /* ������ʱ2uS */
		len = CH375_MAX_DATA_LEN;
		CH375_WR_DAT_PORT( len );  /* �������ݵĳ��� */
		do {  /* ����C51,���DO+WHILE�ṹ�������WHILEЧ�ʸ�,�ٶȿ� */
			CH375_WR_DAT_PORT( *mBufferPoint );
			mBufferPoint ++;
		} while ( -- len );
		CH375_WR_CMD_PORT( CMD_DISK_WR_GO );  /* ����ִ��USB�洢����д���� */
	}
	else {  /* ����ʧ�� */
		UdiskStatus = STATUS_ERROR;
	}
/*	CH375_INT_FLAG = 0;  ���жϱ�־ */
}

/* ����CH375ΪUSB������ʽ */
unsigned char	mCH375Init( )
{
	unsigned char	i;
	UdiskStatus = STATUS_DISCONNECT;
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );  /* ����USB����ģʽ */
	CH375_WR_DAT_PORT( 6 );  /* ģʽ����,�Զ����USB�豸���� */
	for ( i = 0xff; i != 0; i -- ) {  /* �ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS */
		if ( CH375_RD_DAT_PORT( ) == CMD_RET_SUCCESS ) break;  /* �����ɹ� */
	}
	if ( i != 0 ) return( TRUE );  /* �����ɹ� */
	else return( FALSE );  /* CH375����,����оƬ�ͺŴ����ߴ��ڴ��ڷ�ʽ���߲�֧�� */
}

/* �ȴ�U��׼���û��ߵȴ��ϴβ������� */
unsigned char	mWaitReady( )
{
	while( UdiskStatus == STATUS_WAIT );  /* �ȴ��ϴβ������� */
	if ( UdiskStatus == STATUS_READY ) return( TRUE );  /* U���Ѿ�׼���ò���,�ϴβ����ɹ� */
	else if ( UdiskStatus == STATUS_ERROR ) {  /* �ϴβ���ʧ�� */
		UdiskStatus = STATUS_WAIT;
		CH375_WR_CMD_PORT( CMD_DISK_RESET );  /* ��λU�� */
		while( UdiskStatus == STATUS_WAIT );  /* �ȴ��ж�״̬ */
		if ( UdiskStatus == STATUS_READY ) return( TRUE );  /* �ȴ��ж�״̬,�����ɹ� */
		return( FALSE );  /* UdiskStatus = STATUS_ERROR */
	}
	else if ( UdiskStatus == STATUS_DISCONNECT ) {  /* U���Ѿ��Ͽ� */
/*		while ( UdiskStatus == STATUS_DISCONNECT );*/
		return( FALSE );
	}
	else if ( UdiskStatus == STATUS_CONNECT ) {  /* U���Ѿ����� */
		mDelay100mS( );  /* ���U�̸ղ�����ô����Ե�һ���ٲ��� */
		mDelay100mS( );
		CH375_WR_CMD_PORT( CMD_DISK_INIT );  /* ��ʼ��USB�洢�� */
		while ( UdiskStatus == STATUS_CONNECT );  /* �ȴ��ж�״̬ */
		if ( UdiskStatus == STATUS_READY ) {  /* �����ɹ�,�����Ѿ���ʼ������װ��ϵͳ�� */
/* ���U���Ƿ�׼����,�����U�̲���Ҫ��һ��,����ĳЩU�̱���Ҫִ����һ�����ܹ��� */
//	do {
//		mDelay100mS( );
//		printf( "Disk Ready ?\n" );
//		i = CH375DiskReady( );  /* ��ѯ�����Ƿ�׼����,���ʡ������ӳ�����Խ�Լ����1KB�ĳ������ */
//	} while ( i != ERR_SUCCESS );
/* CH375DiskReady ��CH375��U���ļ��ӳ������,��Ϊ����϶�,���Դ˴�ʡȥ */
			return( TRUE );
		}
		else return( FALSE );
	}
	return( FALSE );
}

/* ��U�̶�ȡ������������ݿ鵽������ */
unsigned char	mReadSector( unsigned long iLbaStart, unsigned char iSectorCount, unsigned char *iBuffer )
/* iLbaStart ��׼����ȡ��������ʼ������, iSectorCount ��׼����ȡ��������, iBuffer �Ǵ�ŷ������ݵĻ���������ַ */
{
	if ( mWaitReady( ) == FALSE ) return( FALSE );
	mBufferPoint = iBuffer;  /* ָ�򻺳�����ʼ��ַ */
	UdiskStatus = STATUS_WAIT;
	CH375_WR_CMD_PORT( CMD_DISK_READ );  /* ��USB�洢�������ݿ� */
	CH375_WR_DAT_PORT( (unsigned char)iLbaStart );  /* LBA�����8λ */
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 8 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 16 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 24 ) );  /* LBA�����8λ */
	CH375_WR_DAT_PORT( iSectorCount );  /* ������ */
/* ����Ǵ��жϷ�ʽ,��ô��������������,�жϷ��������ȡ���� */
	return( mWaitReady( ) );
}

/* ���������еĶ�����������ݿ�д��U�� */
unsigned char	mWriteSector( unsigned long iLbaStart, unsigned char iSectorCount, unsigned char *iBuffer )
/* iLbaStart ��д�������ʼ��������, iSectorCount ��д���������, iBuffer �Ǵ��׼��д�����ݵĻ���������ַ */
{
	if ( mWaitReady( ) == FALSE ) return( FALSE );
	mBufferPoint = iBuffer;  /* ָ�򻺳�����ʼ��ַ */
	UdiskStatus = STATUS_WAIT;
	CH375_WR_CMD_PORT( CMD_DISK_WRITE );  /* ��USB�洢��д���ݿ� */
	CH375_WR_DAT_PORT( (unsigned char)iLbaStart );  /* LBA�����8λ */
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 8 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 16 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 24 ) );  /* LBA�����8λ */
	CH375_WR_DAT_PORT( iSectorCount );  /* ������ */
/* ����Ǵ��жϷ�ʽ,��ô��������������,�жϷ��������ȡ���� */
	return( mWaitReady( ) );
}

struct _HD_MBR_DPT {
	unsigned char	PartState;
	unsigned char	StartHead;
	unsigned int	StartSec;
	unsigned char	PartType;
	unsigned char	EndHead;
	unsigned int	EndSec;
	unsigned long	StartSector;
	unsigned long	TotalSector;
};

/* Ϊprintf��getkey���������ʼ������ */
void	mInitSTDIO( )
{
	SCON = 0x50;
	PCON = 0x80;
	TMOD = 0x20;
	TH1 = 0xf3;  /* 24MHz����, 9600bps */
	TR1 = 1;
	TI = 1;
}

main( ) {
	unsigned char	c;
	LED_OUT_ACT( );  /* ������LED��һ����ʾ���� */
	mDelay100mS( );  /* ��ʱ100���� */
	LED_OUT_INACT( );
	mInitSTDIO( );
	printf( "Start\n" );
	c = mCH375Init( );  /* ��ʼ��CH375 */
	if ( c == FALSE ) printf( "Error @CH375Init\n" );
	printf( "Insert USB disk\n" );
	while ( 1 ) {  /* ������ */
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
/* ���������� */
/* ��������U�̶�ȡ���� */
		printf( "Read\n" );
		c = mReadSector( 0, 5, DATA_BUFFER );
		if ( c == FALSE ) printf( "Error @ReadSector\n" );
/* ���������U��д������ */
		c = mWriteSector( 1, 1, DATA_BUFFER );
		if ( c == FALSE ) printf( "Error @WriteSector\n" );
/* ���������� */
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
	}
}