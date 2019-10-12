/* 2004.03.05
  修改记录：
    2007.08支持大扇区, 适用于CH375B芯片, 修改了: mInitDisk, mReadSector, mWriteSector, 增加了解: mClearError
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB 1.1 Host Examples for CH375   **
**  KC7.0@MCS-51                      **
****************************************
*/
/* CH375作为USB主机接口的程序示例 */

/* MCS-51单片机C语言的示例程序, U盘数据读写 */

#include <reg51.h>
#include <string.h>
#include <stdio.h>

#define		MAX_SECTOR_SIZE		4096	/* 以512字节每扇区为主,部分有2K字节每扇区,最大为4K字节 */

/* 定义CH375命令代码及返回状态 */
#include "CH375INC.H"
/* CH375特性 */
#define CH375_BLOCK_SIZE		64		/* CH375 maximum data block size */

/* 以下定义适用于MCS-51单片机,其它单片机参照修改,为了提供C语言的速度需要对本程序进行优化 */
#include <reg51.h>
unsigned char volatile xdata	CH375_CMD_PORT _at_ 0xBDF1;	/* CH375命令端口的I/O地址 */
unsigned char volatile xdata	CH375_DAT_PORT _at_ 0xBCF0;	/* CH375数据端口的I/O地址 */
unsigned char xdata				DATA_BUFFER[ MAX_SECTOR_SIZE ]    _at_ 0x0000;	/* 外部RAM数据缓冲区的起始地址,长度不少于一次读写的数据长度 */
sbit	CH375_INT_WIRE	=		0xB0^2;	/* P3.2, INT0, 连接CH375的INT#引脚,用于查询中断状态 */

unsigned short  BytePerSector;		/* 每扇区字节数，扇区大小 */
unsigned char   BlockPerSector;		/* 每扇区块数，指CH375读写时的块 BlockPerSector=BytePerSector/CH375_BLOCK_SIZE */

/* 在P1.4连接一个LED用于监控演示程序的进度,低电平LED亮,当U盘插入后亮 */
sbit P1_4  = P1^4;
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 低电平驱动LED显示 */


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

