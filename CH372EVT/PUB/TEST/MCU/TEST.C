/*
; CH375/CH372 Bulk Data Test
; U2(AT89C51) Program
; ������������ݴ������ȷ��,�������ڳ�ʱ����������,��Ӧ�ļ�����˵Ĳ��Գ���ΪTEST.EXE
; ����: �´�������ȵ�������ݰ�,����Ƭ�����ղ������ݰ�λȡ���󷵻�,�����ɼ����������պ�Ƚ������Ƿ���ȷ
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.09
*/

/* MCS-51��Ƭ��C���Ե�ʾ������,����������Ƭ��ʱһ��ֻҪ�޸�ǰ�漸���ӿ��ӳ���Ӳ������ */

#pragma NOAREGS
#include <reg52.h>
#include <string.h>
#include "..\..\MCU_IF2\C\CH375INC.H"	/* ͷ�ļ�,���������ص�CH372����CH375�������������� */

unsigned char volatile xdata CH375_CMD_PORT _at_ 0xBDF1;		/* CH375����˿ڵ�I/O��ַ */
unsigned char volatile xdata CH375_DAT_PORT _at_ 0xBCF0;		/* CH375���ݶ˿ڵ�I/O��ַ */

/* ��ʱ2΢��,����ȷ */
void	delay2us( )
{
	unsigned char i;
	for ( i = 2; i != 0; i -- );  /* ���ݵ�Ƭ����ʱ��ѡ���ֵ */
}

/* ��ʱ1΢��,����ȷ,��ΪMCS51��Ƭ����������ʵ����������ʱ */
//void	delay1us( )
//{
//	unsigned char i;
//	for ( i = 1; i != 0; i -- );
//}

/* �������� */

void CH375_WR_CMD_PORT( unsigned char cmd ) {  /* ��CH375������˿�д������,���ڲ�С��4uS,�����Ƭ���Ͽ�����ʱ */
	delay2us();
	CH375_CMD_PORT=cmd;
/* ******************** ע����������ͨI/O����ģ��8λ���ڵ�ʱ��,CH375_CS�����ǿ�ѡ��,����һֱ��GNDǿ��Ƭѡ
	CH375_D0_D7 = cmd;
	CH375_A0 = 1;  ѡ��CH375�������
//	CH375_D0_D7_DIR = output;  ���ڱ�׼˫��I/O,���ڴ�����Ϊ�������
	CH375_RD = 1;  ���I/OĬ�ϵ�ƽ�Ǹߵ�ƽ,��ô���ǿ�ѡ����
	CH375_CS = 0;
	CH375_WR = 0;
//	CH375_CS = 0;  ���ڸ��ٵ�Ƭ��,��ָ��������ʱ,�Ա���CH375_WR������������Ϊ80nS�ĵ͵�ƽ����
	CH375_WR = 1;
	CH375_CS = 1;
	CH375_A0 = 0;
//	CH375_D0_D7_DIR = input;  ���ڱ�׼˫��I/O,���ڴ�����Ϊ���뷽��
	CH375_D0_D7 = 0xFF;  ����׼˫��I/O,���ڴ��������ȫ�ߵ�ƽ
******************** */
	delay2us();
}

void CH375_WR_DAT_PORT( unsigned char dat ) {  /* ��CH375�����ݶ˿�д������,���ڲ�С��1.5uS,�����Ƭ���Ͽ�����ʱ */
	CH375_DAT_PORT=dat;
/* ******************** ע����������ͨI/O����ģ��8λ���ڵ�ʱ��
	CH375_D0_D7 = dat;
//	CH375_D0_D7_DIR = output;  ���ڱ�׼˫��I/O,���ڴ�����Ϊ�������
	CH375_CS = 0;
	CH375_WR = 0;
//	CH375_CS = 0;  ���ڸ��ٵ�Ƭ��,��ָ��������ʱ,�Ա���CH375_WR������������Ϊ80nS�ĵ͵�ƽ����
	CH375_WR = 1;
	CH375_CS = 1;
//	CH375_D0_D7_DIR = input;  ���ڱ�׼˫��I/O,���ڴ�����Ϊ���뷽��
	CH375_D0_D7 = 0xFF;  ����׼˫��I/O,���ڴ��������ȫ�ߵ�ƽ
******************** */
//	delay1us();  /* ��ΪMCS51��Ƭ����������ʵ����������ʱ */
}

