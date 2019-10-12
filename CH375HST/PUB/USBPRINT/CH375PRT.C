/* 2004.03.05, 2004.8.18, 2005.12.29
****************************************
**  Copyright  (C)  W.ch  1999-2005   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB 1.1 Host Examples for CH375   **
**  KC7.0@MCS-51                      **
****************************************
*/
/* 单片机通过CH375控制USB打印机 */
/* 程序示例,C语言,CH375中断为查询方式,只负责数据传输,不涉及打印格式及打印描述语言 */
/* 另可提供多台计算机共享一台USB打印机的方案 */

/* 以下定义适用于MCS-51单片机,其它单片机参照修改,为了提供C语言的速度需要对本程序进行优化 */
#include <reg51.h>
unsigned char volatile xdata	CH375_CMD_PORT _at_ 0xBDF1;	/* CH375命令端口的I/O地址 */
unsigned char volatile xdata	CH375_DAT_PORT _at_ 0xBCF0;	/* CH375数据端口的I/O地址 */
sbit	CH375_INT_WIRE	=		0xB0^2;	/* P3.2, INT0, 连接CH375的INT#引脚,用于查询中断状态 */
typedef	unsigned char BOOL1;  /* typedef	bit	BOOL1; */

/* 以下为通用的单片机C程序 */
#include <string.h>
#include <stdio.h>

/* 定义CH375命令代码及返回状态 */
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

unsigned char buffer[64];		/* 公用缓冲区 */

/* 延时2微秒,不精确 */
void	delay2us( )
{
	unsigned char i;
	for ( i = 2; i != 0; i -- );
}

/* 延时1微秒,不精确 */
void	delay1us( )
{
	unsigned char i;
	for ( i = 1; i != 0; i -- );
}

/* 以毫秒为单位延时,不精确,适用于24MHz时钟 */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
	}
}

/* 基本操作 */

void CH375_WR_CMD_PORT( unsigned char cmd ) {  /* 向CH375的命令端口写入命令,周期不小于4uS,如果单片机较快则延时 */
	delay2us();
	CH375_CMD_PORT=cmd;
	delay2us();
}

void CH375_WR_DAT_PORT( unsigned char dat ) {  /* 向CH375的数据端口写入数据,周期不小于1.5uS,如果单片机较快则延时 */
	CH375_DAT_PORT=dat;
	delay1us();  /* 因为MCS51单片机较慢所以实际上无需延时 */
}

unsigned char CH375_RD_DAT_PORT() {  /* 从CH375的数据端口读出数据,周期不小于1.5uS,如果单片机较快则延时 */
	delay1us();  /* 因为MCS51单片机较慢所以实际上无需延时 */
	return( CH375_DAT_PORT );
}

unsigned char wait_interrupt() {  /* 主机端等待操作完成, 返回操作状态 */
	unsigned short i;
//	while( CH375_INT_WIRE );  /* 查询等待CH375操作完成中断(INT#低电平) */
	for ( i = 0; CH375_INT_WIRE != 0; i ++ ) {  /* 如果CH375的中断引脚输出高电平则等待,通过计数防止超时 */
		delay1us();
		if ( i == 0xF000 ) CH375_WR_CMD_PORT( CMD_ABORT_NAK );  /* 如果超时达61mS以上则强行终止NAK重试,中断返回USB_INT_RET_NAK */
	}

	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 产生操作完成中断, 获取中断状态 */
	return( CH375_RD_DAT_PORT() );
}

#define	TRUE	1
#define	FALSE	0
unsigned char set_usb_mode( unsigned char mode ) {  /* 设置CH375的工作模式 */
	unsigned char i;
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );
	CH375_WR_DAT_PORT( mode );
	for( i=0; i!=100; i++ ) {  /* 等待设置模式操作完成,不超过30uS */
		if ( CH375_RD_DAT_PORT()==CMD_RET_SUCCESS ) return( TRUE );  /* 成功 */
	}
	return( FALSE );  /* CH375出错,例如芯片型号错或者处于串口方式或者不支持 */
}

