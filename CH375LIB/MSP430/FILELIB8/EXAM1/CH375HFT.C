/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, IAR_C/EC++_2.10A@MSP430 **
****************************************
*/
/* CH375 主机文件系统接口 */
/* 支持: FAT12/FAT16/FAT32 */

/* MSP430单片机C语言的U盘文件读写示例程序, 适用于具有不少于1KB容量RAM的单片机, 如果RAM容量少于1100字节, 那么需要合用磁盘缓冲区与文件缓冲区 */
/* 该程序将U盘中的/C51/CH375HFT.C文件中的小写字母转成大写字母后, 写到新建的文件NEWFILE.TXT中,
   如果找不到原文件CH375HFT.C, 那么该程序将显示C51子目录下所有以CH375开头的文件名, 并新建NEWFILE.TXT文件并写入提示信息,
   如果找不到C51子目录, 那么该程序将显示根目录下的所有文件名, 并新建NEWFILE.TXT文件并写入提示信息
*/
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"内部复制", 本程序适用于MSP430F449单片机, 串口0输出监控信息,9600bps */


/* ICC430 CH375HFT.C -l CH375HFT.LST -o CH375HFT.R43 */
/* XLINK CH375HFT.R43 -o CH375HFT.TXT -Fmsp430_txt ..\430\lib\cl430f.r43 CH375HF8.R43 -f ..\430\config\lnk430F449.xcl */

#include <msp430x44x.h>
#include <string.h>
#include <stdio.h>

/* 以下定义的详细说明请看CH375HF8.H文件 */
#define LIB_CFG_FILE_IO			1		/* 文件读写的数据的复制方式,0为"外部子程序",1为"内部复制" */
#define LIB_CFG_INT_EN			0		/* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375命令端口的I/O地址 */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375数据端口的I/O地址 */
/* 单片机的1KB的RAM分为两部分: 0200H-03FFH为磁盘读写缓冲区, 0400H-05FFH为文件数据缓冲区 */
#define	DISK_BASE_BUF_ADDR		0x0200	/* 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */
#define DISK_BASE_BUF_LEN		512		/* 默认的磁盘数据缓冲区大小为512字节,建议选择为2048甚至4096以支持某些大扇区的U盘,为0则禁止在.H文件中定义缓冲区并由应用程序在pDISK_BASE_BUF中指定 */
#define FILE_DATA_BUF_ADDR		0x0400	/* 外部RAM的文件数据缓冲区的起始地址,缓冲区长度不小于一次读写的数据长度 */
/* 单片机的RAM有限,其中CH375子程序用512字节,所以外部RAM剩余长度为512字节,即使是具有2K容量RAM的单片机,减去堆栈和变量的占用,缓冲区最多为1K字节 */
#define FILE_DATA_BUF_LEN		0x0200	/* 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */
/* 如果准备使用双缓冲区交替读写,那么不要定义FILE_DATA_BUF_LEN,而是在参数中指定缓冲区起址,用CH375FileReadX代替CH375FileRead,用CH375FileWriteX代替CH375FileWrite */

#define CH375_INT_WIRE			( P1IN & 0x10 )	/* P1.4, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM		1		/* 未调用CH375FileEnumer程序故禁止以节约代码 */
#define NO_DEFAULT_CH375_F_QUERY	1		/* 未调用CH375FileQuery程序故禁止以节约代码 */

#pragma language=extended
#include "..\CH375HF8.H"
#pragma language=default

/* 由于MSP430不开放系统总线,所以用I/O引脚模拟产生CH375的并口读写时序 */
/* 本例中的硬件连接方式如下(实际应用电路可以参照修改下述3个并口读写子程序) */
/* MSP430单片机的引脚    CH375芯片的引脚
         P1.4                 INT#
         P1.3                 A0
         P1.2                 CS#
         P1.1                 WR#
         P1.0                 RD#
         P4(8位端口)         D7-D0       */

void CH375_PORT_INIT( )  /* 由于使用通用I/O模块并口读写时序,所以进行初始化 */
{
	P1OUT = ( P1OUT | 0x07 ) & 0xF7;  /* 设置A0为低电平,CS,WR,RD默认为高电平 */
	P1DIR = ( P1DIR | 0x0F ) & 0xEF;  /* 设置INT#为输入,设置CS,WR,RD,A0为输出 */
	P4DIR = 0;  /* 设置8位并口输入 */
}

