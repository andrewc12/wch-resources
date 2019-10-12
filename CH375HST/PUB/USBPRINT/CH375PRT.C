/* 2004.03.05, 2004.8.18, 2005.12.29
****************************************
**  Copyright  (C)  W.ch  1999-2005   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB 1.1 Host Examples for CH375   **
**  KC7.0@MCS-51                      **
****************************************
*/
/* ��Ƭ��ͨ��CH375����USB��ӡ�� */
/* ����ʾ��,C����,CH375�ж�Ϊ��ѯ��ʽ,ֻ�������ݴ���,���漰��ӡ��ʽ����ӡ�������� */
/* �����ṩ��̨���������һ̨USB��ӡ���ķ��� */

/* ���¶���������MCS-51��Ƭ��,������Ƭ�������޸�,Ϊ���ṩC���Ե��ٶ���Ҫ�Ա���������Ż� */
#include <reg51.h>
unsigned char volatile xdata	CH375_CMD_PORT _at_ 0xBDF1;	/* CH375����˿ڵ�I/O��ַ */
unsigned char volatile xdata	CH375_DAT_PORT _at_ 0xBCF0;	/* CH375���ݶ˿ڵ�I/O��ַ */
sbit	CH375_INT_WIRE	=		0xB0^2;	/* P3.2, INT0, ����CH375��INT#����,���ڲ�ѯ�ж�״̬ */
typedef	unsigned char BOOL1;  /* typedef	bit	BOOL1; */

/* ����Ϊͨ�õĵ�Ƭ��C���� */
#include <string.h>
#include <stdio.h>

/* ����CH375������뼰����״̬ */
#include "CH375INC.H"

typedef unsigned char	UCHAR;
typedef unsigned short	USHORT;

typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
} USB_DEV_DESCR, *PUSB_DEV_DESCR;

typedef struct _USB_CONFIG_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT wTotalLength;
    UCHAR bNumInterfaces;
    UCHAR bConfigurationValue;
    UCHAR iConfiguration;
    UCHAR bmAttributes;
    UCHAR MaxPower;
} USB_CFG_DESCR, *PUSB_CFG_DESCR;

typedef struct _USB_INTERF_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bInterfaceNumber;
    UCHAR bAlternateSetting;
    UCHAR bNumEndpoints;
    UCHAR bInterfaceClass;
    UCHAR bInterfaceSubClass;
    UCHAR bInterfaceProtocol;
    UCHAR iInterface;
} USB_ITF_DESCR, *PUSB_ITF_DESCR;

typedef struct _USB_ENDPOINT_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bEndpointAddress;
    UCHAR bmAttributes;
    UCHAR wMaxPacketSize;
    UCHAR wMaxPacketSize1;
    UCHAR bInterval;
} USB_ENDP_DESCR, *PUSB_ENDP_DESCR;

typedef struct _USB_CONFIG_DESCRIPTOR_LONG {
	USB_CFG_DESCR	cfg_descr;
	USB_ITF_DESCR	itf_descr;
	USB_ENDP_DESCR	endp_descr[4];
} USB_CFG_DESCR_LONG, *PUSB_CFG_DESCR_LONG;

unsigned char buffer[64];		/* ���û����� */

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

/* �Ժ���Ϊ��λ��ʱ,����ȷ,������24MHzʱ�� */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* ��24MHzʱ������ʱ500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* ��24MHzʱ������ʱ500uS */
	}
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

unsigned char wait_interrupt() {  /* �����˵ȴ��������, ���ز���״̬ */
	unsigned short i;
//	while( CH375_INT_WIRE );  /* ��ѯ�ȴ�CH375��������ж�(INT#�͵�ƽ) */
	for ( i = 0; CH375_INT_WIRE != 0; i ++ ) {  /* ���CH375���ж���������ߵ�ƽ��ȴ�,ͨ��������ֹ��ʱ */
		delay1us();
		if ( i == 0xF000 ) CH375_WR_CMD_PORT( CMD_ABORT_NAK );  /* �����ʱ��61mS������ǿ����ֹNAK����,�жϷ���USB_INT_RET_NAK */
	}

	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* ������������ж�, ��ȡ�ж�״̬ */
	return( CH375_RD_DAT_PORT() );
}

