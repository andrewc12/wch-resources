/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, ICCAVR_6.31@AVR         **
****************************************
*/
/* CH375 主机文件系统接口 */
/* 支持: FAT12/FAT16/FAT32 */

/* AVR单片机C语言的U盘文件读写示例程序 */
/* 该程序将U盘中的/C51/CH375HFT.C文件中的前600个字符显示出来,
   如果找不到原文件CH375HFT.C, 那么该程序将显示C51子目录下所有以CH375开头的文件名,
   如果找不到C51子目录, 那么该程序将显示根目录下的所有文件名,
   最后将程序ROM中的一个字符串写入写入新建的文件"NEWFILE.TXT"中
*/
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"内部复制", 本程序适用于ATmega128单片机, 串口0输出监控信息,9600bps */
/* 本例以字节为单位读写U盘文件,读写速度较扇区模式慢,但是由于字节模式读写文件不需要文件数据缓冲区FILE_DATA_BUF,
   所以总共只需要600字节的RAM,适用于单片机硬件资源有限、数据量小并且读写速度要求不高的系统 */

/* ICCAVR -v -Wp -I\ICC\INCLUDE -Wf -Mavr_enhanced -Wl -L\ICC\LIB -m -bfunc_lit:0x8C.0x20000 -bdata:0x0100.0x0FFF -dram_end:0x0FFF -dhwstk_size:40 -ucrtatmega.o CH375HFB.C CH375HFB.A */

#include <iom128v.h>
#include <string.h>
#include <stdio.h>

/* 以下定义的详细说明请看CH375HF9.H文件 */
#define LIB_CFG_FILE_IO			1		/* 文件读写的数据的复制方式,0为"外部子程序",1为"内部复制" */
#define LIB_CFG_INT_EN			0		/* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */

/* 单片机的RAM有限,其中CH375子程序用512字节,剩余RAM部分可以用于文件读写缓冲 */
#define DISK_BASE_BUF_LEN		2048	/* 默认的磁盘数据缓冲区大小为512字节,建议选择为2048甚至4096以支持某些大扇区的U盘,为0则禁止在.H文件中定义缓冲区并由应用程序在pDISK_BASE_BUF中指定 */
#define FILE_DATA_BUF_LEN		0x0200	/* 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */

#define CH375_INT_WIRE			( PINB & 0x10 )	/* PINB.4, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM		1		/* 未调用CH375FileEnumer程序故禁止以节约代码 */
#define NO_DEFAULT_CH375_F_QUERY	1		/* 未调用CH375FileQuery程序故禁止以节约代码 */

#include "..\CH375HFB.H"

/* 有些AVR单片机提供开放系统总线,那么直接将CH375挂在其系统总线上,以8位I/O方式进行读写 */
/* 虽然Atmega128提供系统总线,不过本例假定不开放系统总线,所以用I/O引脚模拟产生CH375的并口读写时序 */
/* 本例中的硬件连接方式如下(实际应用电路可以参照修改下述3个并口读写子程序) */
/*    单片机的引脚     CH375芯片的引脚
       PINB.4                INT#
       PORTB.3               A0
       PORTB.2               CS#
       PORTB.1               WR#
       PORTB.0               RD#
      PORTA(8位端口)        D7-D0       */

void mDelay1uS( )  /* 至少延时1uS,根据单片机主频调整 */
{
	UINT8	i;
	for ( i = 5; i != 0; i -- );
}

void CH375_PORT_INIT( )  /* 由于使用通用I/O模块并口读写时序,所以进行初始化 */
{
	DDRA = 0x00;  /* 设置8位并口为输入 */
	PORTB = 0x07;  /* 设置CS,WR,RD默认为高电平 */
	DDRB = 0x0F;  /* 设置CS,WR,RD,A0为输出,设置INT#为输入 */
}

void xWriteCH375Cmd( UINT8 mCmd )		/* 外部定义的被CH375程序库调用的子程序,向CH375写命令 */
{
	mDelay1uS( ); mDelay1uS( );  /* 至少延时1uS */
/*	*(volatile unsigned char *)CH375_CMD_PORT_ADDR = mCmd;  通过并口直接读写CH375而非普通I/O模拟 */
	PORTB |= 0x08;  /* 输出A0=1 */
	PORTA = mCmd;  /* 向CH375的并口输出数据 */
	DDRA = 0xFF;  /* 并口D0-D7输出 */
	PORTB &= 0xF9;  /* 输出有效写控制信号, 写CH375芯片的命令端口, A0=1; CS=0; WR=0; RD=1; */
	DDRA = 0xFF;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于100nS */
	PORTB |= 0x07;  /* 输出无效的控制信号, 完成操作CH375芯片, A0=1; CS=1; WR=1; RD=1; */
	DDRA = 0x00;  /* 禁止数据输出 */
	PORTB &= 0xF7;  /* 输出A0=0; 可选操作 */
	mDelay1uS( ); mDelay1uS( );  /* 至少延时2uS */
}

