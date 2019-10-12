/* 2004.03.05, 2004.8.18
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB 1.1 Host Examples for CH375   **
**  KC7.0@MCS-51                      **
****************************************
*/
/* CH375作为USB主机接口的程序示例 */

/* MCS-51单片机C语言的示例程序, 查询方式 */

#include <reg51.h>
#include <string.h>
#include <stdio.h>

/* 定义CH375命令代码及返回状态 */
#include "CH375INC.H"

unsigned char volatile xdata	CH375_CMD_PORT _at_ 0xBDF1;	/* CH375命令端口的I/O地址 */
unsigned char volatile xdata	CH375_DAT_PORT _at_ 0xBCF0;	/* CH375数据端口的I/O地址 */
unsigned char xdata				DATA_BUFFER[ 0x8000 ] _at_ 0x0000;	/* 外部RAM数据缓冲区的起始地址 */
sbit	CH375_INT_WIRE	=		0xB0^2;	/* P3.2, INT0, 连接CH375的INT#引脚,用于查询中断状态 */

/* 在P1.4连接一个LED用于监控演示程序的进度,低电平LED亮,当U盘插入后亮 */
sbit P1_4  = P1^4;
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 低电平驱动LED显示 */

bit		mDeviceOnline = 0;				/* CH375的目标USB设备的连接状态: 0断开,1连接 */


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

/* 延时100毫秒,不精确 */
void	mDelay100mS( )
{
	unsigned char	i, j, c;
	for ( i = 200; i != 0; i -- ) for ( j = 200; j != 0; j -- ) c+=3;
}

/* 延时10毫秒,不精确 */
void	mDelay10mS( )
{
	unsigned char	i, j, c;
	for ( i = 20; i != 0; i -- ) for ( j = 200; j != 0; j -- ) c+=3;
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

/* 等待CH375中断并获取状态 */
unsigned char mWaitInterrupt() {  /* 主机端等待操作完成, 返回操作状态 */
	unsigned char c;
	while( CH375_INT_WIRE );  /* 查询等待CH375操作完成中断(INT#低电平) */
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 产生操作完成中断, 获取中断状态 */
	c = CH375_RD_DAT_PORT( );  /* 返回中断状态 */
	if ( c == USB_INT_DISCONNECT ) mDeviceOnline = 0;  /* 检测到USB设备断开事件 */
	else if ( c == USB_INT_CONNECT ) mDeviceOnline = 1;  /* 检测到USB设备连接事件 */
	return( c );
}

/* 设置CH375为USB主机方式 */
unsigned char	mCH375Init( )
{
	unsigned char	c, i;
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );  /* 设置USB工作模式 */
	CH375_WR_DAT_PORT( 6 );  /* 模式代码,自动检测USB设备连接 */
	for ( i = 0xff; i != 0; i -- ) {  /* 等待操作成功,通常需要等待10uS-20uS */
		c = CH375_RD_DAT_PORT( );
		if ( c == CMD_RET_SUCCESS ) break;  /* 操作成功 */
	}
	if ( i != 0 ) return( USB_INT_SUCCESS );  /* 操作成功 */
	else return( 0xff );  /* CH375出错,例如芯片型号错或者处于串口方式或者不支持 */
}

/* 从CH375的端点缓冲区读取接收到的数据 */
unsigned char	mReadCH375Data( unsigned char *buf )
{
	unsigned char len, i;
	unsigned char *p;
	CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* 从CH375的端点缓冲区读取数据块 */
	p = buf;
	len = CH375_RD_DAT_PORT( );  /* 数据长度 */
	for ( i=0; i<len; i++ ) *p++ = CH375_RD_DAT_PORT( );  /* 连续读取数据 */
	return( len );
}

/* 向CH375的端点缓冲区写入准备发送的数据 */
void	mWriteCH375Data( unsigned char *buf, unsigned char len )
{
	unsigned char i;
	unsigned char *p;
	CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* 向CH375的端点缓冲区写入数据块 */
	p = buf;
	CH375_WR_DAT_PORT( len );  /* 数据长度 */
	for ( i=0; i<len; i++ ) CH375_WR_DAT_PORT( *p++ );  /* 连续写入数据 */
}

/* 对目标USB设备执行控制传输: 获取USB描述符 */
unsigned char	mCtrlGetDescr( unsigned char type )
{
	CH375_WR_CMD_PORT( CMD_GET_DESCR );  /* 控制传输-获取描述符 */
	CH375_WR_DAT_PORT( type );  /* 0:设备描述符, 1:配置描述符 */
	return( mWaitInterrupt( ) );  /* 等待操作完成 */
}