#define	TRUE	1
#define	FALSE	0
unsigned char set_usb_mode( unsigned char mode ) {  /* ����CH375�Ĺ���ģʽ */
	unsigned char i;
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );
	CH375_WR_DAT_PORT( mode );
	for( i=0; i!=100; i++ ) {  /* �ȴ�����ģʽ�������,������30uS */
		if ( CH375_RD_DAT_PORT()==CMD_RET_SUCCESS ) return( TRUE );  /* �ɹ� */
	}
	return( FALSE );  /* CH375����,����оƬ�ͺŴ����ߴ��ڴ��ڷ�ʽ���߲�֧�� */
}

/* ����ͬ�� */
/* USB������ͬ��ͨ���л�DATA0��DATA1ʵ��: ���豸��, USB��ӡ�������Զ��л�;
   ��������, ������SET_ENDP6��SET_ENDP7�������CH375�л�DATA0��DATA1.
   �����˵ĳ�����������Ϊ�豸�˵ĸ����˵�ֱ��ṩһ��ȫ�ֱ���,
   ��ʼֵ��ΪDATA0, ÿִ��һ�γɹ������ȡ��, ÿִ��һ��ʧ��������临λΪDATA1 */

void toggle_recv( BOOL1 tog ) {  /* ��������ͬ������:0=DATA0,1=DATA1 */
	CH375_WR_CMD_PORT( CMD_SET_ENDP6 );
	CH375_WR_DAT_PORT( tog ? 0xC0 : 0x80 );
	delay2us();
}

void toggle_send( BOOL1 tog ) {  /* ��������ͬ������:0=DATA0,1=DATA1 */
	CH375_WR_CMD_PORT( CMD_SET_ENDP7 );
	CH375_WR_DAT_PORT( tog ? 0xC0 : 0x80 );
	delay2us();
}

unsigned char clr_stall( unsigned char endp_addr ) {  /* USBͨѶʧ�ܺ�,��λ�豸�˵�ָ���˵㵽DATA0 */
	CH375_WR_CMD_PORT( CMD_CLR_STALL );
	CH375_WR_DAT_PORT( endp_addr );
	return( wait_interrupt() );
}

/* ���ݶ�д, ��Ƭ����дCH375оƬ�е����ݻ����� */

unsigned char rd_usb_data( unsigned char *buf ) {  /* ��CH37X�������ݿ� */
	unsigned char i, len;
	CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* ��CH375�Ķ˵㻺������ȡ���յ������� */
	len=CH375_RD_DAT_PORT();  /* �������ݳ��� */
	for ( i=0; i!=len; i++ ) *buf++=CH375_RD_DAT_PORT();
	return( len );
}

void wr_usb_data( unsigned char len, unsigned char *buf ) {  /* ��CH37Xд�����ݿ� */
	CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* ��CH375�Ķ˵㻺����д��׼�����͵����� */
	CH375_WR_DAT_PORT( len );  /* �������ݳ���, len���ܴ���64 */
	while( len-- ) CH375_WR_DAT_PORT( *buf++ );
}

/* �������� */
unsigned char endp_out_addr;	/* ��ӡ�����ݽ��ն˵�Ķ˵��ַ */
unsigned char endp_out_size;	/* ��ӡ�����ݽ��ն˵�Ķ˵�ߴ� */
BOOL1	tog_send;				/* ��ӡ�����ݽ��ն˵��ͬ����־ */
unsigned char endp_in_addr;		/* ˫���ӡ�����Ͷ˵�Ķ˵��ַ,һ�㲻�� */
BOOL1	tog_recv;				/* ˫���ӡ�����Ͷ˵��ͬ����־,һ�㲻�� */

