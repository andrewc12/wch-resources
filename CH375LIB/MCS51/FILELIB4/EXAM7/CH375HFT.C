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
/* 本程序用于演示将ADC模数采集的数据保存到U盘文件MY_ADC.TXT中 */
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR复制",兼容性最好但是速度最慢,
   本例适用于V1.5及以上版本的CH375子程序库,以字节为单位读写U盘文件,读写速度较扇区模式慢,
   但是由于字节模式读写文件不需要文件数据缓冲区FILE_DATA_BUF,
   所以总共只需要600字节的RAM,适用于单片机硬件资源有限、数据量小并且读写速度要求不高的系统 */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF4.LIB    如果将CH375HF4换成CH375HF6就可以支持FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>

#define	MAX_BYTE_IO				35		/* 以字节为单位单次读写文件时的最大长度,默认值是29,值大则占用内存多,值小则超过该长度必须分多次读写 */

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
/* 只使用单片机内置的1KB外部RAM: 0000H-01FFH 为磁盘读写缓冲区, 以字节为单位读写文件不需要文件数据读写缓冲区FILE_DATA_BUF */
#define	DISK_BASE_BUF_ADDR		0x0000	/* 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */
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

/* 以毫秒为单位延时,不精确,适用于24MHz时钟 */
void	mDelaymS( UINT8 delay )
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
		mDelaymS( 100 );
		LED_OUT_INACT( );
		mDelaymS( 100 );
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

main( ) {
	UINT8	i, month, hour;
	UINT16	date, adc, len;
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

/* 检查U盘是否准备好,但是某些U盘必须要执行这一步才能工作 */
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
		LED_RUN_ACT( );  /* 开始操作U盘 */

/* 如果MY_ADC.TXT文件已经存在则添加数据到尾部,如果不存在则新建文件 */
		printf( "Open\n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/MY_ADC.TXT" );  /* 文件名,该文件在根目录下 */
		i = CH375FileOpen( );  /* 打开文件 */
		if ( i == ERR_SUCCESS ) {  /* 文件存在并且已经被打开,移动文件指针到尾部以便添加数据 */
			printf( "File size = %ld\n", CH375vFileSize );  /* V1.5以上子程序库在成功打开文件后,全局变量CH375vFileSize中是文件当前长度 */
			printf( "Locate tail\n" );
			mCmdParam.ByteLocate.mByteOffset = 0xffffffff;  /* 移到文件的尾部 */
			i = CH375ByteLocate( );
			mStopIfError( i );
		}
		else if ( i == ERR_MISS_FILE ) {  /* 没有找到文件,必须新建文件 */
			LED_WR_ACT( );  /* 写操作 */
			printf( "Create\n" );
//			mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/MY_ADC.TXT" );  /* 文件名,该文件在根目录下,刚才已经提供给CH375FileOpen */
			i = CH375FileCreate( );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
			mStopIfError( i );
		}
		else mStopIfError( i );  /* 打开文件时出错 */
		LED_WR_ACT( );  /* 写操作 */
		printf( "Write begin\n" );
		i = sprintf( mCmdParam.ByteWrite.mByteBuffer, "此前文件长度= %ld 字节\xd\xa", CH375vFileSize );
		mCmdParam.ByteWrite.mByteCount = i;  /* 指定本次写入的字节数,单次读写的长度不能超过MAX_BYTE_IO */
		i = CH375ByteWrite( );  /* 以字节为单位向文件写入数据,单次读写的长度不能超过MAX_BYTE_IO */
		mStopIfError( i );
		printf( "Write ADC data\n" );
		TR0=1;  /* 用定时器0的计数值代替ADC数据 */
		for ( hour = 8; hour != 20; hour ++  ) {  /* 用循环方式添加12行数据 */
			month = 5;  /* 假定是5月 */
			date = TL1 & 0x1F;  /* 因为测试板上没有实时时钟芯片,所以用定时器1的计数代替进行演示 */
/*			adc = get_adc_data( ); */
			adc = ( (UINT16)TH0 << 8 ) | TL0;  /* 因为测试板上没有ADC,所以用定时器0的计数代替ADC数据演示 */
			len = sprintf( mCmdParam.ByteWrite.mByteBuffer, "%02d.%02d.%02d ADC=%u\xd\xa", (UINT16)month, date, (UINT16)hour, adc );  /* 将二制制数据格式为一行字符串 */
			mCmdParam.ByteWrite.mByteCount = (unsigned char)len;  /* 指定本次写入的字节数,不能超过MAX_BYTE_IO,否则另用缓冲区分多次写入 */
			i = CH375ByteWrite( );  /* 以字节为单位向文件写入数据,单次读写的长度不能超过MAX_BYTE_IO */
/* 有些U盘可能会要求在写数据后等待一会才能继续操作,所以,如果在某些U盘中发生数据丢失现象,建议在每次写入数据后稍作延时再继续 */
			mStopIfError( i );
			printf( "Current offset ( file point ) is %ld\n", CH375vCurrentOffset );
		}
/*		mCmdParam.ByteWrite.mByteCount = 0;  如果全局变量CH375LibConfig的位4为0,可以指定写入0字节,用于刷新文件的长度,
		CH375ByteWrite( );  以字节为单位向文件写入数据,因为是0字节写入,所以只用于更新文件的长度,当阶段性写入数据后,可以用这种办法更新文件长度
        如果全局变量CH375LibConfig的位4为1,则每次写完数据后会自动修改/更新文件长度,从而不必象上面这样写0字节数据: CH375LibConfig |= 0x10; */
		printf( "Write end\n" );
		i = mCopyCodeStringToIRAM( mCmdParam.ByteWrite.mByteBuffer, "今天的ADC数据到此结束\xd\xa" );
		mCmdParam.ByteWrite.mByteCount = i;  /* 指定本次写入的字节数,单次读写的长度不能超过MAX_BYTE_IO */
		i = CH375ByteWrite( );  /* 以字节为单位向文件写入数据,单次读写的长度不能超过MAX_BYTE_IO */
		mStopIfError( i );
/*		printf( "Modify\n" );  如果实际产品中有实时时钟,可以根据需要将文件的日期和时间修改为实际值
		mCmdParam.Modify.mFileAttr = 0xff;   输入参数: 新的文件属性,为0FFH则不修改
		mCmdParam.Modify.mFileTime = MAKE_FILE_TIME( 16, 32, 09 );   输入参数: 新的文件时间: 16:32:09
		mCmdParam.Modify.mFileDate = MAKE_FILE_DATE( 2004, 5, 18 );  输入参数: 新的文件日期: 2004.05.18
		mCmdParam.Modify.mFileSize = 0xffffffff;   输入参数: 新的文件长度,以字节为单位写文件应该由程序库关闭文件时自动更新长度,所以此处不修改
		i = CH375FileModify( );   修改当前文件的信息,修改日期
		mStopIfError( i );
*/
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 1;  /* 自动计算文件长度,以字节为单位写文件,建议让程序库关闭文件以便自动更新文件长度 */
		i = CH375FileClose( );  /* 关闭文件 */
		mStopIfError( i );
		LED_WR_INACT( );
		LED_RUN_INACT( );
		printf( "Take out\n" );
//		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
		while ( CH375DiskStatus >= DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
			CH375DiskConnect( );
			mDelaymS( 100 );
		}
		LED_OUT_INACT( );  /* LED灭 */
		mDelaymS( 200 );
	}
}
