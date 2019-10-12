/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, KEIL_C_2.41@ARM         **
****************************************
*/
/* CH375 主机文件系统接口 */
/* 支持: FAT12/FAT16/FAT32 */

/* ARM单片机C语言的U盘文件读写示例程序 */
/* 该程序将U盘中的/C51/CH375HFT.C文件中的小写字母转成大写字母后, 写到新建的文件NEWFILE.TXT中,
   如果找不到原文件CH375HFT.C, 那么该程序将显示C51子目录下所有以CH375开头的文件名, 并新建NEWFILE.TXT文件并写入提示信息,
   如果找不到C51子目录, 那么该程序将显示根目录下的所有文件名, 并新建NEWFILE.TXT文件并写入提示信息
*/
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"内部复制", 本程序适用于Philips LPC2114单片机, 串口0输出监控信息,9600bps */
/* ENDIAN = "little" */

/* CA CH375HFT.C ARM */
/* LA CH375HFT.OBJ, CH375HFM.LIB */
/* OHA CH375HFT */

#include <Philips\LPC21xx.H>						/* 目标板的专用初始化代码 */

#include <string.h>
#include <stdio.h>


/* 以下定义的详细说明请看CH375HFM.H文件 */
#define LIB_CFG_FILE_IO			1		/* 文件读写的数据的复制方式,0为"外部子程序",1为"内部复制" */
#define LIB_CFG_INT_EN			0		/* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */

/* 单片机的RAM有限,其中CH375子程序用512字节,剩余RAM部分可以用于文件读写缓冲 */
#define FILE_DATA_BUF_LEN		0x2000	/* 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */
/* 如果准备使用双缓冲区交替读写,那么不要定义FILE_DATA_BUF_LEN,而是在参数中指定缓冲区起址,用CH375FileReadX代替CH375FileRead,用CH375FileWriteX代替CH375FileWrite */

#define CH375_INT_WIRE			( IO0PIN & 0x08 )	/* P0.3, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM		1		/* 未调用CH375FileEnumer程序故禁止以节约代码 */
#define NO_DEFAULT_CH375_F_QUERY	1		/* 未调用CH375FileQuery程序故禁止以节约代码 */

#include "..\CH375HFM.H"

/* 有些ARM单片机提供开放系统总线,那么直接将CH375挂在其系统总线上,以8位I/O方式进行读写 */
/* 本例使用的LPC2114不开放系统总线,所以用I/O引脚模拟产生CH375的并口读写时序 */
/* 本例中的硬件连接方式如下(实际应用电路可以参照修改下述3个并口读写子程序) */
/* LPC2114单片机的引脚    CH375芯片的引脚
         P0.3                 INT#
         P0.4                 A0
         P0.7                 CS#
         P0.6                 WR#
         P0.5                 RD#
  P0.15-P0.8(8位端口)         D7-D0       */

void mDelay1_2uS( )  /* 至少延时1.2uS,根据单片机主频调整 */
{
	UINT32	i;
	for ( i = 10; i != 0; i -- );  /* 本例由于模拟I/O较慢故只需少量延时 */
}

void CH375_PORT_INIT( )  /* 由于使用通用I/O模块并口读写时序,所以进行初始化 */
{
	IO0SET |= 0x000000E0;  /* 设置CS,WR,RD默认为高电平 */
	IO0DIR &= 0xFFFF00F7;  /* 设置8位并口和INT#为输入 */
	IO0DIR |= 0x000000F0;  /* 设置CS,WR,RD,A0为输出 */
}

void xWriteCH375Cmd( UINT8 mCmd )		/* 外部定义的被CH375程序库调用的子程序,向CH375写命令 */
{
	mDelay1_2uS( ); mDelay1_2uS( );  /* 至少延时1uS */
	IO0CLR |= 0x0000FF00;  /* 清并口输出 */
	IO0SET |= ( (UINT32)mCmd << 8 ) | 0x00000010;  /* 向CH375的并口输出数据, 输出A0(P0.4)=1; */
	IO0DIR |= 0x0000FFF0;  /* 写操作所以数据输出, 设置CS,WR,RD,A0为输出 */
	IO0CLR |= 0x000000C0;  /* 输出有效写控制信号, 写CH375芯片的命令端口, A0(P0.4)=1; CS(P0.7)=0; WR=(P0.6)=0; RD(P0.5)=1; */
	IO0DIR = IO0DIR; IO0DIR = IO0DIR;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于100nS */
	IO0SET |= 0x000000E0;  /* 输出无效的控制信号, 完成操作CH375芯片, A0(P0.4)=1; CS(P0.7)=1; WR=(P0.6)=1; RD(P0.5)=1; */
	IO0CLR |= 0x00000010;  /* 输出A0(P0.4)=0; 可选操作 */
	IO0DIR &= 0xFFFF00FF;  /* 禁止数据输出 */
	mDelay1_2uS( ); mDelay1_2uS( );  /* 至少延时2uS */
}

