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
/* 该程序将U盘中的/C51/CH375HFT.C文件中的前200个字符显示出来,
   如果找不到原文件CH375HFT.C, 那么该程序将显示C51子目录下所有以CH375开头的文件名,
   如果找不到C51子目录, 那么该程序将显示根目录下的所有文件名,
   最后将程序ROM中的一个字符串写入写入新建的文件"NEWFILE.TXT"中
*/
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR复制",兼容性最好但是速度最慢,
   本例只使用512字节的外部RAM, 同时作为磁盘数据缓冲区和文件数据缓冲区, 演示没有外部RAM但是单片机的内置RAM大于768字节的应用 */

/* 本例以字节为单位读写U盘文件,读写速度较扇区模式慢,但是由于字节模式读写文件不需要文件数据缓冲区FILE_DATA_BUF,
   所以总共只需要600字节的RAM,适用于单片机硬件资源有限、数据量小并且读写速度要求不高的系统 */

/*#define 	NO_DEFAULT_CH375_INT		1*/	/* 在应用程序中定义NO_DEFAULT_CH375_INT可以禁止默认的中断处理程序,然后用自行编写的程序代替它 */
/*#define		CH375HF_NO_CODE		1*/
#include "CH375.H"

#ifdef	NO_DEFAULT_CH375_INT			/* 自行编写中断处理程序,加上了超时处理,并且在等待中断的过程中可以做其它事 */
void xQueryInterrupt( void )			/* 查询CH375中断并更新中断状态,该程序基本框架可以参考CH375HF?.H文件 */
{
	UINT16	i;
	for ( i = 65535; i != 0; i -- ) {  /* 正常情况下该过程为几毫秒到几十毫秒,偶尔也会达到几百毫秒 */
		if ( CH375_INT_WIRE == 0 ) break;  /* 如果CH375的中断引脚输出低电平则说明CH375操作完成 */
/*		if ( ( CH375_CMD_PORT & 0x80 ) == 0 ) break;  对于CH375B芯片,也查询CH375B的命令端口的位7为0说明中断引脚输出低电平 */
		在等待CH375中断的过程中,可以做些需要及时处理的事情
	}
	if ( i == 0 ) CH375超时,通常是硬件问题;
	CH375_CMD_PORT = CMD_GET_STATUS;  /* 获取当前中断状态 */
	mDelay2uS( );  /* 操作无意义,用于至少延时2uS,可以用多个NOP空操作指令实现 */
	CH375IntStatus = CH375_DAT_PORT;  /* 获取中断状态 */
	if ( CH375IntStatus == USB_INT_DISCONNECT ) CH375DiskStatus = DISK_DISCONNECT;  /* 检测到USB设备断开事件 */
	else if ( CH375IntStatus == USB_INT_CONNECT ) CH375DiskStatus = DISK_CONNECT;  /* 检测到USB设备连接事件 */
}
#endif

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
		LED_UDISK_IN( );  /* LED闪烁 */
		mDelaymS( 100 );
		LED_UDISK_OUT( );
		mDelaymS( 100 );
	}
}