/* 数据同步 */
/* USB的数据同步通过切换DATA0和DATA1实现: 在设备端, USB打印机可以自动切换;
   在主机端, 必须由SET_ENDP6和SET_ENDP7命令控制CH375切换DATA0与DATA1.
   主机端的程序处理方法是为设备端的各个端点分别提供一个全局变量,
   初始值均为DATA0, 每执行一次成功事务后取反, 每执行一次失败事务后将其复位为DATA1 */

void toggle_recv( BOOL1 tog ) {  /* 主机接收同步控制:0=DATA0,1=DATA1 */
	CH375_WR_CMD_PORT( CMD_SET_ENDP6 );
	CH375_WR_DAT_PORT( tog ? 0xC0 : 0x80 );
	delay2us();
}

void toggle_send( BOOL1 tog ) {  /* 主机发送同步控制:0=DATA0,1=DATA1 */
	CH375_WR_CMD_PORT( CMD_SET_ENDP7 );
	CH375_WR_DAT_PORT( tog ? 0xC0 : 0x80 );
	delay2us();
}

unsigned char clr_stall( unsigned char endp_addr ) {  /* USB通讯失败后,复位设备端的指定端点到DATA0 */
	CH375_WR_CMD_PORT( CMD_CLR_STALL );
	CH375_WR_DAT_PORT( endp_addr );
	return( wait_interrupt() );
}

/* 数据读写, 单片机读写CH375芯片中的数据缓冲区 */

unsigned char rd_usb_data( unsigned char *buf ) {  /* 从CH37X读出数据块 */
	unsigned char i, len;
	CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* 从CH375的端点缓冲区读取接收到的数据 */
	len=CH375_RD_DAT_PORT();  /* 后续数据长度 */
	for ( i=0; i!=len; i++ ) *buf++=CH375_RD_DAT_PORT();
	return( len );
}

void wr_usb_data( unsigned char len, unsigned char *buf ) {  /* 向CH37X写入数据块 */
	CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* 向CH375的端点缓冲区写入准备发送的数据 */
	CH375_WR_DAT_PORT( len );  /* 后续数据长度, len不能大于64 */
	while( len-- ) CH375_WR_DAT_PORT( *buf++ );
}

/* 主机操作 */
unsigned char endp_out_addr;	/* 打印机数据接收端点的端点地址 */
unsigned char endp_out_size;	/* 打印机数据接收端点的端点尺寸 */
BOOL1	tog_send;				/* 打印机数据接收端点的同步标志 */
unsigned char endp_in_addr;		/* 双向打印机发送端点的端点地址,一般不用 */
BOOL1	tog_recv;				/* 双向打印机发送端点的同步标志,一般不用 */

unsigned char issue_token( unsigned char endp_and_pid ) {  /* 执行USB事务 */
/* 执行完成后, 将产生中断通知单片机, 如果是USB_INT_SUCCESS就说明操作成功 */
	CH375_WR_CMD_PORT( CMD_ISSUE_TOKEN );
	CH375_WR_DAT_PORT( endp_and_pid );  /* 高4位目的端点号, 低4位令牌PID */
	return( wait_interrupt() );  /* 等待CH375操作完成 */
}

unsigned char issue_token_X( unsigned char endp_and_pid, unsigned char tog ) {  /* 执行USB事务,适用于CH375A */
/* 执行完成后, 将产生中断通知单片机, 如果是USB_INT_SUCCESS就说明操作成功 */
	CH375_WR_CMD_PORT( CMD_ISSUE_TKN_X );
	CH375_WR_DAT_PORT( tog );  /* 同步标志的位7为主机端点IN的同步触发位, 位6为主机端点OUT的同步触发位, 位5~位0必须为0 */
	CH375_WR_DAT_PORT( endp_and_pid );  /* 高4位目的端点号, 低4位令牌PID */
	return( wait_interrupt() );  /* 等待CH375操作完成 */
}

void soft_reset_print( ) {  /* 控制传输:软复位打印机 */
	tog_send=tog_recv=0;  /* 复位USB数据同步标志 */
	toggle_send( 0 );  /* SETUP阶段为DATA0 */
	buffer[0]=0x21; buffer[1]=2; buffer[2]=buffer[3]=buffer[4]=buffer[5]=buffer[6]=buffer[7]=0;  /* SETUP数据,SOFT_RESET */
	wr_usb_data( 8, buffer );  /* SETUP数据总是8字节 */
	if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_SETUP )==USB_INT_SUCCESS ) {  /* SETUP阶段操作成功 */
		toggle_recv( 1 );  /* STATUS阶段,准备接收DATA1 */
		if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_IN )==USB_INT_SUCCESS ) return;  /* STATUS阶段操作成功,操作成功返回 */
	}
}

