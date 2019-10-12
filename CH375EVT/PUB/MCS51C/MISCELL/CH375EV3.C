/* 2004.03.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB 1.1 Host Examples for CH375   **
**  KC7.0@MCS-51                      **
****************************************
*/
/* CH375作为USB主机接口的程序示例,中断方式 */

/* MCS-51单片机C语言的示例程序, U盘数据读写 */

#include <reg51.h>
#include <string.h>
#include <stdio.h>

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif

/* 定义CH375命令代码及返回状态 */
#include "CH375INC.H"

/* 以下定义适用于MCS-51单片机,其它单片机参照修改,为了提供C语言的速度需要对本程序进行优化 */
#include <reg51.h>
unsigned char volatile xdata	CH375_CMD_PORT _at_ 0xBDF1;	/* CH375命令端口的I/O地址 */
unsigned char volatile xdata	CH375_DAT_PORT _at_ 0xBCF0;	/* CH375数据端口的I/O地址 */
unsigned char xdata				DATA_BUFFER[8192] _at_ 0x0000;	/* 外部RAM数据缓冲区的起始地址,长度不少于一次读写的数据长度 */

/* 在P1.4连接一个LED用于监控演示程序的进度,低电平LED亮,当U盘插入后亮 */
sbit P1_4  = P1^4;
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 低电平驱动LED显示 */

unsigned char volatile			UdiskStatus;	/* 当前U盘状态,定义如下 */
#define		STATUS_DISCONNECT		0	/* U盘尚未连接或者已经断开 */
#define		STATUS_CONNECT			1	/* U盘刚刚连接 */
#define		STATUS_ERROR			2	/* U盘操作错误或者不支持 */
#define		STATUS_WAIT				3	/* U盘正在操作 */
#define		STATUS_READY			4	/* U盘准备好接受操作 */

/*
  下面是USB移动存储设备的简单应用参考。在单片机应用程序中，
  可以定义USB存储设备的4种状态：已断开、已连接、初始化、可读写。其中"已连接"状态可选。
● 已断开是指USB存储设备已经断开（从USB插座中拔出），完全不可用；
● 已连接是指USB存储设备已经连接（插入USB插座中），但是尚未初始化；
● 初始化是指USB存储设备正在初始化或者初始化失败，所以不可以进行读写；
● 可读写是指USB存储设备初始化成功，可以进行数据读写。

单片机处理CH375中断的一般步骤如下：
㈠ CH375将INT#引脚设置为低电平，向单片机请求中断；
㈡ 单片机进入中断服务程序，首先执行GET_STATUS命令获取中断状态；
㈢ CH375在GET_STATUS命令完成后将INT#引脚恢复为高电平，取消中断请求；
㈣ 单片机可以参考下面的顺序分析GET_STATUS命令获取的中断状态：
  ⑴ 如果中断状态是USB_INT_DISCONNECT，则说明USB设备已经断开，置USB存储设备状态为"已断开"，
     如果在此之前USB存储设备有读写操作尚未完成，则放弃操作并按操作失败处理，然后退出中断，必要时可以通知单片机主程序。
  ⑵ 如果中断状态是USB_INT_CONNECT，则说明USB设备已经连接，置USB存储设备状态为"已连接"，
     接着发出DISK_INIT命令，并置USB存储设备状态为初始化，然后退出中断，必要时可以通知单片机主程序。
  ⑶ 如果当前的USB存储设备状态是"初始化"：
    ① 如果中断状态是USB_INT_SUCCESS，则说明USB存储设备初始化成功，置USB存储设备状态为"可读写"，
       然后退出中断，必要时可以通知单片机主程序。
    ② 如果中断状态是其它状态，则说明USB存储设备初始化失败，应该通知单片机主程序，
       提示该USB设备不是存储设备或者该USB设备不支持，然后退出中断。或者，单片机通过其它命令自行处理该USB存储设备的通讯协议。
  ⑷ 如果当前的USB存储设备状态是"可读写"，则可以保存中断状态并通知单片机主程序处理，
     然后直接退出中断；或者，在中断服务程序中继续处理：
    ① 如果中断状态是USB_INT_DISK_READ，则说明正在进行USB存储设备的读操作，需要取走64个字节的数据，
       可以发出RD_USB_DATA命令取走数据，接着再发出DISK_RD_GO命令使CH375继续读，然后退出中断。
    ② 如果中断状态是USB_INT_DISK_WRITE，则说明正在进行USB存储设备的写操作，需要提供64个字节的数据，
       可以发出WR_USB_DATA7命令提供数据，接着再发出DISK_WR_GO命令使CH375继续写，然后退出中断。
    ③ 如果中断状态是USB_INT_SUCCESS，则说明读写操作成功，直接退出中断并通知主程序该操作成功；
    ④ 如果中断状态是USB_INT_DISK_ERR，则说明读写操作失败，直接退出中断并通知主程序该操作失败；
    ⑤ 通常不会返回其它中断状态，如果有，则说明出现错误。
  ⑸ 通常不会在其它USB存储设备状态下返回其它中断状态，如果有，则说明出现错误，可以参照USB设备断开的状态处理。

当单片机主程序需要从USB存储设备中读写数据时，可以查询USB存储设备状态，
如果是"可读写"状态，则可以发出DISK_READ命令读数据，或者发出DISK_WRITE命令写数据。
后续的数据读写过程可以在单片机的中断服务程序中完成，也可以由中断程序置标志通知等待中的主程序，
由主程序分析中断状态再完成数据读写过程。
*/

