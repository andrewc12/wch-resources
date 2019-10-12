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
/* 该程序将U盘中的/C51/CH375HFT.C文件中的小写字母转成大写字母后, 写到新建的文件NEWFILE.TXT中,
   如果找不到原文件CH375HFT.C, 那么该程序将显示C51子目录下所有以CH375开头的文件名, 并新建NEWFILE.TXT文件并写入提示信息,
   如果找不到C51子目录, 那么该程序将显示根目录下的所有文件名, 并新建NEWFILE.TXT文件并写入提示信息
*/
/* CH375的INT#引脚采用中断方式处理, 数据复制方式为"单DPTR和P2+R0复制", 所以速度最快, 适用于标准MCS51单片机, 不使用内置的外部RAM */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF6.LIB    如果将CH375HF6换成CH375HF4就可以不支持FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>

/* 以下定义的详细说明请看CH375HF6.H文件 */
#define LIB_CFG_DISK_IO			3		/* 磁盘读写的数据的复制方式,1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_FILE_IO			3		/* 文件读写的数据的复制方式,0为"外部子程序",1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_INT_EN			1		/* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375命令端口的I/O地址 */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375数据端口的I/O地址 */
/* 62256提供的32KB的RAM分为两部分: 0000H-7DFFH为文件读写缓冲区, 7E00H-7FFFH为磁盘数据缓冲区 */
#define	DISK_BASE_BUF_ADDR		0x7000	/* 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */
#define DISK_BASE_BUF_LEN		4096	/* 默认的磁盘数据缓冲区大小为512字节,建议选择为2048甚至4096以支持某些大扇区的U盘,为0则禁止在.H文件中定义缓冲区并由应用程序在pDISK_BASE_BUF中指定 */
#define FILE_DATA_BUF_ADDR		0x0200	/* 外部RAM的文件数据缓冲区的起始地址,缓冲区长度不小于一次读写的数据长度 */
/* 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以外部RAM剩余长度为32256字节 */
#define FILE_DATA_BUF_LEN		0x6800	/* 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */
/* 如果准备使用双缓冲区交替读写,那么不要定义FILE_DATA_BUF_LEN,而是在参数中指定缓冲区起址,用CH375FileReadX代替CH375FileRead,用CH375FileWriteX代替CH375FileWrite */

#define CH375_INT_NO			0		/* CH375中断号, CH375的中断线INT#引脚连接单片机的INT0引脚 */
#define CH375_INT_FLAG			IE0		/* IE0,CH375中断标志 */
#define CH375_INT_EN			EX0		/* EX0,CH375中断允许 */

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
	UINT8	i, c, SecCount;
	UINT16	NewSize, count;  /* 因为演示板的RAM容量只有32KB,所以NewSize限制为16位,实际上如果文件大于32256字节,应该分几次读写并且将NewSize改为UINT32以便累计 */
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

	CH375_INT_FLAG = 0;  /* 清中断标志 */
	i = CH375LibInit( );  /* 初始化CH375程序库和CH375芯片,操作成功返回0 */
	mStopIfError( i );
	CH375_INT_EN = 1;  /* 允许CH375中断 */
/* 其它电路初始化 */
	EA = 1;  /* 初始化完成,开中断 */

	while ( 1 ) {
		printf( "Wait Udisk\n" );
		while ( CH375DiskStatus != DISK_CONNECT );  /* 等待U盘插入,单片机可以做其它事情 */
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
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C" );  /* 文件名,该文件在C51子目录下 */
		i = CH375FileOpen( );  /* 打开文件 */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* 没有找到文件 */
/* 列出文件 */
			if ( i == ERR_MISS_DIR ) pCodeStr = "\\*";  /* C51子目录不存在则列出根目录下的文件 */
			else pCodeStr = "\\C51\\CH375*";  /* CH375HFT.C文件不存在则列出\C51子目录下的以CH375开头的文件 */
			printf( "List file %s\n", pCodeStr );
			for ( c = 0; c < 254; c ++ ) {  /* 最多搜索前254个文件,超过254需参考EXAM0使用CH375vFileSize */
				i = mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, pCodeStr );  /* 搜索文件名,*为通配符,适用于所有文件或者子目录 */
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
				SecCount = ( CH375vFileSize + CH375vSectorSize - 1 ) / CH375vSectorSize;  /* 计算文件的扇区数,因为读写是以扇区为单位的,先加CH375vSectorSize-1是为了读出文件尾部不足1个扇区的部分 */
				NewSize = (UINT16)CH375vFileSize;  /* 原文件的长度 */
			}
			printf( "Size=%ld, Len=%d, Sec=%d\n", CH375vFileSize, NewSize, (UINT16)SecCount );
			mCmdParam.Read.mSectorCount = SecCount;  /* 读取全部数据,如果超过60个扇区则只读取60个扇区 */
/*			current_buffer = & FILE_DATA_BUF[0];  如果文件读写的数据的复制方式为"外部子程序",那么需要设置存放数据的缓冲区的起始地址 */
			CH375vFileSize += CH375vSectorSize - 1;  /* 默认情况下,以扇区方式读取数据时,无法读出文件尾部不足1个扇区的部分,所以必须临时加大文件长度以读取尾部零头 */
			i = CH375FileRead( );  /* 从文件读取数据 */
			CH375vFileSize -= CH375vSectorSize - 1;  /* 恢复原文件长度 */
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

或者自行定义数据缓冲区的起始地址
		mCmdParam.ReadX.mSectorCount = 2;
		mCmdParam.ReadX.mDataBuffer = 0x2000;  将读出的数据放到2000H开始的缓冲区中
		CH375FileReadX();   从文件中读取2个扇区到指定缓冲区

	    如果希望将新数据添加到原文件的尾部,可以移动文件指针
		i = CH375FileOpen( );
		mCmdParam.Locate.mSectorOffset = 0xffffffff;  移到文件的尾部,以扇区为单位,如果原文件是3字节,则从CH375vSectorSize字节开始添加
		i = CH375FileLocate( );
		mCmdParam.Write.mSectorCount = 10;
		CH375FileWrite();   在原文件的后面添加数据

或者自行定义数据缓冲区的起始地址
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
		LED_WR_ACT( );  /* 写操作 */
		printf( "Create\n" );
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "\\NEWFILE.TXT" );  /* 新文件名,在根目录下 */
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
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "\\OLD" );  将被删除的文件名,在根目录下
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
		while ( CH375DiskStatus != DISK_DISCONNECT );  /* 等待U盘拔出,单片机可以做其它事情 */
		LED_OUT_INACT( );  /* LED灭 */
		mDelay100mS( );
		mDelay100mS( );
	}
}