#define	USB_INT_RET_NAK		0x2A		/* 00101010B,返回NAK */
void send_data( unsigned short len, unsigned char *buf ) {  /* 主机发送数据块,一次最多64KB */
	unsigned char l, s;
	while( len ) {  /* 连续输出数据块给USB打印机 */
		toggle_send( tog_send );  /* 数据同步 */
		l = len>endp_out_size?endp_out_size:len;  /* 单次发送不能超过端点尺寸 */
		wr_usb_data( l, buf );  /* 将数据先复制到CH375芯片中 */
		s = issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_OUT );  /* 请求CH375输出数据 */
		if ( s==USB_INT_SUCCESS ) {  /* CH375成功发出数据 */
			tog_send = ~ tog_send;  /* 切换DATA0和DATA1进行数据同步 */
			len-=l;  /* 计数 */
			buf+=l;  /* 操作成功 */
		}
		else if ( s==USB_INT_RET_NAK ) {  /* USB打印机正忙,如果未执行SET_RETRY命令则CH375自动重试,所以不会返回USB_INT_RET_NAK状态 */
			/* USB打印机正忙,正常情况下应该稍后重试 */
			/* s=get_port_status( );  如果有必要,可以检查是什么原因导致打印机忙 */
		}
		else {  /* 操作失败,正常情况下不会失败 */
			clr_stall( endp_out_addr );  /* 清除打印机的数据接收端点,或者 soft_reset_print() */
/*			soft_reset_print();  打印机出现意外错误,软复位 */
			tog_send = 0;  /* 操作失败 */
		}
/* 如果数据量较大,可以定期调用get_port_status()检查打印机状态 */
	}
}

unsigned char get_port_status( ) {  /* 查询打印机端口状态,返回状态码,如果为0FFH则说明操作失败 */
/* 返回状态码中: 位5(Paper Empty)为1说明无纸, 位4(Select)为1说明打印机联机, 位3(Not Error)为0说明打印机出错 */
	toggle_send( 0 );  /* 下面通过控制传输获取打印机的状态, SETUP阶段为DATA0 */
	buffer[0]=0xA1; buffer[1]=1; buffer[2]=buffer[3]=buffer[4]=buffer[5]=0; buffer[6]=1; buffer[7]=0;  /* SETUP数据,GET_PORT_STATUS */
	wr_usb_data( 8, buffer );  /* SETUP数据总是8字节 */
	if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_SETUP )==USB_INT_SUCCESS ) {  /* SETUP阶段操作成功 */
		toggle_recv( 1 );  /* DATA阶段,准备接收DATA1 */
		if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_IN )==USB_INT_SUCCESS ) {  /* DATA阶段操作成功 */
			rd_usb_data( buffer );  /* 读出接收到的数据,通常只有1个字节 */
			toggle_send( 1 );  /* STATUS阶段为DATA1 */
			wr_usb_data( 0, buffer );  /* 发送0长度的数据说明控制传输成功 */
			if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_OUT )==USB_INT_SUCCESS ) return( buffer[0] );  /* 返回状态码 */
		}
	}
	return( 0xFF );  /* 返回操作失败 */
}

unsigned char get_port_status_X( ) {  /* 查询打印机端口状态,返回状态码,如果为0FFH则说明操作失败,适用于CH375A */
/* 返回状态码中: 位5(Paper Empty)为1说明无纸, 位4(Select)为1说明打印机联机, 位3(Not Error)为0说明打印机出错 */
	buffer[0]=0xA1; buffer[1]=1; buffer[2]=buffer[3]=buffer[4]=buffer[5]=0; buffer[6]=1; buffer[7]=0;  /* 控制传输获取打印机状态,SETUP数据 */
	wr_usb_data( 8, buffer );  /* SETUP数据总是8字节 */
	if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_SETUP, 0x00 )==USB_INT_SUCCESS ) {  /* SETUP阶段DATA0操作成功 */
		if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_IN, 0x80 )==USB_INT_SUCCESS ) {  /* DATA阶段DATA1接收操作成功 */
			rd_usb_data( buffer );  /* 读出接收到的数据,通常只有1个字节 */
			wr_usb_data( 0, buffer );  /* 发送0长度的数据DATA1说明控制传输成功 */
			if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_OUT, 0x40 )==USB_INT_SUCCESS ) return( buffer[0] );  /* STATUS阶段操作成功,返回状态码 */
		}
	}
	return( 0xFF );  /* 返回操作失败 */
}