void xWriteCH375Data( UINT8 mData )		/* 外部定义的被CH375程序库调用的子程序,向CH375写数据 */
{
/*	*(volatile unsigned char *)CH375_DAT_PORT_ADDR = mData;  通过并口直接读写CH375而非普通I/O模拟 */
	PORTA = mData;  /* 向CH375的并口输出数据 */
	DDRA = 0xFF;  /* 并口D0-D7输出 */
	PORTB &= 0xF1;  /* 输出有效写控制信号, 写CH375芯片的数据端口, A0=0; CS=0; WR=0; RD=1; */
	DDRA = 0xFF;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于100nS */
	PORTB |= 0x07;  /* 输出无效的控制信号, 完成操作CH375芯片, A0=0; CS=1; WR=1; RD=1; */
	DDRA = 0x00;  /* 禁止数据输出 */
	mDelay1uS( );  /* 至少延时1.2uS */
}

UINT8 xReadCH375Data( void )			/* 外部定义的被CH375程序库调用的子程序,从CH375读数据 */
{
	UINT8	mData;
/*	mData = *(volatile unsigned char *)CH375_DAT_PORT_ADDR;  通过并口直接读写CH375而非普通I/O模拟 */
	mDelay1uS( );  /* 至少延时1.2uS */
	DDRA = 0x00;  /* 数据输入 */
	PORTB &= 0xF2;  /* 输出有效读控制信号, 读CH375芯片的数据端口, A0=0; CS=0; WR=1; RD=0; */
	DDRA = 0x00;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于100nS */
	mData = PINA;  /* 从CH375的并口PA输入数据 */
	PORTB |= 0x07;  /* 输出无效的控制信号, 完成操作CH375芯片, A0=0; CS=1; WR=1; RD=1; */
	return( mData );
}

/* 在P0.2连接一个LED用于监控演示程序的进度,低电平LED亮 */
#define LED_OUT_INIT( )		{ PORTB |= 0x80; DDRB |= 0x80; }	/* PORTB.7 高电平为输出方向 */
#define LED_OUT_ACT( )		{ PORTB &= 0x7F; }	/* PORTB.7 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ PORTB |= 0x80; }	/* PORTB.7 低电平驱动LED显示 */

/* 延时指定毫秒时间,根据单片机主频调整,不精确 */
void	mDelaymS( UINT8 ms )
{
	UINT16	i;
	while ( ms -- ) for ( i = 2600; i != 0; i -- );
}

/* 检查操作状态,如果错误则显示错误代码并停机 */
void	mStopIfError( UINT8 iError )
{
	if ( iError == ERR_SUCCESS ) return;  /* 操作成功 */
	printf( "Error: %02X\n", (UINT16)iError );  /* 显示错误 */
	while ( 1 ) {
		LED_OUT_ACT( );  /* LED闪烁 */
		mDelaymS( 100 );
		LED_OUT_INACT( );
		mDelaymS( 100 );
	}
}

/* 为printf和getkey输入输出初始化串口 */
extern int _textmode;
void	mInitSTDIO( )
{
	UBRR0H = 0;
	UBRR0L = 103;  /* 9600bps@16MHz */
	UCSR0B = 0x18; /* BIT(RXEN) | BIT(TXEN); */
	UCSR0C = 0x06; /* BIT(UCSZ1) | BIT(UCSZ0); */
	_textmode = 1;
}

