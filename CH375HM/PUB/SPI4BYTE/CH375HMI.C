/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Module      @CH375  **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* U盘文件读写模块, 连接方式: 软件模拟SPI时序+查询 */
/* MCS-51单片机C语言示例程序 */
/* 因为使用U盘文件读写模块而不是使用U盘文件级子程序库,所以占用较少的单片机资源,可以使用89C51单片机测试 */
/* 以字节为单位进行U盘文件读写,单片机的RAM只需要几十个字节,不需要外部RAM */

#include <reg51.h>
#include <absacc.h>
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN			32		/* 最大路径长度,含所有斜杠分隔符和小数点间隔符以及路径结束符00H,CH375模块支持的最大值是62,最小值是13 */
#include "..\CH375HM.H"

/* 电路连接方式,4线SPI,除SCS外均可共用SPI总线
   单片机    模块
    P1.0  =  SDI
    P1.1  =  SDO
    P1.2  =  SCK
    P1.3  =  SCS
    悬空  =  INT#  本程序通过SPI操作查询模块的INT#的状态
*/
sbit	P10					=	P1^0;
sbit	P11					=	P1^1;
sbit	P12					=	P1^2;
sbit	P13					=	P1^3;
#define	CH375HM_SPI_SDI			P10		/* 假定CH375模块的SDI引脚连接到单片机的P10引脚 */
#define	CH375HM_SPI_SDO			P11		/* 假定CH375模块的SDO引脚连接到单片机的P11引脚 */
#define	CH375HM_SPI_SCK			P12		/* 假定CH375模块的SCK引脚连接到单片机的P12引脚 */
#define	CH375HM_SPI_SCS			P13		/* 假定CH375模块的SCS引脚连接到单片机的P13引脚 */

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

/* 发送一个字节数据给CH375模块,同时接收一个字节,以SPI模式0方式(SPI_SCK默认为0) */
unsigned char	mSpiExchange( unsigned char c )
{
	unsigned char	i, d;
	d = 0;
	CH375HM_SPI_SCK = 0;
	for ( i = 8; i != 0; i -- ) {  /* 8个位 */
		if ( c & 0x80 ) CH375HM_SPI_SDI = 1;  /* 向模块的SDI输入引脚输出数据 */
		else CH375HM_SPI_SDI = 0;
		d <<= 1;
		if ( CH375HM_SPI_SDO ) d ++;  /* 如果模块的SDO输出引脚为高电平则输入位1 */
		CH375HM_SPI_SCK = 1;  /* SPI时钟上升沿，模块接收数据并输出数据 */
		c <<= 1;
		CH375HM_SPI_SCK = 0;
	}
/* 如果单片机较快使该mSpiExchange子程序执行时间短于10uS，或者是大于1MHz的硬件SPI接口，那么此处应该加延时确保该子程序调用周期大于10uS */
/* 以上数据适用于模块单片机晶振为18.432MHz的情况，如果模块单片机晶振频率加倍，那么上述时间要求应该减少一半 */
/* 普通MCS51单片机以软件模拟的SPI接口较慢，无需任何延时 */
	return( d );
}

#if 0
/* 发送一个字节数据给CH375模块,同时接收一个字节,以SPI模式3方式(SPI_SCK默认为1) */
unsigned char	mSpiExchange3( unsigned char c )
{
	unsigned char	i, d;
	d = 0;
	for ( i = 8; i != 0; i -- ) {   /* 8个位 */
		CH375HM_SPI_SCK = 0;
		if ( c & 0x80 ) CH375HM_SPI_SDI = 1;  /* 向模块的SDI输入引脚输出数据 */
		else CH375HM_SPI_SDI = 0;
		d <<= 1;
		if ( CH375HM_SPI_SDO ) d ++;  /* 如果模块的SDO输出引脚为高电平则输入位1 */
		CH375HM_SPI_SCK = 1;  /* SPI时钟上升沿，模块接收数据并输出数据 */
		c <<= 1;
	}
/* 时间要求同上面mSpiExchange子程序SPI模式0 */
	return( d );
}
#endif

