/* 2004.03.05, 2004.8.18
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB 1.1 Host Examples for CH375   **
**  KC7.0@MCS-51                      **
****************************************
*/
/* 用CH375+CH372连接两个单片机系统,如果设备端不是CH37X,那么可以参考CH375PRT.C分析描述符 */
/* 主机端的程序示例,C语言,CH375中断为查询方式 */

/* 以下定义适用于MCS-51单片机,其它单片机参照修改,为了提供C语言的速度需要对本程序进行优化 */
#include <reg51.h>
unsigned char volatile xdata	CH375_CMD_PORT _at_ 0xBDF1;	/* CH375命令端口的I/O地址 */
unsigned char volatile xdata	CH375_DAT_PORT _at_ 0xBCF0;	/* CH375数据端口的I/O地址 */
sbit	CH375_INT_WIRE	=		0xB0^2;	/* P3.2, INT0, 连接CH375的INT#引脚,用于查询中断状态 */

/* 以下为通用的单片机C程序 */
#include <string.h>
#include <stdio.h>

/* 定义CH375命令代码及返回状态 */
#include "CH375INC.H"

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

/* 基本操作 */

void ERROR() {
	while(1);
}

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
	while( CH375_INT_WIRE );  /* 查询等待CH375操作完成中断(INT#低电平) */
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 产生操作完成中断, 获取中断状态 */
	return( CH375_RD_DAT_PORT() );
}

unsigned char endp6_mode, endp7_mode;

#define	TRUE	1
#define	FALSE	0
unsigned char set_usb_mode( unsigned char mode ) {  /* 设置CH37X的工作模式 */
	unsigned char i;
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );
	CH375_WR_DAT_PORT( mode );
	endp6_mode=endp7_mode=0x80;  /* 主机端复位USB数据同步标志 */
	for( i=0; i!=100; i++ ) {  /* 等待设置模式操作完成,不超过30uS */
		if ( CH375_RD_DAT_PORT()==CMD_RET_SUCCESS ) return( TRUE );  /* 成功 */
	}
	return( FALSE );  /* CH375出错,例如芯片型号错或者处于串口方式或者不支持 */
}

/* 数据同步 */
/* USB的数据同步通过切换DATA0和DATA1实现: 在设备端, CH372/CH375可以自动切换;
   在主机端, 必须由SET_ENDP6和SET_ENDP7命令控制CH375切换DATA0与DATA1.
   主机端的程序处理方法是为SET_ENDP6和SET_ENDP7分别提供一个全局变量,
   初始值均为80H, 每执行一次成功事务后将位6取反, 每执行一次失败事务后将其复位为80H. */

void toggle_recv() {  /* 主机接收成功后,切换DATA0和DATA1实现数据同步 */
	CH375_WR_CMD_PORT( CMD_SET_ENDP6 );
	CH375_WR_DAT_PORT( endp6_mode );
	endp6_mode^=0x40;
	delay2us();
}

void toggle_send() {  /* 主机发送成功后,切换DATA0和DATA1实现数据同步 */
	CH375_WR_CMD_PORT( CMD_SET_ENDP7 );
	CH375_WR_DAT_PORT( endp7_mode );
	endp7_mode^=0x40;
	delay2us();
}

unsigned char clr_stall6() {  /* 主机接收失败后,复位设备端的数据同步到DATA0 */
	CH375_WR_CMD_PORT( CMD_CLR_STALL );
	CH375_WR_DAT_PORT( 2 | 0x80 );  /* 如果设备端不是CH37X芯片,那么需要修改端点号 */
	endp6_mode=0x80;
	return( wait_interrupt() );
}

unsigned char clr_stall7() {  /* 主机发送失败后,复位设备端的数据同步到DATA0 */
	CH375_WR_CMD_PORT( CMD_CLR_STALL );
	CH375_WR_DAT_PORT( 2 );  /* 如果设备端不是CH37X芯片,那么需要修改端点号 */
	endp7_mode=0x80;
	return( wait_interrupt() );
}

/* 数据读写, 单片机读写CH372或者CH375芯片中的数据缓冲区 */

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

unsigned char issue_token( unsigned char endp_and_pid ) {  /* 执行USB事务 */
/* 执行完成后, 将产生中断通知单片机, 如果是USB_INT_SUCCESS就说明操作成功 */
	unsigned char status;
	CH375_WR_CMD_PORT( CMD_ISSUE_TOKEN );
	CH375_WR_DAT_PORT( endp_and_pid );  /* 高4位目的端点号, 低4位令牌PID */
	status=wait_interrupt();  /* 等待CH375操作完成 */
	if ( status!=USB_INT_SUCCESS && (endp_and_pid&0xF0)==0x20 ) {  /* 操作失败,如果设备端不是CH37X芯片,那么需要修改端点号 */
		if ( (endp_and_pid&0x0F)==DEF_USB_PID_OUT ) clr_stall7();  /* 复位设备端接收 */
		else if ( (endp_and_pid&0x0F)==DEF_USB_PID_IN ) clr_stall6();  /* 复位设备端发送 */
	}
	return( status );
}