unsigned char get_descr( unsigned char type ) {  /* 从设备端获取描述符 */
	CH375_WR_CMD_PORT( CMD_GET_DESCR );
	CH375_WR_DAT_PORT( type );  /* 描述符类型, 只支持1(设备)或者2(配置) */
	return( wait_interrupt() );  /* 等待CH375操作完成 */
}

unsigned char set_addr( unsigned char addr ) {  /* 设置设备端的USB地址 */
	unsigned char status;
	CH375_WR_CMD_PORT( CMD_SET_ADDRESS );  /* 设置USB设备端的USB地址 */
	CH375_WR_DAT_PORT( addr );  /* 地址, 从1到127之间的任意值, 常用2到20 */
	status=wait_interrupt();  /* 等待CH375操作完成 */
	if ( status==USB_INT_SUCCESS ) {  /* 操作成功 */
		CH375_WR_CMD_PORT( CMD_SET_USB_ADDR );  /* 设置USB主机端的USB地址 */
		CH375_WR_DAT_PORT( addr );  /* 当目标USB设备的地址成功修改后,应该同步修改主机端的USB地址 */
	}
	mDelaymS( 5 );
	return( status );
}

unsigned char set_config( unsigned char cfg ) {  /* 设置设备端的USB配置 */
	tog_send=tog_recv=0;  /* 复位USB数据同步标志 */
	CH375_WR_CMD_PORT( CMD_SET_CONFIG );  /* 设置USB设备端的配置值 */
	CH375_WR_DAT_PORT( cfg );  /* 此值取自USB设备的配置描述符中 */
	return( wait_interrupt() );  /* 等待CH375操作完成 */
}

#define	UNKNOWN_USB_DEVICE	0xF1
#define	UNKNOWN_USB_PRINT	0xF2

unsigned char init_print() {  /* 初始化USB打印机,完成打印机枚举 */
#define	p_dev_descr		((PUSB_DEV_DESCR)buffer)
#define	p_cfg_descr		((PUSB_CFG_DESCR_LONG)buffer)
	unsigned char status, len, c;
	status=get_descr(1);  /* 获取设备描述符 */
	if ( status==USB_INT_SUCCESS ) {
		len=rd_usb_data( buffer );  /* 将获取的描述符数据从CH375中读出到单片机的RAM缓冲区中,返回描述符长度 */
		if ( len<18 || p_dev_descr->bDescriptorType!=1 ) return( UNKNOWN_USB_DEVICE );  /* 意外错误:描述符长度错误或者类型错误 */
		if ( p_dev_descr->bDeviceClass!=0 ) return( UNKNOWN_USB_DEVICE );  /* 连接的USB设备不是USB打印机,或者不符合USB规范 */
		status=set_addr(3);  /* 设置打印机的USB地址 */
		if ( status==USB_INT_SUCCESS ) {
			status=get_descr(2);  /* 获取配置描述符 */
			if ( status==USB_INT_SUCCESS ) {  /* 操作成功则读出描述符并分析 */
				len=rd_usb_data( buffer );  /* 将获取的描述符数据从CH375中读出到单片机的RAM缓冲区中,返回描述符长度 */
				if ( p_cfg_descr->itf_descr.bInterfaceClass!=7 || p_cfg_descr->itf_descr.bInterfaceSubClass!=1 ) return( UNKNOWN_USB_PRINT );  /* 不是USB打印机或者不符合USB规范 */
				endp_out_addr=endp_in_addr=0;
				c=p_cfg_descr->endp_descr[0].bEndpointAddress;  /* 第一个端点的地址 */
				if ( c&0x80 ) endp_in_addr=c&0x0f;  /* IN端点的地址 */
				else {  /* OUT端点 */
					endp_out_addr=c&0x0f;
					endp_out_size=p_cfg_descr->endp_descr[0].wMaxPacketSize;  /* 数据接收端点的最大包长度 */
				}
				if ( p_cfg_descr->itf_descr.bNumEndpoints>=2 ) {  /* 接口有两个以上的端点 */
					if ( p_cfg_descr->endp_descr[1].bDescriptorType==5 ) {  /* 端点描述符 */
						c=p_cfg_descr->endp_descr[1].bEndpointAddress;  /* 第二个端点的地址 */
						if ( c&0x80 ) endp_in_addr=c&0x0f;  /* IN端点 */
						else {  /* OUT端点 */
							endp_out_addr=c&0x0f;
							endp_out_size=p_cfg_descr->endp_descr[1].wMaxPacketSize;
						}
					}
				}
				if ( p_cfg_descr->itf_descr.bInterfaceProtocol<=1 ) endp_in_addr=0;  /* 单向接口不需要IN端点 */
				if ( endp_out_addr==0 ) return( UNKNOWN_USB_PRINT );  /* 不是USB打印机或者不符合USB规范 */
				status=set_config( p_cfg_descr->cfg_descr.bConfigurationValue );  /* 加载USB配置值 */
				if ( status==USB_INT_SUCCESS ) {
					CH375_WR_CMD_PORT( CMD_SET_RETRY );  /* 设置USB事务操作的重试次数 */
					CH375_WR_DAT_PORT( 0x25 );
					CH375_WR_DAT_PORT( 0x89 );  /* 位7为1则收到NAK时无限重试, 位3~位0为超时后的重试次数 */
/* 如果单片机在打印机忙时并无事可做,建议设置位7为1,使CH375在收到NAK时自动重试直到操作成功或者失败 */
/* 如果希望单片机在打印机忙时能够做其它事,那么应该设置位7为0,使CH375在收到NAK时不重试,
   所以在下面的USB通讯过程中,如果USB打印机正忙,issue_token等子程序将得到状态码USB_INT_RET_NAK */
				}
			}
		}
	}
	return(status);
}

