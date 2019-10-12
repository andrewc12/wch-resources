/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2007   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Module      @CH375  **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* U盘文件读写模块, 连接方式: 并口+查询 */
/* MCS-51单片机C语言示例程序, 仅适用于V3.0A及以上版本的模块 */
/* 因为使用U盘文件读写模块而不是使用U盘文件级子程序库,所以占用较少的单片机资源,可以使用89C51单片机测试 */

#include <reg51.h>
#include <absacc.h>
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN			32		/* 最大路径长度,含所有斜杠分隔符和小数点间隔符以及路径结束符00H,CH375模块支持的最大值是64,最小值是13 */
#include "..\CH375HM.H"

/* 电路连接方式
   单片机    模块
    P0    =  D0-D7
    RD    =  RD#
    WR    =  WR#
    ?     =  CS#   如果没有外部RAM,那么CS#=P26,如果有超过16KB的外部RAM,那么CS#=P27 & ! P26 & ...,所以CS#的片选地址为BXXXH
    P20   =  A0
    INT0  =  INT#  虽然连接到INT0,但是本程序只是查询模块的INT#的状态,所以可以用P1口等普通I/O引脚代替INT0
*/
#define CH375HM_INDEX	XBYTE[0xBCF0]	/* CH375模块的索引端口的I/O地址 */
#define CH375HM_DATA	XBYTE[0xBDF1]	/* CH375模块的数据端口的I/O地址 */
#define CH375HM_INT_WIRE		INT0	/* 假定CH375模块的INT#引脚连接到单片机的INT0引脚 */

/* 假定文件数据缓冲区: ExtRAM: 0000H-7FFFH */
unsigned char xdata DATA_BUF[ 512 * 64 ] _at_ 0x0000;	/* 外部RAM的文件数据缓冲区,从该单元开始的缓冲区长度不小于一次读写的数据长度,最少为512字节 */

CMD_PARAM		mCmdParam;				/* 默认情况下该结构将占用64字节的RAM,可以修改MAX_PATH_LEN常量,当修改为32时,只占用32字节的RAM */
unsigned char	mIntStatus;				/* CH375模块的中断状态或者操作完成状态 */

sbit	LED_OUT		=	P1^4;			/* P1.4 低电平驱动LED显示,用于监控演示程序的进度 */

/* 对于模拟的并口读写时序或者其它并口读写方式,请修改以下3个子程序 */
#define CH375HM_INDEX_WR( Index )	{ CH375HM_INDEX = (Index); }	/* 写索引地址 */
#define CH375HM_DATA_WR( Data )		{ CH375HM_DATA = (Data); }		/* 写数据 */
#define CH375HM_DATA_RD( )			( CH375HM_DATA )				/* 读数据 */


/* 以毫秒为单位延时,适用于24MHz时钟 */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
	}
}