void host_send( unsigned char len, unsigned char *buf ) {  /* 主机发送 */
	wr_usb_data( len, buf );
	toggle_send();
	if ( issue_token( ( 2 << 4 ) | DEF_USB_PID_OUT )!=USB_INT_SUCCESS ) ERROR();  /* 如果设备端不是CH37X芯片,那么需要修改端点号 */
}

unsigned char host_recv( unsigned char *buf ) {  /* 主机接收, 返回长度 */
	toggle_recv();
	if ( issue_token( ( 2 << 4 ) | DEF_USB_PID_IN )!=USB_INT_SUCCESS ) ERROR();  /* 如果设备端不是CH37X芯片,那么需要修改端点号 */
	return( rd_usb_data( buf ) );
}

unsigned char get_descr( unsigned char type ) {  /* 从设备端获取描述符 */
	unsigned char status;
	CH375_WR_CMD_PORT( CMD_GET_DESCR );
	CH375_WR_DAT_PORT( type );  /* 描述符类型, 只支持1(设备)或者2(配置) */
	status=wait_interrupt();  /* 等待CH375操作完成 */
	if ( status==USB_INT_SUCCESS ) {  /* 操作成功 */
		unsigned char buffer[64];
		unsigned char i, len;
		len=rd_usb_data( buffer );
		printf( "%s描述符是:", type==1?"设备":"配置" );
		for ( i=0; i!=len; i++ ) printf( "%02X ", buffer[i] );
		printf( "\n" );
	}
	return( status );
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
	return( status );
}

unsigned char set_config( unsigned char cfg ) {  /* 设置设备端的USB配置 */
	endp6_mode=endp7_mode=0x80;  /* 复位USB数据同步标志 */
	CH375_WR_CMD_PORT( CMD_SET_CONFIG );  /* 设置USB设备端的配置值 */
	CH375_WR_DAT_PORT( cfg );  /* 此值取自USB设备的配置描述符中 */
	return( wait_interrupt() );  /* 等待CH375操作完成 */
}

/* 主机端的主程序简单示例 */
main() {
	unsigned char xdata data_to_send[250], data_by_recv[250];  /* 收发缓冲区 */
	unsigned char i, len;
	set_usb_mode( 6 );  /* 设置USB主机模式, 如果设备端是CH37X, 那么5和6均可 */
	while ( wait_interrupt()!=USB_INT_CONNECT );  /* 等待设备端连接上来 */

#ifdef DEVICE_NOT_CH37X
/* 如果设备端是CH37X,那么以下步骤是可选的,
   如果是其它USB芯片,那么需要执行以下步骤,并且要分析配置描述符的数据获得配置值以及端点号,并修改本程序中的端点号,
   关于如何分析配置描述符请参考CH375PRT.C文件 */
#define USB_RESET_FIRST	1  /* USB规范中未要求在USB设备插入后必须复位该设备,但是计算机的WINDOWS总是这样做,所以有些USB设备也要求在插入后必须先复位才能工作 */
#ifdef USB_RESET_FIRST
	set_usb_mode( 7 );  /* 复位USB设备,CH375向USB信号线的D+和D-输出低电平 */
/* 如果单片机对CH375的INT#引脚采用中断方式而不是查询方式,那么应该在复制USB设备期间禁止CH375中断,在USB设备复位完成后清除CH375中断标志再允许中断 */
	for ( i=0; i<250; i++ ) { delay2us(); delay2us(); delay2us(); delay2us(); }  /* 复位时间不少于1mS,建议为10mS */
	set_usb_mode( 6 );  /* 结束复位 */
	while ( wait_interrupt()!=USB_INT_CONNECT );  /* 等待复位之后的设备端再次连接上来 */
	for ( i=0; i<250; i++ ) delay2us();  /* 有些USB设备要求延时数百毫秒后才能工作 */
#endif
	get_descr(1);  /* 获取USB设备的设备描述符 */
	set_addr(5);  /* 设置USB设备的地址,因为只有一个USB设备,所以可以分配1到126之间的任意值 */
	get_descr(2);  /* 获取USB设备的配置描述符 */
	set_config(1);  /* 设置USB配置值,该数值来自USB设备的配置描述符 */
#endif

	for ( i=0; i<250; i+=64 ) host_send( 64, &data_to_send[i] );  /* 发送256字节的数据给设备端 */
	host_send( 0, NULL );  /* 假定, 发送空数据给设备端就能通知设备端发送数据 */
	for ( i=0; i<250; i+=len ) len=host_recv( &data_by_recv[i] );  /* 从设备端接收256字节的数据 */
	/* 可以再次继续发送数据或者接收数据 */
	while(1);
}