/* 主机端的主程序简单示例 */
main() {
	unsigned char xdata data_to_send[200];  /* 缓冲区 */
	unsigned char str_to_print[]="OK, support text print\n";
	unsigned char s;
	mDelaymS( 200 );
	set_usb_mode( 6 );  /* 设置USB主机模式 */
	while ( wait_interrupt()!=USB_INT_CONNECT );  /* 等待USB打印机连接上来 */

/* 如果设备端是CH341转打印口或者是CH37X,那么以下步骤是可选的,如果是其它USB芯片,那么可能需要执行以下步骤 */
#define USB_RESET_FIRST	1  /* USB规范中未要求在USB设备插入后必须复位该设备,但是计算机的WINDOWS总是这样做,所以有些USB设备也要求在插入后必须先复位才能工作 */
#ifdef USB_RESET_FIRST
	set_usb_mode( 7 );  /* 复位USB设备,CH375向USB信号线的D+和D-输出低电平 */
/* 如果单片机对CH375的INT#引脚采用中断方式而不是查询方式,那么应该在复制USB设备期间禁止CH375中断,在USB设备复位完成后清除CH375中断标志再允许中断 */
	mDelaymS( 10 );  /* 复位时间不少于1mS,建议为10mS */
	set_usb_mode( 6 );  /* 结束复位 */
	mDelaymS( 100 );
	while ( wait_interrupt()!=USB_INT_CONNECT );  /* 等待复位之后的设备端再次连接上来 */
#endif

	mDelaymS( 200 );  /* 有些USB设备要等待数百毫秒才能正常工作 */
	if ( init_print()!=USB_INT_SUCCESS ) while(1);  /* 错误 */
	while ( 1 ) {
		s = get_port_status( );
		if ( s!=0xFF ) {
/*			if ( s&0x20 ) printf("No paper!\n");
			if ( (s&0x08) == 0 ) printf("Print ERROR!\n");*/
		}
		send_data( strlen(str_to_print), str_to_print );  /* 输出数据给打印机,与通过并口方式输出一样 */
		send_data( sizeof(data_to_send), data_to_send );  /* 输出的数据必须按照打印机的格式要求或者遵循打印描述语言 */
		/* 可以再次继续发送数据或者接收数据 */
	}
}