/* 执行命令 */
unsigned char	ExecCommand( unsigned char cmd, unsigned char len )
/* 输入命令码和输入参数长度,返回操作状态码,输入参数和返回参数都在CMD_PARAM结构中 */
{
	unsigned char		i, j, status;
	CH375HM_SPI_SCS = 0;  /* 产生SPI片选 */
	mSpiExchange( cmd );  /* 写入命令码 */
	mSpiExchange( len );  /* 写入后续参数的长度 */
	if ( len ) {  /* 有参数 */
		for ( i = 0; i != len; i ++ ) mSpiExchange( mCmdParam.Other.mBuffer[ i ] );  /* 依次写入参数 */
	}
	CH375HM_SPI_SCS = 1;  /* 结束SPI片选 */
	mDelaymS( 1 );  /* 延时不小于5uS即可 */
	while ( 1 ) {  /* 处理数据传输,直到操作完成才退出 */
//		while ( CH375HM_INT_WIRE );  /* 等待模块完成操作产生低电平中断,最佳检测方式是对模块的INT#信号进行下降沿边沿检测 */
		CH375HM_SPI_SCS = 0;  /* 产生SPI片选 */
		status = mSpiExchange( 0xFF );  /* 写入0xFF作为无效命令码(不应该写其它值),返回模块操作状态 */
		if ( status == 0xFF ) {  /* 模块操作尚未完成,也就是INT#没有中断产生 */
			CH375HM_SPI_SCS = 1;  /* 结束SPI片选 */
			mDelaymS( 1 );
			continue;  /* 继续等待模块完成操作 */
		}
		if ( status == ERR_SUCCESS ) {  /* 操作成功 */
			i = mSpiExchange( 0 );  /* 返回结果数据的长度,写入0没有意义,可以是任何值 */
			if ( i ) {  /* 有结果数据 */
				j = 0;
				do {  /* 使用do+while结构是因为其效率高于for */
					mCmdParam.Other.mBuffer[ j ] = mSpiExchange( 0 );  /* 接收结果数据并保存到参数结构中,写入0没有意义 */
					j ++;
				} while ( -- i );
			}
			CH375HM_SPI_SCS = 1;  /* 结束SPI片选 */
			break;  /* 操作成功返回 */
		}
#if 0
/* 如果需要以扇区为单位读写文件，就必须处理USB_INT_DISK_READ/USB_INT_DISK_WRITE，可参考SPI5扇区读写例子中ExecCommand子程序 */
		else if ( status == USB_INT_DISK_READ ) {  /* 正在从U盘读数据块,请求数据读出 */
			i = 64;
			do {
				*buffer = mSpiExchange( 0 );  /* 依次接收64字节的数据 */
				buffer ++;  /* 接收的数据保存到外部缓冲区 */
			} while ( -- i );
		}
		else if ( status == USB_INT_DISK_WRITE ) {  /* 正在向U盘写数据块,请求数据写入 */
			i = 64;
			do {
				mSpiExchange( *buffer );  /* 依次发送64字节的数据 */
				buffer ++;  /* 发送的数据来自外部缓冲区 */
			} while ( -- i );
		}
		else if ( status == USB_INT_DISK_RETRY ) {  /* 读写数据块失败重试,应该向回修改缓冲区指针 */
			i = mSpiExchange( 0 );  /* 大端模式下为回改指针字节数的高8位,如果是小端模式那么接收到的是回改指针字节数的低8位 */
			status = mSpiExchange( 0 );  /* 大端模式下为回改指针字节数的低8位,如果是小端模式那么接收到的是回改指针字节数的高8位 */
			buffer -= ( (unsigned short)i << 8 ) + status;  /* 这是大端模式下的回改指针,对于小端模式,应该是( (unsigned short)status << 8 ) + i */
		}
#endif
		else {  /* 操作失败 */
			CH375HM_SPI_SCS = 1;  /* 结束SPI片选 */
			if ( status == ERR_DISK_DISCON || status == ERR_USB_CONNECT ) mDelaymS( 100 );  /* U盘刚刚连接或者断开,应该延时几十毫秒再操作 */
			break;  /* 操作失败返回 */
		}
		CH375HM_SPI_SCS = 1;  /* 结束SPI片选 */
	}
	return( status );
}

