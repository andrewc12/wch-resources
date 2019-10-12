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

#include <reg51.h>
#include <absacc.h>
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN			32		/* 最大路径长度,含所有斜杠分隔符和小数点间隔符以及路径结束符00H,CH375模块支持的最大值是64,最小值是13 */
#include "..\CH375HM.H"

/* 电路连接方式
   单片机    模块
    TXD   =  SIN
    RXD   =  SOUT
    P15   =  STA#
*/
sbit	P15					=	P1^5;
#define	CH375HM_STA				P15		/* 假定CH375模块的STA#引脚连接到单片机的P15引脚 */

/* 假定文件数据缓冲区: ExtRAM: 0000H-7FFFH */
unsigned char xdata DATA_BUF[ 512 * 64 ] _at_ 0x0000;	/* 外部RAM的文件数据缓冲区,从该单元开始的缓冲区长度不小于一次读写的数据长度,最少为512字节 */

unsigned char xdata *buffer;			/* 数据缓冲区指针,用于读写数据块 */

CMD_PARAM		mCmdParam;				/* 默认情况下该结构将占用64字节的RAM,可以修改MAX_PATH_LEN常量,当修改为32时,只占用32字节的RAM */

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
		else if ( status == USB_INT_DISK_READ ) {  /* 正在从U盘读数据块,请求数据读出 */
			i = 64;
			do {
				*buffer = mRecvByte( );  /* 依次接收64字节的数据 */
				buffer ++;  /* 接收的数据保存到外部缓冲区 */
			} while ( -- i );
		}
		else if ( status == USB_INT_DISK_WRITE ) {  /* 正在向U盘写数据块,请求数据写入 */
			i = 64;
			do {
				mSendByte( *buffer );  /* 依次发送64字节的数据 */
				buffer ++;  /* 发送的数据来自外部缓冲区 */
			} while ( -- i );
		}
		else if ( status == USB_INT_DISK_RETRY ) {  /* 读写数据块失败重试,应该向回修改缓冲区指针 */
			i = mRecvByte( );  /* 大端模式下为回改指针字节数的高8位,如果是小端模式那么接收到的是回改指针字节数的低8位 */
			status = mRecvByte( );  /* 大端模式下为回改指针字节数的低8位,如果是小端模式那么接收到的是回改指针字节数的高8位 */
			buffer -= ( (unsigned short)i << 8 ) + status;  /* 这是大端模式下的回改指针,对于小端模式,应该是( (unsigned short)status << 8 ) + i */
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

main( ) {
	unsigned char	i, c, SecCount;
	unsigned long	OldSize;
	unsigned short	NewSize, count;
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
		memcpy( mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C", MAX_PATH_LEN );  /* 文件名,该文件在C51子目录下 */
		i = ExecCommand( CMD_FileOpen, MAX_PATH_LEN );  /* 打开文件,输入参数置为最大值,省得再计算参数长度 */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* ERR_MISS_DIR说明没有找到C51子目录,ERR_MISS_FILE说明没有找到文件 */
/* 列出根目录下的文件 */
/*			printf( "List file \\*\n" );*/
			for ( c = 0; c < 255; c ++ ) {  /* 最多搜索前255个文件 */
/*				memcpy( mCmdParam.Enumer.mPathName, "\\C51\\CH375*", MAX_PATH_LEN );*/  /* 搜索C51子目录下以CH375开头的文件名,*为通配符 */
				memcpy( mCmdParam.Enumer.mPathName, "\\*", MAX_PATH_LEN );  /* 搜索文件名,*为通配符,适用于所有文件或者子目录 */
/*				i = strlen( mCmdParam.Enumer.mPathName );*/  /* 计算文件名的长度 */
				for ( i = 0; i < MAX_PATH_LEN - 1; i ++ ) if ( mCmdParam.Enumer.mPathName[ i ] == 0 ) break;  /* 指向搜索文件名的结束符 */
				mCmdParam.Enumer.mPathName[ i ] = c;  /* 将结束符替换为搜索的序号,从0到255 */
				i = ExecCommand( CMD_FileEnumer, i+1 );  /* 枚举文件,如果文件名中含有通配符*,则为搜索文件而不打开,输入参数的长度很好计算 */
				if ( i == ERR_MISS_FILE ) break;  /* 再也搜索不到匹配的文件,已经没有匹配的文件名 */
				if ( i == ERR_SUCCESS || i == ERR_FOUND_NAME ) {  /* 搜索到与通配符相匹配的文件名,文件名及其完整路径在命令缓冲区中 */
/*					printf( "  match file %03d#: %s\n", (unsigned int)c, mCmdParam.Enumer.mPathName );*/  /* 显示序号和搜索到的匹配文件名或者子目录名 */
					continue;  /* 继续搜索下一个匹配的文件名,下次搜索时序号会加1 */
				}
				else {  /* 出错 */
					mStopIfError( i );
					break;
				}
			}
			strcpy( DATA_BUF, "Note: \n原本是打算将/C51/CH375HFT.C文件中的小写字母转成大写后写入新的文件,但是找不到这个文件\n" );
			OldSize = 0;
			NewSize = strlen( DATA_BUF );  /* 新文件的长度 */
			SecCount = ( NewSize + 511 ) >> 9;  /* (NewSize+511)/512, 计算文件的扇区数,因为读写是以扇区为单位的 */
		}
		else {  /* 找到文件\C51\CH375HFT.C或者出错 */
			mStopIfError( i );
/*			printf( "Query\n" );*/
			i = ExecCommand( CMD_FileQuery, 0 );  /* 查询当前文件的信息,没有输入参数 */
			mStopIfError( i );
/*			printf( "Read\n" );*/
			OldSize = mCmdParam.Modify.mFileSize;  /* 原文件的长度 */
			if ( OldSize > (unsigned long)(64*512) ) {  /* 演示板用的62256只有32K字节 */
				SecCount = 64;  /* 由于演示板用的62256只有32K字节,所以只读取不超过64个扇区,也就是不超过32768字节 */
				NewSize = 64*512;  /* 由于RAM有限所以限制长度 */
			}
			else {  /* 如果原文件较小,那么使用原长度 */
				SecCount = ( OldSize + 511 ) >> 9;  /* (OldSize+511)/512, 计算文件的扇区数,因为读写是以扇区为单位的 */
				NewSize = (unsigned short)OldSize;  /* 原长度 */
			}
/*			printf( "Size=%ld, Len=%d, Sec=%d\n", OldSize, NewSize, (unsigned short)SecCount );*/
			mCmdParam.Read.mSectorCount = SecCount;  /* 读取全部数据,如果超过60个扇区则只读取60个扇区 */
			buffer = & DATA_BUF;  /* 存放数据的缓冲区的起始地址,由CH375模块中断服务程序负责读出数据 */
			i = ExecCommand( CMD_FileRead, 1 );  /* 从文件读取数据 */
			mStopIfError( i );
/*
			如果文件比较大,一次读不完,可以再用命令CMD_FileRead继续读取,文件指针自动向后移动
			while ( 剩余未读完 ) {
				mCmdParam.Read.mSectorCount = 32;
				ExecCommand( CMD_FileRead, 1 );   读完后文件指针自动后移
				TotalLength += 32*512;  累计文件总长度
			}

		    如果希望从指定位置开始读写,可以移动文件指针
			mCmdParam.Locate.mSectorOffset = 3;  跳过文件的前3个扇区开始读写
			i = ExecCommand( CMD_FileLocate, 4 );  输入参数的长度4是sizeof( mCmdParam.Locate.mSectorOffset )
			mCmdParam.Read.mSectorCount = 10;
			ExecCommand( CMD_FileRead, 1 );   直接读取从文件的第(512*3)个字节开始的数据,前3个扇区被跳过

			如果希望将新数据添加到原文件的尾部,可以移动文件指针
			i = ExecCommand( CMD_FileOpen, (unsigned char)( strlen( mCmdParam.Open.mPathName ) + 1 ) );
			mCmdParam.Locate.mSectorOffset = 0xffffffff;  移到文件的尾部,以扇区为单位,如果原文件是3字节,则从512字节开始添加
			i = ExecCommand( CMD_FileLocate, sizeof( mCmdParam.Locate.mSectorOffset ) );
			mCmdParam.Write.mSectorCount = 10;
			ExecCommand( CMD_FileWrite, 1 );   在原文件的后面添加数据
*/
/*			printf( "Close\n" );*/
			mCmdParam.Close.mUpdateLen = 0;
			i = ExecCommand( CMD_FileClose, 1 );  /* 关闭文件 */
			mStopIfError( i );

/*			i = DATA_BUF[200];*/
/*			DATA_BUF[200] = 0;  置字符串结束标志,最多显示200个字符 */
/*			printf( "Line 1: %s\n", DATA_BUF );*/
/*			DATA_BUF[200] = i;  恢复原字符 */
			for ( count=0; count < NewSize; count ++ ) {  /* 将文件中的小写字符转换为大写 */
				c = DATA_BUF[ count ];
				if ( c >= 'a' && c <= 'z' ) DATA_BUF[ count ] = c - ( 'a' - 'A' );
			}
		}
/* 产生新文件 */
/*		printf( "Create\n" );*/
/*		memcpy( mCmdParam.Create.mPathName, "\\NEWFILE.TXT", MAX_PATH_LEN );*/
		memcpy( mCmdParam.Create.mPathName, "\\双击我吧.TXT", MAX_PATH_LEN );  /* 新文件名,在根目录下 */
		i = ExecCommand( CMD_FileCreate, MAX_PATH_LEN );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
/*		printf( "Write\n" );*/
		mCmdParam.Write.mSectorCount = 0x1;  /* 写入一个扇区512字节 */
		buffer = & DATA_BUF;  /* 存放数据的缓冲区的起始地址,由CH375模块中断服务程序负责写入数据 */
		i = ExecCommand( CMD_FileWrite, 1 );  /* 向文件写入数据 */
		mStopIfError( i );
		if ( SecCount > 1 ) {  /* 因为数据不超过255个扇区,所以完成能够一次写入,但是为了演示,特意分两次写入 */
			mCmdParam.Write.mSectorCount = SecCount - 1;
/*	buffer = & DATA_BUF + 512;  接着刚才的写,不必设置缓冲区的起始地址 */
			i = ExecCommand( CMD_FileWrite, 1 );  /* 向文件写入数据 */
			mStopIfError( i );
		}
/*		printf( "Modify\n" );*/
		mCmdParam.Modify.mFileAttr = 0xff;  /* 输入参数: 新的文件属性,为0FFH则不修改 */
		mCmdParam.Modify.mFileTime = 0xffff;  /* 输入参数: 新的文件时间,为0FFFFH则不修改,使用新建文件产生的默认时间 */
		mCmdParam.Modify.mFileDate = ( (2004-1980)<<9 ) + ( 5<<5 ) + 18;  /* 输入参数: 新的文件日期: 2004.05.18 */
		mCmdParam.Modify.mFileSize = NewSize;  /* 输入参数: 如果原文件较小,那么新的文件长度与原文件一样长,否则被RAM所限 */
		i = ExecCommand( CMD_FileModify, 4+2+2+1 );  /* 修改当前文件的信息,修改日期和长度,参数长度为sizeof(mCmdParam.Modify.mFileSize)+... */
		mStopIfError( i );
/*		printf( "Close\n" );*/
		mCmdParam.Close.mUpdateLen = 0;  /* 不要自动计算文件长度,如果自动计算,那么该长度总是512的倍数 */
		i = ExecCommand( CMD_FileClose, 1 );
		mStopIfError( i );

/* 删除某文件 */
/*		printf( "Erase\n" );*/
		memcpy( mCmdParam.Create.mPathName, "\\OLD", MAX_PATH_LEN );  /* 将被删除的文件名,在根目录下 */
		i = ExecCommand( CMD_FileErase, MAX_PATH_LEN );  /* 删除文件并关闭 */
/*		mStopIfError( i );*/

/* 查询磁盘信息 */
/*		printf( "Disk\n" );
		i = ExecCommand( CMD_DiskQuery, 0 );
		mStopIfError( i );
		i = mCmdParam.Query.mDiskFat;
		if ( i == 1 ) i = 12;
		else if ( i == 2 ) i = 16;
		else if ( i == 3 ) i = 32;
		printf( "FatCode=FAT%d, TotalSector=%ld, FreeSector=%ld\n", (unsigned short)i, mCmdParam.Query.mTotalSector, mCmdParam.Query.mFreeSector );*/
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
