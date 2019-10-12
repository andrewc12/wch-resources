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
/* 本程序用于演示将ADC模数采集的数据保存到U盘文件MY_ADC.TXT中 */
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR和P2+R0复制",速度最快,但是不能用于带有内置XRAM的单片机,
   本例适用于V1.6及以上版本的CH375子程序库,以扇区为单位读写U盘文件,读写速度较字节模式快,
   由于扇区模式以扇区为基本单位,对于需要处理零碎数据的应用,不如字节模式方便,
   本范例演示在扇区模式下处理零碎数据,同时兼顾操作方便和较高速度,需要文件数据缓冲区FILE_DATA_BUF */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF4.LIB    如果将CH375HF4换成CH375HF6就可以支持FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>
#include <string.h>

/* 以下定义的详细说明请看CH375HF6.H文件 */
#define LIB_CFG_DISK_IO			3		/* 磁盘读写的数据的复制方式,1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_FILE_IO			3		/* 文件读写的数据的复制方式,0为"外部子程序",1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
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
/* 如果需要复用磁盘数据缓冲区以节约RAM,那么可将DISK_BASE_BUF_LEN定义为0以禁止在.H文件中定义缓冲区,而由应用程序在调用CH375Init之前将与其它程序合用的缓冲区起始地址置入pDISK_BASE_BUF变量 */
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

UINT16	total;	/* 记录当前缓冲在FILE_DATA_BUF中的数据长度 */

/* 将准备写入U盘的零碎数据进行集中缓冲,组合成大数据块时再通过CH375真正写入U盘 */
/* 这样做的好处是: 提高速度(因为大数据块写入时效率高), 减少U盘损耗(U盘内部的内存寿命有限,不宜频繁擦写) */
void	mFlushBufferToDisk( UINT8 force )
/* force = 0 则自动刷新(检查缓冲区中的数据长度,满则写盘,不满则暂时放在缓冲区中), force != 0 则强制刷新(不管缓冲区中的数据有多少都写盘,通常在系统关机前应该强制写盘) */
{
	UINT8	i;
	UINT32	NewSize;
	if ( force ) {  /* 强制刷新 */
		mCmdParam.Write.mSectorCount = ( total + CH375vSectorSize - 1 ) / CH375vSectorSize;  /* 将缓冲区中的字节数转换为扇区数(除以CH375vSectorSize),长度加上CH375vSectorSize-1用于确保写入最后的零头数据 */
		i = CH375FileWrite( );  /* 以扇区为单位向文件写入数据,写入缓冲区中的所有数据,含最后的零头 */
		mStopIfError( i );
/* 有些U盘可能会要求在写数据后等待一会才能继续操作,所以,如果在某些U盘中发生数据丢失现象,建议在每次写入数据后稍作延时再继续 */
		mDelaymS( 1 );  /* 写后延时,可选的,大多数U盘不需要 */
		memcpy( FILE_DATA_BUF+0, FILE_DATA_BUF+(total & ~ ( CH375vSectorSize - 1 ) ), total & ( CH375vSectorSize - 1 ) );  /* 将刚才已写入U盘的零头数据复制到缓冲区的头部 */
		total &= CH375vSectorSize - 1;  /* 缓冲区中只剩下刚才已写入U盘的零头数据,继续保留在缓冲区中是为了方便以后在其后面追加数据 */
		if ( total ) NewSize = CH375vFileSize - CH375vSectorSize + total;  /* 以扇区为单位,有零头数据,计算出真正的文件长度(有效数据的长度) */
		else NewSize = CH375vFileSize;  /* 以扇区为单位,没有零头数据,文件长度是CH375vSectorSize的倍数 */
		mCmdParam.Modify.mFileSize = NewSize;   /* 输入参数: 新的文件长度,扇区模式下涉及到零头数据不便自动更新长度 */
		mCmdParam.Modify.mFileAttr = 0xff;  /* 输入参数: 新的文件属性,为0FFH则不修改 */
		mCmdParam.Modify.mFileTime = 0xffff;  /* 输入参数: 新的文件时间,为0FFH则不修改 */
		mCmdParam.Modify.mFileDate = 0xffff;  /* 输入参数: 新的文件日期,为0FFH则不修改 */
		i = CH375FileModify( );   /* 修改当前文件的信息,修改文件长度 */
		mStopIfError( i );
		printf( "Current file size is %ld\n", CH375vFileSize );
		mCmdParam.Locate.mSectorOffset = 0xffffffff;  /* 移到文件的尾部,以扇区为单位,所以会忽略文件尾部的零头数据 */
		i = CH375FileLocate( );  /* 重新回到原文件的尾部,下面如果再写入数据将覆盖尾部零头数据,不过该零头数据有一份副本保留在缓冲区中,所以请放心 */
		mStopIfError( i );
	}
	else if ( total >= FILE_DATA_BUF_LEN - CH375vSectorSize ) {  /* 缓冲区中的数据快要满了,所以应该先将缓冲区中的原有数据写入U盘 */
		mCmdParam.Write.mSectorCount = total / CH375vSectorSize;  /* 将缓冲区中的字节数转换为扇区数(除以CH375vSectorSize),最后的零头数据先不管 */
/* 使用CH375FileWriteX()代替CH375FileWrite()可以自行定义数据缓冲区的起始地址,例如
		mCmdParam.WriteX.mDataBuffer = 0x4600;  将4600H开始的缓冲区中的数据写入
		i = CH375FileWriteX();   将指定缓冲区4600H中的数据写入到文件中 */
		i = CH375FileWrite( );  /* 以扇区为单位向文件写入数据,写入缓冲区中的所有数据,不含最后的零头 */
		mStopIfError( i );
		memcpy( FILE_DATA_BUF+0, FILE_DATA_BUF+(total & ~ ( CH375vSectorSize - 1 ) ), total & ( CH375vSectorSize - 1 ) );  /* 将刚才未写入U盘的零头数据复制到缓冲区的头部 */
		total &= CH375vSectorSize - 1;  /* 缓冲区中只剩下刚才未写入U盘的零头数据 */
/*		mCmdParam.Write.mSectorCount = 0;  如果全局变量CH375LibConfig的位4为0,可以指定写入0扇区,用于刷新文件的长度
		CH375FileWrite( );  以扇区为单位向文件写入数据,因为是0扇区写入,所以只用于更新文件的长度,当阶段性写入数据后,可以用这种办法更新文件长度 */
	}
}

