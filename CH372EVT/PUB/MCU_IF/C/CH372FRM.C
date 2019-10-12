/*
; ��Ƭ��ͨ��CH372����CH375���ӵ�PC������USBͨѶ�Ŀ�ܳ���
; ������includeֱ�Ӱ�����Ӧ��ϵͳ����������,�������ӵ�������Ŀ��
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; V1.0 @2004.09, V1.1 @2004.12
;****************************************************************************
*/

/* C����, �����ô������Ƭ��,����MCS51�Լ������ں˵ĵ�Ƭ�� */
/* �����Ƭ�����ͻ���Ӳ����Դ��ͬʱ, �ó���Ӧ�ø�����Ҫ���оֲ��޸� */

#include "CH375INC.H"

#ifdef __CX51__
#ifndef __C51__
#define __C51__		1
#endif
#endif

typedef unsigned char                 UINT8;
typedef unsigned short                UINT16;
typedef unsigned long                 UINT32;
#ifdef __C51__
typedef unsigned char  idata         *PUINT8;
typedef unsigned char volatile xdata  IOPORT;
#pragma NOAREGS
#include <reg52.h>
#else
typedef unsigned char                *PUINT8;
typedef unsigned char volatile        IOPORT;
#endif

#define DELAY_START_VALUE		1	  /* ���ݵ�Ƭ����ʱ��ѡ����ʱ��ֵ */

/*#define MY_USB_VENDOR_ID		0x4348*/	/* �����Լ���USB�豸�ĳ���ID */
/*#define MY_USB_PRODUCT_ID		0x5537*/	/* �����Լ���USB�豸�Ĳ�ƷID */
/*#define ENABLE_USB_SUSPEND		1*/		/* ���USB���ߵĹ���״̬,���ڽ���͹���ģʽ */

IOPORT		CH375_CMD_PORT _at_ 0xBDF1;		/* CH375����˿ڵ�I/O��ַ,����ʵ��Ӳ����·�����޸� */
IOPORT		CH375_DAT_PORT _at_ 0xBCF0;		/* CH375���ݶ˿ڵ�I/O��ַ,����ʵ��Ӳ����·�����޸� */

/* ��ʱ1΢��,����ȷ,��Ҫ����Ӳ��ʵ��������� */
void	Delay1us( )
{
#if DELAY_START_VALUE != 0
	UINT8 i;
	for ( i=DELAY_START_VALUE; i!=0; i-- );
#endif
}

/* ��ʱ2΢��,����ȷ,��Ҫ����Ӳ��ʵ��������� */
void	Delay2us( )
{
	UINT8 i;
	for ( i=DELAY_START_VALUE*2+1; i!=0; i-- );
}

/* ��CH372/CH375�йصĻ���I/O���� */

void CH375_WR_CMD_PORT( UINT8 cmd ) {  /* ��CH375������˿�д������,���ڲ�С��4uS,�����Ƭ���Ͽ�����ʱ */
	Delay2us();
	CH375_CMD_PORT=cmd;
	Delay2us();
}

void CH375_WR_DAT_PORT( UINT8 dat ) {  /* ��CH375�����ݶ˿�д������,���ڲ�С��1.5uS,�����Ƭ���Ͽ�����ʱ */
	CH375_DAT_PORT=dat;
	Delay1us();  /* �����MCS51��Ƭ��,�������,����ʵ����������ʱ */
}

UINT8 CH375_RD_DAT_PORT( void ) {  /* ��CH375�����ݶ˿ڶ�������,���ڲ�С��1.5uS,�����Ƭ���Ͽ�����ʱ */
	Delay1us();  /* �����MCS51��Ƭ��,�������,����ʵ����������ʱ */
	return( CH375_DAT_PORT );
}

/* CH375��ʼ���ӳ��� */
void	CH375_Init( void ) {
	UINT8 i;

#ifdef MY_USB_VENDOR_ID
#ifdef MY_USB_PRODUCT_ID
/* �����ⲿ�Զ����USB�豸VID��PID,��ѡ����,��ִ�и�������ʹ��Ĭ�ϵ�VID��PID,
   �������ʹ���Զ����ID,��ô���������������INF��װ�ļ��е�"USB\VID_4348&PID_5537"��Ҫ���������޸� */
	CH375_WR_CMD_PORT( CMD_SET_USB_ID );  /* �����ⲿ�Զ����USB�豸VID��PID,��ѡ���� */
	CH375_WR_DAT_PORT( (UINT8)MY_USB_VENDOR_ID );  /* д�볧��ID�ĵ��ֽ� */
	CH375_WR_DAT_PORT( (UINT8)(MY_USB_VENDOR_ID>>8) );  /* д�볧��ID�ĸ��ֽ� */
	CH375_WR_DAT_PORT( (UINT8)MY_USB_PRODUCT_ID );  /* д���ƷID�ĵ��ֽ� */
	CH375_WR_DAT_PORT( (UINT8)(MY_USB_PRODUCT_ID>>8) );  /* д���ƷID�ĸ��ֽ� */
#endif
#endif

/* ����USB����ģʽ, ��Ҫ���� */
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );
	CH375_WR_DAT_PORT( 2 );  /* ����Ϊʹ�����ù̼���USB�豸��ʽ */
	for ( i=100; i!=0; i-- ) if ( CH375_RD_DAT_PORT( ) == CMD_RET_SUCCESS ) break;  /* �ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS */
/*	if ( i == 0 ) { CH372/CH375оƬ�ڲ������������Ӵ���Ӳ������ }; */