void	main( ) {
	UINT8	i, c;
	UINT16	TotalCount;
	UINT8	*pCodeStr;
	CH375_PORT_INIT( );
	LED_OUT_INIT( );
	LED_OUT_ACT( );  /* 开机后LED亮一下以示工作 */
	mDelaymS( 100 );  /* 延时100毫秒 */
	LED_OUT_INACT( );
	mInitSTDIO( );  /* 为了让计算机通过串口监控演示过程 */
	printf( "Start\n" );

#if DISK_BASE_BUF_LEN == 0
	pDISK_BASE_BUF = &my_buffer[0];  /* 不在.H文件中定义CH375的专用缓冲区,而是用缓冲区指针指向其它应用程序的缓冲区便于合用以节约RAM */
#endif

	i = CH375LibInit( );  /* 初始化CH375程序库和CH375芯片,操作成功返回0 */
	mStopIfError( i );
/* 其它电路初始化 */

	while ( 1 ) {
		printf( "Wait Udisk\n" );
		while ( CH375DiskStatus != DISK_CONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘插入 */
		LED_OUT_ACT( );  /* LED亮 */
		mDelaymS( 200 );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */

/* 检查U盘是否准备好,有些U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 10; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
			mDelaymS( 100 );
			printf( "Ready ?\n" );
			if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
		}
#if DISK_BASE_BUF_LEN
		if ( DISK_BASE_BUF_LEN < CH375vSectorSize ) {  /* 检查磁盘数据缓冲区是否足够大,CH375vSectorSize是U盘的实际扇区大小 */
			printf( "Too large sector size\n" );
			while ( CH375DiskConnect( ) == ERR_SUCCESS ) mDelaymS( 100 );
			continue;
		}
#endif
/* 查询磁盘物理容量 */
/*		printf( "DiskSize\n" );
		i = CH375DiskSize( );  
		mStopIfError( i );
		printf( "TotalSize = %u MB \n", (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * (CH375vSectorSize/512) / 2048 ) );  // 显示为以MB为单位的容量
		// 原计算方法 (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * CH375vSectorSize / 1000000 ) 有可能前两个数据相乘后导致溢出, 所以修改成上式
*/

/* 读取原文件 */
		printf( "Open\n" );
		strcpy( (char *)mCmdParam.Open.mPathName, "/C51/CH375HFT.C" );  /* 文件名,该文件在C51子目录下 */
		i = CH375FileOpen( );  /* 打开文件 */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* 没有找到文件 */
/* 列出文件 */
			if ( i == ERR_MISS_DIR ) pCodeStr = (UINT8 *)"/*";  /* C51子目录不存在则列出根目录下的文件 */
			else pCodeStr = (UINT8 *)"/C51/CH375*";  /* CH375HFT.C文件不存在则列出\C51子目录下的以CH375开头的文件 */
			printf( "List file %s\n", pCodeStr );
			for ( c = 0; c < 254; c ++ ) {  /* 最多搜索前254个文件 */
				strcpy( (char *)mCmdParam.Open.mPathName, (char *)pCodeStr );  /* 搜索文件名,*为通配符,适用于所有文件或者子目录 */
				i = strlen( (char *)mCmdParam.Open.mPathName );  /* 计算文件名长度,以处理文件名结束符 */
				mCmdParam.Open.mPathName[ i ] = c;  /* 根据字符串长度将结束符替换为搜索的序号,从0到255 */
				i = CH375FileOpen( );  /* 打开文件,如果文件名中含有通配符*,则为搜索文件而不打开 */
				if ( i == ERR_MISS_FILE ) break;  /* 再也搜索不到匹配的文件,已经没有匹配的文件名 */
				if ( i == ERR_FOUND_NAME ) {  /* 搜索到与通配符相匹配的文件名,文件名及其完整路径在命令缓冲区中 */
					printf( "  match file %03d#: %s\n", (unsigned int)c, mCmdParam.Open.mPathName );  /* 显示序号和搜索到的匹配文件名或者子目录名 */
					continue;  /* 继续搜索下一个匹配的文件名,下次搜索时序号会加1 */
				}
				else {  /* 出错 */
					mStopIfError( i );
					break;
				}
			}
		}
		else {  /* 找到文件或者出错 */
			mStopIfError( i );
			TotalCount = 600;  /* 准备读取总长度 */
			printf( "从文件中读出的前%d个字符是:\n",TotalCount );
			while ( TotalCount ) {  /* 如果文件比较大,一次读不完,可以再调用CH375ByteRead继续读取,文件指针自动向后移动 */
				if ( TotalCount > MAX_BYTE_IO ) c = MAX_BYTE_IO;  /* 剩余数据较多,限制单次读写的长度不能超过 sizeof( mCmdParam.ByteRead.mByteBuffer ) */
				else c = TotalCount;  /* 最后剩余的字节数 */
				mCmdParam.ByteRead.mByteCount = c;  /* 请求读出几十字节数据 */
				i = CH375ByteRead( );  /* 以字节为单位读取数据块,单次读写的长度不能超过MAX_BYTE_IO,第二次调用时接着刚才的向后读 */
				mStopIfError( i );
				TotalCount -= mCmdParam.ByteRead.mByteCount;  /* 计数,减去当前实际已经读出的字符数 */
				for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%c", mCmdParam.ByteRead.mByteBuffer[i] );  /* 显示读出的字符 */
				if ( mCmdParam.ByteRead.mByteCount < c ) {  /* 实际读出的字符数少于要求读出的字符数,说明已经到文件的结尾 */
					printf( "\n" );
					printf( "文件已经结束\n" );
					break;
				}
			}
/*	    如果希望从指定位置开始读写,可以移动文件指针
		mCmdParam.ByteLocate.mByteOffset = 608;  跳过文件的前608个字节开始读写
		CH375ByteLocate( );
		mCmdParam.ByteRead.mByteCount = 5;  读取5个字节
		CH375ByteRead( );   直接读取文件的第608个字节到612个字节数据,前608个字节被跳过

	    如果希望将新数据添加到原文件的尾部,可以移动文件指针
		CH375FileOpen( );
		mCmdParam.ByteLocate.mByteOffset = 0xffffffff;  移到文件的尾部
		CH375ByteLocate( );
		mCmdParam.ByteWrite.mByteCount = 13;  写入13个字节的数据
		CH375ByteWrite( );   在原文件的后面添加数据,新加的13个字节接着原文件的尾部放置
		mCmdParam.ByteWrite.mByteCount = 2;  写入2个字节的数据
		CH375ByteWrite( );   继续在原文件的后面添加数据
		mCmdParam.ByteWrite.mByteCount = 0;  写入0个字节的数据,实际上该操作用于通知程序库更新文件长度
		CH375ByteWrite( );   写入0字节的数据,用于自动更新文件的长度,所以文件长度增加15,如果不这样做,那么执行CH375FileClose时也会自动更新文件长度
*/
			printf( "Close\n" );
			i = CH375FileClose( );  /* 关闭文件 */
			mStopIfError( i );
		}

#ifdef EN_DISK_WRITE  /* 子程序库支持写操作 */
/* 产生新文件 */
		printf( "Create\n" );
		strcpy( (char *)mCmdParam.Create.mPathName, "/NEWFILE.TXT" );  /* 新文件名,在根目录下,中文文件名 */
		i = CH375FileCreate( );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
		printf( "Write\n" );
		pCodeStr = (UINT8 *)"Note: \xd\xa这个程序是以字节为单位进行U盘文件读写,单片机只需要有600字节的RAM\xd\xa";
		while( 1 ) {  /* 分多次写入文件数据 */
			for ( i=0; i<MAX_BYTE_IO; i++ ) {
				c = *pCodeStr;
				mCmdParam.ByteWrite.mByteBuffer[i] = c;
				if ( c == 0 ) break;  /* 源字符串结束 */
				pCodeStr++;
			}
			if ( i == 0 ) break;  /* 源字符串结束,完成写文件 */
			mCmdParam.ByteWrite.mByteCount = i;  /* 写入数据的字符数,单次读写的长度不能超过MAX_BYTE_IO,第二次调用时接着刚才的向后写 */
			i = CH375ByteWrite( );  /* 向文件写入数据 */
			mStopIfError( i );
		}
/*		printf( "Modify\n" );
		mCmdParam.Modify.mFileAttr = 0xff;   输入参数: 新的文件属性,为0FFH则不修改
		mCmdParam.Modify.mFileTime = 0xffff;   输入参数: 新的文件时间,为0FFFFH则不修改,使用新建文件产生的默认时间
		mCmdParam.Modify.mFileDate = MAKE_FILE_DATE( 2004, 5, 18 );  输入参数: 新的文件日期: 2004.05.18
		mCmdParam.Modify.mFileSize = 0xffffffff;   输入参数: 新的文件长度,以字节为单位写文件应该由程序库关闭文件时自动更新长度,所以此处不修改
		i = CH375FileModify( );   修改当前文件的信息,修改日期
		mStopIfError( i );
*/
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 1;  /* 自动计算文件长度,以字节为单位写文件,建议让程序库关闭文件以便自动更新文件长度 */
		i = CH375FileClose( );
		mStopIfError( i );

/* 删除某文件 */
/*		printf( "Erase\n" );
		strcpy( (char *)mCmdParam.Create.mPathName, "/OLD" );  将被删除的文件名,在根目录下
		i = CH375FileErase( );  删除文件并关闭
		if ( i != ERR_SUCCESS ) printf( "Error: %02X\n", (UINT16)i );  显示错误
*/

/* 查询磁盘信息 */
/*		printf( "Disk\n" );
		i = CH375DiskQuery( );
		mStopIfError( i );
		printf( "Fat=%d, Total=%ld, Free=%ld\n", (UINT16)mCmdParam.Query.mDiskFat, mCmdParam.Query.mTotalSector, mCmdParam.Query.mFreeSector );
*/
#endif
		printf( "Take out\n" );
		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
		LED_OUT_INACT( );  /* LED灭 */
		mDelaymS( 200 );
/*		i = FILE_DATA_BUF[0];  因为是以字节为单位读写文件,未用到文件数据缓冲区,为了防止编译器优化掉该缓冲区而用一下缓冲区 */
	}
}
