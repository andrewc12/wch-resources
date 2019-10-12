/* 2004.03.05, 2004.8.18
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB 1.1 Host Examples for CH375   **
**  KC7.0@MCS-51                      **
****************************************
*/
/* CH375��ΪUSB�����ӿڵĳ���ʾ�� */

/* MCS-51��Ƭ��C���Ե�ʾ������, ��ѯ��ʽ */

#include <reg51.h>
#include <string.h>
#include <stdio.h>

/* ����CH375������뼰����״̬ */
#include "CH375INC.H"

unsigned char volatile xdata	CH375_CMD_PORT _at_ 0xBDF1;	/* CH375����˿ڵ�I/O��ַ */
unsigned char volatile xdata	CH375_DAT_PORT _at_ 0xBCF0;	/* CH375���ݶ˿ڵ�I/O��ַ */
unsigned char xdata				DATA_BUFFER[ 0x8000 ] _at_ 0x0000;	/* �ⲿRAM���ݻ���������ʼ��ַ */
sbit	CH375_INT_WIRE	=		0xB0^2;	/* P3.2, INT0, ����CH375��INT#����,���ڲ�ѯ�ж�״̬ */

/* ��P1.4����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,��U�̲������ */
sbit P1_4  = P1^4;
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 �͵�ƽ����LED��ʾ */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 �͵�ƽ����LED��ʾ */

bit		mDeviceOnline = 0;				/* CH375��Ŀ��USB�豸������״̬: 0�Ͽ�,1���� */


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

/* ��ʱ10����,����ȷ */
void	mDelay10mS( )
{
	unsigned char	i, j, c;
	for ( i = 20; i != 0; i -- ) for ( j = 200; j != 0; j -- ) c+=3;
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

/* �ȴ�CH375�жϲ���ȡ״̬ */
unsigned char mWaitInterrupt() {  /* �����˵ȴ��������, ���ز���״̬ */
	unsigned char c;
	while( CH375_INT_WIRE );  /* ��ѯ�ȴ�CH375��������ж�(INT#�͵�ƽ) */
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* ������������ж�, ��ȡ�ж�״̬ */
	c = CH375_RD_DAT_PORT( );  /* �����ж�״̬ */
	if ( c == USB_INT_DISCONNECT ) mDeviceOnline = 0;  /* ��⵽USB�豸�Ͽ��¼� */
	else if ( c == USB_INT_CONNECT ) mDeviceOnline = 1;  /* ��⵽USB�豸�����¼� */
	return( c );
}

/* ����CH375ΪUSB������ʽ */
unsigned char	mCH375Init( )
{
	unsigned char	c, i;
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );  /* ����USB����ģʽ */
	CH375_WR_DAT_PORT( 6 );  /* ģʽ����,�Զ����USB�豸���� */
	for ( i = 0xff; i != 0; i -- ) {  /* �ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS */
		c = CH375_RD_DAT_PORT( );
		if ( c == CMD_RET_SUCCESS ) break;  /* �����ɹ� */
	}
	if ( i != 0 ) return( USB_INT_SUCCESS );  /* �����ɹ� */
	else return( 0xff );  /* CH375����,����оƬ�ͺŴ����ߴ��ڴ��ڷ�ʽ���߲�֧�� */
}

/* ��CH375�Ķ˵㻺������ȡ���յ������� */
unsigned char	mReadCH375Data( unsigned char *buf )
{
	unsigned char len, i;
	unsigned char *p;
	CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* ��CH375�Ķ˵㻺������ȡ���ݿ� */
	p = buf;
	len = CH375_RD_DAT_PORT( );  /* ���ݳ��� */
	for ( i=0; i<len; i++ ) *p++ = CH375_RD_DAT_PORT( );  /* ������ȡ���� */
	return( len );
}

/* ��CH375�Ķ˵㻺����д��׼�����͵����� */
void	mWriteCH375Data( unsigned char *buf, unsigned char len )
{
	unsigned char i;
	unsigned char *p;
	CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* ��CH375�Ķ˵㻺����д�����ݿ� */
	p = buf;
	CH375_WR_DAT_PORT( len );  /* ���ݳ��� */
	for ( i=0; i<len; i++ ) CH375_WR_DAT_PORT( *p++ );  /* ����д������ */
}

/* ��Ŀ��USB�豸ִ�п��ƴ���: ��ȡUSB������ */
unsigned char	mCtrlGetDescr( unsigned char type )
{
	CH375_WR_CMD_PORT( CMD_GET_DESCR );  /* ���ƴ���-��ȡ������ */
	CH375_WR_DAT_PORT( type );  /* 0:�豸������, 1:���������� */
	return( mWaitInterrupt( ) );  /* �ȴ�������� */
}