unsigned char issue_token( unsigned char endp_and_pid ) {  /* ִ��USB���� */
/* ִ����ɺ�, �������ж�֪ͨ��Ƭ��, �����USB_INT_SUCCESS��˵�������ɹ� */
	CH375_WR_CMD_PORT( CMD_ISSUE_TOKEN );
	CH375_WR_DAT_PORT( endp_and_pid );  /* ��4λĿ�Ķ˵��, ��4λ����PID */
	return( wait_interrupt() );  /* �ȴ�CH375������� */
}

unsigned char issue_token_X( unsigned char endp_and_pid, unsigned char tog ) {  /* ִ��USB����,������CH375A */
/* ִ����ɺ�, �������ж�֪ͨ��Ƭ��, �����USB_INT_SUCCESS��˵�������ɹ� */
	CH375_WR_CMD_PORT( CMD_ISSUE_TKN_X );
	CH375_WR_DAT_PORT( tog );  /* ͬ����־��λ7Ϊ�����˵�IN��ͬ������λ, λ6Ϊ�����˵�OUT��ͬ������λ, λ5~λ0����Ϊ0 */
	CH375_WR_DAT_PORT( endp_and_pid );  /* ��4λĿ�Ķ˵��, ��4λ����PID */
	return( wait_interrupt() );  /* �ȴ�CH375������� */
}

void soft_reset_print( ) {  /* ���ƴ���:����λ��ӡ�� */
	tog_send=tog_recv=0;  /* ��λUSB����ͬ����־ */
	toggle_send( 0 );  /* SETUP�׶�ΪDATA0 */
	buffer[0]=0x21; buffer[1]=2; buffer[2]=buffer[3]=buffer[4]=buffer[5]=buffer[6]=buffer[7]=0;  /* SETUP����,SOFT_RESET */
	wr_usb_data( 8, buffer );  /* SETUP��������8�ֽ� */
	if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_SETUP )==USB_INT_SUCCESS ) {  /* SETUP�׶β����ɹ� */
		toggle_recv( 1 );  /* STATUS�׶�,׼������DATA1 */
		if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_IN )==USB_INT_SUCCESS ) return;  /* STATUS�׶β����ɹ�,�����ɹ����� */
	}
}

#define	USB_INT_RET_NAK		0x2A		/* 00101010B,����NAK */
void send_data( unsigned short len, unsigned char *buf ) {  /* �����������ݿ�,һ�����64KB */
	unsigned char l, s;
	while( len ) {  /* ����������ݿ��USB��ӡ�� */
		toggle_send( tog_send );  /* ����ͬ�� */
		l = len>endp_out_size?endp_out_size:len;  /* ���η��Ͳ��ܳ����˵�ߴ� */
		wr_usb_data( l, buf );  /* �������ȸ��Ƶ�CH375оƬ�� */
		s = issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_OUT );  /* ����CH375������� */
		if ( s==USB_INT_SUCCESS ) {  /* CH375�ɹ��������� */
			tog_send = ~ tog_send;  /* �л�DATA0��DATA1��������ͬ�� */
			len-=l;  /* ���� */
			buf+=l;  /* �����ɹ� */
		}
		else if ( s==USB_INT_RET_NAK ) {  /* USB��ӡ����æ,���δִ��SET_RETRY������CH375�Զ�����,���Բ��᷵��USB_INT_RET_NAK״̬ */
			/* USB��ӡ����æ,���������Ӧ���Ժ����� */
			/* s=get_port_status( );  ����б�Ҫ,���Լ����ʲôԭ���´�ӡ��æ */
		}
		else {  /* ����ʧ��,��������²���ʧ�� */
			clr_stall( endp_out_addr );  /* �����ӡ�������ݽ��ն˵�,���� soft_reset_print() */
/*			soft_reset_print();  ��ӡ�������������,����λ */
			tog_send = 0;  /* ����ʧ�� */
		}
/* ����������ϴ�,���Զ��ڵ���get_port_status()����ӡ��״̬ */
	}
}