#ifdef ENABLE_USB_SUSPEND
/* ���ü��USB���߹���״̬,������USB���߹���ʱʹCH375Ҳ����͹���״̬ */
	CH375_WR_CMD_PORT( CMD_CHK_SUSPEND );  /* ���ü��USB���߹���״̬�ķ�ʽ */
	CH375_WR_DAT_PORT( 0x10 );
	CH375_WR_DAT_PORT( 0x04 );  /* ��50mSΪ������USB���� */
#endif

/* ��������USB�ж�,CH375��INT#���ſ������ӵ���Ƭ�����ж�����,�ж�Ϊ�͵�ƽ��Ч�����½�����Ч,
   �����ʹ���ж�,��ôҲ�����ò�ѯ��ʽ,�ɵ�Ƭ�������ѯCH375��INT#����Ϊ�͵�ƽ��˵��CH375�����ж� */
#ifdef __C51__
	IT0 = 0;  /* ���ⲿ�ź�Ϊ�͵�ƽ���� */
	IE0 = 0;  /* ���жϱ�־ */
	EX0 = 1;  /* ����CH375�ж�,�ٶ�CH375��INT#�������ӵ���Ƭ����INT0 */
#endif
}

UINT8	UsbLength;							/* USB���ݻ����������ݵĳ��� */
UINT8	UsbBuffer[ CH375_MAX_DATA_LEN ];	/* USB���ݻ����� */

/* CH375�жϷ������,�ٶ�CH375��INT#�������ӵ���Ƭ����INT0,ʹ�üĴ�����1 */
void	mCH375Interrupt( void ) interrupt 0 using 1 {
	UINT8			IntStatus;
	UINT8			cnt;
	PUINT8			buf;
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* ��ȡ�ж�״̬��ȡ���ж����� */
	IntStatus = CH375_RD_DAT_PORT( );  /* ��ȡ�ж�״̬ */
/*	IE0 = 0;  ���жϱ�־,�뵥Ƭ��Ӳ���й�,��Ӧ��INT0�ж� */
	switch( IntStatus ) {  /* �����ж�״̬ */
		case USB_INT_EP2_OUT:  /* �����˵��´��ɹ�,���յ����� */
			CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* �ӵ�ǰUSB�жϵĶ˵㻺������ȡ���ݿ�,���ͷŻ����� */
/* ���ʹ��CMD_RD_USB_DATA0������,��ô��ȡ���ݿ�󲻻��Զ��ͷŻ�����,��ҪCMD_UNLOCK_USB�������ͷ� */
			UsbLength = cnt = CH375_RD_DAT_PORT( );  /* ���ȶ�ȡ�������ݳ��� */
			if ( cnt ) {  /* ���յ����ݷŵ��������� */
				buf = UsbBuffer;  /* ָ�򻺳��� */
				do {
					*buf = CH375_RD_DAT_PORT( );  /* ��������������� */
					buf ++;
				} while ( -- cnt );
			}
			else break;  /* ����Ϊ0,û������,��ĳЩӦ����Ҳ���Խ�����0����Ϊһ���������� */
/* �������յ������ݲ�����,�˴���ȥ,������ʾ�ش����� */
			CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* ��USB�˵�2�ķ��ͻ�����д�����ݿ� */
			cnt = UsbLength;
			CH375_WR_DAT_PORT( cnt );  /* ����д��������ݳ��� */
			if ( cnt ) {  /* ���������е����ݷ��� */
				buf = UsbBuffer;  /* ָ�򻺳��� */
				do {
					CH375_WR_DAT_PORT( *buf );  /* д�����ݵ�CH375 */
					buf ++;
				} while ( -- cnt );
			}
			break;
		case USB_INT_EP2_IN:  /* �����˵��ϴ��ɹ�,���ݷ��ͳɹ� */
/* �������������Ҫ���ŷ���,���ڴ�ͨ��CMD_WR_USB_DATA7����д��,�ο�ǰ��Ļش� */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* �ͷŵ�ǰUSB������,�յ��ϴ��ɹ��жϺ�,�������USB������,�Ա�����շ� */
			break;
		case USB_INT_EP1_IN:  /* �ж϶˵��ϴ��ɹ�,�ж����ݷ��ͳɹ� */
/* �ж϶˵�������ڵ�Ƭ����ʱ֪ͨ�������,������δ�õ� */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* �ͷŵ�ǰUSB������ */
			break;
		case USB_INT_EP1_OUT:  /* �����˵��´��ɹ�,���յ��������� */
/* �����˵�������ڼ��������Ƭ���˷��Ͱ�,������δ�õ�,�������ݵĽ��տ��Բο������˵� */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* �ͷŵ�ǰUSB������ */
			break;
#ifdef ENABLE_USB_SUSPEND
/* Ĭ������²��������Щ�ж�״̬,ֻ�е�ִ����Ӧ�������Ż������Щ״̬,������Ҫ���� */
		case USB_INT_USB_SUSPEND:  /* USB���߹����¼�,ֻ�е�ִ�й�CMD_CHK_SUSPEND�����ſ��ܳ��ָ��ж�״̬ */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* �ͷŵ�ǰUSB������ */
			CH375_WR_CMD_PORT( CMD_ENTER_SLEEP );  /* ���ǿ�ѡ����,����͹���˯�߹���״̬ */
			break;
		case USB_INT_WAKE_UP:  /* ��˯���б������¼�,ֻ�е�ִ�й�CMD_ENTER_SLEEP�����ſ��ܳ��ָ��ж�״̬ */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* �ͷŵ�ǰUSB������ */
			break;
#endif
		default:
/* ���ù̼���USB��ʽ�²�Ӧ�ó��������ж�״̬,�������USB���߹�����,��ô��Ҫ����USB���߹����˯�߻����¼� */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* �ͷŵ�ǰUSB������ */
			break;
	}
}

/*
main( void ) {
	CH375_Init( );
	EA = 1;
	while ( 1 );
}
*/