/* ��Ŀ��USB�豸ִ�п��ƴ���: ����USB��ַ */
unsigned char	mCtrlSetAddress( unsigned char addr )
{
	unsigned char c;
	CH375_WR_CMD_PORT( CMD_SET_ADDRESS );  /* ���ƴ���-����USB��ַ */
	CH375_WR_DAT_PORT( addr );  /* 1 - 7eh */
	c = mWaitInterrupt( );  /* �ȴ�������� */
	if ( c != USB_INT_SUCCESS ) return( c );  /* ����ʧ�� */
/* ��Ŀ��USB�豸�ĵ�ַ�ɹ��޸ĺ�,Ӧ��ͬ���޸�CH375��USB��ַ,����CH375���޷���Ŀ���豸ͨѶ */
	CH375_WR_CMD_PORT( CMD_SET_USB_ADDR );  /* ����CH375��USB��ַ */
	CH375_WR_DAT_PORT( addr );  /* �޸�CH375��USB�豸�ܹ��������,��������ж�֪ͨ */
	return( c );
}

/* ��������,ִ��USB���� */
unsigned char	mIssueToken( unsigned char endp, unsigned char token )
{
	CH375_WR_CMD_PORT( CMD_ISSUE_TOKEN );  /* ��������,ִ������ */
	CH375_WR_DAT_PORT( endp << 4 | token );  /* ��4λ�Ƕ˵��,��4λ������PID */
	return( mWaitInterrupt( ) );  /* �ȴ�������� */
}

/* ������״̬,�����������ʾ������벢ͣ�� */
void	mStopIfError( unsigned char err )
{
	if ( err == USB_INT_SUCCESS ) return;  /* �����ɹ� */
	printf( "Error: %02X, %d\n", (unsigned int)err, (unsigned int)mDeviceOnline );  /* ��ʾ���� */
	while ( 1 ) {
		LED_OUT_ACT( );  /* LED��˸ */
		mDelay100mS( );
		LED_OUT_INACT( );
		mDelay100mS( );
	}
}

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
	unsigned char	i, len;
	LED_OUT_ACT( );  /* ������LED��һ����ʾ���� */
	mDelay100mS( );  /* ��ʱ100���� */
	LED_OUT_INACT( );
	mInitSTDIO( );
	printf( "Start\n" );
	i = mCH375Init( );  /* ��ʼ��CH375 */
	mStopIfError( i );
/* ������·��ʼ�� */
	printf( "Insert USB device\n" );
	while ( mDeviceOnline == 0 ) mWaitInterrupt( );  /* �ȴ�USB�豸���� */

#if 1
/* ʵ���ϲ�û�б�Ҫ��USB�豸�����λ���豸,���Ǽ������WINDOWS����������,��������Ҳ���Բ��� */
/* ����˵��, ��ЩUSB�豸Ҫ���ڲ������븴λUSB���߲��ܹ���, �����ⲿ�ֳ�����ȥ�� */
	printf( "Begin: reset USB device\n" );
	while ( mDeviceOnline == 0 ) mWaitInterrupt( );  /* �ȴ�USB�豸���� */
//	CH375_INT_EN = 0;  /* ��λUSB�����ڼ�Ӧ�ý�ֹCH375�ж� */
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );  /* ����USB����ģʽ */
	CH375_WR_DAT_PORT( 7 );  /* ģʽ����,��λUSB�豸 */
	mDelay10mS( );  /* ��λʱ��Ӧ����10mS���� */
	mDeviceOnline = 0;
//	CH375_INT_FLAG = 0;  /* ���жϱ�־ */
//	CH375_INT_EN = 1;  /* ����CH375�ж� */
/*	printf( "End: reset USB device\n" ); */
	i = mCH375Init( );  /* ֹͣ��λ,���³�ʼ��CH375 */
	mStopIfError( i );
	while ( mDeviceOnline == 0 ) mWaitInterrupt( );  /* �ȴ�USB�豸���� */
#endif

/*	mDelay10mS( );  �ʵ���ʱ������,��ѡ�Ĳ��� */
	i = mCtrlGetDescr( 1 );  /* ��ȡ�豸������ */
	mStopIfError( i );
	len = mReadCH375Data( DATA_BUFFER );  /* ��ȡ�豸���������� */
	printf( "Device descr data len: %d, data: ", len );
	for ( i = 0; i < len; i ++ ) printf( "%02X,", (unsigned int)DATA_BUFFER[i] );
	printf( "\n" );
	i = mCtrlSetAddress( 5 );  /* ����USB��ַ,��ֵַΪ1��7EH,����ѡ */
	mStopIfError( i );
#if 0
	......
	i = mIssueToken( mBulkOutEndp, DEF_USB_PID_OUT );  /* ����һ��OUT����,������DATA0 */
	mStopIfError( i );
#endif
	printf( "Stop\n" );
	LED_OUT_ACT( );  /* LED���� */
	while ( 1 );  /* ͣ�� */
}