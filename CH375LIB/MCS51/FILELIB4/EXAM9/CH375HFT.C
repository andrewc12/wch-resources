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
/* 本程序用于演示创建子目录 */
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR复制", 所以速度较慢, 适用于所有MCS51单片机
   本例适用于V1.6及以上版本的CH375子程序库 */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF6 */
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
#define FILE_DATA_BUF_ADDR		0x1000	/* 外部RAM的文件数据缓冲区的起始地址,缓冲区长度不小于一次读写的数据长度 */
/* 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以外部RAM剩余长度为32256字节 */
#define FILE_DATA_BUF_LEN		0x3E00	/* 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度,本例要求不小于0x400即可 */
/* 如果准备使用双缓冲区交替读写,那么不要定义FILE_DATA_BUF_LEN,而是在参数中指定缓冲区起址,用CH375FileReadX代替CH375FileRead,用CH375FileWriteX代替CH375FileWrite */

#define CH375_INT_WIRE			INT0	/* P3.2, INT0, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM		1		/* 未调用CH375FileEnumer程序故禁止以节约代码 */
#define NO_DEFAULT_CH375_F_QUERY	1		/* 未调用CH375FileQuery程序故禁止以节约代码 */

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

/* 检查操作状态,如果错误则显示错误代码并停机 */
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

/* 新建目录并打开,如果目录已经存在则直接打开 */
/* 输入参数:   目录名在mCmdParam.Create.mPathName中,与文件名规则相同 */
/* 返回状态码: ERR_SUCCESS = 打开目录成功或者创建目录成功,
               ERR_FOUND_NAME = 已经存在同名文件,
               ERR_MISS_DIR = 路径名无效或者上级目录不存在,
               其它状态码参考CH375HF?.H */
