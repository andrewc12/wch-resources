/* 该程序用180行C代码就能够读取FAT16文件系统U盘的根目录,可以看到根目录下的文件名,并可显示首文件内容 */
/* 注意,该程序很不严谨,也没有任何错误处理,对U盘兼容性较差,只是用于简单试验,作为参考 */
/* 单片机读写U盘的程序分为4层: 硬件USB接口层, BulkOnly传输协议层, RBC/SCSI命令层, FAT文件系统层 */

/* 这个程序可以支持WINDOWS按FAT16格式化的U盘,因为程序精简,所以只兼容超过50%以上的U盘品牌,如果换
成CH375A芯片则兼容性可提高到85%,当然,如果使用公司的子程序库或者正式版本的C源程序兼容性更好。
测试以下U盘通过:郎科/超稳经典64M/超稳迷你128M/U160-64M/超稳普及128M,爱国者/迷你王16M/邮箱型,
黑匣子/64M,微闪/64M,飙王/32M/64M/128M,晶彩/C200-64M,新科/256M,昂达/128M...,欢迎提供测试结果
未通过U盘:爱国者/智慧棒128M,清华普天/USB2.0-128M,当然,使用子程序库或CH375A都可以测试通过 */

#include <stdio.h>
#include "CH375INC.H"		/* 定义CH375命令代码及返回状态 */
#include <reg51.h>			/* 以下定义适用于MCS-51单片机,其它单片机参照修改 */
#define	UINT8		unsigned char
#define	UINT16		unsigned short
#define	UINT32		unsigned long
#define	UINT8X		unsigned char xdata
#define	UINT8VX		unsigned char volatile xdata
UINT8VX		CH375_CMD_PORT _at_ 0xBDF1;	/* CH375命令端口的I/O地址 */
UINT8VX		CH375_DAT_PORT _at_ 0xBCF0;	/* CH375数据端口的I/O地址 */
#define		CH375_INT_WIRE		INT0	/* P3.2, 连接CH375的INT#引脚,用于查询中断状态 */
UINT8X		DISK_BUFFER[512*32] _at_ 0x0000;	/* 外部RAM数据缓冲区的起始地址,长度不少于一次读写的数据长度 */

UINT32	DiskStart;		/* 逻辑盘的起始绝对扇区号LBA */
UINT8	SecPerClus;		/* 逻辑盘的每簇扇区数 */
UINT8	RsvdSecCnt;		/* 逻辑盘的保留扇区数 */
UINT16	FATSz16;		/* FAT16逻辑盘的FAT表占用的扇区数 */

/* ********** 硬件USB接口层,无论如何这层省不掉,单片机总要与CH375接口吧 ************************************************************ */

void	mDelaymS( UINT8 delay ) {		/* 以毫秒为单位延时,不精确,适用于24MHz时钟MCS51 */
	UINT8	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
	}
}

void CH375_WR_CMD_PORT( UINT8 cmd ) {	/* 向CH375的命令端口写入命令,周期不小于4uS,如果单片机较快则延时 */
	CH375_CMD_PORT=cmd;
	for ( cmd = 2; cmd != 0; cmd -- );	/* 发出命令码前后应该各延时2uS,对于MCS51可以不需要延时 */
}
void CH375_WR_DAT_PORT( UINT8 dat ) {	/* 向CH375的数据端口写入数据,周期不小于1.5uS,如果单片机较快则延时 */
	CH375_DAT_PORT=dat;					/* 因为MCS51单片机较慢所以实际上无需延时 */
}
UINT8 CH375_RD_DAT_PORT( void ) {		/* 从CH375的数据端口读出数据,周期不小于1.5uS,如果单片机较快则延时 */
	return( CH375_DAT_PORT );			/* 因为MCS51单片机较慢所以实际上无需延时 */
}
UINT8 mWaitInterrupt( void ) {	/* 等待CH375中断并获取状态,主机端等待操作完成,返回操作状态 */
	while( CH375_INT_WIRE );  /* 查询等待CH375操作完成中断(INT#低电平) */
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 产生操作完成中断,获取中断状态 */
	return( CH375_RD_DAT_PORT( ) );
}

/* ********** BulkOnly传输协议层,被CH375内置了,无需编写单片机程序 ************************************************************ */

/* ********** RBC/SCSI命令层,虽然被CH375内置了,但是要写程序发出命令及收发数据 ************************************************************ */

