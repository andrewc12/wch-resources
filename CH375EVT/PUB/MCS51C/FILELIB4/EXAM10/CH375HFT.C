/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* CH375 主机文件系统接口 */
/* 支持: FAT12/FAT16/FAT32 */

/* MCS-51单片机C语言的U盘文件读写示例程序, 适用于89C52或者更大程序空间的单片机 */
/* 本程序用于演示处理文件目录项,例如:修改文件名,设置文件的创建日期和时间等 */
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR复制", 所以速度较慢, 适用于所有MCS51单片机
   本例适用于V2.4及以上版本的CH375子程序库 */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF6.LIB */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>
#include <string.h>

/* 以下定义的详细说明请看CH375HF6.H文件 */
#define LIB_CFG_DISK_IO			1		/* 磁盘读写的数据的复制方式,1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_FILE_IO			1		/* 文件读写的数据的复制方式,0为"外部子程序",1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_INT_EN			0		/* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */
/*#define LIB_CFG_FILE_IO_DEFAULT	1*/		/* 使用CH375HF6.H提供的默认"外部子程序" */
/*#define LIB_CFG_UPD_SIZE		1*/		/* 在添加数据后是否自动更新文件长度: 0为"不更新",1为"自动更新" */
/* 默认情况下,如果扇区数/字节数不为0那么CH375FileWrite/CH375ByteWrite只负责写入数据而不修改文件长度,
   如果需要每次写完数据后会自动修改/更新文件长度,那么可以使全局变量CH375LibConfig的位4为1,
   如果长时间不写入数据则应该更新文件长度,防止突然断电后前面写入的数据与文件长度不相符,
   如果确定不会突然断电或者后面很快有数据不断写入则不必更新文件长度,可以提高速度并减少U盘损耗(U盘内部的内存寿命有限,不宜频繁改写) */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375命令端口的I/O地址 */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375数据端口的I/O地址 */
/* 62256提供的32KB的RAM分为两部分: 0000H-01FFH为磁盘读写缓冲区, 0200H-7FFFH为文件数据缓冲区 */
#define	DISK_BASE_BUF_ADDR		0x0000	/* 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */
#define DISK_BASE_BUF_LEN		4096	/* 默认的磁盘数据缓冲区大小为512字节,建议选择为2048甚至4096以支持某些大扇区的U盘,为0则禁止在.H文件中定义缓冲区并由应用程序在pDISK_BASE_BUF中指定 */

#define CH375_INT_WIRE			INT0	/* P3.2, INT0, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM		1		/* 未调用CH375FileEnumer程序故禁止以节约代码 */

#include "..\CH375HF6.H"				/* 如果不需要支持FAT32,那么请选用CH375HF4.H */

/* 在P1.4连接一个LED用于监控演示程序的进度,低电平LED亮,当U盘插入后亮 */
sbit P1_4  = P1^4;
#define LED_OUT_INIT( )		{ P1_4 = 1; }	/* P1.4 高电平 */
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 低电平驱动LED显示 */
sbit P1_5  = P1^5;
/* 在P1.5连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘操作时亮 */
#define LED_RUN_ACT( )		{ P1_5 = 0; }	/* P1.5 低电平驱动LED显示 */
#define LED_RUN_INACT( )	{ P1_5 = 1; }	/* P1.5 低电平驱动LED显示 */
sbit P1_6  = P1^6;
/* 在P1.6连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘写操作时亮 */
#define LED_WR_ACT( )		{ P1_6 = 0; }	/* P1.6 低电平驱动LED显示 */
#define LED_WR_INACT( )		{ P1_6 = 1; }	/* P1.6 低电平驱动LED显示 */

/* 以毫秒为单位延时,不精确,适用于24MHz时钟 */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
	}
}

/* 将程序空间的字符串复制到内部RAM中,返回字符串长度 */
UINT8	mCopyCodeStringToIRAM( UINT8 idata *iDestination, UINT8 code *iSource )
{
	UINT8	i = 0;
	while ( *iDestination = *iSource ) {
		iDestination ++;
		iSource ++;
		i ++;
	}
	return( i );
}

/* 检查操作状态,如果错误则显示错误代码并停机,应该替换为实际的处理措施 */
void	mStopIfError( UINT8 iError )
{
	if ( iError == ERR_SUCCESS ) return;  /* 操作成功 */
	printf( "Error: %02X\n", (UINT16)iError );  /* 显示错误 */
	while ( 1 ) {
		LED_OUT_ACT( );  /* LED闪烁 */
		mDelaymS( 200 );
		LED_OUT_INACT( );
		mDelaymS( 200 );
	}
}

/* 为printf和getkey输入输出初始化串口 */
void	mInitSTDIO( )
{
	SCON = 0x50;
	PCON = 0x80;
	TMOD = 0x21;
	TH1 = 0xf3;  /* 24MHz晶振, 9600bps */
	TR1 = 1;
	TI = 1;
}

/* 修改指定文件的文件名,如果是C文件则修改为TXT文件 */
/* 输入参数:   原始文件名在mCmdParam.Open.mPathName中 */
/* 返回状态码: ERR_SUCCESS = 修改文件名成功,
               其它状态码参考CH375HF?.H */
