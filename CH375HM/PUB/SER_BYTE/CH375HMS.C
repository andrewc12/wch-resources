/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Module      @CH375  **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* U盘文件读写模块, 连接方式: 串口+查询 */
/* MCS-51单片机C语言示例程序 */
/* 因为使用U盘文件读写模块而不是使用U盘文件级子程序库,所以占用较少的单片机资源,可以使用89C51单片机测试 */
/* 以字节为单位进行U盘文件读写,单片机的RAM只需要几十个字节,不需要外部RAM */

#include <reg51.h>
#include <absacc.h>
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN			32		/* 最大路径长度,含所有斜杠分隔符和小数点间隔符以及路径结束符00H,CH375模块支持的最大值是62,最小值是13 */
#include "..\CH375HM.H"

/* 电路连接方式
   单片机    模块
    TXD   =  SIN
    RXD   =  SOUT
    P15   =  STA#
*/
sbit	P15					=	P1^5;
#define	CH375HM_STA				P15		/* 假定CH375模块的STA#引脚连接到单片机的P15引脚 */

CMD_PARAM	idata	mCmdParam;			/* 默认情况下该结构将占用60字节的RAM,可以修改MAX_PATH_LEN常量,当修改为32时,只占用32字节的RAM */
unsigned char		TempLength;			/* 临时缓冲区中的数据长度,从原文件中第二次读出的字节数 */
unsigned char idata	TempBuffer[20];		/* 临时缓冲区,存放从原文件中读出的内容 */

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
	CH375HM_STA = 0;  /* 产生下降沿通知模块,说明命令码开始发送,请求开始执行命令 */
	CH375HM_STA = 0;  /* 仅作延时,低电平宽度不小于1uS */
	RI = 0;
	CH375HM_STA = 1;
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

/* 检查操作状态,如果错误则显示错误代码并停机 */
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