/* 检查操作状态,如果错误则显示错误代码并停机 */
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
	unsigned char	i;
	unsigned short	count;
	unsigned char	*pStr;
	LED_OUT = 0;  /* 开机后LED亮一下以示工作 */
	mDelaymS( 250 );  /* 延时500毫秒,CH375模块上电后需要500毫秒左右的复位时间 */
	mDelaymS( 250 );
	LED_OUT = 1;
	mInitSTDIO( );
	printf( "Start\n" );
	while ( 1 ) {  /* 主循环 */
		printf( "Wait\n" );
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
			printf( "Ready ?\n" );
			if ( ExecCommand( CMD_DiskReady, 0 ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
		}
/* 读取原文件 */
		printf( "Open\n" );
		strcpy( mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C" );  /* 文件名,该文件在C51子目录下 */
		i = ExecCommand( CMD_FileOpen, MAX_PATH_LEN );  /* 打开文件,输入参数置为最大值,省得再计算参数长度 */
		TempLength = 0;
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* ERR_MISS_DIR说明没有找到C51子目录,ERR_MISS_FILE说明没有找到文件 */
			printf( "找不到原文件/C51/CH375HFT.C\n" );
		}
		else {  /* 找到文件\C51\CH375HFT.C或者出错 */
			mStopIfError( i );
			mCmdParam.ByteRead.mByteCount = 6;  /* 请求读出6字节数据, 单次读写的长度不能超过 sizeof( mCmdParam.ByteWrite.mByteBuffer ) */
			i = ExecCommand( CMD_ByteRead, 1 );  /* 以字节为单位读取数据 */
			mStopIfError( i );
			printf( "从文件中读出的前6个字符是[" );
			for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );
			printf( "]\n" );
			if ( mCmdParam.ByteRead.mByteCount<6 ) printf( "已经到文件的末尾\n" );
			if ( mCmdParam.ByteRead.mByteCount==6 ) {  /* 未到文件末尾 */
				mCmdParam.ByteRead.mByteCount = 20;  /* 请求再读出20字节数据, 单次读写的长度不能超过 sizeof( mCmdParam.ByteWrite.mByteBuffer ) */
				i = ExecCommand( CMD_ByteRead, 1 );  /* 以字节为单位读取数据,接着刚才的向后读 */
				mStopIfError( i );
				TempLength = mCmdParam.ByteRead.mByteCount;  /* 第二次读出字节数 */
				memcpy( TempBuffer, mCmdParam.ByteRead.mByteBuffer, TempLength );  /* 暂存第二次读出的数据以便写入新文件中 */
				printf( "从文件中读出的第6个字符开始依次是[" );
				for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );
				printf( "]\n" );
				if ( mCmdParam.ByteRead.mByteCount<20 ) printf( "已经到文件的末尾\n" );
			}
			printf( "Close\n" );
			mCmdParam.Close.mUpdateLen = 0;
			i = ExecCommand( CMD_FileClose, 1 );  /* 关闭文件 */
			mStopIfError( i );
		}
/* 产生新文件 */
		printf( "Create\n" );
/*		strcpy( mCmdParam.Create.mPathName, "\\NEWFILE.TXT" );*/
		strcpy( mCmdParam.Create.mPathName, "\\双击我吧.TXT" );  /* 新文件名,在根目录下 */
		i = ExecCommand( CMD_FileCreate, MAX_PATH_LEN );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
		printf( "ByteLocate\n" );
//		mCmdParam.ByteLocate.mByteOffset = 0;  /* 移动到文件头,用于重新回到文件头,以便写入数据覆盖原数据 */
//		ExecCommand( CMD_ByteLocate, 4 );  /* 以字节为单位移动文件指针 */
//		mCmdParam.ByteLocate.mByteOffset = 0xFFFFFFFF;  /* 移动到文件尾,用于在CMD_FileOpen打开文件后,继续追加数据到已打开文件的末尾 */
//		ExecCommand( CMD_ByteLocate, 4 );  /* 以字节为单位移动文件指针 */
		printf( "Write\n" );
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
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 1;  /* 自动计算文件长度,当以字节为单位向文件写入数据后,如果没有用0长度的CMD_ByteWrite更新文件长度,那么可以在关闭文件时让模块自动更新文件长度 */
		i = ExecCommand( CMD_FileClose, 1 );  /* 关闭文件,当以字节为单位向文件写入(追加)数据后,必须在用完文件后关闭文件 */
		mStopIfError( i );

/* 等待U盘断开 */
		printf( "Take_out\n" );
		while ( 1 ) {  /* 使用查询方式看U盘是否断开 */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* 查询当前模块的状态 */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus <= DISK_DISCONNECT ) break;  /* U盘已经断开 */
			mDelaymS( 100 );  /* 没有必要一直连续不停地查询,可以让单片机做其它事,没事可做就延时等待一会再查询 */
		}
		LED_OUT = 1;  /* LED灭 */
	}
}
