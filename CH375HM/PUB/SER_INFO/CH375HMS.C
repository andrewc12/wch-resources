/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Module      @CH375  **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* U盘文件读写模块, 连接方式: 3线制串口+查询 */
/* MCS-51单片机C语言示例程序 */
/* 因为使用U盘文件读写模块而不是使用U盘文件级子程序库,所以占用较少的单片机资源,可以使用89C51单片机测试 */
/* 本程序用于演示处理文件目录项,例如:修改文件名,设置文件的创建日期和时间等 */

#include <reg51.h>
#include <absacc.h>
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN			40		/* 最大路径长度,含所有斜杠分隔符和小数点间隔符以及路径结束符00H,CH375模块支持的最大值是62,最小值是13 */
/* 为了处理文件目录项,MAX_PATH_LEN至少为36,sizeof( mCmdParam.FileDirInfo ) */
#include "..\CH375HM.H"

/* 电路连接方式,只需要连接3根线,使用串口同步码启动操作
   单片机    模块
    TXD   =  SIN
    RXD   =  SOUT
             STA# 悬空或接高电平
             INT# 接地或接低电平
    GND   =  GND
*/
sbit	P15					=	P1^5;

CMD_PARAM	idata	mCmdParam;			/* 默认情况下该结构将占用60字节的RAM,可以修改MAX_PATH_LEN常量,当修改为32时,只占用32字节的RAM */

sbit	LED_OUT		=	P1^4;			/* P1.4 低电平驱动LED显示,用于监控演示程序的进度 */

/* 以毫秒为单位延时,适用于24MHz时钟 */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
	}
}

/* 发送一个字节数据给CH375模块 */
void	mSendByte( unsigned char c )
{
	TI = 0;
	SBUF = c;
	while ( TI == 0 );
}

/* 从CH375模块接收一个字节数据 */
unsigned char	mRecvByte( )
{
	unsigned char	c;
	while ( RI == 0 );
	c = SBUF;
	RI = 0;
	return( c );
}

/* 执行命令 */
unsigned char	ExecCommand( unsigned char cmd, unsigned char len )
/* 输入命令码和输入参数长度,返回操作状态码,输入参数和返回参数都在CMD_PARAM结构中 */
{
	unsigned char		i, j, status;
	mSendByte( SER_SYNC_CODE1 );  /* 发送串口同步码通知模块,说明命令码开始发送,请求开始执行命令 */
	mSendByte( SER_SYNC_CODE2 );  /* 用两个串口同步码代替STA#的下降沿 */
/* 上面两个串口同步码应该连续发送,如果不连续,那么间隔时间不能超过20mS,否则命令无效 */
	RI = 0;
	mSendByte( cmd );  /* 写入命令码 */
	mSendByte( len );  /* 写入后续参数的长度 */
	if ( len ) {  /* 有参数 */
		for ( i = 0; i != len; i ++ ) mSendByte( mCmdParam.Other.mBuffer[ i ] );  /* 依次写入参数 */
	}
	while ( 1 ) {  /* 处理数据传输,直到操作完成才退出 */
		status = mRecvByte( );  /* 等待模块完成操作并返回操作状态 */
		if ( status == ERR_SUCCESS ) {  /* 操作成功 */
			i = mRecvByte( );  /* 返回结果数据的长度 */
			if ( i ) {  /* 有结果数据 */
				j = 0;
				do {  /* 使用do+while结构是因为其效率高于for */
					mCmdParam.Other.mBuffer[ j ] = mRecvByte( );  /* 接收结果数据并保存到参数结构中 */
					j ++;
				} while ( -- i );
			}
			break;  /* 操作成功返回 */
		}
		else if ( status == USB_INT_DISK_READ || status == USB_INT_DISK_WRITE || status == USB_INT_DISK_RETRY ) {  /* 正在从U盘读数据块,请求数据读出,正在向U盘写数据块,请求数据写入,读写数据块失败重试 */
			break;  /* 本程序只使用以字节为单位的文件读写子程序,所以正常情况下不会收到该状态码,操作失败返回 */
		}
		else {  /* 操作失败 */
			if ( status == ERR_DISK_DISCON || status == ERR_USB_CONNECT ) mDelaymS( 100 );  /* U盘刚刚连接或者断开,应该延时几十毫秒再操作 */
			break;  /* 操作失败返回 */
		}
	}
	return( status );
}

/* 检查操作状态,如果错误则显示错误代码并停机,应该替换为实际的处理措施 */
void	mStopIfError( unsigned char iError )
{
	unsigned char	led;
	if ( iError == ERR_SUCCESS ) return;  /* 操作成功 */
/*	printf( "Error: %02X\n", (unsigned short)iError );*/  /* 显示错误 */
	led=0;
	while ( 1 ) {
		LED_OUT = led&1;  /* LED闪烁 */
		mDelaymS( 100 );
		led^=1;
	}
}

/* 大端与小端格式的数据处理 */
unsigned short	SwapUINT16( unsigned short d )
{
	return( ( d << 8 ) & 0xFF00 | ( d >> 8 ) & 0xFF );
}