void xWriteCH375Cmd( UINT8 mCmd )		/* 外部定义的被CH375程序库调用的子程序,向CH375写命令 */
{
	_NOP( ); _NOP( ); _NOP( );  /* 至少延时2uS,实际由于模拟I/O较慢而只需少量延时 */
	P1DIR |= 0x0F;  /* 设置P1口A0,CS,WR,RD为输出控制信号 */
	P4OUT = mCmd;  /* 向CH375的并口输出数据 */
	P4DIR = 0xFF;  /* 写操作所以数据输出 */
	P1OUT |= 0x0F;  /* 指向CH375芯片的命令端口, A0(P1.3)=1; */
	P1OUT &= 0xF9;  /* 输出有效写控制信号, 写CH375芯片的命令端口, A0(P1.3)=1; CS(P1.2)=0; WR=(P1.1)=0; RD(P1.0)=1; */
	_NOP( );  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度为100nS */
	P1OUT |= 0x07;  /* 输出无效的控制信号, 完成操作CH375芯片, A0(P1.3)=1; CS(P1.2)=1; WR=(P1.1)=1; RD(P1.0)=1; */
	P1OUT &= 0xF7;  /* 输出A0(P1.3)=0; 可选操作 */
	P4DIR = 0;  /* 禁止数据输出 */
	_NOP( ); _NOP( ); _NOP( ); _NOP( ); _NOP( );  /* 至少延时2uS,实际由于模拟I/O较慢而只需少量延时 */
}

void xWriteCH375Data( UINT8 mData )		/* 外部定义的被CH375程序库调用的子程序,向CH375写数据 */
{
	P4OUT = mData;  /* 向CH375的并口输出数据 */
	P4DIR = 0xFF;  /* 写操作所以数据输出 */
	P1OUT &= 0xF1;  /* 输出有效写控制信号, 写CH375芯片的数据端口, A0(P1.3)=0; CS(P1.2)=0; WR=(P1.1)=0; RD(P1.0)=1; */
	_NOP( );  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度为100nS */
	P1OUT |= 0x07;  /* 输出无效的控制信号, 完成操作CH375芯片, A0(P1.3)=0; CS(P1.2)=1; WR=(P1.1)=1; RD(P1.0)=1; */
	P4DIR = 0;  /* 禁止数据输出 */
	_NOP( );  /* 至少延时1uS,实际由于模拟I/O较慢而不一定有必要 */
}

UINT8 xReadCH375Data( void )			/* 外部定义的被CH375程序库调用的子程序,从CH375读数据 */
{
	UINT8	mData;
	_NOP( );  /* 至少延时1uS,实际由于模拟I/O较慢而不一定有必要 */
	P4DIR = 0;  /* 读操作所以数据输入 */
	P1OUT &= 0xF2;  /* 输出有效读控制信号, 读CH375芯片的数据端口, A0(P1.3)=0; CS(P1.2)=0; WR=(P1.1)=1; RD(P1.0)=0; */
	_NOP( );  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度为100nS */
	mData = P4IN;  /* 从CH375的并口输入数据 */
	P1OUT |= 0x07;  /* 输出无效的控制信号, 完成操作CH375芯片, A0(P1.3)=0; CS(P1.2)=1; WR=(P1.1)=1; RD(P1.0)=1; */
	return( mData );
}

/* 在P1.7连接一个LED用于监控演示程序的进度,低电平LED亮 */
#define LED_OUT_INIT( )		{ P1DIR |= 0x80; }	/* P1.7 高电平为输出方向 */
#define LED_OUT_ACT( )		{ P1OUT &= 0x7F; }	/* P1.7 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ P1OUT |= 0x80; }	/* P1.7 低电平驱动LED显示 */