main( ) {
	unsigned char	i;
	unsigned short	count;
	unsigned char	*pStr;
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
/* 由于4800bps较慢,所以下面用命令将其修改为9600bps */
	mCmdParam.BaudRate.mDivisor = 18432000/32/9600;  /* 输入参数: 通讯波特率除数,假定模块的晶体X2的频率为18.432MHz */
	i = ExecCommand( CMD_BaudRate, 1 );  /* 设置串口通讯波特率 */
	mStopIfError( i );
	TH1 = 0xF3;  /* 24MHz晶振, 将自身串口的通讯波特率调整到9600bps */
	mDelaymS( 5 );  /* 延时5毫秒,确保CH375模块切换到新设定的通讯波特率 */
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
/* 检查U盘是否准备好,大多数U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 5; i ++ ) {
			mDelaymS( 100 );
//			printf( "Ready ?\n" );
			if ( ExecCommand( CMD_DiskReady, 0 ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
		}
/* 读取原文件 */
/*		printf( "Open\n" );*/
		strcpy( mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C" );  /* 文件名,该文件在C51子目录下 */
		i = ExecCommand( CMD_FileOpen, MAX_PATH_LEN );  /* 打开文件,输入参数置为最大值,省得再计算参数长度 */
		TempLength = 0;
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* ERR_MISS_DIR说明没有找到C51子目录,ERR_MISS_FILE说明没有找到文件 */
//			printf( "找不到原文件/C51/CH375HFT.C\n" );
		}
		else {  /* 找到文件\C51\CH375HFT.C或者出错 */
			mStopIfError( i );
			mCmdParam.ByteRead.mByteCount = 6;  /* 请求读出6字节数据, 单次读写的长度不能超过 sizeof( mCmdParam.ByteWrite.mByteBuffer ) */
			i = ExecCommand( CMD_ByteRead, 1 );  /* 以字节为单位读取数据 */
			mStopIfError( i );
//			printf( "从文件中读出的前6个字符是[" );
//			for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );
//			printf( "]\n" );
//			if ( mCmdParam.ByteRead.mByteCount<6 ) printf( "已经到文件的末尾\n" );
			if ( mCmdParam.ByteRead.mByteCount==6 ) {  /* 未到文件末尾 */
				mCmdParam.ByteRead.mByteCount = 20;  /* 请求再读出20字节数据, 单次读写的长度不能超过 sizeof( mCmdParam.ByteWrite.mByteBuffer ) */
				i = ExecCommand( CMD_ByteRead, 1 );  /* 以字节为单位读取数据,接着刚才的向后读 */
				mStopIfError( i );
				TempLength = mCmdParam.ByteRead.mByteCount;  /* 第二次读出字节数 */
				memcpy( TempBuffer, mCmdParam.ByteRead.mByteBuffer, TempLength );  /* 暂存第二次读出的数据以便写入新文件中 */
//				printf( "从文件中读出的第6个字符开始依次是[" );
//				for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );
//				printf( "]\n" );
//				if ( mCmdParam.ByteRead.mByteCount<20 ) printf( "已经到文件的末尾\n" );
			}
/*			printf( "Close\n" );*/
			mCmdParam.Close.mUpdateLen = 0;
			i = ExecCommand( CMD_FileClose, 1 );  /* 关闭文件 */
			mStopIfError( i );
		}
/* 产生新文件 */
/*		printf( "Create\n" );*/
/*		strcpy( mCmdParam.Create.mPathName, "\\NEWFILE.TXT" );*/
		strcpy( mCmdParam.Create.mPathName, "\\双击我吧.TXT" );  /* 新文件名,在根目录下 */
		i = ExecCommand( CMD_FileCreate, MAX_PATH_LEN );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
/*		printf( "ByteLocate\n" );*/
//		mCmdParam.ByteLocate.mByteOffset = 0;  /* 移动到文件头,用于重新回到文件头,以便写入数据覆盖原数据 */
//		ExecCommand( CMD_ByteLocate, 4 );  /* 以字节为单位移动文件指针 */
//		mCmdParam.ByteLocate.mByteOffset = 0xFFFFFFFF;  /* 移动到文件尾,用于在CMD_FileOpen打开文件后,继续追加数据到已打开文件的末尾 */
//		ExecCommand( CMD_ByteLocate, 4 );  /* 以字节为单位移动文件指针 */
/*		printf( "Write\n" );*/
		pStr = "Note: \xd\xa这个程序是以字节为单位进行U盘文件读写,单片机只需要有几十字节的RAM,不需要外部RAM,\xd\xa首先从/C51/CH375HFT.C文件中读出前20个字符,然后写到本说明的下一行\xd\xa";
		count = strlen( pStr );  /* 准备写入的数据的总长度 */
		while ( count ) {  /* 如果较大,分多次写入 */
			if ( count < sizeof( mCmdParam.ByteWrite.mByteBuffer ) ) i = count;  /* 只剩最后一些数据要写入 */
			else i = sizeof( mCmdParam.ByteWrite.mByteBuffer );  /* 数据较多,分多次写入 */
			count -= i;  /* 计数 */
			memcpy( mCmdParam.ByteWrite.mByteBuffer, pStr, i );  /* 复制准备写入的数据到参数结构中,源数据可以来自ADC等,本例是来自程序空间的说明信息 */
			pStr += i;
			mCmdParam.ByteWrite.mByteCount = i;  /* 指定本次写入的字节数 */
			i = ExecCommand( CMD_ByteWrite, 1+i );  /* 以字节为单位向文件写入数据 */
			mStopIfError( i );
		}
//		mCmdParam.ByteWrite.mByteCount = 0;  /* 指定写入0字节,用于刷新文件的长度,注意如果字节数不为0那么CMD_ByteWrite只负责写入数据而不修改文件长度 */
//		ExecCommand( CMD_ByteWrite, 1 );  /* 以字节为单位向文件写入数据,因为是0字节写入,所以只用于更新文件的长度,当阶段性写入数据后,可以用这种办法更新文件长度 */
		memcpy( mCmdParam.ByteWrite.mByteBuffer, TempBuffer, TempLength );
		mCmdParam.ByteWrite.mByteCount = TempLength;  /* 将原文件中的20个字节的数据添加到新文件的末尾 */
		i = ExecCommand( CMD_ByteWrite, 1+TempLength );  /* 以字节为单位向文件写入数据 */
		mStopIfError( i );
/*		printf( "Close\n" );*/
		mCmdParam.Close.mUpdateLen = 1;  /* 自动计算文件长度,当以字节为单位向文件写入数据后,如果没有用0长度的CMD_ByteWrite更新文件长度,那么可以在关闭文件时让模块自动更新文件长度 */
		i = ExecCommand( CMD_FileClose, 1 );  /* 关闭文件,当以字节为单位向文件写入(追加)数据后,必须在用完文件后关闭文件 */
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