void xWriteCH375Data( UINT8 mData )		/* 外部定义的被CH375程序库调用的子程序,向CH375写数据 */
{
	IO0CLR |= 0x0000FF00;  /* 清并口输出 */
	IO0SET |= (UINT32)mData << 8;  /* 向CH375的并口输出数据 */
	IO0DIR |= 0x0000FF00;  /* 写操作所以数据输出 */
	IO0CLR |= 0x000000D0;  /* 输出有效写控制信号, 写CH375芯片的数据端口, A0(P0.4)=0; CS(P0.7)=0; WR=(P0.6)=0; RD(P0.5)=1; */
	IO0DIR = IO0DIR; IO0DIR = IO0DIR;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于100nS */
	IO0SET |= 0x000000E0;  /* 输出无效的控制信号, 完成操作CH375芯片, A0(P0.4)=0; CS(P0.7)=1; WR=(P0.6)=1; RD(P0.5)=1; */
	IO0DIR &= 0xFFFF00FF;  /* 禁止数据输出 */
	mDelay1_2uS( );  /* 至少延时1.2uS */
}

UINT8 xReadCH375Data( void )			/* 外部定义的被CH375程序库调用的子程序,从CH375读数据 */
{
	UINT8	mData;
	mDelay1_2uS( );  /* 至少延时1.2uS */
	IO0DIR &= 0xFFFF00FF;  /* 读操作所以数据输入 */
	IO0CLR |= 0x000000B0;  /* 输出有效读控制信号, 读CH375芯片的数据端口, A0(P0.4)=0; CS(P0.7)=0; WR=(P0.6)=1; RD(P0.5)=0; */
	IO0DIR = IO0DIR; IO0DIR = IO0DIR;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于100nS */
	mData = (UINT8)( IO0PIN >> 8 );  /* 从CH375的并口P0.15-P0.8输入数据 */
	IO0SET |= 0x000000E0;  /* 输出无效的控制信号, 完成操作CH375芯片, A0(P0.4)=0; CS(P0.7)=1; WR=(P0.6)=1; RD(P0.5)=1; */
	return( mData );
}

/* 在P0.2连接一个LED用于监控演示程序的进度,低电平LED亮 */
#define LED_OUT_INIT( )		{ IO0DIR |= 0x04; }	/* P0.2 高电平为输出方向 */
#define LED_OUT_ACT( )		{ IO0CLR |= 0x04; }	/* P0.2 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ IO0SET |= 0x04; }	/* P0.2 低电平驱动LED显示 */