/* 执行命令 */
unsigned char	ExecCommandBuf( unsigned char cmd, unsigned char len, unsigned char xdata *bufstart )
/* 输入命令码和输入参数长度,返回操作状态码,输入参数和返回参数都在CMD_PARAM结构中 */
/* 输入参数bufstart仅用于CMD_FileRead或者CMD_FileWrite命令,指定外部RAM缓冲区的起始地址,可以参考中断方式C程序采用全局变量buffer的方式 */
{
	unsigned char		i, status;
	unsigned char data	*buf;
	unsigned char xdata	*CurrentBuf;
	CH375HM_INDEX_WR( PARA_COMMAND_ADDR );
	CH375HM_DATA_WR( cmd );  /* 向索引地址PARA_COMMAND_ADDR写入命令码 */
	if ( len ) {  /* 有参数 */
		i = len;
		CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* 指向缓冲区 */
		buf = (unsigned char *)&mCmdParam;  /* 指向输入参数的起始地址 */
		do {
			CH375HM_DATA_WR( *buf );  /* 从索引地址PARA_BUFFER_ADDR开始,写入参数 */
			buf ++;
		} while ( -- i );
	}
	CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
	CH375HM_DATA_WR( len | PARA_CMD_BIT_ACT );  /* 向索引地址PARA_CMD_LEN_ADDR写入后续参数的长度,最高位通知模块,说明命令包已经写入,请求开始执行命令 */
	CurrentBuf = bufstart;  /* 外部RAM缓冲区起始地址,仅用于FileRead或者FileWrite命令 */
	while ( 1 ) {  /* 处理数据传输,直到操作完成才退出 */

#if 1
		while ( CH375HM_INT_WIRE );  /* 等待模块完成操作产生低电平中断,最佳检测方式是对模块的INT#信号进行下降沿边沿检测 */
#else
		do {  /* 如果不需要扇区方式读写,那么可以查询模块的命令码单元代替查询模块INT#引脚 */
			CH375HM_INDEX_WR( PARA_COMMAND_ADDR );
		} while ( CH375HM_DATA_RD( ) );  /* 模块操作完成时该值会清0,仅适用于非扇区方式读写 */
#endif

		CH375HM_INDEX_WR( PARA_STATUS_ADDR );  /* 写入索引地址 */
		status = CH375HM_DATA_RD( );  /* 从索引地址PARA_STATUS_ADDR读取中断状态 */
		CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
		CH375HM_DATA_WR( PARA_CMD_BIT_INACT );  /* 中断应答,取消来自模块的中断请求 */
/* 因为模块在收到中断应答后3uS之内才撤消中断请求,所以,如果是查询INT#信号的低电平,那么在发出中断应答后3uS之内不应该再查询INT#信号的状态
   但是由于51单片机较慢,下面的处理时间已经超过3uS,所以不必另加延时等待模块撤消中断请求 */
		if ( status == ERR_SUCCESS ) {  /* 操作成功 */
			CH375HM_INDEX_WR( PARA_STS_LEN_ADDR );
			i = CH375HM_DATA_RD( );  /* 从索引地址PARA_STS_LEN_ADDR读取返回结果数据的长度,计数 */
			if ( i ) {  /* 有结果数据 */
				CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* 指向缓冲区 */
				buf = (unsigned char *)&mCmdParam;  /* 指向输出参数的起始地址 */
				do {
					*buf = CH375HM_DATA_RD( );  /* 从索引地址PARA_BUFFER_ADDR开始,读取结果 */
					buf ++;
				} while ( -- i );
			}
//			status = ERR_SUCCESS;
			break;  /* 操作成功返回 */
		}
		else if ( status == USB_INT_DISK_READ ) {  /* 正在从U盘读数据块,请求数据读出 */
			CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* 指向缓冲区 */
			i = 64;  /* 计数 */
			do {  /* 要提高文件数据读写速度,这段程序用汇编程序效率更高,在C51中,do+while比for或者while结构效率高 */
				*CurrentBuf = CH375HM_DATA_RD( );  /* 从索引地址0到63依次读出64字节的数据 */
				CurrentBuf ++;  /* 读取的数据保存到外部缓冲区 */
			} while ( -- i );  /* 上面这一小段C程序用汇编程序效率要高近一倍 */
			CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
			CH375HM_DATA_WR( PARA_CMD_BIT_ACT );  /* 通知模块继续,说明64字节数据已经读取完成 */
		}
		else if ( status == USB_INT_DISK_WRITE ) {  /* 正在向U盘写数据块,请求数据写入 */
			CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* 指向缓冲区 */
			i = 64;  /* 计数 */
			do {
				CH375HM_DATA_WR( *CurrentBuf );  /* 向索引地址0到63依次写入64字节的数据 */
				CurrentBuf ++;  /* 写入的数据来自外部缓冲区 */
			} while ( -- i );
			CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
			CH375HM_DATA_WR( PARA_CMD_BIT_ACT );  /* 通知模块继续,说明64字节数据已经写入完成 */
		}
		else if ( status == USB_INT_DISK_RETRY ) {  /* 读写数据块失败重试,应该向回修改缓冲区指针 */
			CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* 指向缓冲区 */
			i = CH375HM_DATA_RD( );  /* 大端模式下为回改指针字节数的高8位,如果是小端模式那么接收到的是回改指针字节数的低8位 */
			status = CH375HM_DATA_RD( );  /* 大端模式下为回改指针字节数的低8位,如果是小端模式那么接收到的是回改指针字节数的高8位 */
			CurrentBuf -= ( (unsigned short)i << 8 ) + status;  /* 这是大端模式下的回改指针,对于小端模式,应该是( (unsigned short)status << 8 ) + i */
			CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
			CH375HM_DATA_WR( PARA_CMD_BIT_ACT );  /* 通知模块继续,说明重试状态码已经处理完成 */
		}
		else {  /* 操作失败 */
			if ( status == ERR_DISK_DISCON || status == ERR_USB_CONNECT ) {  /* U盘刚刚连接或者断开,应该延时几十毫秒再操作 */
				mDelaymS( 100 );
				if ( CH375HM_INT_WIRE ) break;  /* 没有中断则返回,如果仍然有中断请求说明之前的中断是U盘插拔通知中断,现在再处理命令完成中断而暂不返回 */
			}
			else break;  /* 操作失败返回 */
		}
	}
/*	while( CH375HM_INT_WIRE == 0 );  如果单片机速度很快,有可能该程序返回前模块尚未撤消中断请求,那么应该等待中断请求引脚无效 */
	return( status );
}