main( ) {
	UINT8	i, month, date, hour;
	UINT16	year, adc;
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

/* 检查U盘是否准备好,某些U盘必须要执行这一步才能工作 */
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
		printf( "DiskSize\n" );
		i = CH375DiskSize( );  
		mStopIfError( i );
		printf( "TotalSize = %u MB \n", (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * (CH375vSectorSize/512) / 2048 ) );  // 显示为以MB为单位的容量
		// 原计算方法 (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * CH375vSectorSize / 1000000 ) 有可能前两个数据相乘后导致溢出, 所以修改成上式
		LED_RUN_ACT( );  /* 开始操作U盘 */

/* 如果MY_ADC.TXT文件已经存在则添加数据到尾部,如果不存在则新建文件 */
		printf( "Open\n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/MY_ADC.TXT" );  /* 文件名,该文件在根目录下 */
		i = CH375FileOpen( );  /* 打开文件 */
		if ( i == ERR_SUCCESS ) {  /* 文件存在并且已经被打开,移动文件指针到尾部以便添加数据 */
			printf( "File size = %ld\n", CH375vFileSize );  /* V1.5以上子程序库在成功打开文件后,全局变量CH375vFileSize中是文件当前长度 */
			printf( "Locate tail\n" );
			mCmdParam.Locate.mSectorOffset = 0xffffffff;  /* 移到文件的尾部,CH375子程序库内部是将文件长度按扇区长度CH375vSectorSize进行取整处理 */
			i = CH375FileLocate( );  /* 以扇区为单位移到文件尾部,如果文件尾部有不足一个扇区的零头数据则被忽略,如果不做处理那么零头数据将可能被写入数据覆盖 */
			mStopIfError( i );
			total = CH375vFileSize & ( CH375vSectorSize - 1 );  /* 上次保存文件时如果尾部有零头数据,那么先取得零头字节数,不满扇区长度的零碎数据 */
			printf( "Read last tail = %d Bytes\n", total );
			CH375vFileSize += CH375vSectorSize - 1;  /* 人为地将文件长度增加一个扇区减1,以便读出最后一个扇区中的零头数据 */
			mCmdParam.Read.mSectorCount = 1;  /* 读取文件尾部的零头数据,如果不人为增加文件长度,那么由于文件长度按CH375vSectorSize取整,导致尾部零头数据无法读出 */
/* 使用CH375FileReadX()代替CH375FileRead()可以自行定义数据缓冲区的起始地址,例如
			mCmdParam.ReadX.mDataBuffer = 0x2C00;  将读出的数据放到2C00H开始的缓冲区中
			i = CH375FileReadX();   从文件中读取扇区到指定缓冲区,起始地址为2C00H */
			i = CH375FileRead( );  /* 从文件读取尾部零头数据,如果原尾部没有零头数据,那么什么也读不到,返回时mCmdParam.Read.mSectorCount为实际读出扇区数 */
			mStopIfError( i );
			CH375vFileSize -= CH375vSectorSize - 1;  /* 恢复真正的文件长度 */
			mCmdParam.Locate.mSectorOffset = 0xffffffff;  /* 移到文件的尾部,以扇区为单位,所以会忽略文件尾部的零头数据 */
			i = CH375FileLocate( );  /* 重新回到原文件的尾部,下面如果写入数据将覆盖原尾部零头数据,不过原零头数据刚才已经被读入内存,所以请放心 */
			mStopIfError( i );
		}
		else if ( i == ERR_MISS_FILE ) {  /* 没有找到文件,必须新建文件 */
			LED_WR_ACT( );  /* 写操作 */
			printf( "Create\n" );
//			mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/MY_ADC.TXT" );  /* 文件名,该文件在根目录下,刚才已经提供给CH375FileOpen */
			i = CH375FileCreate( );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
			mStopIfError( i );
			total = 0;  /* 此前没有零头数据 */
		}
		else mStopIfError( i );  /* 打开文件时出错 */
		LED_WR_ACT( );  /* 写操作 */
		printf( "Write begin\n" );
		total += sprintf( FILE_DATA_BUF + total, "在本次添加数据之前,该文件已有数据的长度是 %ld 字节\xd\xa", CH375vFileSize );  /* 将新数据添加到缓冲区的尾部,累计缓冲区内的数据长度 */
		mFlushBufferToDisk( 0 );  /* 自动刷新缓冲区,检查缓冲区是否已满,满则写盘 */
		printf( "Write ADC data\n" );
		TR0=1;  /* 用定时器0的计数值代替ADC数据 */
		for ( month = 1; month != 12; month ++ ) {  /* 因为测试板上没有实时时钟芯片,所以用循环方式模拟月份 */
			for ( date = 1; date != 30; date ++ ) {  /* 因为测试板上没有实时时钟芯片,所以用循环方式模拟日期 */
				year = 2004;  /* 假定为2004年 */
				hour = TL1 & 0x1F;  /* 因为测试板上没有实时时钟芯片,所以用定时器1的计数代替进行演示 */
/*				adc = get_adc_data( ); */
				adc = ( (UINT16)TH0 << 8 ) | TL0;  /* 因为测试板上没有ADC,所以用定时器0的计数代替ADC数据演示 */
				total += sprintf( FILE_DATA_BUF + total, "Year=%04d, Month=%02d, Date=%02d, Hour=%02d, ADC_data=%u\xd\xa", year, (UINT16)month, (UINT16)date, (UINT16)hour, adc );  /* 将二制制数据格式为一行字符串 */
				if ( month == 6 && ( date & 0x0F ) == 0 ) mFlushBufferToDisk( 1 );  /* 强制刷新缓冲区,定期强制刷新缓冲区,这样在突然断电后可以减少数据丢失 */
				else mFlushBufferToDisk( 0 );  /* 自动刷新缓冲区,检查缓冲区是否已满,满则写盘 */
				printf( "Current total is %d\n", total );  /* 用于监控检查 */
			}
		}
		printf( "Write end\n" );
		total += sprintf( FILE_DATA_BUF + total, " ********************************* " );  /* 将新数据添加到缓冲区的尾部,累计缓冲区内的数据长度 */
		total += sprintf( FILE_DATA_BUF + total, "这次的ADC数据到此结束,程序即将退出\xd\xa" );  /* 将新数据添加到缓冲区的尾部,累计缓冲区内的数据长度 */
		mFlushBufferToDisk( 1 );  /* 强制刷新缓冲区,因为系统要退出了,所以必须强制刷新 */
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 0;  /* 因为强制刷新缓冲区时已经更新了文件长度,所以这里不需要自动更新文件长度 */
		i = CH375FileClose( );  /* 关闭文件 */
		mStopIfError( i );
		LED_WR_INACT( );
		LED_RUN_INACT( );
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