void host( ) {
	UINT8	i, c, TotalCount;
	UINT8	code *pCodeStr;
	UINT16	EnumCount;

#if DISK_BASE_BUF_LEN == 0
	pDISK_BASE_BUF = &my_buffer[0];  /* 不在.H文件中定义CH375的专用缓冲区,而是用缓冲区指针指向其它应用程序的缓冲区便于合用以节约RAM */
#endif

	i = CH375LibInit( );  /* 初始化CH375程序库和CH375芯片,操作成功返回0 */
	mStopIfError( i );
	while ( 1 ) {
		printf( "Insert USB disk\n" );
		while ( CH375DiskStatus < DISK_CONNECT ) {  /* 等待U盘插入 */
			if ( IsKeyPress( ) ) {  /* 有键按下 */
				printf( "Exit USB host mode\n" );
				return;
			}
/*			if ( CH375_INT_WIRE == 0 ) xQueryInterrupt( );*/  /* 如果CH375中断,那么查询CH375中断并更新中断状态,可以改成中断方式 */
			mDelaymS( 100 );  /* 没必要频繁查询 */
			if ( CH375DiskConnect( ) == ERR_SUCCESS ) break;	/* 查询方式: 检查磁盘是否连接,返回成功说明连接 */
		}
		LED_UDISK_IN( );  /* LED亮 */
		mDelaymS( 250 );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */

/* 检查U盘是否准备好,但是某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 5; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
			mDelaymS( 100 );
			printf( "Ready ?\n" );
//			if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好,不支持CH375S,节约代码空间 */
			if ( CH375sDiskReady( ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好,支持CH375S和CH375A,但占用更多的代码空间 */
		}

#if DISK_BASE_BUF_LEN
		if ( DISK_BASE_BUF_LEN < CH375vSectorSize ) {  /* 检查磁盘数据缓冲区是否足够大,CH375vSectorSize是U盘的实际扇区大小 */
			printf( "Too large sector size\n" );
			while ( CH375DiskConnect( ) == ERR_SUCCESS ) mDelaymS( 100 );
			continue;
		}
#endif
/* 读取原文件 */
		printf( "Open\n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/C51/CH375HFT.C" );  /* 文件名,该文件在C51子目录下 */
		i = CH375FileOpen( );  /* 打开文件 */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* 没有找到C51子目录,没有找到CH375HFT.C文件 */
/* 列出文件 */
			if ( i == ERR_MISS_DIR ) pCodeStr = "/*";  /* C51子目录不存在则列出根目录下的所有文件 */
			else pCodeStr = "/C51/CH375*";  /* CH375HFT.C文件不存在则列出\C51子目录下的以CH375开头的文件 */
			printf( "List file %s\n", pCodeStr );
			for ( EnumCount = 0; EnumCount < 10000; EnumCount ++ ) {  /* 最多搜索前10000个文件,实际上没有限制 */
				i = mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, pCodeStr );  /* 搜索文件名,*为通配符,适用于所有文件或者子目录 */
				mCmdParam.Open.mPathName[ i ] = 0xFF;  /* 根据字符串长度将结束符替换为搜索的序号,从0到254,如果是0xFF即255则说明搜索序号在CH375vFileSize变量中 */
				CH375vFileSize = EnumCount;  /* 指定搜索/枚举的序号 */
				i = CH375FileOpen( );  /* 打开文件,如果文件名中含有通配符*,则为搜索文件而不打开 */
/* CH375FileEnum 与 CH375FileOpen 的唯一区别是当后者返回ERR_FOUND_NAME时那么对应于前者返回ERR_SUCCESS */
				if ( i == ERR_MISS_FILE ) break;  /* 再也搜索不到匹配的文件,已经没有匹配的文件名 */
				if ( i == ERR_FOUND_NAME ) {  /* 搜索到与通配符相匹配的文件名,文件名及其完整路径在命令缓冲区中 */
					printf( "  match file %04d#: %s\n", (unsigned int)EnumCount, mCmdParam.Open.mPathName );  /* 显示序号和搜索到的匹配文件名或者子目录名 */
					continue;  /* 继续搜索下一个匹配的文件名,下次搜索时序号会加1 */
				}
				else {  /* 出错 */
					mStopIfError( i );
					break;
				}
			}
		}
		else {  /* 找到文件或者出错 */
			mStopIfError( i );
			TotalCount = 200;  /* 准备读取总长度 */
			printf( "从文件中读出的前%d个字符是:\n",(UINT16)TotalCount );
			while ( TotalCount ) {  /* 如果文件比较大,一次读不完,可以再调用CH375ByteRead继续读取,文件指针自动向后移动 */
				if ( TotalCount > MAX_BYTE_IO ) c = MAX_BYTE_IO;  /* 剩余数据较多,限制单次读写的长度不能超过 sizeof( mCmdParam.ByteRead.mByteBuffer ) */
				else c = TotalCount;  /* 最后剩余的字节数 */
				mCmdParam.ByteRead.mByteCount = c;  /* 请求读出几十字节数据 */
				i = CH375ByteRead( );  /* 以字节为单位读取数据块,单次读写的长度不能超过MAX_BYTE_IO,第二次调用时接着刚才的向后读 */
				mStopIfError( i );
				TotalCount -= mCmdParam.ByteRead.mByteCount;  /* 计数,减去当前实际已经读出的字符数 */
				for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );  /* 显示读出的字符 */
				if ( mCmdParam.ByteRead.mByteCount < c ) {  /* 实际读出的字符数少于要求读出的字符数,说明已经到文件的结尾 */
					printf( "\n" );
					printf( "文件已经结束\n" );
					break;
				}
			}
/*	    如果希望从指定位置开始读写,可以移动文件指针
		mCmdParam.ByteLocate.mByteOffset = 608;  跳过文件的前608个字节开始读写
		CH375ByteLocate( );
		mCmdParam.ByteRead.mByteCount = 5;  读取5个字节
		CH375ByteRead( );   直接读取文件的第608个字节到612个字节数据,前608个字节被跳过

	    如果希望将新数据添加到原文件的尾部,可以移动文件指针
		CH375FileOpen( );
		mCmdParam.ByteLocate.mByteOffset = 0xffffffff;  移到文件的尾部
		CH375ByteLocate( );
		mCmdParam.ByteWrite.mByteCount = 13;  写入13个字节的数据
		CH375ByteWrite( );   在原文件的后面添加数据,新加的13个字节接着原文件的尾部放置
		mCmdParam.ByteWrite.mByteCount = 2;  写入2个字节的数据
		CH375ByteWrite( );   继续在原文件的后面添加数据
		mCmdParam.ByteWrite.mByteCount = 0;  写入0个字节的数据,实际上该操作用于通知程序库更新文件长度
		CH375ByteWrite( );   写入0字节的数据,用于自动更新文件的长度,所以文件长度增加15,如果不这样做,那么执行CH375FileClose时也会自动更新文件长度
*/
			printf( "Close\n" );
			i = CH375FileClose( );  /* 关闭文件 */
			mStopIfError( i );
		}