UINT8	RenameFileName( void )
{
	UINT8			i;
	P_FAT_DIR_INFO	mFileDir;
	i = CH375FileOpen( );  /* 打开文件 */
	if ( i == ERR_SUCCESS ) {
		/* 文件读写操作等... */
		i = CH375FileQuery( );  /* 查询文件属性,以便将相关数据调到内存中再修改 */
		if ( i == ERR_SUCCESS ) {
			mFileDir = (P_FAT_DIR_INFO)( (PUINT8X)(&DISK_BASE_BUF[0]) + CH375vFdtOffset );  /* 在内存中,当前FDT的起始地址 */
			if ( mFileDir -> DIR_Name[8] == 'C' && mFileDir -> DIR_Name[9] == ' ' ) {  /* 文件扩展名是C */
				mFileDir -> DIR_Name[8] = 'T';  /* 修改文件扩展名为TXT */
				mFileDir -> DIR_Name[9] = 'X';  /* 同样方法可以修改文件主名 */
				mFileDir -> DIR_Name[10] = 'T';
			}
/* 以下将修改过的内容从内存中真正刷新到U盘中 */
			mCmdParam.Modify.mFileAttr = mFileDir -> DIR_Attr;  /* 准备假修改文件属性,实际保持原值 */
			mCmdParam.Modify.mFileDate = mCmdParam.Modify.mFileTime = 0xFFFF;  /* 不修改文件日期和时间 */
			mCmdParam.Modify.mFileSize = 0xFFFFFFFF;  /* 不修改文件长度 */
			i = CH375FileModify( );  /* 实际是将内存中刚刚修改过的文件名保存 */
			if ( i == ERR_SUCCESS ) {
				/* 文件读写操作等... */
				mCmdParam.Close.mUpdateLen = 0;
				i = CH375FileClose( );  /* 关闭文件 */
			}
		}
	}
	return( i );
}

/* 大端与小端格式的数据处理,文件系统本身是小端,而MCS51程序是大端,所以才需要转换 */
UINT16	SwapUINT16( UINT16 d )
{
	return( ( d << 8 ) & 0xFF00 | ( d >> 8 ) & 0xFF );
}

/* 为指定文件设置创建日期和时间 */
/* 输入参数:   原始文件名在mCmdParam.Open.mPathName中, 新的创建日期和时间: iCreateDate, iCreateTime */
/* 返回状态码: ERR_SUCCESS = 设置成功,
               其它状态码参考CH375HF?.H */
UINT8	SetFileCreateTime( UINT16 iCreateDate, UINT16 iCreateTime )
{
	UINT8			i;
	P_FAT_DIR_INFO	mFileDir;
	i = CH375FileOpen( );  /* 打开文件 */
	if ( i == ERR_SUCCESS ) {
		/* 文件读写操作等... */
		i = CH375FileQuery( );  /* 查询文件属性,以便将相关数据调到内存中再修改 */
		if ( i == ERR_SUCCESS ) {
			mFileDir = (P_FAT_DIR_INFO)( (PUINT8X)(&DISK_BASE_BUF[0]) + CH375vFdtOffset );  /* 在内存中,当前FDT的起始地址 */
//			mFileDir -> DIR_CrtTime = iCreateTime;  /* 文件创建的时间,适用于小端格式 */
			mFileDir -> DIR_CrtTime = SwapUINT16( iCreateTime );  /* MCS51单片机是大端格式 */
//			mFileDir -> DIR_CrtDate = iCreateDate;  /* 文件创建的日期,适用于小端格式 */
			mFileDir -> DIR_CrtDate = SwapUINT16( iCreateDate );  /* MCS51单片机是大端格式 */

//			mFileDir -> DIR_WrtTime = MAKE_FILE_TIME( 时, 分, 秒 );  /* 文件修改时间 */
//			mFileDir -> DIR_LstAccDate = MAKE_FILE_DATE( 年, 月, 日 );  /* 最近一次存取操作的日期 */

/* 以下将修改过的内容从内存中真正刷新到U盘中 */
			mCmdParam.Modify.mFileAttr = mFileDir -> DIR_Attr;  /* 准备假修改文件属性,实际保持原值 */
			mCmdParam.Modify.mFileDate = mCmdParam.Modify.mFileTime = 0xFFFF;  /* 不修改文件日期和时间 */
			mCmdParam.Modify.mFileSize = 0xFFFFFFFF;  /* 不修改文件长度 */
			i = CH375FileModify( );  /* 实际是将内存中刚刚修改过的文件名保存 */
			if ( i == ERR_SUCCESS ) {
				/* 文件读写操作等... */
				mCmdParam.Close.mUpdateLen = 0;
				i = CH375FileClose( );  /* 关闭文件 */
			}
		}
	}
	return( i );
}

main( ) {
	UINT8	i;
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
		while ( CH375DiskStatus < DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘插入 */
			CH375DiskConnect( );
			mDelaymS( 100 );
		}
		LED_OUT_ACT( );  /* LED亮 */
		mDelaymS( 200 );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */

/* 检查U盘是否准备好,有些U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 3; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
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
		LED_RUN_ACT( );  /* 开始操作U盘 */

		printf( "Open and rename CH375HFT.C to CH375HFT.TXT \n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/CH375HFT.C" );  /* 原始文件名,该文件在根目录下 */
		LED_WR_ACT( );  /* 写操作 */
		i = RenameFileName( );  /* 修改文件名, C文件 => TXT文件 */
		mStopIfError( i );
		printf( "Set file create date & time to 2004.06.08 15:39:20 \n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/CH375HFT.TXT" );  /* 原始文件名 */
		i = SetFileCreateTime( MAKE_FILE_DATE( 2004, 6, 8 ), MAKE_FILE_TIME( 15, 39, 20 ) );  /* 为指定文件设置创建日期和时间 */
		mStopIfError( i );
		LED_WR_INACT( );
		LED_RUN_INACT( );
		printf( "Take out\n" );
		while ( CH375DiskStatus >= DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
			CH375DiskConnect( );
			mDelaymS( 100 );
		}
		LED_OUT_INACT( );  /* LED灭 */
		mDelaymS( 200 );
	}
}