main( ) {
	unsigned char	i;
	unsigned short	FileCreateDate, FileCreateTime;
	unsigned char	*name;
	LED_OUT = 0;  /* 开机后LED亮一下以示工作 */
	mDelaymS( 100 );  /* 延时100毫秒,CH375模块上电后需要100毫秒左右的复位时间 */
	mDelaymS( 100 );
	LED_OUT = 1;
/* 设置与CH375模块通讯的串口 */
	SCON = 0x50;
	PCON = 0x80;
	TMOD = 0x20;
	TH1 = 0xE6;  /* 24MHz晶振, 4800bps */
	TR1 = 1;

/*	printf( "Start\n" );*/
	while ( 1 ) {  /* 主循环 */
/*		printf( "Wait\n" );*/
		while ( 1 ) {  /* 使用查询方式看U盘是否连接 */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* 查询当前模块的状态 */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus >= DISK_CONNECT ) break;  /* U盘已经连接 */
			mDelaymS( 100 );  /* 可以在打算读写U盘时再查询,没有必要一直连续不停地查询,可以让单片机做其它事,没事可做就延时等待一会再查询 */
		}
		mDelaymS( 200 );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */
		LED_OUT = 0;  /* LED亮 */
/* 检查U盘是否准备好,某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 3; i ++ ) {
			mDelaymS( 100 );
//			printf( "Ready ?\n" );
			if ( ExecCommand( CMD_DiskReady, 0 ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
		}
/* 将MY_ADC.TXT文件名修改为WY_ADC.C,并设置创建文件的日期和时间,首先打开原文件 */
		name = "/MY_ADC.TXT";  /* 文件名,斜杠说明是从根目录开始 */
/*		printf( "Open\n" );*/
		strcpy( mCmdParam.Open.mPathName, name );  /* 原文件名 */
		i = ExecCommand( CMD_FileOpen, MAX_PATH_LEN );  /* 打开文件,输入参数置为最大值,省得再计算参数长度 */
		if ( i == ERR_MISS_FILE ) mStopIfError( i );  /* 文件不存在,当然无法修改文件目录信息 */
		mStopIfError( i );
		/* 文件读写操作等... */

/*		printf( "Get file directory information\n" );*/
		mCmdParam.FileDirInfo.mAccessMode = 0;  /* 读取文件目录信息 */
		mCmdParam.FileDirInfo.mReserved[0] = mCmdParam.FileDirInfo.mReserved[1] = mCmdParam.FileDirInfo.mReserved[2] = 0;  /* 保留单元 */
		i = ExecCommand( CMD_FileDirInfo, 4 );  /* 存取当前已打开文件的目录信息 */
		mStopIfError( i );

/* 以下修改文件目录信息中的文件名 */
		mCmdParam.FileDirInfo.mDir.DIR_Name[0] = 'W';  /* 修改文件名首字节为W */
		mCmdParam.FileDirInfo.mDir.DIR_Name[8] = 'C';  /* 修改文件扩展名为C */
		mCmdParam.FileDirInfo.mDir.DIR_Name[9] = ' ';
		mCmdParam.FileDirInfo.mDir.DIR_Name[10] = ' ';

/* 以下修改文件目录信息中的文件创建时间,DIR_CrtTime是创建时间,DIR_WrtTime是修改时间 */
		FileCreateTime = MAKE_FILE_TIME( 16, 49, 28 );  /* 设置文件创建时间是16时49分28秒 */
//		mCmdParam.FileDirInfo.mDir.DIR_CrtTime = FileCreateTime;  /* 文件创建的时间,适用于小端格式 */
		mCmdParam.FileDirInfo.mDir.DIR_CrtTime = SwapUINT16( FileCreateTime );  /* MCS51单片机C语言是大端格式,所以必须转换后输出 */
		FileCreateDate = MAKE_FILE_DATE( 2004, 12, 8 );  /* 设置文件创建日期是2004年12月8日 */
//		mCmdParam.FileDirInfo.mDir.DIR_CrtDate = FileCreateDate;  /* 文件创建的日期,适用于小端格式 */
		mCmdParam.FileDirInfo.mDir.DIR_CrtDate = SwapUINT16( FileCreateDate );  /* MCS51单片机C语言是大端格式,所以必须转换后输出 */

//		mCmdParam.FileDirInfo.mDir.DIR_WrtTime = SwapUINT16( MAKE_FILE_TIME( 时, 分, 秒 ) );  /* 文件修改时间 */
//		mCmdParam.FileDirInfo.mDir.DIR_LstAccDate = SwapUINT16( MAKE_FILE_DATE( 年, 月, 日 ) );  /* 最近一次存取操作的日期 */

/* 以下将修改过的内容真正刷新到U盘中 */
/*		printf( "Save new file directory information\n" );*/
		mCmdParam.FileDirInfo.mAccessMode = 0xF0;  /* 写入/更新文件目录信息 */
		i = ExecCommand( CMD_FileDirInfo, sizeof( mCmdParam.FileDirInfo ) );  /* 存取当前已打开文件的目录信息 */
		mStopIfError( i );

		/* 文件读写操作等... */
/*		printf( "Close\n" );*/
		mCmdParam.Close.mUpdateLen = 0;
		i = ExecCommand( CMD_FileClose, 1 );  /* 关闭文件 */
		mStopIfError( i );

/* 等待U盘断开 */
/*		printf( "Take_out\n" );*/
		while ( 1 ) {  /* 使用查询方式看U盘是否断开 */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* 查询当前模块的状态 */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus <= DISK_DISCONNECT ) break;  /* U盘已经断开 */
			mDelaymS( 100 );  /* 没有必要一直连续不停地查询,可以让单片机做其它事,没事可做就延时等待一会再查询 */
		}
		LED_OUT = 1;  /* LED灭 */
	}
}
