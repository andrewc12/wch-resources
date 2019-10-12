/*
; CH375/CH372 Bulk Data Test
; U2(AT89C51) Program
; ������������ݴ������ȷ��, ��ͨ���շ������ݿ������Դ����ٶ�,
; ������������ʱ��Ƶ�ʲ�����24MHz�ı�׼MCS-51��Ƭ��, �������24MHz��Ӧ���ʵ�������ʱ, �������24MHz������ʵ�������ʱ
; ֻҪ��Ƭ������, ����ʹ�ýϸߵ�ʱ��, ���罫ԭ12MHz����Ϊ24MHz,
; ��ΪMCS51��Ƭ�������ٶȽ���,��д�ⲿRAM��Ҫ2�������Լ����ӵ�ѭ��ָ��,��ʹ24MHzʱ��Ҳ�޷�����500KB/S
; Ϊ�˼��ٵ�Ƭ���������ٶȲ��Ե�Ӱ��,���������ٶȲ��Բ����ڵ�Ƭ���жϷ�����������
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.09
*/

/* MCS-51��Ƭ��C���Ե�ʾ������ */

#include <reg52.h>
#include <string.h>
#include "..\..\MCU_IF1\MCS51C\CH375INC.H"

/* #define USE_MY_USB_ID	YES */
#define MY_USB_VENDOR_ID	0x1234		/* ����ID */
#define MY_USB_DEVICE_ID	0x5678		/* �豸ID */

unsigned char volatile xdata CH375_CMD_PORT _at_ 0xBDF1;		/* CH375����˿ڵ�I/O��ַ */
unsigned char volatile xdata CH375_DAT_PORT _at_ 0xBCF0;		/* CH375���ݶ˿ڵ�I/O��ַ */

unsigned char THIS_CMD_CODE;  /* ���浱ǰ������ */
unsigned char RECV_LEN;       /* �ս��յ������ݵĳ��� */
unsigned char RECV_BUFFER[ CH375_MAX_DATA_LEN ];  /* ���ݻ�����,���ڱ�����յ����´�����,����Ϊ0��64�ֽ� */
/* ����MCS-51��Ƭ����ȡ�ⲿRAM�Ķ�д�ٶȵ����ڲ�RAM, ������Ҫ�õ�DPTR, ���Զ�д�ٶȽ���, �������Բο��������޸� */

/* ����λ��־ */
bit bdata FLAG_RECV_OK;       /* ���ճɹ���־,1ָʾ�ɹ����յ����ݿ� */
bit bdata FLAG_SEND_WAIT;     /* ���͵ȴ���־,1ָʾ�����ݿ�����CH375�еȴ����� */

/* Ӧ�ò㶨�� */
/* TEST_OTHER		EQU    00H				;�����Զ���������� */
#define TEST_START		0x20  /* ���Թ��̿�ʼ */
#define TEST_DATA		0x21  /* ����������ȷ�� */
#define TEST_UPLOAD		0x22  /* �����ϴ����ݿ� */
#define TEST_DOWNLOAD	0x23  /* �����´����ݿ� */

/* �й�CH451�Ķ���,��ʾ������ӷ�ʽ,�ó���û���õ����� */
sbit   CH451_dclk=P1^7;      /* ��������ʱ�������Ӽ��� */
sbit   CH451_din=P1^6;	     /* �����������,��CH451���������� */
sbit   CH451_load=P1^5;      /* �����������,�����Ӽ��� */

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

/* CH451��ʼ���ӳ��� */
void	CH451_Init( )
{
  CH451_din=0;         /* �ȵͺ��,ѡ��4������ */
  CH451_din=1;
}

/* CH451��������ӳ��� */
/* ����һ�޷������ͱ����洢12�ֽڵ������� */
void	CH451_Write( unsigned int command )
{
  unsigned char i;
  CH451_load=0;  /* ���ʼ */
  for( i=0; i<12; i++ ) {  /* ����12λ����,��λ��ǰ */
    CH451_din = command & 1;
    CH451_dclk = 0;
    command >>= 1;
    CH451_dclk = 1;  /* ��������Ч */
  }
  CH451_load = 1;  /* �������� */
}