/* 执行命令 */
unsigned char	ExecCommand( unsigned char cmd, unsigned char len )
/* 输入命令码和输入参数长度,返回操作状态码,输入参数和返回参数都在CMD_PARAM结构中 */
{
	return( ExecCommandBuf( cmd, len, 0 ) );  /* 只有CMD_FileRead或者CMD_FileWrite命令用到输入参数bufstart,其它命令没有用到 */
}

/* 检查操作状态,如果错误则显示错误代码并停机,应该替换为实际的处理措施 */
void	mStopIfError( unsigned char iError )
{
	unsigned char	led;
	if ( iError == ERR_SUCCESS ) return;  /* 操作成功 */
	printf( "Error: %02X\n", (unsigned short)iError );  /* 显示错误 */
	led=0;
	while ( 1 ) {
		LED_OUT = led&1;  /* LED闪烁 */
		mDelaymS( 100 );
		led^=1;
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
	unsigned char	i, c, SecCount;
	unsigned long	OldSize;
	unsigned short	NewSize, count;
	LED_OUT = 0;  /* 开机后LED亮一下以示工作 */
	mDelaymS( 200 );  /* 延时100毫秒,CH375模块上电后需要100毫秒左右的复位时间 */
	mDelaymS( 250 );
	LED_OUT = 1;
	mInitSTDIO( );
	printf( "Start\n" );
/* 其它电路初始化 */
	while ( 1 ) {  /* 主循环 */
		printf( "Wait Udisk\n" );
		while ( 1 ) {  /* 使用查询方式看U盘是否连接 */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* 查询当前模块的状态 */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus >= DISK_CONNECT ) break;  /* U盘已经连接 */
			mDelaymS( 200 );  /* 可以在打算读写U盘时再查询,没有必要一直连续不停地查询,可以让单片机做其它事,没事可做就延时等待一会再查询 */
		}
		mDelaymS( 200 );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */
		LED_OUT = 0;  /* LED亮 */
/* 检查U盘是否准备好,大多数U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 5; i ++ ) {
			mDelaymS( 100 );
			printf( "Ready ?\n" );
			if ( ExecCommand( CMD_DiskReady, 0 ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
		}
/* 读取原文件 */
		printf( "Open\n" );
		memcpy( mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C", MAX_PATH_LEN );  /* 文件名,该文件在C51子目录下 */
		i = ExecCommand( CMD_FileOpen, MAX_PATH_LEN );  /* 打开文件,输入参数置为最大值,省得再计算参数长度 */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* ERR_MISS_DIR说明没有找到C51子目录,ERR_MISS_FILE说明没有找到文件 */
/* 列出根目录下的文件 */
			printf( "List file \\*\n" );
			for ( c = 0; c < 255; c ++ ) {  /* 最多搜索前255个文件 */
/*				memcpy( mCmdParam.Enumer.mPathName, "\\C51\\CH375*", MAX_PATH_LEN );*/  /* 搜索C51子目录下以CH375开头的文件名,*为通配符 */
				memcpy( mCmdParam.Enumer.mPathName, "\\*", MAX_PATH_LEN );  /* 搜索文件名,*为通配符,适用于所有文件或者子目录 */
/*				i = strlen( mCmdParam.Enumer.mPathName );*/  /* 计算文件名的长度 */
				for ( i = 0; i < MAX_PATH_LEN - 1; i ++ ) if ( mCmdParam.Enumer.mPathName[ i ] == 0 ) break;  /* 指向搜索文件名的结束符 */
				mCmdParam.Enumer.mPathName[ i ] = c;  /* 将结束符替换为搜索的序号,从0到255 */
				i = ExecCommand( CMD_FileEnumer, i+1 );  /* 枚举文件,如果文件名中含有通配符*,则为搜索文件而不打开,输入参数的长度很好计算 */
				if ( i == ERR_MISS_FILE ) break;  /* 再也搜索不到匹配的文件,已经没有匹配的文件名 */
				if ( i == ERR_SUCCESS ) {  /* 搜索到与通配符相匹配的文件名,文件名及其完整路径在命令缓冲区中 */
					printf( "  match file %03d#: %s\n", (unsigned int)c, mCmdParam.Enumer.mPathName );  /* 显示序号和搜索到的匹配文件名或者子目录名 */
					continue;  /* 继续搜索下一个匹配的文件名,下次搜索时序号会加1 */
				}
				else {  /* 出错 */
					mStopIfError( i );
					break;
				}
			}
			strcpy( DATA_BUF, "Note: \xd\n原本是打算将/C51/CH375HFT.C文件中的小写字母转成大写后写入新的文件,但是找不到这个文件\xd\n" );
			OldSize = 0;
			NewSize = strlen( DATA_BUF );  /* 新文件的长度 */
			SecCount = ( NewSize + 511 ) >> 9;  /* (NewSize+511)/512, 计算文件的扇区数,因为读写是以扇区为单位的 */
		}
		else {  /* 找到文件\C51\CH375HFT.C或者出错 */
			mStopIfError( i );
			printf( "Query\n" );
			i = ExecCommand( CMD_FileQuery, 0 );  /* 查询当前文件的信息,没有输入参数 */
			mStopIfError( i );
			printf( "Read\n" );
			OldSize = mCmdParam.Modify.mFileSize;  /* 原文件的长度 */
			if ( OldSize > (unsigned long)(64*512) ) {  /* 演示板用的62256只有32K字节 */
				SecCount = 64;  /* 由于演示板用的62256只有32K字节,所以只读取不超过64个扇区,也就是不超过32768字节 */
				NewSize = 64*512;  /* 由于RAM有限所以限制长度 */
			}
			else {  /* 如果原文件较小,那么使用原长度 */
				SecCount = ( OldSize + 511 ) >> 9;  /* (OldSize+511)/512, 计算文件的扇区数,因为读写是以扇区为单位的 */
				NewSize = (unsigned short)OldSize;  /* 原长度 */
			}
			printf( "Size=%ld, Len=%d, Sec=%d\n", OldSize, NewSize, (unsigned short)SecCount );
			mCmdParam.Read.mSectorCount = SecCount;  /* 读取全部数据,如果超过60个扇区则只读取60个扇区 */
			i = ExecCommandBuf( CMD_FileRead, 1, &DATA_BUF );  /* 从文件读取数据 */
			mStopIfError( i );
/*
			如果文件比较大,一次读不完,可以再用命令CMD_FileRead继续读取,文件指针自动向后移动
			while ( 剩余未读完 ) {
				mCmdParam.Read.mSectorCount = 32;
				ExecCommandBuf( CMD_FileRead, 1, &DATA_BUF + 已经读取的长度 );   读完后文件指针自动后移
				TotalLength += 32*512;  累计文件总长度
			}

		    如果希望从指定位置开始读写,可以移动文件指针
			mCmdParam.Locate.mSectorOffset = 3;  跳过文件的前3个扇区开始读写
			i = ExecCommand( CMD_FileLocate, 4 );  输入参数的长度4是sizeof( mCmdParam.Locate.mSectorOffset )
			mCmdParam.Read.mSectorCount = 10;
			ExecCommandBuf( CMD_FileRead, 1, &DATA_BUF );   直接读取从文件的第(512*3)个字节开始的数据,前3个扇区被跳过

			如果希望将新数据添加到原文件的尾部,可以移动文件指针
			i = ExecCommand( CMD_FileOpen, (unsigned char)( strlen( mCmdParam.Open.mPathName ) + 1 ) );
			mCmdParam.Locate.mSectorOffset = 0xffffffff;  移到文件的尾部,以扇区为单位,如果原文件是3字节,则从512字节开始添加
			i = ExecCommand( CMD_FileLocate, sizeof( mCmdParam.Locate.mSectorOffset ) );
			mCmdParam.Write.mSectorCount = 10;
			ExecCommandBuf( CMD_FileWrite, 1, &DATA_BUF );   在原文件的后面添加数据
*/
			printf( "Close\n" );
			mCmdParam.Close.mUpdateLen = 0;
			i = ExecCommand( CMD_FileClose, 1 );  /* 关闭文件 */
			mStopIfError( i );

			i = DATA_BUF[200];
			DATA_BUF[200] = 0;  /* 置字符串结束标志,最多显示200个字符 */
			printf( "Line 1: %s\n", DATA_BUF );
			DATA_BUF[200] = i;  /* 恢复原字符 */
			for ( count=0; count < NewSize; count ++ ) {  /* 将文件中的小写字符转换为大写 */
				c = DATA_BUF[ count ];
				if ( c >= 'a' && c <= 'z' ) DATA_BUF[ count ] = c - ( 'a' - 'A' );
			}
		}
/* 产生新文件 */
		printf( "Create\n" );
/*		memcpy( mCmdParam.Create.mPathName, "\\NEWFILE.TXT", MAX_PATH_LEN );*/
		memcpy( mCmdParam.Create.mPathName, "\\双击我吧.TXT", MAX_PATH_LEN );  /* 新文件名,在根目录下 */
		i = ExecCommand( CMD_FileCreate, MAX_PATH_LEN );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
		printf( "Write\n" );
		mCmdParam.Write.mSectorCount = 0x1;  /* 写入一个扇区512字节 */
		i = ExecCommandBuf( CMD_FileWrite, 1, &DATA_BUF );  /* 向文件写入数据 */
		mStopIfError( i );
		if ( SecCount > 1 ) {  /* 因为数据不超过255个扇区,所以完成能够一次写入,但是为了演示,特意分两次写入 */
			mCmdParam.Write.mSectorCount = SecCount - 1;
/*	buffer = & DATA_BUF + 512;  接着刚才的写,不必设置缓冲区的起始地址 */
			i = ExecCommandBuf( CMD_FileWrite, 1, &DATA_BUF + 512 );  /* 向文件写入数据,缓冲区接着前面写入的512字节之后 */
			mStopIfError( i );
		}
		printf( "Modify\n" );
		mCmdParam.Modify.mFileAttr = 0xff;  /* 输入参数: 新的文件属性,为0FFH则不修改 */
		mCmdParam.Modify.mFileTime = 0xffff;  /* 输入参数: 新的文件时间,为0FFFFH则不修改,使用新建文件产生的默认时间 */
		mCmdParam.Modify.mFileDate = ( (2004-1980)<<9 ) + ( 5<<5 ) + 18;  /* 输入参数: 新的文件日期: 2004.05.18 */
		mCmdParam.Modify.mFileSize = NewSize;  /* 输入参数: 如果原文件较小,那么新的文件长度与原文件一样长,否则被RAM所限 */
		i = ExecCommand( CMD_FileModify, 4+2+2+1 );  /* 修改当前文件的信息,修改日期和长度,参数长度为sizeof(mCmdParam.Modify.mFileSize)+... */
		mStopIfError( i );
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 0;  /* 不要自动计算文件长度,如果自动计算,那么该长度总是512的倍数 */
		i = ExecCommand( CMD_FileClose, 1 );
		mStopIfError( i );

/* 删除某文件 */
		printf( "Erase\n" );
		memcpy( mCmdParam.Create.mPathName, "\\OLD", MAX_PATH_LEN );  /* 将被删除的文件名,在根目录下 */
		i = ExecCommand( CMD_FileErase, MAX_PATH_LEN );  /* 删除文件并关闭 */
/*		mStopIfError( i );*/

/* 查询磁盘信息 */
		printf( "Disk\n" );
		i = ExecCommand( CMD_DiskQuery, 0 );
		mStopIfError( i );
		i = mCmdParam.Query.mDiskFat;
		if ( i == 1 ) i = 12;
		else if ( i == 2 ) i = 16;
		else if ( i == 3 ) i = 32;
		printf( "FatCode=FAT%d, TotalSector=%ld, FreeSector=%ld\n", (unsigned short)i, mCmdParam.Query.mTotalSector, mCmdParam.Query.mFreeSector );
/* 等待U盘断开 */
		printf( "Take_out\n" );
		while ( 1 ) {  /* 使用查询方式看U盘是否断开 */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* 查询当前模块的状态 */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus <= DISK_DISCONNECT ) break;  /* U盘已经断开 */
			mDelaymS( 200 );  /* 没有必要一直连续不停地查询,可以让单片机做其它事,没事可做就延时等待一会再查询 */
		}
		LED_OUT = 1;  /* LED灭 */
	}
}