unsigned char get_port_status( ) {  /* ��ѯ��ӡ���˿�״̬,����״̬��,���Ϊ0FFH��˵������ʧ�� */
/* ����״̬����: λ5(Paper Empty)Ϊ1˵����ֽ, λ4(Select)Ϊ1˵����ӡ������, λ3(Not Error)Ϊ0˵����ӡ������ */
	toggle_send( 0 );  /* ����ͨ�����ƴ����ȡ��ӡ����״̬, SETUP�׶�ΪDATA0 */
	buffer[0]=0xA1; buffer[1]=1; buffer[2]=buffer[3]=buffer[4]=buffer[5]=0; buffer[6]=1; buffer[7]=0;  /* SETUP����,GET_PORT_STATUS */
	wr_usb_data( 8, buffer );  /* SETUP��������8�ֽ� */
	if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_SETUP )==USB_INT_SUCCESS ) {  /* SETUP�׶β����ɹ� */
		toggle_recv( 1 );  /* DATA�׶�,׼������DATA1 */
		if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_IN )==USB_INT_SUCCESS ) {  /* DATA�׶β����ɹ� */
			rd_usb_data( buffer );  /* �������յ�������,ͨ��ֻ��1���ֽ� */
			toggle_send( 1 );  /* STATUS�׶�ΪDATA1 */
			wr_usb_data( 0, buffer );  /* ����0���ȵ�����˵�����ƴ���ɹ� */
			if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_OUT )==USB_INT_SUCCESS ) return( buffer[0] );  /* ����״̬�� */
		}
	}
	return( 0xFF );  /* ���ز���ʧ�� */
}

unsigned char get_port_status_X( ) {  /* ��ѯ��ӡ���˿�״̬,����״̬��,���Ϊ0FFH��˵������ʧ��,������CH375A */
/* ����״̬����: λ5(Paper Empty)Ϊ1˵����ֽ, λ4(Select)Ϊ1˵����ӡ������, λ3(Not Error)Ϊ0˵����ӡ������ */
	buffer[0]=0xA1; buffer[1]=1; buffer[2]=buffer[3]=buffer[4]=buffer[5]=0; buffer[6]=1; buffer[7]=0;  /* ���ƴ����ȡ��ӡ��״̬,SETUP���� */
	wr_usb_data( 8, buffer );  /* SETUP��������8�ֽ� */
	if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_SETUP, 0x00 )==USB_INT_SUCCESS ) {  /* SETUP�׶�DATA0�����ɹ� */
		if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_IN, 0x80 )==USB_INT_SUCCESS ) {  /* DATA�׶�DATA1���ղ����ɹ� */
			rd_usb_data( buffer );  /* �������յ�������,ͨ��ֻ��1���ֽ� */
			wr_usb_data( 0, buffer );  /* ����0���ȵ�����DATA1˵�����ƴ���ɹ� */
			if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_OUT, 0x40 )==USB_INT_SUCCESS ) return( buffer[0] );  /* STATUS�׶β����ɹ�,����״̬�� */
		}
	}
	return( 0xFF );  /* ���ز���ʧ�� */
}

unsigned char get_descr( unsigned char type ) {  /* ���豸�˻�ȡ������ */
	CH375_WR_CMD_PORT( CMD_GET_DESCR );
	CH375_WR_DAT_PORT( type );  /* ����������, ֻ֧��1(�豸)����2(����) */
	return( wait_interrupt() );  /* �ȴ�CH375������� */
}