/* 延时毫秒,不精确 */
void	mDelaymS( unsigned char cnt )
{
	unsigned char	i, c;
	while ( cnt -- ) {
		for ( i = 250; i != 0; i -- ) c+=3;
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

/* 等待CH375中断并获取状态 */
unsigned char mWaitInterrupt() {  /* 主机端等待操作完成, 返回操作状态 */
	while( CH375_INT_WIRE );  /* 查询等待CH375操作完成中断(INT#低电平) */
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 产生操作完成中断, 获取中断状态 */
	return( CH375_RD_DAT_PORT( ) );
/*	c = CH375_RD_DAT_PORT( );   返回中断状态 */
/*	if ( c == USB_INT_DISCONNECT ) ?;   检测到USB设备断开事件 */
/*	else if ( c == USB_INT_CONNECT ) ?;   检测到USB设备连接事件 */
}

/* 设置CH375为USB主机方式 */
unsigned char	mCH375Init( )
{
	unsigned char	i;
#ifdef	TEST_CH375_PORT
	unsigned char	c;
	CH375_WR_CMD_PORT( CMD_CHECK_EXIST );  /* 测试工作状态 */
	CH375_WR_DAT_PORT( 0x55 );  /* 测试数据 */
	c = CH375_RD_DAT_PORT( );  /* 返回数据应该是测试数据取反 */
	if ( c != 0xaa ) {  /* CH375出错 */
		for ( i = 100; i != 0; i -- ) {  /* 强制数据同步 */
			CH375_WR_CMD_PORT( CMD_RESET_ALL );  /* CH375执行硬件复位 */
			c = CH375_RD_DAT_PORT( );  /* 延时 */
		}
		mDelaymS( 50 );  /* 延时至少30mS */
	}
#endif
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );  /* 设置USB工作模式 */
	CH375_WR_DAT_PORT( 6 );  /* 模式代码,自动检测USB设备连接 */
	for ( i = 0xff; i != 0; i -- )  /* 等待操作成功,通常需要等待10uS-20uS */
		if ( CH375_RD_DAT_PORT( ) == CMD_RET_SUCCESS ) break;  /* 操作成功 */
	if ( i != 0 ) return( 0 );  /* 操作成功 */
	else return( 0xff );  /* CH375出错,例如芯片型号错或者处于串口方式或者不支持 */
}

/* 初始化磁盘 */
unsigned char	mInitDisk( )
{
	unsigned char mIntStatus, i;
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 产生操作完成中断, 获取中断状态 */
	mIntStatus = CH375_RD_DAT_PORT( );
	if ( mIntStatus == USB_INT_DISCONNECT ) return( mIntStatus );  /* USB设备断开 */
	CH375_WR_CMD_PORT( CMD_DISK_INIT );  /* 初始化USB存储器 */
	mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
	if ( mIntStatus != USB_INT_SUCCESS ) return( mIntStatus );  /* 出现错误 */
	CH375_WR_CMD_PORT( CMD_DISK_SIZE );  /* 获取USB存储器的容量 */
	mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
	if ( mIntStatus != USB_INT_SUCCESS ) {  /* 出错重试 */
		mDelaymS( 200 );
		CH375_WR_CMD_PORT( CMD_DISK_SIZE );  /* 获取USB存储器的容量 */
		mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
	}
	if ( mIntStatus != USB_INT_SUCCESS ) return( mIntStatus );  /* 出现错误 */

/* 可以由CMD_RD_USB_DATA命令将容量数据读出,分析每扇区字节数 */
	CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* 从CH375缓冲区读取数据块 */
	i = CH375_RD_DAT_PORT( );  /* 后续数据的长度 */
	if ( i != 8 ) return( USB_INT_DISK_ERR );  /* 异常 */
	for ( i = 0; i != 8; i ++ ) {  /* 根据长度读取数据 */
		DATA_BUFFER[ i ] = CH375_RD_DAT_PORT( );  /* 读出数据并保存 */
	}
	i = DATA_BUFFER[ 6 ];  /* U盘容量数据中的每扇区字节数,大端格式 */
	if ( i == 0x04 ) BlockPerSector = 1024/CH375_BLOCK_SIZE;  /* 磁盘的物理扇区是1K字节 */
	else if ( i == 0x08 ) BlockPerSector = 2048/CH375_BLOCK_SIZE;  /* 磁盘的物理扇区是2K字节 */
	else if ( i == 0x10 ) BlockPerSector = 4096/CH375_BLOCK_SIZE;  /* 磁盘的物理扇区是4K字节 */
	else BlockPerSector = 512/CH375_BLOCK_SIZE;  /* 默认的磁盘的物理扇区是512字节 */
	BytePerSector = BlockPerSector*CH375_BLOCK_SIZE;  /* 物理磁盘的扇区大小 */
	CH375_WR_CMD_PORT( CMD_SET_PKT_P_SEC );  /* 设置USB存储器的每扇区数据包总数 */
	CH375_WR_DAT_PORT( 0x39 );
	CH375_WR_DAT_PORT( BlockPerSector );  /* 设置每扇区数据包总数 */
	return( 0 );  /* U盘已经成功初始化 */
}

/* 清除U盘错误以便重试 */
void	mClearError( void )
{
	mDelaymS( 10 );  /* 延时10mS */
	CH375_WR_CMD_PORT( CMD_DISK_R_SENSE );  /* 检查USB存储器错误 */
	mDelaymS( 10 );  /* 延时10mS */
	mWaitInterrupt( );  /* 等待中断并获取状态 */
}

/* 从U盘读取多个扇区的数据块到缓冲区 */
unsigned char	mReadSector( unsigned long iLbaStart, unsigned char iSectorCount )
/* iLbaStart 是准备读取的线性起始扇区号, iSectorCount 是准备读取的扇区数 */
{
	unsigned char mIntStatus;
	unsigned char *mBufferPoint;
	unsigned int  mBlockCount;
	unsigned char mLength;
	CH375_WR_CMD_PORT( CMD_DISK_READ );  /* 从USB存储器读数据块 */
	CH375_WR_DAT_PORT( (unsigned char)iLbaStart );  /* LBA的最低8位 */
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 8 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 16 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 24 ) );  /* LBA的最高8位 */
	CH375_WR_DAT_PORT( iSectorCount );  /* 扇区数 */
	mBufferPoint = DATA_BUFFER;  /* 指向缓冲区起始地址 */
	for ( mBlockCount = iSectorCount * BlockPerSector; mBlockCount != 0; mBlockCount -- ) {  /* 数据块计数 */
		mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
		if ( mIntStatus == USB_INT_DISK_READ ) {  /* USB存储器读数据块,请求数据读出 */
			CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* 从CH375缓冲区读取数据块 */
			mLength = CH375_RD_DAT_PORT( );  /* 后续数据的长度 */
			while ( mLength ) {  /* 根据长度读取数据 */
				*mBufferPoint = CH375_RD_DAT_PORT( );  /* 读出数据并保存 */
				mBufferPoint ++;
				mLength --;
			}
			CH375_WR_CMD_PORT( CMD_DISK_RD_GO );  /* 继续执行USB存储器的读操作 */
		}
		else break;  /* 返回错误状态 */
	}
	if ( mBlockCount == 0 ) {
		mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
		if ( mIntStatus == USB_INT_SUCCESS ) return( 0 );  /* 操作成功 */
	}
//	if ( mIntStatus == USB_INT_DISCONNECT ) return( mIntStatus );  /* U盘断开 */
	mClearError( );  /* 清除U盘错误以便重试 */
	return( mIntStatus );  /* 操作失败 */
}

