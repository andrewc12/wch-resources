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

/* MCS-51单片机C语言的U盘文件读写示例程序, 适用于89C52或者更大程序空间的单片机,也适用于ATMEL/PHILIPS/SST等具有1KB内部RAM的单片机 */
/* 该程序将U盘中的/C51/CH375HFT.C文件中的前600个字符显示出来,
   如果找不到原文件CH375HFT.C, 那么该程序将显示C51子目录下所有以CH375开头的文件名,
   如果找不到C51子目录, 那么该程序将显示根目录下的所有文件名,
   最后将程序ROM中的一个字符串写入写入新建的文件"NEWFILE.TXT"中
*/
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR复制",兼容性最好但是速度最慢,
   本例只使用512字节的外部RAM, 同时作为磁盘数据缓冲区和文件数据缓冲区, 演示没有外部RAM但是单片机的内置RAM大于768字节的应用 */

/* 本例以字节为单位读写U盘文件,读写速度较扇区模式慢,但是由于字节模式读写文件不需要文件数据缓冲区FILE_DATA_BUF,
   所以总共只需要600字节的RAM,适用于单片机硬件资源有限、数据量小并且读写速度要求不高的系统 */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF6.LIB    如果将CH375HF6换成CH375HF4就禁止支持FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>

/* 以下定义的详细说明请看CH375HF6.H文件 */
#define LIB_CFG_DISK_IO			1		/* 磁盘读写的数据的复制方式,1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_FILE_IO			1		/* 文件读写的数据的复制方式,0为"外部子程序",1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_INT_EN			0		/* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */
/*#define LIB_CFG_FILE_IO_DEFAULT	1*/		/* 使用CH375HF6.H提供的默认"外部子程序" */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375命令端口的I/O地址 */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375数据端口的I/O地址 */
/* 只使用单片机内置的1KB外部RAM: 0000H-01FFH 为磁盘读写缓冲区, 以字节为单位读写文件不需要文件数据读写缓冲区FILE_DATA_BUF */
#define	DISK_BASE_BUF_ADDR		0x0000	/* 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */
#define DISK_BASE_BUF_LEN		4096	/* 默认的磁盘数据缓冲区大小为512字节,建议选择为2048甚至4096以支持某些大扇区的U盘,为0则禁止在.H文件中定义缓冲区并由应用程序在pDISK_BASE_BUF中指定 */
/* 如果需要复用磁盘数据缓冲区以节约RAM,那么可将DISK_BASE_BUF_LEN定义为0以禁止在.H文件中定义缓冲区,而由应用程序在调用CH375Init之前将与其它程序合用的缓冲区起始地址置入pDISK_BASE_BUF变量 */
#define FILE_DATA_BUF_ADDR		0x0000	/* 外部RAM的文件数据缓冲区的起始地址,缓冲区长度不小于一次读写的数据长度,字节模式不用该缓冲区 */
/* 由于单片机内置的外部RAM只有1KB, 有些单片机还要去掉256字节内部RAM, 只剩下768字节的外部RAM, 其中前512字节由CH375子程序用于磁盘数据缓冲 */
// #define FILE_DATA_BUF_LEN		0x0200	/* 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度,字节模式不用该缓冲区 */
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

/* 延时100毫秒,不精确 */
void	mDelay100mS( )
{
	UINT8	i, j, c;
	for ( i = 200; i != 0; i -- ) for ( j = 200; j != 0; j -- ) c+=3;
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
	UINT8	i, c;
	UINT16	TotalCount;
	UINT8	code *pCodeStr;
	LED_OUT_INIT( );
	LED_OUT_ACT( );  /* 开机后LED亮一下以示工作 */
	mDelay100mS( );  /* 延时100毫秒 */
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
//		while ( CH375DiskStatus < DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘插入 */
//			if ( CH375DiskConnect( ) == ERR_SUCCESS ) break;  /* 有设备连接则返回成功,CH375DiskConnect同时会更新全局变量CH375DiskStatus */
//			mDelay100mS( );
//		}
		LED_OUT_ACT( );  /* LED亮 */
		mDelay100mS( );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */
		mDelay100mS( );

/* 检查U盘是否准备好,有些U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 5; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
			mDelay100mS( );
			printf( "Ready ?\n" );
			if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
		}
#if DISK_BASE_BUF_LEN
		if ( DISK_BASE_BUF_LEN < CH375vSectorSize ) {  /* 检查磁盘数据缓冲区是否足够大,CH375vSectorSize是U盘的实际扇区大小 */
			printf( "Too large sector size\n" );
			while ( CH375DiskConnect( ) == ERR_SUCCESS ) mDelay100mS( );
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

/* 读取原文件 */
		printf( "Open\n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/C51/CH375HFT.C" );  /* 文件名,该文件在C51子目录下 */
		i = CH375FileOpen( );  /* 打开文件 */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* 没有找到文件 */
/* 列出文件 */
			if ( i == ERR_MISS_DIR ) pCodeStr = "/*";  /* C51子目录不存在则列出根目录下的文件 */
			else pCodeStr = "/C51/CH375*";  /* CH375HFT.C文件不存在则列出\C51子目录下的以CH375开头的文件 */
			printf( "List file %s\n", pCodeStr );
			for ( c = 0; c < 254; c ++ ) {  /* 最多搜索前254个文件 */
				i = mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, pCodeStr );  /* 搜索文件名,*为通配符,适用于所有文件或者子目录 */
/* 另外一种枚举方法是，将此处的c置为0xFF，然后将真正的枚举序号存放于CH375vFileSize中，从而可以搜索大于254个文件，从0到2147483647个 */
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
				for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );  /* 显示读出的字符 */
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
		LED_WR_ACT( );  /* 写操作 */
		printf( "Create\n" );
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/NEWFILE.TXT" );  /* 新文件名,在根目录下,中文文件名 */
		i = CH375FileCreate( );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
		printf( "Write\n" );
		pCodeStr = "Note: \xd\xa这个程序是以字节为单位进行U盘文件读写,单片机只需要有600字节的RAM\xd\xa";
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
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/OLD" );  将被删除的文件名,在根目录下
		i = CH375FileErase( );  删除文件并关闭
		if ( i != ERR_SUCCESS ) printf( "Error: %02X\n", (UINT16)i );  显示错误
*/
		LED_WR_INACT( );

/* 查询磁盘信息 */
/*		printf( "Disk\n" );
		i = CH375DiskQuery( );
		mStopIfError( i );
		printf( "Fat=%d, Total=%ld, Free=%ld\n", (UINT16)mCmdParam.Query.mDiskFat, mCmdParam.Query.mTotalSector, mCmdParam.Query.mFreeSector );
*/
#endif
		LED_RUN_INACT( );
		printf( "Take out\n" );
		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
//		while ( CH375DiskStatus >= DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
//			if ( CH375DiskConnect( ) != ERR_SUCCESS ) break;
//			mDelay100mS( );
//		}
		LED_OUT_INACT( );  /* LED灭 */
		mDelay100mS( );
		mDelay100mS( );
	}
}
