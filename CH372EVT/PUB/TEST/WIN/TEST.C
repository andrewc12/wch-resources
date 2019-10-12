// 2003.09.08, 2003.12.28
//****************************************
//**  Copyright  (C)  W.ch  1999-2005   **
//**  Web:  http://www.winchiphead.com  **
//****************************************
//**  DLL for USB interface chip CH375  **
//**  C, VC5.0                          **
//****************************************
//
// USB总线接口芯片CH375的数据块测试程序 V1.0
// 南京沁恒电子有限公司  作者: W.ch 2003.12
// CH375-BLK  V1.0
// 运行环境: Windows 98/ME, Windows 2000/XP
// support USB chip: CH372/CH375
//

// 本程序测试数据传输的正确性,可以用于长时间连续测试,对应的单片机端的测试程序为TEST.C
// 方法: 下传随机长度的随机数据包,被单片机接收并将数据按位取反后返回,最终由计算机程序接收后比较数据是否正确


#include	<windows.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<conio.h>
#include	<winioctl.h>

#include	"CH375DLL.H"				// CH375的动态链接库

// 我们在计算机和单片机的应用程序之间约定下传数据的首字节是命令码
#define		TEST_DATA			0x21	// 测试数据正确性


//程序入口
void main ( )
{
	int				key;
	unsigned char	mBuffer[100];
	unsigned char	mReadBuf[100];
	unsigned long	i, mLength, mTestCount, mErrCnt, mStep, mTotal;

	printf( "\nCH372/CH375 Bulk Data Test Program V1.1 ,   Copyright (C) W.ch 2004.12\n" );
	printf( "test data correctness \n" );

// 需要使用DLL则需要先加载,没有此句则会自动加载
	printf( "*** Load DLL: CH375DLL.DLL \n" );
	if ( LoadLibrary( "CH375DLL.DLL" ) == NULL ) return;  // 加载DLL失败,可能未安装到系统中

	printf( "*** CH375OpenDevice: 0# \n" );
	if ( CH375OpenDevice( 0 ) == INVALID_HANDLE_VALUE ) return;  /* 使用之前必须打开设备 */

	CH375SetTimeout( 0, 5000, 5000 );  // 设置USB数据读写的超时,超过5000mS未完成读写将强制返回,避免一直等待下去

	mErrCnt=0;

	printf( "*** Step-1: test data correctness: 10000000 times, random length and data\n" );
	for ( mTestCount=0; mTestCount<10000000; ++mTestCount )  // 循环测试
	{
		mStep=mTestCount&0x03;
		switch( mStep )
		{
			case 0: memset( mBuffer, 0x00, 64 );  // 发送全00H/返回全FFH
					break;
			case 1: memset( mBuffer, 0xff, 64 );  // 发送全FFH/返回全00H
					break;
			default: for ( i=0; i<64; i++ ) mBuffer[i]=(unsigned char)( rand() );  // 发送随机数/返回位反码
					break;
		}
		mBuffer[0]=TEST_DATA;
		mTotal=rand();  // 随机数长度
		mTotal=mTotal%100;
		if ( mTotal == 0 || mTotal > 64 ) mTotal=64;
		printf( "Cnt=%4ld, Len=%2ld, Data: %02XH,%02XH,%02XH,%02XH,%02XH,%02XH...\xd", mTestCount, mTotal, mBuffer[0], mBuffer[1], mBuffer[2], mBuffer[3], mBuffer[4], mBuffer[5] );
		if ( kbhit() )
		{
			key = getch( );
			if ( key == 0x1b || key == 0x20 ) {  // ESC键或者空格键终止循环
				printf( "*** CH375CloseDevice by break: 0              \n" );
				CH375CloseDevice( 0 );
				exit(2);
				break;
			}
		}
		if ( CH375WriteData( 0, mBuffer, &mTotal ) )  // 发送成功
		{
			mLength = 64;
			if ( mTestCount == 0 ) Sleep( 200 );  // 考虑到之前单片机准备上传的数据可能未被计算机取走,导致首次回传有可能直接读到之前的数据而不是本次数据的取反,所以首次回传先等待单片机准备好取反数据
			if ( CH375ReadData( 0, mReadBuf, &mLength ) )  // 接收成功
			{
				if ( mLength != mTotal || mLength==0 ) {  // 长度错误
					mErrCnt++;
					printf( "S1-T%0ld-C%ld return length error: %ld (%ld)\n", mStep, mTestCount, mLength, mTotal );
				}
				else {
					for ( i=0; i<mLength; ++i ) {
						if ( (mReadBuf[i]^0xff)!=mBuffer[i] ) {  // 取反值比较错误
							mErrCnt++;
							printf( "S1-T%0ld-C%ld return data error at %ld: %02XH (%02XH)\n", mStep, mTestCount, i, mReadBuf[i], mBuffer[i] );
						}
					}
				}
			}
			else {  // 读操作失败
				mErrCnt++;
				printf( "S1-T%0ld-C%ld CH375ReadData return error, length=%ld\n", mStep, mTestCount, mTotal );
			}
		}
		else {  // 写操作失败
			mErrCnt++;
			printf( "S1-T%0ld-C%ld CH375WriteData return error, length=%ld\n", mStep, mTestCount, mTotal );
		}
	}
	if ( mErrCnt==0 ) printf( "*** passed                                             \n" );
// 关闭CH375设备,如果不继续使用则必须关闭设备,就象读写完硬盘中的数据文件后要关闭一样
	printf( "*** Total error = %ld \n", mErrCnt );
	printf( "*** CH375CloseDevice: 0 \n" );
	CH375CloseDevice( 0 );

	printf( "\nExit.\n" );
	getch();
}