/* 将缓冲区中的多个扇区的数据块写入U盘 */
unsigned char	mWriteSector( unsigned long iLbaStart, unsigned char iSectorCount )
/* iLbaStart 是写入的线起始性扇区号, iSectorCount 是写入的扇区数 */
{
	unsigned char mIntStatus;
	unsigned char *mBufferPoint;
	unsigned int  mBlockCount;
	unsigned char mLength;
	CH375_WR_CMD_PORT( CMD_DISK_WRITE );  /* 向USB存储器写数据块 */
	CH375_WR_DAT_PORT( (unsigned char)iLbaStart );  /* LBA的最低8位 */
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 8 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 16 ) );
	CH375_WR_DAT_PORT( (unsigned char)( iLbaStart >> 24 ) );  /* LBA的最高8位 */
	CH375_WR_DAT_PORT( iSectorCount );  /* 扇区数 */
	mBufferPoint = DATA_BUFFER;  /* 指向缓冲区起始地址 */
	for ( mBlockCount = iSectorCount * BlockPerSector; mBlockCount != 0; mBlockCount -- ) {  /* 数据块计数 */
		mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
		if ( mIntStatus == USB_INT_DISK_WRITE ) {  /* USB存储器写数据块,请求数据写入 */
			CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* 向CH375缓冲区写入数据块 */
			mLength = CH375_BLOCK_SIZE;
			CH375_WR_DAT_PORT( mLength );  /* 后续数据的长度 */
			while ( mLength ) {  /* 根据长度写入数据 */
				CH375_WR_DAT_PORT( *mBufferPoint );  /* 将数据写入 */
				mBufferPoint ++;
				mLength --;
			}
/*			do { 对于C51,这个DO+WHILE结构比上面的WHILE效率高,速度快
				CH375_WR_DAT_PORT( *mBufferPoint );
				mBufferPoint ++;
			} while ( -- mLength );*/
			CH375_WR_CMD_PORT( CMD_DISK_WR_GO );  /* 继续执行USB存储器的写操作 */
		}
		else break;  /* 返回错误状态 */
	}
	if ( mBlockCount == 0 ) {
		mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
		if ( mIntStatus == USB_INT_SUCCESS ) return( 0 );  /* 操作成功 */
	}
//	if ( mIntStatus == USB_INT_DISCONNECT ) return( mIntStatus );  /* U盘断开 */
	mClearError( );  /* 清除U盘错误以便重试 */
	return( mIntStatus );  /* 操作失败 */
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
	unsigned char	c, mIntStatus;
	LED_OUT_ACT( );  /* 开机后LED亮一下以示工作 */
	mDelaymS( 100 );  /* 延时100毫秒 */
	LED_OUT_INACT( );
	mInitSTDIO( );
	printf( "Start\n" );
	c = mCH375Init( );  /* 初始化CH375 */
	if ( c ) printf( "Error @CH375Init\n" );
	printf( "Insert USB disk\n" );
	do {  /* 等待U盘连接 */
		mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
	} while ( mIntStatus != USB_INT_CONNECT );  /* U盘没有连接或者已经拔出 */
	mDelaymS( 200 );  /* 延时等待U盘进入正常工作状态 */
	printf( "InitDisk\n" );
	c = mInitDisk( );  /* 初始化U盘,实际是识别U盘的类型,不影响U盘中的数据,在所有读写操作之前必须进行此步骤 */
	if ( c ) printf( "Error @InitDisk, %02X\n", c );
	LED_OUT_ACT( );

/* 检查U盘是否准备好,大多数U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
//	do {
//		mDelaymS( 100 );
//		printf( "Disk Ready ?\n" );
//		i = CH375DiskReady( );  /* 查询磁盘是否准备好,如果省掉这个子程序可以节约将近1KB的程序代码 */
//	} while ( i != ERR_SUCCESS );
/* CH375DiskReady 在CH375的U盘文件子程序库中,因为代码较多,所以此处省去 */

	printf( "ReadSector 0# to buffer\n" );
	c = mReadSector( 0, 1 );
	if ( c ) printf( "Error @ReadSector, %02X\n", c );
	if ( DATA_BUFFER[0x01FF] == 0xAA ) {  /* 磁盘分区有效 */
		printf( "WriteSector 1# from buffer\n" );
		c = mWriteSector( 1, 1 );
		if ( c ) printf( "Error @WriteSector, %02X\n", c );
		memset( DATA_BUFFER, 0, sizeof(DATA_BUFFER) );  /* 清空数据缓冲区,代替原来的分区信息 */
		printf( "WriteSector 0# for clear\n" );
		c = mWriteSector( 0, 1 );
		if ( c ) printf( "Error @WriteSector, %02X\n", c );
	}
	else {
		printf( "ReadSector 1# to buffer\n" );
		c = mReadSector( 1, 1 );
		if ( c ) printf( "Error @ReadSector, %02X\n", c );
		printf( "WriteSector 0# from buffer\n" );
		c = mWriteSector( 0, 1 );
		if ( c ) printf( "Error @WriteSector, %02X\n", c );
	}
	printf( "Stop\n" );
	while ( 1 ) {
		mIntStatus = mWaitInterrupt( );  /* 等待中断并获取状态 */
		if ( mIntStatus == USB_INT_DISCONNECT ) {  /* U盘没有连接或者已经拔出 */
			printf( "Out\n" );
			LED_OUT_INACT( );
		}
		else if ( mIntStatus == USB_INT_CONNECT ) {  /* U盘已经连接 */
			printf( "In\n" );
			LED_OUT_ACT( );
		}
	}
}