UINT8	mInitDisk( void ) {	/* 初始化磁盘 */
	UINT8 Status;
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 产生操作完成中断, 获取中断状态 */
	Status = CH375_RD_DAT_PORT( );
	if ( Status == USB_INT_DISCONNECT ) return( Status );  /* USB设备断开 */
	CH375_WR_CMD_PORT( CMD_DISK_INIT );  /* 初始化USB存储器 */
	Status = mWaitInterrupt( );  /* 等待中断并获取状态 */
	if ( Status != USB_INT_SUCCESS ) return( Status );  /* 出现错误 */
	CH375_WR_CMD_PORT( CMD_DISK_SIZE );  /* 获取USB存储器的容量 */
	Status = mWaitInterrupt( );  /* 等待中断并获取状态 */
	if ( Status != USB_INT_SUCCESS ) {  /* 出错重试 */
/* 对于CH375A芯片,建议在此执行一次CMD_DISK_R_SENSE命令 */
		mDelaymS( 250 );
		CH375_WR_CMD_PORT( CMD_DISK_SIZE );  /* 获取USB存储器的容量 */
		Status = mWaitInterrupt( );  /* 等待中断并获取状态 */
	}
	if ( Status != USB_INT_SUCCESS ) return( Status );  /* 出现错误 */
	return( 0 );  /* U盘已经成功初始化 */
}

UINT8	mReadSector( UINT32 iLbaStart, UINT8 iSectorCount, UINT8X *oDataBuffer ) {		/* 从U盘读取数据块到缓冲区 */
/* iLbaStart 起始扇区号, iSectorCount 扇区数, oDataBuffer 缓冲区起址 */
	UINT16	mBlockCount;
	UINT8	c;
	CH375_WR_CMD_PORT( CMD_DISK_READ );  /* 从USB存储器读数据块 */
	CH375_WR_DAT_PORT( (UINT8)iLbaStart );  /* LBA的最低8位 */
	CH375_WR_DAT_PORT( (UINT8)( iLbaStart >> 8 ) );
	CH375_WR_DAT_PORT( (UINT8)( iLbaStart >> 16 ) );
	CH375_WR_DAT_PORT( (UINT8)( iLbaStart >> 24 ) );  /* LBA的最高8位 */
	CH375_WR_DAT_PORT( iSectorCount );  /* 扇区数 */
	for ( mBlockCount = iSectorCount * 8; mBlockCount != 0; mBlockCount -- ) {  /* 数据块计数 */
		c = mWaitInterrupt( );  /* 等待中断并获取状态 */
		if ( c == USB_INT_DISK_READ ) {  /* 等待中断并获取状态,USB存储器读数据块,请求数据读出 */
			CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* 从CH375缓冲区读取数据块 */
			c = CH375_RD_DAT_PORT( );  /* 后续数据的长度 */
			while ( c -- ) *oDataBuffer++ = CH375_RD_DAT_PORT( );  /* 根据长度读取数据并保存 */
			CH375_WR_CMD_PORT( CMD_DISK_RD_GO );  /* 继续执行USB存储器的读操作 */
		}
		else break;  /* 返回错误状态 */
	}
	if ( mBlockCount == 0 ) {
		c = mWaitInterrupt( );  /* 等待中断并获取状态 */
		if ( c== USB_INT_SUCCESS ) return( 0 );  /* 操作成功 */
	}
	return( c );  /* 操作失败 */
}

/* ********** FAT文件系统层,这层程序量实际较大,不过,该程序仅演示极简单的功能,所以精简 ************************************************************ */

UINT16	mGetPointWord( UINT8X *iAddr ) {	/* 获取字数据,因为MCS51是大端格式,U盘FAT通常是小端格式,所以转换 */
	return( iAddr[0] | (UINT16)iAddr[1] << 8 );
}

UINT8	mIdenDisk( void ) {		/* 识别分析当前逻辑盘 */
	UINT8	Status;
	DiskStart = 0;  /* 以下是非常简单的FAT文件系统的分析,正式应用绝对不应该如此简单,否则兼容性和容错性差 */
	Status = mReadSector( 0, 1, DISK_BUFFER );  /* 读取逻辑盘引导信息 */
	if ( Status != 0 ) return( Status );
	if ( DISK_BUFFER[0] != 0xEB && DISK_BUFFER[0] != 0xE9 ) {  /* 不是逻辑引导扇区 */
		DiskStart = DISK_BUFFER[0x1C6] | (UINT16)DISK_BUFFER[0x1C7] << 8 | (UINT32)DISK_BUFFER[0x1C8] << 16 | (UINT32)DISK_BUFFER[0x1C9] << 24;
		Status = mReadSector( DiskStart, 1, DISK_BUFFER );  /* 根据新的起始扇区号读取逻辑盘引导信息 */
		if ( Status != 0 ) return( Status );
	}
	SecPerClus = DISK_BUFFER[0x0D];  /* 每簇扇区数 */
	RsvdSecCnt = DISK_BUFFER[0x0E];  /* 逻辑盘的保留扇区数 */
	FATSz16 = mGetPointWord( &DISK_BUFFER[0x16] );  /* FAT表占用扇区数 */
	return( 0 );  /* 成功 */
}