/* 对目标USB设备执行控制传输: 设置USB地址 */
unsigned char	mCtrlSetAddress( unsigned char addr )
{
	unsigned char c;
	CH375_WR_CMD_PORT( CMD_SET_ADDRESS );  /* 控制传输-设置USB地址 */
	CH375_WR_DAT_PORT( addr );  /* 1 - 7eh */
	c = mWaitInterrupt( );  /* 等待操作完成 */
	if ( c != USB_INT_SUCCESS ) return( c );  /* 操作失败 */
/* 当目标USB设备的地址成功修改后,应该同步修改CH375的USB地址,否则CH375将无法与目标设备通讯 */
	CH375_WR_CMD_PORT( CMD_SET_USB_ADDR );  /* 设置CH375的USB地址 */
	CH375_WR_DAT_PORT( addr );  /* 修改CH375的USB设备能够立即完成,不会产生中断通知 */
	return( c );
}

/* 发出令牌,执行USB事务 */
unsigned char	mIssueToken( unsigned char endp, unsigned char token )
{
	CH375_WR_CMD_PORT( CMD_ISSUE_TOKEN );  /* 发出令牌,执行事务 */
	CH375_WR_DAT_PORT( endp << 4 | token );  /* 高4位是端点号,低4位是令牌PID */
	return( mWaitInterrupt( ) );  /* 等待操作完成 */
}

/* 检查操作状态,如果错误则显示错误代码并停机 */
void	mStopIfError( unsigned char err )
{
	if ( err == USB_INT_SUCCESS ) return;  /* 操作成功 */
	printf( "Error: %02X, %d\n", (unsigned int)err, (unsigned int)mDeviceOnline );  /* 显示错误 */
	while ( 1 ) {
		LED_OUT_ACT( );  /* LED闪烁 */
		mDelay100mS( );
		LED_OUT_INACT( );
		mDelay100mS( );
	}
}

/* 为printf和getkey输入输出初始化串口 */
void	mInitSTDIO( )
{
	SCON = 0x50;
	PCON = 0x80;
	TMOD = 0x20;
	TH1 = 0xf3;  /* 24MHz晶振, 9600bps */
	TR1 = 1;
	TI = 1;
}

main( ) {
	unsigned char	i, len;
	LED_OUT_ACT( );  /* 开机后LED亮一下以示工作 */
	mDelay100mS( );  /* 延时100毫秒 */
	LED_OUT_INACT( );
	mInitSTDIO( );
	printf( "Start\n" );
	i = mCH375Init( );  /* 初始化CH375 */
	mStopIfError( i );
/* 其它电路初始化 */
	printf( "Insert USB device\n" );
	while ( mDeviceOnline == 0 ) mWaitInterrupt( );  /* 等待USB设备连接 */

#if 1
/* 实际上并没有必要在USB设备插入后复位该设备,但是计算机的WINDOWS总是这样做,所以我们也可以参照 */
/* 最新说明, 有些USB设备要求在插入后必须复位USB总线才能工作, 所以这部分程序不能去掉 */
	printf( "Begin: reset USB device\n" );
	while ( mDeviceOnline == 0 ) mWaitInterrupt( );  /* 等待USB设备连接 */
//	CH375_INT_EN = 0;  /* 复位USB总线期间应该禁止CH375中断 */
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );  /* 设置USB工作模式 */
	CH375_WR_DAT_PORT( 7 );  /* 模式代码,复位USB设备 */
	mDelay10mS( );  /* 复位时间应该在10mS左右 */
	mDeviceOnline = 0;
//	CH375_INT_FLAG = 0;  /* 清中断标志 */
//	CH375_INT_EN = 1;  /* 允许CH375中断 */
/*	printf( "End: reset USB device\n" ); */
	i = mCH375Init( );  /* 停止复位,重新初始化CH375 */
	mStopIfError( i );
	while ( mDeviceOnline == 0 ) mWaitInterrupt( );  /* 等待USB设备连接 */
#endif

/*	mDelay10mS( );  适当延时几毫秒,可选的操作 */
	i = mCtrlGetDescr( 1 );  /* 获取设备描述符 */
	mStopIfError( i );
	len = mReadCH375Data( DATA_BUFFER );  /* 读取设备描述符数据 */
	printf( "Device descr data len: %d, data: ", len );
	for ( i = 0; i < len; i ++ ) printf( "%02X,", (unsigned int)DATA_BUFFER[i] );
	printf( "\n" );
	i = mCtrlSetAddress( 5 );  /* 设置USB地址,地址值为1到7EH,任意选 */
	mStopIfError( i );
#if 0
	......
	i = mIssueToken( mBulkOutEndp, DEF_USB_PID_OUT );  /* 发出一个OUT令牌,数据是DATA0 */
	mStopIfError( i );
#endif
	printf( "Stop\n" );
	LED_OUT_ACT( );  /* LED常亮 */
	while ( 1 );  /* 停机 */
}