unsigned char *mBufferPoint;

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

/* CH375的INT#引脚连接51单片机的INT0引脚, 采用中断方式 */
/* CH375中断服务程序,使用寄存器组1 */
void	CH375Interrupt( ) interrupt 0 using 1
{
	unsigned char i, s, len;
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 获取中断状态并取消中断请求 */
	for ( i = 2; i != 0; i -- );  /* 至少延时2uS */
	s = CH375_RD_DAT_PORT( );  /* 获取中断状态 */
	if ( s == USB_INT_SUCCESS ) UdiskStatus = STATUS_READY;  /* 操作成功 */
	else if ( s == USB_INT_DISCONNECT ) {
		UdiskStatus = STATUS_DISCONNECT;  /* 检测到USB设备断开事件 */
		LED_OUT_INACT( );
	}
	else if ( s == USB_INT_CONNECT ) {
		UdiskStatus = STATUS_CONNECT;  /* 检测到USB设备连接事件 */
		LED_OUT_ACT( );
	}
	else if ( s == USB_INT_DISK_READ ) {  /* USB存储器读数据块,请求数据读出 */
		CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* 从CH375缓冲区读取数据块 */
		for ( i = 2; i != 0; i -- );  /* 至少延时2uS */
		len = CH375_RD_DAT_PORT( );  /* 后续数据的长度 */
		while ( len ) {  /* 根据长度读取数据 */
			*mBufferPoint = CH375_RD_DAT_PORT( );  /* 读出数据并保存 */
			mBufferPoint ++;
			len --;
		}
		CH375_WR_CMD_PORT( CMD_DISK_RD_GO );  /* 继续执行USB存储器的读操作 */
	}
	else if ( s == USB_INT_DISK_WRITE ) {  /* USB存储器写数据块,请求数据写入 */
		CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* 向CH375缓冲区写入数据块 */
		for ( i = 2; i != 0; i -- );  /* 至少延时2uS */
		len = CH375_MAX_DATA_LEN;
		CH375_WR_DAT_PORT( len );  /* 后续数据的长度 */
		do {  /* 对于C51,这个DO+WHILE结构比上面的WHILE效率高,速度快 */
			CH375_WR_DAT_PORT( *mBufferPoint );
			mBufferPoint ++;
		} while ( -- len );
		CH375_WR_CMD_PORT( CMD_DISK_WR_GO );  /* 继续执行USB存储器的写操作 */
	}
	else {  /* 操作失败 */
		UdiskStatus = STATUS_ERROR;
	}
/*	CH375_INT_FLAG = 0;  清中断标志 */
}

/* 设置CH375为USB主机方式 */
unsigned char	mCH375Init( )
{
	unsigned char	i;
	UdiskStatus = STATUS_DISCONNECT;
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );  /* 设置USB工作模式 */
	CH375_WR_DAT_PORT( 6 );  /* 模式代码,自动检测USB设备连接 */
	for ( i = 0xff; i != 0; i -- ) {  /* 等待操作成功,通常需要等待10uS-20uS */
		if ( CH375_RD_DAT_PORT( ) == CMD_RET_SUCCESS ) break;  /* 操作成功 */
	}
	if ( i != 0 ) return( TRUE );  /* 操作成功 */
	else return( FALSE );  /* CH375出错,例如芯片型号错或者处于串口方式或者不支持 */
}

