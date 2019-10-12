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
/* 本程序用于演示检查U盘是否写保护,演示模拟计算机端的安全移除,也可以参考用于自行处理其它命令 */
/* CH375的INT#引脚采用查询方式处理, 适用于所有MCS51单片机, 本例适用于V2.4及以上版本的CH375子程序库, 适用于CH375A芯片 */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF4.LIB    如果将CH375HF4换成CH375HF6就可以支持FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>
#include <string.h>
#include <intrins.h>

/* 以下定义的详细说明请看CH375HF6.H文件 */
#define	MAX_BYTE_IO				48		/* 以字节为单位单次读写文件时的最大长度,默认值是29,值大则占用内存多,值小则超过该长度必须分多次读写 */

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

/* 以下为CH375硬件接口子程序 */
#ifdef __C51__  // MCS51并口

#define xWriteCH375Cmd( c )		{ CH375_CMD_PORT = ( c ); _nop_( ); _nop_( ); _nop_( ); _nop_( ); }	/* 写命令并延时2uS */
#define xWriteCH375Data( c )	{ CH375_DAT_PORT = ( c ); _nop_( ); }	/* 写数据并延时1uS */
#define xReadCH375Data( )		( CH375_DAT_PORT )	/* 连续读数据,最小周期为1uS,否则延时 */

#else  // MCS51或者其它单片机的模拟并口

void xWriteCH375Cmd( UINT8 mCmd )		/* 外部定义的被CH375程序库调用的子程序,向CH375写命令 */
{
	mDelay1uS( ); mDelay1uS( );  /* 至少延时1uS */
	P0 = mCmd;  /* 向CH375的并口输出数据 */
	CH375_A0 = 1;
	CH375_CS = 0;
	CH375_WR = 0;  /* 输出有效写控制信号, 写CH375芯片的命令端口 */
	CH375_CS = 0;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于50nS */
	CH375_WR = 1;  /* 输出无效的控制信号, 完成操作CH375芯片 */
	CH375_CS = 1;
	CH375_A0 = 0;
	P0 = 0xFF;  /* 禁止数据输出 */
	mDelay1uS( ); mDelay1uS( );  /* 至少延时2uS */
}

void xWriteCH375Data( UINT8 mData )		/* 外部定义的被CH375程序库调用的子程序,向CH375写数据 */
{
	P0 = mData;  /* 向CH375的并口输出数据 */
	CH375_A0 = 0;
	CH375_CS = 0;
	CH375_WR = 0;  /* 输出有效写控制信号, 写CH375芯片的数据端口 */
	CH375_CS = 0;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于50nS */
	CH375_WR = 1;  /* 输出无效的控制信号, 完成操作CH375芯片 */
	CH375_CS = 1;
	P0 = 0xFF;  /* 禁止数据输出 */
	mDelay1uS( );  /* 至少延时1uS */
}

UINT8 xReadCH375Data( void )			/* 外部定义的被CH375程序库调用的子程序,从CH375读数据 */
{
	UINT8	mData;
	mDelay1uS( );  /* 至少延时1uS */
	P0 = 0xFF;  /* 输入 */
	CH375_A0 = 0;
	CH375_CS = 0;
	CH375_RD = 0;  /* 输出有效写控制信号, 读CH375芯片的数据端口 */
	CH375_CS = 0;  /* 该操作无意义,仅作延时,CH375要求读写脉冲宽度大于50nS */
	mData = P0;  /* 从CH375的并口输入数据 */
	CH375_RD = 1;  /* 输出无效的控制信号, 完成操作CH375芯片 */
	CH375_CS = 1;
	return( mData );
}

#endif

/* 从CH375主机端点的接收缓冲区读取数据块,返回读取的数据总长度 */
UINT8	ReadUsbData( UINT8 *iBuffer )
{
	UINT8	mCount, mLength;
	xWriteCH375Cmd( CMD_RD_USB_DATA0 );  /* 从当前USB中断的端点缓冲区读取数据块 */
	mLength = xReadCH375Data( );  /* 后续数据的长度 */
	for ( mCount = mLength; mCount != 0; mCount -- ) {  /* 根据长度读取数据 */
		*iBuffer = xReadCH375Data( );  /* 读出数据并保存 */
		iBuffer ++;
	}
	return( mLength );
}