UINT8	CreateDirectory( void )
{
	UINT8	i, j;
	UINT16	count;
	UINT32	UpDirCluster;
	PUINT8X	DirXramBuf;
	UINT8 code *DirConstData;
	j = 0xFF;
	for ( i = 0; i != sizeof( mCmdParam.Create.mPathName ); i ++ ) {  /* 检查目录路径 */
		if ( mCmdParam.Create.mPathName[ i ] == 0 ) break;
		if ( mCmdParam.Create.mPathName[ i ] == PATH_SEPAR_CHAR1 || mCmdParam.Create.mPathName[ i ] == PATH_SEPAR_CHAR2 ) j = i;  /* 记录上级目录 */
	}
	i = ERR_SUCCESS;
	if ( j == 0 || j == 2 && mCmdParam.Create.mPathName[1] == ':' ) UpDirCluster = 0;  /* 在根目录下创建子目录 */
	else {
		if ( j != 0xFF ) {  /* 对于绝对路径应该获取上级目录的起始簇号 */
			mCmdParam.Create.mPathName[ j ] = 0;
			i = CH375FileOpen( );  /* 打开上级目录 */
			if ( i == ERR_SUCCESS ) i = ERR_MISS_DIR;  /* 是文件而非目录 */
			else if ( i == ERR_OPEN_DIR ) i = ERR_SUCCESS;  /* 成功打开上级目录 */
			mCmdParam.Create.mPathName[ j ] = PATH_SEPAR_CHAR1;  /* 恢复目录分隔符 */
		}
		UpDirCluster = CH375vStartCluster;  /* 保存上级目录的起始簇号 */
	}
	if ( i == ERR_SUCCESS ) {  /* 成功获取上级目录的起始簇号 */
		i = CH375FileOpen( );  /* 打开本级子目录 */
		if ( i == ERR_SUCCESS ) i = ERR_FOUND_NAME;  /* 是文件而非目录 */
		else if ( i == ERR_OPEN_DIR ) i = ERR_SUCCESS;  /* 目录已经存在 */
		else if ( i == ERR_MISS_FILE ) {  /* 目录不存在,可以新建 */
			i = CH375FileCreate( );  /* 以创建文件的方法创建目录 */
			if ( i == ERR_SUCCESS ) {
//				if ( &FILE_DATA_BUF[0] == &DISK_BASE_BUF[0] ) CH375DirtyBuffer( );  /* 如果FILE_DATA_BUF与DISK_BASE_BUF合用则必须清除磁盘缓冲区 */
				DirXramBuf = &FILE_DATA_BUF[0];  /* 文件数据缓冲区 */
				DirConstData = ".          \x10\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x21\x30\x0\x0\x0\x0\x0\x0..         \x10\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x21\x30\x0\x0\x0\x0\x0\x0";
				for ( i = 0x40; i != 0; i -- ) {  /* 目录的保留单元,分别指向自身和上级目录 */
					*DirXramBuf = *DirConstData;
					DirXramBuf ++;
					DirConstData ++;
				}
#ifdef __C51__	// 针对MCS51提速
				FILE_DATA_BUF[0x1A] = ( (PUINT8I)&CH375vStartCluster )[3];  /* 自身的起始簇号 */
				FILE_DATA_BUF[0x1B] = ( (PUINT8I)&CH375vStartCluster )[2];
				FILE_DATA_BUF[0x14] = ( (PUINT8I)&CH375vStartCluster )[1];
				FILE_DATA_BUF[0x15] = ( (PUINT8I)&CH375vStartCluster )[0];
				FILE_DATA_BUF[0x20+0x1A] = ( (PUINT8I)&UpDirCluster )[3];  /* 上级目录的起始簇号 */
				FILE_DATA_BUF[0x20+0x1B] = ( (PUINT8I)&UpDirCluster )[2];
				FILE_DATA_BUF[0x20+0x14] = ( (PUINT8I)&UpDirCluster )[1];
				FILE_DATA_BUF[0x20+0x15] = ( (PUINT8I)&UpDirCluster )[0];
#else  // 其它单片机
				FILE_DATA_BUF[0x1A] = (UINT8)CH375vStartCluster;  /* 自身的起始簇号 */
				FILE_DATA_BUF[0x1B] = (UINT8)(CH375vStartCluster>>8);
				FILE_DATA_BUF[0x14] = (UINT8)(CH375vStartCluster>>16);
				FILE_DATA_BUF[0x15] = (UINT8)(CH375vStartCluster>>24);
				FILE_DATA_BUF[0x20+0x1A] = (UINT8)UpDirCluster;  /* 上级目录的起始簇号 */
				FILE_DATA_BUF[0x20+0x1B] = (UINT8)(UpDirCluster>>8);
				FILE_DATA_BUF[0x20+0x14] = (UINT8)(UpDirCluster>>16);
				FILE_DATA_BUF[0x20+0x15] = (UINT8)(UpDirCluster>>24);
#endif
				for ( count = 0x40; count != CH375vSectorSize; count ++ ) {  /* 清空目录区剩余部分 */
					*DirXramBuf = 0;
					DirXramBuf ++;
				}
				mCmdParam.Write.mSectorCount = 1;
				i = CH375FileWrite( );  /* 写目录的第一个扇区 */
				if ( i == ERR_SUCCESS ) {
					DirXramBuf = &FILE_DATA_BUF[0];
					for ( i = 0x40; i != 0; i -- ) {  /* 清空目录区 */
						*DirXramBuf = 0;
						DirXramBuf ++;
					}
					for ( j = 1; j != CH375vSecPerClus; j ++ ) {
//						if ( &FILE_DATA_BUF[0] == &DISK_BASE_BUF[0] ) CH375DirtyBuffer( );  /* 如果FILE_DATA_BUF与DISK_BASE_BUF合用则必须清除磁盘缓冲区 */
						mCmdParam.Write.mSectorCount = 1;
						i = CH375FileWrite( );  /* 清空目录的剩余扇区 */
						if ( i != ERR_SUCCESS ) break;
					}
					if ( j == CH375vSecPerClus ) {  /* 成功清空目录 */
						mCmdParam.Modify.mFileSize = 0;  /* 目录的长度总是0 */
						mCmdParam.Modify.mFileDate = 0xFFFF;
						mCmdParam.Modify.mFileTime = 0xFFFF;
						mCmdParam.Modify.mFileAttr = 0x10;  /* 置目录属性 */
						i = CH375FileModify( );  /* 将文件信息修改为目录 */
					}
				}
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
		while ( CH375DiskStatus != DISK_CONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘插入 */
		LED_OUT_ACT( );  /* LED亮 */
		mDelaymS( 200 );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */

/* 检查U盘是否准备好,但是某些U盘必须要执行这一步才能工作 */
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
		LED_RUN_ACT( );  /* 开始操作U盘 */

		printf( "Create Level 1 Directory /YEAR2004 \n" );
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/YEAR2004" );  /* 目录名,该目录建在根目录下 */
		LED_WR_ACT( );  /* 写操作 */
		i = CreateDirectory( );  /* 新建或者打开目录 */
		mStopIfError( i );
/* 目录新建或者打开成功,下面在这个子目录中新建一个演示文件 */
		printf( "Create New File /YEAR2004/DEMO2004.TXT \n" );
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/YEAR2004/DEMO2004.TXT" );  /* 文件名 */
		i = CH375FileCreate( );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
		printf( "Write some data to file DEMO2004.TXT \n" );
		i = mCopyCodeStringToIRAM( mCmdParam.ByteWrite.mByteBuffer, "演示数据\xd\xa" );
		mCmdParam.ByteWrite.mByteCount = i;  /* 指定本次写入的字节数,单次读写的长度不能超过MAX_BYTE_IO */
		i = CH375ByteWrite( );  /* 以字节为单位向文件写入数据,单次读写的长度不能超过MAX_BYTE_IO */
		mStopIfError( i );
		printf( "Close file DEMO2004.TXT \n" );
		mCmdParam.Close.mUpdateLen = 1;  /* 自动更新文件长度 */
		i = CH375FileClose( );  /* 关闭文件 */
		mStopIfError( i );
/* 下面新建二级子目录,方法与前面的一级子目录完全相同 */
		printf( "Create Level 2 Directory /YEAR2004/MONTH05 \n" );
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/YEAR2004/MONTH05" );  /* 目录名,该目录建在YEAR2004子目录下,YEAR2004目录必须事先存在 */
		i = CreateDirectory( );  /* 新建或者打开目录 */
		mStopIfError( i );
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 0;  /* 对于目录不需要自动更新文件长度 */
		i = CH375FileClose( );  /* 关闭目录,目录不需要关闭,关闭只是为了防止下面误操作 */
		LED_WR_INACT( );
		LED_RUN_INACT( );
		printf( "Take out\n" );
		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
		LED_OUT_INACT( );  /* LED灭 */
		mDelaymS( 200 );
	}
}