/* 延时指定毫秒时间,根据单片机主频调整,不精确 */
void	mDelaymS( UINT32 ms )
{
	UINT32	i;
	while ( ms -- ) for ( i = 25000; i != 0; i -- );
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
/*     本例子的配置             */
/* 系统设置, Fosc、Fcclk、Fcco、Fpclk必须定义*/
#define Fosc            11059200                    //晶振频率,10MHz~25MHz，应当与实际一至
#define Fcclk           (Fosc * 4)                  //系统频率，必须为Fosc的整数倍(1~32)，且<=60MHZ
#define Fcco            (Fcclk * 4)                 //CCO频率，必须为Fcclk的2、4、8、16倍，范围为156MHz~320MHz
#define Fpclk           (Fcclk / 4) * 1             //VPB时钟频率，只能为(Fcclk / 4)的1、2、4倍
void	mInitSTDIO( )
{
	UINT32	x;
	PINSEL0 = PINSEL0 & 0xFFFFFFF0 | 0x00000005;  // 设置I/O连接到UART0
	U0LCR = 0x80;                    // DLAB位置1
	x = (Fpclk>>4)/9600;             // 9600bps
	U0DLM = x>>8;
	U0DLL = x&0xff;
	U0LCR = 0x03;                    // 8位数据位,1位停止位,无奇偶校验
	U0FCR = 0x01;
}

/* 通过串口输出监控信息 */
/* 由于KEIL C 2.41有问题，无法实现PUTCHAR的代替，包括KEIL提供的HELLO例子均无法工作，
   所以本功能无法实现，详情请咨询KEIL技术人员，等KEIL自身问题解决后将下面的PUTCHAR子程序去掉注释
int		putchar( int ch )
{
	U0THR = ch;                           // 发送数据
	while( ( U0LSR & 0x20 ) == 0 );       // 等待数据发送
	return( ch );
}*/

int		main( ) {
	UINT8	i, c, SecCount;
	UINT16	NewSize, count;  /* 因为RAM容量有限,所以NewSize限制为16位,实际上如果文件较大,应该分几次读写并且将NewSize改为UINT32以便累计 */
	UINT8	*pCodeStr;
	CH375_PORT_INIT( );
	LED_OUT_INIT( );
	LED_OUT_ACT( );  /* 开机后LED亮一下以示工作 */
	mDelaymS( 100 );  /* 延时100毫秒 */
	LED_OUT_INACT( );
	mInitSTDIO( );  /* 为了让计算机通过串口监控演示过程 */
	printf( "Start\n" );

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
		for ( i = 0; i < 10; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
			mDelaymS( 100 );
			printf( "Ready ?\n" );
			if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
		}
/* 查询磁盘物理容量 */
/*		printf( "DiskSize\n" );
		i = CH375DiskSize( );  
		mStopIfError( i );
		printf( "TotalSize = %d MB \n", (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec >> 11 ) );  显示为以MB为单位的容量
*/

/* 读取原文件 */
		printf( "Open\n" );
		strcpy( (char *)mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C" );  /* 文件名,该文件在C51子目录下 */
		i = CH375FileOpen( );  /* 打开文件 */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* 没有找到文件 */
/* 列出文件 */
			if ( i == ERR_MISS_DIR ) pCodeStr = (UINT8 *)"\\*";  /* C51子目录不存在则列出根目录下的文件 */
			else pCodeStr = (UINT8 *)"\\C51\\CH375*";  /* CH375HFT.C文件不存在则列出\C51子目录下的以CH375开头的文件 */
			printf( "List file %s\n", pCodeStr );
			for ( c = 0; c < 255; c ++ ) {  /* 最多搜索前255个文件 */
				strcpy( (char *)mCmdParam.Open.mPathName, (char *)pCodeStr );  /* 搜索文件名,*为通配符,适用于所有文件或者子目录 */
				i = strlen( (char const *)mCmdParam.Open.mPathName );  /* 计算文件名长度,以处理文件名结束符 */
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
			pCodeStr = (UINT8 *)"找不到/C51/CH375HFT.C文件\xd\n";
			for ( i = 0; i != 255; i ++ ) {
				if ( ( FILE_DATA_BUF[i] = *pCodeStr ) == 0 ) break;
				pCodeStr++;
			}
			NewSize = i;  /* 新文件的长度 */
			SecCount = 1;  /* (NewSize+511)/512, 计算文件的扇区数,因为读写是以扇区为单位的 */
		}
		else {  /* 找到文件或者出错 */
			mStopIfError( i );
/*			printf( "Query\n" );
			i = CH375FileQuery( );  查询当前文件的信息
			mStopIfError( i );*/
			printf( "Read\n" );
			if ( CH375vFileSize > FILE_DATA_BUF_LEN ) {  /* 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以只读取不超过63个扇区,也就是不超过32256字节 */
				SecCount = FILE_DATA_BUF_LEN / 512;  /* 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以只读取不超过63个扇区,也就是不超过32256字节 */
				NewSize = FILE_DATA_BUF_LEN;  /* 由于RAM有限所以限制长度 */
			}
			else {  /* 如果原文件较小,那么使用原长度 */
				SecCount = ( CH375vFileSize + 511 ) >> 9;  /* (CH375vFileSize+511)/512, 计算文件的扇区数,因为读写是以扇区为单位的,先加511是为了读出文件尾部不足1个扇区的部分 */
				NewSize = (UINT16)CH375vFileSize;  /* 原文件的长度 */
			}
			printf( "Size=%ld, Len=%d, Sec=%d\n", CH375vFileSize, NewSize, (UINT16)SecCount );
			mCmdParam.Read.mSectorCount = SecCount;  /* 读取全部数据,如果超过60个扇区则只读取60个扇区 */
/*			current_buffer = & FILE_DATA_BUF[0];  如果文件读写的数据的复制方式为"外部子程序",那么需要设置存放数据的缓冲区的起始地址 */
			CH375vFileSize += 511;  /* 默认情况下,以扇区方式读取数据时,无法读出文件尾部不足1个扇区的部分,所以必须临时加大文件长度以读取尾部零头 */
			i = CH375FileRead( );  /* 从文件读取数据 */
			CH375vFileSize -= 511;  /* 恢复原文件长度 */
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
		CH375FileRead();   直接读取从文件的第(512*3)个字节开始的数据,前3个扇区被跳过

	    如果希望将新数据添加到原文件的尾部,可以移动文件指针
		i = CH375FileOpen( );
		mCmdParam.Locate.mSectorOffset = 0xffffffff;  移到文件的尾部,以扇区为单位,如果原文件是3字节,则从512字节开始添加
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
		mCmdParam.Close.mUpdateLen = 0;  /* 不要自动计算文件长度,如果自动计算,那么该长度总是512的倍数 */
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