unsigned char set_addr( unsigned char addr ) {  /* �����豸�˵�USB��ַ */
	unsigned char status;
	CH375_WR_CMD_PORT( CMD_SET_ADDRESS );  /* ����USB�豸�˵�USB��ַ */
	CH375_WR_DAT_PORT( addr );  /* ��ַ, ��1��127֮�������ֵ, ����2��20 */
	status=wait_interrupt();  /* �ȴ�CH375������� */
	if ( status==USB_INT_SUCCESS ) {  /* �����ɹ� */
		CH375_WR_CMD_PORT( CMD_SET_USB_ADDR );  /* ����USB�����˵�USB��ַ */
		CH375_WR_DAT_PORT( addr );  /* ��Ŀ��USB�豸�ĵ�ַ�ɹ��޸ĺ�,Ӧ��ͬ���޸������˵�USB��ַ */
	}
	mDelaymS( 5 );
	return( status );
}

unsigned char set_config( unsigned char cfg ) {  /* �����豸�˵�USB���� */
	tog_send=tog_recv=0;  /* ��λUSB����ͬ����־ */
	CH375_WR_CMD_PORT( CMD_SET_CONFIG );  /* ����USB�豸�˵�����ֵ */
	CH375_WR_DAT_PORT( cfg );  /* ��ֵȡ��USB�豸�������������� */
	return( wait_interrupt() );  /* �ȴ�CH375������� */
}

#define	UNKNOWN_USB_DEVICE	0xF1
#define	UNKNOWN_USB_PRINT	0xF2

unsigned char init_print() {  /* ��ʼ��USB��ӡ��,��ɴ�ӡ��ö�� */
#define	p_dev_descr		((PUSB_DEV_DESCR)buffer)
#define	p_cfg_descr		((PUSB_CFG_DESCR_LONG)buffer)
	unsigned char status, len, c;
	status=get_descr(1);  /* ��ȡ�豸������ */
	if ( status==USB_INT_SUCCESS ) {
		len=rd_usb_data( buffer );  /* ����ȡ�����������ݴ�CH375�ж�������Ƭ����RAM��������,�������������� */
		if ( len<18 || p_dev_descr->bDescriptorType!=1 ) return( UNKNOWN_USB_DEVICE );  /* �������:���������ȴ���������ʹ��� */
		if ( p_dev_descr->bDeviceClass!=0 ) return( UNKNOWN_USB_DEVICE );  /* ���ӵ�USB�豸����USB��ӡ��,���߲�����USB�淶 */
		status=set_addr(3);  /* ���ô�ӡ����USB��ַ */
		if ( status==USB_INT_SUCCESS ) {
			status=get_descr(2);  /* ��ȡ���������� */
			if ( status==USB_INT_SUCCESS ) {  /* �����ɹ������������������ */
				len=rd_usb_data( buffer );  /* ����ȡ�����������ݴ�CH375�ж�������Ƭ����RAM��������,�������������� */
				if ( p_cfg_descr->itf_descr.bInterfaceClass!=7 || p_cfg_descr->itf_descr.bInterfaceSubClass!=1 ) return( UNKNOWN_USB_PRINT );  /* ����USB��ӡ�����߲�����USB�淶 */
				endp_out_addr=endp_in_addr=0;
				c=p_cfg_descr->endp_descr[0].bEndpointAddress;  /* ��һ���˵�ĵ�ַ */
				if ( c&0x80 ) endp_in_addr=c&0x0f;  /* IN�˵�ĵ�ַ */
				else {  /* OUT�˵� */
					endp_out_addr=c&0x0f;
					endp_out_size=p_cfg_descr->endp_descr[0].wMaxPacketSize;  /* ���ݽ��ն˵���������� */
				}
				if ( p_cfg_descr->itf_descr.bNumEndpoints>=2 ) {  /* �ӿ����������ϵĶ˵� */
					if ( p_cfg_descr->endp_descr[1].bDescriptorType==5 ) {  /* �˵������� */
						c=p_cfg_descr->endp_descr[1].bEndpointAddress;  /* �ڶ����˵�ĵ�ַ */
						if ( c&0x80 ) endp_in_addr=c&0x0f;  /* IN�˵� */
						else {  /* OUT�˵� */
							endp_out_addr=c&0x0f;
							endp_out_size=p_cfg_descr->endp_descr[1].wMaxPacketSize;
						}
					}
				}
				if ( p_cfg_descr->itf_descr.bInterfaceProtocol<=1 ) endp_in_addr=0;  /* ����ӿڲ���ҪIN�˵� */
				if ( endp_out_addr==0 ) return( UNKNOWN_USB_PRINT );  /* ����USB��ӡ�����߲�����USB�淶 */
				status=set_config( p_cfg_descr->cfg_descr.bConfigurationValue );  /* ����USB����ֵ */
				if ( status==USB_INT_SUCCESS ) {
					CH375_WR_CMD_PORT( CMD_SET_RETRY );  /* ����USB������������Դ��� */
					CH375_WR_DAT_PORT( 0x25 );
					CH375_WR_DAT_PORT( 0x89 );  /* λ7Ϊ1���յ�NAKʱ��������, λ3~λ0Ϊ��ʱ������Դ��� */
/* �����Ƭ���ڴ�ӡ��æʱ�����¿���,��������λ7Ϊ1,ʹCH375���յ�NAKʱ�Զ�����ֱ�������ɹ�����ʧ�� */
/* ���ϣ����Ƭ���ڴ�ӡ��æʱ�ܹ���������,��ôӦ������λ7Ϊ0,ʹCH375���յ�NAKʱ������,
   �����������USBͨѶ������,���USB��ӡ����æ,issue_token���ӳ��򽫵õ�״̬��USB_INT_RET_NAK */
				}
			}
		}
	}
	return(status);
}