/* 向CH375主机端点的发送缓冲区写入数据块 */
void	WriteUsbData( UINT8 *iBuffer, UINT8 iCount )
{
	xWriteCH375Cmd( CMD_WR_USB_DATA7 );  /* 向USB主机端点的发送缓冲区写入数据块 */
	xWriteCH375Data( iCount );  /* 后续数据的长度 */
	for ( ; iCount != 0; iCount -- ) {  /* 根据长度写入数据 */
		xWriteCH375Data( *iBuffer );  /* 将数据写入 */
		iBuffer ++;
	}
}

/* 检查U盘是否写保护, 返回USB_INT_SUCCESS说明可以写,返回0xFF说明只读/写保护,返回其它值说明是错误代码 */
/* 其它BulkOnly传输协议的命令可以参考这个例子处理,修改前必须了解USB MassStorage和SCSI规范 */
UINT8	IsDiskWriteProtect( void )
{
	UINT8	mIfSubClass, mLength, mDevSpecParam;
	if ( CH375Version2 == 0 ) return( ERR_USB_DISK_ERR );  /* CH375S不支持 */
	CH375IntStatus = 0;  /* 清中断标志 */
	xWriteCH375Cmd( CMD_GET_DESCR );  /* 获取描述符的命令 */
	xWriteCH375Data( 2 );  /* 配置描述符 */
	xQueryInterrupt( );  /* 查询CH375中断并更新中断状态 */
	if ( CH375IntStatus == USB_INT_SUCCESS ) {  /* 操作成功 */
		ReadUsbData( (UINT8 *)&mCmdParam );  /* 读取描述符,请确保mCmdParam长度MAX_BYTE_IO大于32字节,否则需另选缓冲区 */
		mIfSubClass = mCmdParam.Other.mBuffer[9+6];  /* 子类 USB_CFG_DESCR_LONG.itf_descr.bInterfaceSubClass */
		mCmdParam.BOC.mCBW.mCBW_DataLen = 0x10;  /* 数据传输长度 */
		mCmdParam.BOC.mCBW.mCBW_Flag = 0x80;  /* 传输方向为输入 */
		if ( mIfSubClass == 6 ) {  /* 根据子类选择命令码 */
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[0] = 0x1A;  /* 命令块首字节, MODE SENSE(6) */
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[1] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[2] = 0x3F;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[3] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[4] = 0x10;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[5] = 0x00;
		}
		else {
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[0] = 0x5A;  /* 命令块首字节, MODE SENSE(10) */
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[1] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[2] = 0x3F;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[3] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[4] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[5] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[6] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[7] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[8] = 0x10;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[9] = 0x00;
		}
		WriteUsbData( (UINT8 *)&mCmdParam, 31 );  /* 向CH375主机端点的发送缓冲区写入CBW数据块,剩余部分CH375自动填补 */
		CH375IntStatus = 0;
		xWriteCH375Cmd( CMD_DISK_BOC_CMD );  /* 对USB存储器执行BulkOnly传输协议 */
		xQueryInterrupt( );  /* 查询CH375中断并更新中断状态 */
		if ( CH375IntStatus == USB_INT_SUCCESS ) {  /* 操作成功 */
			mLength = ReadUsbData( (UINT8 *)&mCmdParam );  /* 从CH375主机端点的接收缓冲区读取数据块 */
			if ( mLength > 3 ) {  /* MODE SENSE命令返回数据的长度有效 */
				if ( mIfSubClass == 6 ) mDevSpecParam = mCmdParam.Other.mBuffer[2];  /* MODE SENSE(6), device specific parameter */
				else mDevSpecParam = mCmdParam.Other.mBuffer[3];  /* MODE SENSE(10), device specific parameter */
				if ( mDevSpecParam & 0x80 ) return( 0xFF );  /* U盘写保护 */
				else return( USB_INT_SUCCESS );  /* U盘没有写保护 */
			}
			return( ERR_USB_DISK_ERR );
		}
		mIfSubClass = CH375IntStatus;  /* 暂存 */
		xWriteCH375Cmd( CMD_DISK_R_SENSE );  /* 检查USB存储器错误 */
		xQueryInterrupt( );  /* 查询CH375中断并更新中断状态 */
		return( mIfSubClass );
	}
	return( CH375IntStatus );
}