/* CH375��ʼ���ӳ��� */
void	CH375_Init( )
{
	unsigned char i;
	FLAG_RECV_OK=0;  /* ����ճɹ���־,1ָʾ�ɹ����յ����ݿ� */
	FLAG_SEND_WAIT=0;  /* �巢�͵ȴ���־,1ָʾ�����ݿ�����CH375�еȴ����� */
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

/* CH375�жϷ������,ʹ�üĴ�����1 */
void	mCh375Interrupt( ) interrupt 0 using 1
{
	unsigned char InterruptStatus;
	unsigned char length, c1;
	unsigned char data *cmd_buf;
	unsigned char data *ret_buf;
	CH375_CMD_PORT = CMD_GET_STATUS;  /* ��ȡ�ж�״̬��ȡ���ж����� */
	Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
	InterruptStatus = CH375_DAT_PORT;  /* ��ȡ�ж�״̬ */
	IE0 = 0;  /* ���жϱ�־,��Ӧ��INT0�ж� */
	if ( InterruptStatus == USB_INT_EP2_OUT ) {  /* �����˵��´��ɹ� */
		CH375_CMD_PORT = CMD_RD_USB_DATA;  /* �ӵ�ǰUSB�жϵĶ˵㻺������ȡ���ݿ�,���ͷŻ����� */
		Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
		length = CH375_DAT_PORT;  /* ���ȶ�ȡ�������ݳ��� */
		if ( length != 0 ) {  /* �������Ϊ0�򲻴��� */
			THIS_CMD_CODE = CH375_DAT_PORT;  /* ���浱ǰ������,��Ϊ���ǲ��Գ�����PC��Ӧ�ó���Լ�����ֽ�Ϊ������ */
			if ( THIS_CMD_CODE == TEST_DOWNLOAD ) {  /* �����´��ٶ� */
				while ( --length != 0 )  /* �ȼ�1��ȥ�����ֽں� */
					c1 = CH375_DAT_PORT;  /* ��������,Ϊ�˲����ٶ�,��������,24MHz��MCS51ÿ��ȡһ���ֽ���Ҫ2uS */
			}
			else {  /* ���ǲ����´��ٶȵ�����,�Ƚ�����������ٷ��� */
				RECV_LEN = length;  /* ����������ݳ��� */
				cmd_buf = RECV_BUFFER;  /* ���ջ����� */
				*cmd_buf = THIS_CMD_CODE;
				while ( --length != 0 ) {  /* �ȼ�1��ȥ�����ֽں� */
					cmd_buf++;
					*cmd_buf = CH375_DAT_PORT;
				}
/* ���ϳ���C����Ҫÿ��һ���ֽڿ���Ҫʮ�����������,����û������ֻҪ4����������
					mov  a,length
					jz   skip_get
					mov  r7,a
					mov  dptr,#CH375_DAT_PORT
get_next_byte:		movx a,@dptr    ����ֱ�ӷ���
					djnz r7,get_next_byte
skip_get:			nop
*/
				if ( THIS_CMD_CODE == TEST_UPLOAD ) {  /* �����ϴ��ٶ� */
					CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* ��USB�˵�2�ķ��ͻ�����д�����ݿ� */
					Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
					length = CH375_MAX_DATA_LEN;
					CH375_DAT_PORT = length;  /* ����д��������ݳ��� */
					do {
						CH375_DAT_PORT = TL0;  /* ����α���������,Ϊ�˲����ٶ�,������Ч,24MHz��MCS51ÿд��һ���ֽ���Ҫ2uS */
					} while ( --length != 0 );
				}
				else if ( THIS_CMD_CODE == TEST_START ) {  /* ���Թ��̿�ʼ */
/* ������һ�β��������ϴ��ٶ�ʱ�������ϴ�������������������, �����ڵڶ��β���ǰ��Ҫ����ϴ������� */
					CH375_CMD_PORT = CMD_SET_ENDP7;  /* ����USB�˵�2��IN */
					Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
					CH375_DAT_PORT = 0x0e;  /* ͬ������λ����,����USB�˵�2��IN��æ,����NAK */
					FLAG_SEND_WAIT = 0;  /* ������͵ȴ���־,֪ͨӦ�ó�����Լ����������� */
				}
				else if ( THIS_CMD_CODE == TEST_DATA ) {  /* ����������ȷ��,�����յ������������ȡ���󷵻ظ�PC�� */
					CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* ��USB�˵�2�ķ��ͻ�����д�����ݿ� */
					Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
					ret_buf = RECV_BUFFER;  /* ���ջ����� */
					length = RECV_LEN;  /* �ս��յ������ݳ��� */
					CH375_DAT_PORT = length;  /* ����д��������ݳ��� */
					if ( length ) {
						do {
							CH375_DAT_PORT = ~ *ret_buf;  /* ����ȡ���󷵻�,�ɼ����Ӧ�ó�����������Ƿ���ȷ */
							ret_buf++;
						} while ( --length != 0 );
					}
				}
				else {  /* ��������,��δ���� */
					FLAG_RECV_OK = 1;  /* ��������,���ý��ճɹ���־,֪ͨӦ�ó���ȡ�������ٷ��� */
				}
			}
		}
	}
	else if ( InterruptStatus == USB_INT_EP2_IN ) {  /* �������ݷ��ͳɹ� */
		if ( THIS_CMD_CODE == TEST_UPLOAD ) {  /* �����ϴ��ٶ�,����׼���ϴ����� */
			CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* ��USB�˵�2�ķ��ͻ�����д�����ݿ� */
			Delay2us( );  /* ���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ */
			length = CH375_MAX_DATA_LEN;
			CH375_DAT_PORT = length;  /* ����д��������ݳ��� */
			do {
				CH375_DAT_PORT = TL0;  /* ����α���������,Ϊ�˲����ٶ�,������Ч,24MHz��MCS51ÿд��һ���ֽ���Ҫ2uS */
			} while ( --length != 0 );
		}
		CH375_CMD_PORT = CMD_UNLOCK_USB;  /* �ͷŵ�ǰUSB������ */
		FLAG_SEND_WAIT = 1;  /* ������͵ȴ���־,֪ͨӦ�ó�����Լ����������� */
	}
	else if ( InterruptStatus == USB_INT_EP1_IN ) {  /* �ж����ݷ��ͳɹ� */
		CH375_CMD_PORT = CMD_UNLOCK_USB;  /* �ͷŵ�ǰUSB������ */
	}
	else {  /* ���ù̼���USB��ʽ�²�Ӧ�ó��������ж�״̬ */
	}
}

main( ) {
	unsigned char i;
	Delay50ms( );	/* ��ʱ�ȴ�CH375��ʼ�����,�����Ƭ����CH375�ṩ��λ�ź��򲻱���ʱ */
	CH375_Init( );  /* ��ʼ��CH375 */
	CH451_Init( );  /* ��ʼ��CH451 */
/* ����CH451����ʾ���� */
	CH451_Write( 0x0401 );  /* ����ϵͳ��������,ʹ����ʾ���� */
	CH451_Write( 0x0588 );  /* ������ʾ����,BCD���뷽ʽ,8������ */
	for ( i=0; i<8; ++i ) CH451_Write( (unsigned int)i << 8 | 0x0800 | i );  /* ������ʾ12345678 */
	EA = 1;  /* �����ж� */
	TR0 = 1;  /* �ɶ�ʱ��0���ɼ���ֵ����α����� */
	while ( 1 ) {  /* ����ָ�ʼ����ѭ��,�ȴ�PC��������в��� */
		if ( FLAG_RECV_OK ) {  /* �յ�δ��������� */
			CH451_Write( 0x0300 );  /* ��������ʹCH451��ʾ����һλ,��ʾ�յ�δ�������� */
		}
	}
}