/* �����˵��������ʾ�� */
main() {
	unsigned char xdata data_to_send[200];  /* ������ */
	unsigned char str_to_print[]="OK, support text print\n";
	unsigned char s;
	mDelaymS( 200 );
	set_usb_mode( 6 );  /* ����USB����ģʽ */
	while ( wait_interrupt()!=USB_INT_CONNECT );  /* �ȴ�USB��ӡ���������� */

/* ����豸����CH341ת��ӡ�ڻ�����CH37X,��ô���²����ǿ�ѡ��,���������USBоƬ,��ô������Ҫִ�����²��� */
#define USB_RESET_FIRST	1  /* USB�淶��δҪ����USB�豸�������븴λ���豸,���Ǽ������WINDOWS����������,������ЩUSB�豸ҲҪ���ڲ��������ȸ�λ���ܹ��� */
#ifdef USB_RESET_FIRST
	set_usb_mode( 7 );  /* ��λUSB�豸,CH375��USB�ź��ߵ�D+��D-����͵�ƽ */
/* �����Ƭ����CH375��INT#���Ų����жϷ�ʽ�����ǲ�ѯ��ʽ,��ôӦ���ڸ���USB�豸�ڼ��ֹCH375�ж�,��USB�豸��λ��ɺ����CH375�жϱ�־�������ж� */
	mDelaymS( 10 );  /* ��λʱ�䲻����1mS,����Ϊ10mS */
	set_usb_mode( 6 );  /* ������λ */
	mDelaymS( 100 );
	while ( wait_interrupt()!=USB_INT_CONNECT );  /* �ȴ���λ֮����豸���ٴ��������� */
#endif

	mDelaymS( 200 );  /* ��ЩUSB�豸Ҫ�ȴ����ٺ�������������� */
	if ( init_print()!=USB_INT_SUCCESS ) while(1);  /* ���� */
	while ( 1 ) {
		s = get_port_status( );
		if ( s!=0xFF ) {
/*			if ( s&0x20 ) printf("No paper!\n");
			if ( (s&0x08) == 0 ) printf("Print ERROR!\n");*/
		}
		send_data( strlen(str_to_print), str_to_print );  /* ������ݸ���ӡ��,��ͨ�����ڷ�ʽ���һ�� */
		send_data( sizeof(data_to_send), data_to_send );  /* ��������ݱ��밴�մ�ӡ���ĸ�ʽҪ�������ѭ��ӡ�������� */
		/* �����ٴμ����������ݻ��߽������� */
	}
}