/* 安全移除U盘, 返回USB_INT_SUCCESS说明可以安全移除,否则说明不能安全移除,只能强行移除 */
/* 在操作完U盘准备让用户拔出U盘前调用, 防止用户过早拔出U盘丢失数据 */
UINT8	SafeRemoveDisk( void )
{
	UINT8	i;
	for ( i = 0; i < 10; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
		mDelaymS( 100 );
		if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
	}
	mDelaymS( 10 );
	xWriteCH375Cmd( CMD_SET_CONFIG );  /* 设置USB配置 */
	xWriteCH375Data( 0 );  /* 取消配置,会导致很多U盘的LED灯灭 */
	xQueryInterrupt( );  /* 查询CH375中断并更新中断状态 */
	mDelaymS( 10 );
	if ( i < 5 && CH375IntStatus == USB_INT_SUCCESS ) return( USB_INT_SUCCESS );  /* 操作成功 */
	else return( 0xFF );
/* 以下取消SOF包会导致绝大多数U盘的LED灯灭 */
/* 如果此处取消SOF, 那么与此对应, 在检测到U盘插入后应该再恢复为模式6即恢复SOF包 */
//	xWriteCH375Cmd( CMD_SET_USB_MODE );  /* 设置USB工作模式 */
//	xWriteCH375Data( 5 );  /* 停止发出SOF包,检测到U盘插入后必须用模式6恢复SOF包发出 */
//	mDelaymS( 1 );
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

		printf( "Check Disk Write Protect ? ...\n" );
		i = IsDiskWriteProtect( );  /* 检查U盘是否写保护, 返回USB_INT_SUCCESS说明可以写,返回0xFF说明只读/写保护,返回其它值说明是错误代码 */
		if ( i != USB_INT_SUCCESS && i != 0xFF ) {  /* 操作失败 */
			printf( "Again ...\n" );
			mDelaymS( 100 );
			i = IsDiskWriteProtect( );  /* 再试一次 */
		}
		if ( i == USB_INT_SUCCESS ) {  /* 可以写 */
			printf( "... No !\n" );
			LED_WR_ACT( );  /* 写操作 */
			printf( "Create a File\n" );
			mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "\\NEWFILE.TXT" );  /* 新文件名,在根目录下 */
			i = CH375FileCreate( );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
			mStopIfError( i );
			mCmdParam.ByteWrite.mByteBuffer[0] = 'O';
			mCmdParam.ByteWrite.mByteBuffer[1] = 'K';
			mCmdParam.ByteWrite.mByteCount = 2;  /* 写入数据的字符数,单次读写的长度不能超过MAX_BYTE_IO,第二次调用时接着刚才的向后写 */
			i = CH375ByteWrite( );  /* 向文件写入数据 */
			mStopIfError( i );
			printf( "Close\n" );
			mCmdParam.Close.mUpdateLen = 0;  /* 不需要自动计算文件长度 */
			i = CH375FileClose( );
			mStopIfError( i );
			LED_WR_INACT( );
			if ( SafeRemoveDisk( ) == USB_INT_SUCCESS ) {  /* 安全移除U盘 */
				printf( "Safe Remove !\n" );
			}
			else {
				printf( "Unsafe Remove !\n" );
			}
		}
		else if ( i == 0xFF ) {  /* 写保护 */
			printf( "... Yes !\n" );
		}
		else {
			mStopIfError( i );  /* 显示错误代码 */
		}
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