/* 延时指定毫秒时间,适用于8MHz时钟,不精确 */
void	mDelaymS( UINT16 ms )
{
	UINT16	i;
	while ( ms -- ) for ( i = 2000; i != 0; i -- );
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
void	mInitSTDIO( )
{
	UTCTL0 = SSEL1;                       // UCLK = SMCLK
	UBR00 = 0x41;                         // 7.99MHz 9600bps
	UBR10 = 0x03;                         // 7.99MHz 9600bps
	UMCTL0 = 0x00;                        // no modulation
	UCTL0 = CHAR;                         // 8-bit character *SWRST*
	ME1 |= UTXE0 + URXE0;                 // Enable USART0 TXD/RXD
	P2SEL |= 0x30;                        // P2.4,5 = USART0 TXD/RXD
	P2DIR |= 0x10;                        // P2.4 output direction
}

/* 通过串口输出监控信息 */
int		putchar( int c )
{
	while ( ( IFG1 & UTXIFG0 ) == 0 );    // USART0 TX buffer ready?
	TXBUF0 = c;                           // char to TXBUF0
	return( c );
}

/* 选择8MHz时钟 */
void	init_clk( )
{
	WDTCTL = WDTPW + WDTHOLD;       // stop watchdog timer
	SCFI0 |= FN_4;                  // x2 DCO frequency, 8MHz nominal DCO  
	SCFQCTL = 121;                  // (121+1) x 32768 x 2 = 7.99 Mhz
	FLL_CTL0 = DCOPLUS + XCAP18PF;  // DCO+ set so freq = xtal x D x N+1
}

main( ) {
	UINT8	i, c, SecCount;
	UINT16	NewSize, count;  /* 因为RAM容量有限,所以NewSize限制为16位,实际上如果文件较大,应该分几次读写并且将NewSize改为UINT32以便累计 */
	UINT8	*pCodeStr;
	init_clk( );
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
//		while ( CH375DiskStatus != DISK_CONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘插入 */
		while ( CH375DiskStatus < DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘插入 */
			if ( CH375DiskConnect( ) == ERR_SUCCESS ) break;  /* 有设备连接则返回成功,CH375DiskConnect同时会更新全局变量CH375DiskStatus */
			mDelaymS( 100 );
		}
		LED_OUT_ACT( );  /* LED亮 */
		mDelaymS( 200 );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */

/* 检查U盘是否准备好,有些U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 5; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
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
		strcpy( (char *)mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C" );  /* 文件名,该文件在C51子目录下 */
		i = CH375FileOpen( );  /* 打开文件 */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* 没有找到文件 */
/* 列出文件 */
			if ( i == ERR_MISS_DIR ) pCodeStr = "\\*";  /* C51子目录不存在则列出根目录下的文件 */
			else pCodeStr = "\\C51\\CH375*";  /* CH375HFT.C文件不存在则列出\C51子目录下的以CH375开头的文件 */
			printf( "List file %s\n", pCodeStr );
			for ( c = 0; c < 254; c ++ ) {  /* 最多搜索前254个文件 */
				strcpy( (char *)mCmdParam.Open.mPathName, (char *)pCodeStr );  /* 搜索文件名,*为通配符,适用于所有文件或者子目录 */
				i = strlen( (char const *)mCmdParam.Open.mPathName );  /* 计算文件名长度,以处理文件名结束符 */
				mCmdParam.Open.mPathName[ i ] = c;  /* 根据字符串长度将结束符替换为搜索的序号,从0到254 */
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
			pCodeStr = "找不到/C51/CH375HFT.C文件\xd\n";
			for ( i = 0; i != 255; i ++ ) {
				if ( ( FILE_DATA_BUF[i] = *pCodeStr ) == 0 ) break;
				pCodeStr++;
			}
			NewSize = i;  /* 新文件的长度 */
			SecCount = 1;  /* (NewSize+CH375vSectorSize-1)/CH375vSectorSize, 计算文件的扇区数,因为读写是以扇区为单位的 */
		}
		else {  /* 找到文件或者出错 */
			mStopIfError( i );
/*			printf( "Query\n" );
			i = CH375FileQuery( );  查询当前文件的信息
			mStopIfError( i );*/
			printf( "Read\n" );
			if ( CH375vFileSize > FILE_DATA_BUF_LEN ) {  /* 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以只读取不超过63个扇区,也就是不超过32256字节 */
				SecCount = FILE_DATA_BUF_LEN / CH375vSectorSize;  /* 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以只读取不超过63个扇区,也就是不超过32256字节 */
				NewSize = FILE_DATA_BUF_LEN;  /* 由于RAM有限所以限制长度 */
			}
			else {  /* 如果原文件较小,那么使用原长度 */
				SecCount = ( CH375vFileSize + CH375vSectorSize-1 ) / CH375vSectorSize;  /* 计算文件的扇区数,因为读写是以扇区为单位的,先加CH375vSectorSize-1是为了读出文件尾部不足1个扇区的部分 */
				NewSize = (UINT16)CH375vFileSize;  /* 原文件的长度 */
			}
			printf( "Size=%ld, Len=%d, Sec=%d\n", CH375vFileSize, NewSize, (UINT16)SecCount );
			mCmdParam.Read.mSectorCount = SecCount;  /* 读取全部数据,如果超过60个扇区则只读取60个扇区 */
/*			current_buffer = & FILE_DATA_BUF[0];  如果文件读写的数据的复制方式为"外部子程序",那么需要设置存放数据的缓冲区的起始地址 */
			CH375vFileSize += CH375vSectorSize-1;  /* 默认情况下,以扇区方式读取数据时,无法读出文件尾部不足1个扇区的部分,所以必须临时加大文件长度以读取尾部零头 */
			i = CH375FileRead( );  /* 从文件读取数据 */
			CH375vFileSize -= CH375vSectorSize-1;  /* 恢复原文件长度 */
			mStopIfError( i );
/*
		如果文件比较大,一次读不完,可以再调用CH375FileRead继续读取,文件指针自动向后移动
		while ( 1 ) {
			c = 32;   每次读取32个扇区
			mCmdParam.Read.mSectorCount = c;   指定读取的扇区数
			CH375FileRead();   读完后文件指针自动后移
			处理数据
			if ( mCmdParam.Read.mSectorCount < c ) break;   实际读出的扇区数较小则说明文件已经结束
		}

	    如果希望从指定位置开始读写,可以移动文件指针
		mCmdParam.Locate.mSectorOffset = 3;  跳过文件的前3个扇区开始读写
		i = CH375FileLocate( );
		mCmdParam.Read.mSectorCount = 10;
		CH375FileRead();   直接读取从文件的第(CH375vSectorSize*3)个字节开始的数据,前3个扇区被跳过

	    如果希望将新数据添加到原文件的尾部,可以移动文件指针
		i = CH375FileOpen( );
		mCmdParam.Locate.mSectorOffset = 0xffffffff;  移到文件的尾部,以扇区为单位,如果原文件是3字节,则从CH375vSectorSize字节开始添加
		i = CH375FileLocate( );
		mCmdParam.Write.mSectorCount = 10;
		CH375FileWrite();   在原文件的后面添加数据

使用CH375FileReadX可以自行定义数据缓冲区的起始地址
		mCmdParam.ReadX.mSectorCount = 2;
		mCmdParam.ReadX.mDataBuffer = 0x2000;  将读出的数据放到2000H开始的缓冲区中
		CH375FileReadX();   从文件中读取2个扇区到指定缓冲区

使用CH375FileWriteX可以自行定义数据缓冲区的起始地址
		mCmdParam.WiiteX.mSectorCount = 2;
		mCmdParam.WriteX.mDataBuffer = 0x4600;  将4600H开始的缓冲区中的数据写入
		CH375FileWriteX();   将指定缓冲区中的数据写入2个扇区到文件中

*/
			printf( "Close\n" );
			i = CH375FileClose( );  /* 关闭文件 */
			mStopIfError( i );

			i = FILE_DATA_BUF[100];
			FILE_DATA_BUF[100] = 0;  /* 置字符串结束标志,最多显示100个字符 */
			printf( "Line 1: %s\n", FILE_DATA_BUF );
			FILE_DATA_BUF[100] = i;  /* 恢复原字符 */
			for ( count=0; count < NewSize; count ++ ) {  /* 将文件中的小写字符转换为大写 */
				c = FILE_DATA_BUF[ count ];
				if ( c >= 'a' && c <= 'z' ) FILE_DATA_BUF[ count ] = c - ( 'a' - 'A' );
			}
		}

#ifdef EN_DISK_WRITE  /* 子程序库支持写操作 */
/* 产生新文件 */
		printf( "Create\n" );
		strcpy( (char *)mCmdParam.Create.mPathName, "\\NEWFILE.TXT" );  /* 新文件名,在根目录下 */
		i = CH375FileCreate( );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
		printf( "Write\n" );
		mCmdParam.Write.mSectorCount = SecCount;  /* 写入所有扇区的数据 */
/*		current_buffer = & FILE_DATA_BUF[0];  如果文件读写的数据的复制方式为"外部子程序",那么需要设置存放数据的缓冲区的起始地址 */
		i = CH375FileWrite( );  /* 向文件写入数据 */
		mStopIfError( i );
		printf( "Modify\n" );
		mCmdParam.Modify.mFileAttr = 0xff;  /* 输入参数: 新的文件属性,为0FFH则不修改 */
		mCmdParam.Modify.mFileTime = 0xffff;  /* 输入参数: 新的文件时间,为0FFFFH则不修改,使用新建文件产生的默认时间 */
		mCmdParam.Modify.mFileDate = MAKE_FILE_DATE( 2004, 5, 18 );  /* 输入参数: 新的文件日期: 2004.05.18 */
		mCmdParam.Modify.mFileSize = NewSize;  /* 输入参数: 如果原文件较小,那么新的文件长度与原文件一样长,否则被RAM所限,如果文件长度大于64KB,那么NewSize必须为UINT32 */
		i = CH375FileModify( );  /* 修改当前文件的信息,修改日期和长度 */
		mStopIfError( i );
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 0;  /* 不要自动计算文件长度,如果自动计算,那么该长度总是CH375vSectorSize的倍数 */
		i = CH375FileClose( );
		mStopIfError( i );

/* 删除某文件 */
/*		printf( "Erase\n" );
		strcpy( (char *)mCmdParam.Create.mPathName, "\\OLD" );  将被删除的文件名,在根目录下
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
//		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
		while ( CH375DiskStatus >= DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
			if ( CH375DiskConnect( ) != ERR_SUCCESS ) break;
			mDelaymS( 100 );
		}
		LED_OUT_INACT( );  /* LED灭 */
		mDelaymS( 200 );
	}
}