/* 等待U盘准备好或者等待上次操作结束 */
unsigned char	mWaitReady( )
{
	while( UdiskStatus == STATUS_WAIT );  /* 等待上次操作结束 */
	if ( UdiskStatus == STATUS_READY ) return( TRUE );  /* U盘已经准备好操作,上次操作成功 */
	else if ( UdiskStatus == STATUS_ERROR ) {  /* 上次操作失败 */
		UdiskStatus = STATUS_WAIT;
		CH375_WR_CMD_PORT( CMD_DISK_RESET );  /* 复位U盘 */
		while( UdiskStatus == STATUS_WAIT );  /* 等待中断状态 */
		if ( UdiskStatus == STATUS_READY ) return( TRUE );  /* 等待中断状态,操作成功 */
		return( FALSE );  /* UdiskStatus = STATUS_ERROR */
	}
	else if ( UdiskStatus == STATUS_DISCONNECT ) {  /* U盘已经断开 */
/*		while ( UdiskStatus == STATUS_DISCONNECT );*/
		return( FALSE );
	}
	else if ( UdiskStatus == STATUS_CONNECT ) {  /* U盘已经连接 */
		mDelay100mS( );  /* 如果U盘刚插入那么最好稍等一下再操作 */
		mDelay100mS( );
		CH375_WR_CMD_PORT( CMD_DISK_INIT );  /* 初始化USB存储器 */
		while ( UdiskStatus == STATUS_CONNECT );  /* 等待中断状态 */
		if ( UdiskStatus == STATUS_READY ) {  /* 操作成功,磁盘已经初始化并安装到系统中 */
/* 检查U盘是否准备好,大多数U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
//	do {
//		mDelay100mS( );
//		printf( "Disk Ready ?\n" );
//		i = CH375DiskReady( );  /* 查询磁盘是否准备好,如果省掉这个子程序可以节约将近1KB的程序代码 */
//	} while ( i != ERR_SUCCESS );
/* CH375DiskReady 在CH375的U盘文件子程序库中,因为代码较多,所以此处省去 */
			return( TRUE );
		}
		else return( FALSE );
	}
	return( FALSE );
}

/* 从U盘读取多个扇区的数据块到缓冲区 */
unsigned char	mReadSector( unsigned long iLbaStart, unsigned char iSectorCount, unsigned char *iBuffer )
/* iLbaStart 是准备读取的线性起始扇区号, iSectorCount 是准备读取的扇区数, iBuffer 是存放返回数据的缓冲区的起址 */
{
	if ( mWaitReady( ) == FALSE ) return( FALSE );
	mBufferPoint = iBuffer;  /* 指向缓冲区起始地址 */
	UdiskStatus = STATUS_WAIT;
	CH375_WR_CMD_PORT( CMD_DISK_READ );  /* 从USB存储器读数据块 */
	CH375_WR_DAT_PORT( (unsigned char)iLbaStart );  /* LBA的最低8位 */
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 8 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 16 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 24 ) );  /* LBA的最高8位 */
	CH375_WR_DAT_PORT( iSectorCount );  /* 扇区数 */
/* 如果是纯中断方式,那么可以做其它事情,中断服务程序会读取数据 */
	return( mWaitReady( ) );
}

/* 将缓冲区中的多个扇区的数据块写入U盘 */
unsigned char	mWriteSector( unsigned long iLbaStart, unsigned char iSectorCount, unsigned char *iBuffer )
/* iLbaStart 是写入的线起始性扇区号, iSectorCount 是写入的扇区数, iBuffer 是存放准备写入数据的缓冲区的起址 */
{
	if ( mWaitReady( ) == FALSE ) return( FALSE );
	mBufferPoint = iBuffer;  /* 指向缓冲区起始地址 */
	UdiskStatus = STATUS_WAIT;
	CH375_WR_CMD_PORT( CMD_DISK_WRITE );  /* 向USB存储器写数据块 */
	CH375_WR_DAT_PORT( (unsigned char)iLbaStart );  /* LBA的最低8位 */
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 8 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 16 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 24 ) );  /* LBA的最高8位 */
	CH375_WR_DAT_PORT( iSectorCount );  /* 扇区数 */
/* 如果是纯中断方式,那么可以做其它事情,中断服务程序会读取数据 */
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
	unsigned char	c;
	LED_OUT_ACT( );  /* 开机后LED亮一下以示工作 */
	mDelay100mS( );  /* 延时100毫秒 */
	LED_OUT_INACT( );
	mInitSTDIO( );
	printf( "Start\n" );
	c = mCH375Init( );  /* 初始化CH375 */
	if ( c == FALSE ) printf( "Error @CH375Init\n" );
	printf( "Insert USB disk\n" );
	while ( 1 ) {  /* 主程序 */
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
/* 做其它工作 */
/* 如果打算从U盘读取数据 */
		printf( "Read\n" );
		c = mReadSector( 0, 5, DATA_BUFFER );
		if ( c == FALSE ) printf( "Error @ReadSector\n" );
/* 如果打算向U盘写入数据 */
		c = mWriteSector( 1, 1, DATA_BUFFER );
		if ( c == FALSE ) printf( "Error @WriteSector\n" );
/* 做其它工作 */
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
		mDelay100mS( );
	}
}