#ifdef EN_DISK_WRITE  /* 子程序库支持写操作 */
/* 产生新文件(覆盖原文件数据),关于向原有文件中添加数据的例子请参考EXAM7和EXAM8 */
		LED_WR_NOW( );  /* 写操作 */
		printf( "Create\n" );
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/NEWFILE.TXT" );  /* 新文件名,在根目录下,中文文件名 */
		i = CH375FileCreate( );  /* 新建文件并打开,如果文件已经存在则先删除后再新建 */
		mStopIfError( i );
		printf( "Write\n" );
		pCodeStr = "Note: \xd\xa这个程序是以字节为单位进行U盘文件读写,单片机只需要有600字节的RAM\xd\xa";
		while( 1 ) {  /* 分多次写入文件数据 */
			for ( i=0; i<MAX_BYTE_IO; i++ ) {
				c = *pCodeStr;
				mCmdParam.ByteWrite.mByteBuffer[i] = c;
				if ( c == 0 ) break;  /* 源字符串结束 */
				pCodeStr++;
			}
			if ( i == 0 ) break;  /* 源字符串结束,完成写文件 */
			mCmdParam.ByteWrite.mByteCount = i;  /* 写入数据的字符数,单次读写的长度不能超过MAX_BYTE_IO,第二次调用时接着刚才的向后写 */
			i = CH375ByteWrite( );  /* 向文件写入数据 */
			mStopIfError( i );
		}
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 1;  /* 自动计算文件长度,以字节为单位写文件,建议让程序库关闭文件以便自动更新文件长度 */
		i = CH375FileClose( );
		mStopIfError( i );
		LED_NOT_WR( );
#endif

		printf( "Take out USB disk\n" );
//		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
		while ( CH375DiskStatus >= DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
			if ( CH375DiskConnect( ) != ERR_SUCCESS ) break;
			mDelaymS( 100 );
		}
		LED_UDISK_OUT( );  /* LED灭 */
		mDelaymS( 100 );
	}
}