UINT16	mLinkCluster( UINT16 iCluster ) {	/* 获得指定簇号的链接簇 */
/* 输入: iCluster 当前簇号, 返回: 原链接簇号, 如果为0则说明错误 */
	UINT8	Status;
	Status = mReadSector( DiskStart + RsvdSecCnt + iCluster / 256, 1, DISK_BUFFER );  /* 读取簇号所在的FAT扇区 */
	if ( Status != 0 ) return( 0 );  /* 错误 */
	return( mGetPointWord( &DISK_BUFFER[ ( iCluster + iCluster ) & 0x01FF ] ) );  /* 取原簇链接 */
}

UINT32	mClusterToLba( UINT16 iCluster ) {	/* 将簇号转换为绝对LBA扇区地址 */
	return( DiskStart + RsvdSecCnt + FATSz16 + FATSz16 + 32 + ( iCluster - 2 ) * SecPerClus );  /* 将簇号转换为LBA,得当前操作的起始LBA */
}

void	mInitSTDIO( void ) {	/* 仅用于调试用途及显示内容到PC机,与该程序功能完全无关,为printf和getkey输入输出初始化串口 */
	SCON = 0x50; PCON = 0x80; TMOD = 0x20; TH1 = 0xf3; TR1 = 1; TI = 1;  /* 24MHz晶振, 9600bps */
}
void	mStopIfError( UINT8 iErrCode ) {	/* 如果错误则停止运行并显示错误状态,正式应用还需要分析处理 */
	if ( iErrCode == 0 ) return;
	printf( "Error status, %02X\n", (UINT16)iErrCode );
}

main( ) {
	UINT8	Status;
	UINT8X	*CurrentDir;
	UINT16	Cluster;
	mDelaymS( 200 );  /* 延时200毫秒 */
	mInitSTDIO( );
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );  /* 初始化CH375,设置USB工作模式 */
	CH375_WR_DAT_PORT( 6 );  /* 模式代码,自动检测USB设备连接 */
	while ( 1 ) {
		printf( "Insert USB disk\n" );
		while ( mWaitInterrupt( ) != USB_INT_CONNECT );  /* 等待U盘连接 */
		mDelaymS( 250 );  /* 延时等待U盘进入正常工作状态 */
		Status = mInitDisk( );  /* 初始化U盘,实际是识别U盘的类型,不影响U盘中的数据,在所有读写操作之前必须进行此步骤 */
		mStopIfError( Status );
		Status = mIdenDisk( );  /* 识别分析U盘文件系统,必要操作 */
		mStopIfError( Status );
		Status = mReadSector( DiskStart + RsvdSecCnt + FATSz16 + FATSz16, 32, DISK_BUFFER );  /* 读取FAT16逻辑盘的根目录,通常根目录占用32个扇区 */
		mStopIfError( Status );
		for ( CurrentDir = DISK_BUFFER; CurrentDir[0] != 0; CurrentDir += 32 ) {  /* 分析处理已经读到内存中的根目录 */
			if ( ( CurrentDir[0x0B] & 0x08 ) == 0 && CurrentDir[0] != 0xE5 ) {  /* 是文件名或者目录名,不是被删除的 */
				CurrentDir[0x0B] = 0;  /* 为了便于显示,设置文件名或者目录名的结束标志,否则会一直显示下去,实际应用不能置0 */
				printf( "Name: %s\n", CurrentDir );  /* 通过串口输出显示,当然在仿真器中可以直接看内存 */
			}
		}  /* 以上显示根目录下的所有文件名,以下打开第一个文件,如果是C文件的话 */
		if ( ( DISK_BUFFER[0x0B] & 0x08 ) == 0 && DISK_BUFFER[0] != 0xE5 && DISK_BUFFER[8] == 'C' ) {  /* 第一个文件是C文件 */
			Cluster = mGetPointWord( &DISK_BUFFER[0x1A] );  /* 文件的首簇 */
			while ( Cluster < 0xFFF8 ) {  /* 文件簇未结束 */
				if ( Cluster == 0 ) mStopIfError( 0x8F );  /* 对于首簇,可能是0长度文件而非错误,对于链接簇,可能是错误 */
				Status = mReadSector( mClusterToLba( Cluster ), SecPerClus, DISK_BUFFER );  /* 读取首簇到缓冲区 */
				mStopIfError( Status );
				DISK_BUFFER[30] = 0; printf( "Data: %s\n", DISK_BUFFER );  /* 先强制字符串结束,显示簇的第一行字符 */
				Cluster = mLinkCluster( Cluster );  /* 获取链接簇,返回0说明错误 */
			}
		}
		while ( mWaitInterrupt( ) != USB_INT_DISCONNECT );  /* 等待U盘拔出 */
		mDelaymS( 250 );
	}
}