unsigned char CH375_RD_DAT_PORT( void ) {  /* ��CH375�����ݶ˿ڶ�������,���ڲ�С��1.5uS,�����Ƭ���Ͽ�����ʱ */
//	delay1us();  /* ��ΪMCS51��Ƭ����������ʵ����������ʱ */
/* ******************** ע����������ͨI/O����ģ��8λ���ڵ�ʱ��
//	CH375_D0_D7_DIR = input;  ���ڱ�׼˫��I/O,���ڴ�����Ϊ���뷽��
	CH375_D0_D7 = 0xFF;  ����׼˫��I/O,���ڴ��������ȫ�ߵ�ƽ,��������
	CH375_CS = 0;
	CH375_RD = 0;
//	CH375_CS = 0;  ���ڸ��ٵ�Ƭ��,��ָ��������ʱ,�Ա���CH375_RD������������Ϊ80nS�ĵ͵�ƽ����
	unsigned char dat = CH375_D0_D7;
	CH375_RD = 1;
	CH375_CS = 1;
	CH375_D0_D7 = 0xFF;  ����׼˫��I/O,���ڴ��������ȫ�ߵ�ƽ
	return( dat );
******************** */
	return( CH375_DAT_PORT );
}

/* ��ʱ50����,����ȷ */
void	Delay50ms( )
{
	unsigned char i, j;
	for ( i=200; i!=0; i-- ) for ( j=250; j!=0; j-- );
}

/* CH375��ʼ���ӳ��� */
void	CH375_Init( )
{
	unsigned char i;
/* ����CH375�Ƿ���������,��ѡ����,ͨ������Ҫ */
	CH375_WR_CMD_PORT( CMD_CHECK_EXIST );  /* ����CH375�Ƿ��������� */
	CH375_WR_DAT_PORT( 0x55 );  /* д��������� */
	i = ~ 0x55;  /* ��������Ӧ���ǲ�������ȡ�� */
	if ( CH375_RD_DAT_PORT( ) != i ) {  /* CH375������ */
		for ( i=80; i!=0; i-- ) {
			CH375_WR_CMD_PORT( CMD_RESET_ALL );  /* ����ظ�������,ִ��Ӳ����λ */
			CH375_RD_DAT_PORT( );
		}
		CH375_WR_CMD_PORT( 0 );
		Delay50ms( );  /* ��ʱ50ms */
	}
/* ����USB����ģʽ, ��Ҫ���� */
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );
	CH375_WR_DAT_PORT( 2 );  /* ����Ϊʹ�����ù̼���USB�豸��ʽ */
	for ( i=100; i!=0; i-- ) {  /* �ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS */
		if ( CH375_RD_DAT_PORT( ) == CMD_RET_SUCCESS ) break;
	}
/*	if ( i==0 ) { CH372/CH375����Ӳ������ }; */
/* ���������ж�,�ٶ�CH375������INT0 */
	IT0 = 0;  /* ���ⲿ�ź�Ϊ�͵�ƽ���� */
	IE0 = 0;  /* ���жϱ�־ */
	EX0 = 1;  /* ����CH375�ж� */
}

/* CH375�жϷ������,ʹ�üĴ�����1 */
void	mCh375Interrupt( ) interrupt 0 using 1
{
	unsigned char InterruptStatus;
	unsigned char i, length;
	unsigned char data buffer[ 64 ];
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* ��ȡ�ж�״̬��ȡ���ж����� */
	InterruptStatus = CH375_RD_DAT_PORT( );  /* ��ȡ�ж�״̬ */
	switch ( InterruptStatus ) {  /* �����ж�״̬���� */
		case USB_INT_EP2_OUT: {  /* �����˵��´��ɹ� */
			CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* �ӵ�ǰUSB�жϵĶ˵㻺������ȡ���ݿ�,���ͷŻ����� */
			length = CH375_RD_DAT_PORT( );  /* ���ȶ�ȡ�������ݳ��� */
			for ( i = 0; i < length; i ++ ) buffer[ i ] = CH375_RD_DAT_PORT( );  /* �������ݰ� */
			/* ����������ȷ��,�����յ������������ȡ���󷵻ظ�PC�� */
			CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* ��USB�˵�2�ķ��ͻ�����д�����ݿ� */
			CH375_WR_DAT_PORT( length );  /* ����д��������ݳ���,�ش��ս��յ������ݳ��� */
			for ( i = 0; i < length; i ++ ) CH375_WR_DAT_PORT( ~ buffer[ i ] );  /* ����ȡ���󷵻�,�ɼ����Ӧ�ó�����������Ƿ���ȷ */
			break;
		}
		case USB_INT_EP2_IN: {  /* �������ݷ��ͳɹ� */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* �ͷŵ�ǰUSB������ */
			break;
		}
		default: {  /* �����ж�,δ�õ�,�������˳����� */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* �ͷŵ�ǰUSB������ */
			break;
		}
	}
}

main( ) {
	Delay50ms( );	/* ��ʱ�ȴ�CH375��ʼ�����,�����Ƭ����CH375�ṩ��λ�ź��򲻱���ʱ */
	CH375_Init( );  /* ��ʼ��CH375 */
	EA = 1;  /* �����ж� */
	while ( 1 );  /* ����ָ�ʼ����ѭ��,�ȴ�PC��������в��� */
}