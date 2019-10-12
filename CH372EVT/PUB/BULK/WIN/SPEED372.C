// 2003.09.08, 2003.12.28, 2005.02.23
//****************************************
//**  Copyright  (C)  W.ch  1999-2004   **
//**  Web:  http://www.winchiphead.com  **
//****************************************
//**  DLL for USB interface chip CH375  **
//**  C, VC5.0                          **
//****************************************
//
// USB总线接口芯片CH375的数据块测试程序 V1.2
// 南京沁恒电子有限公司  作者: W.ch 2003.12
// CH375-BLK V1.2 for speed testing
// 运行环境: Windows 98/ME, Windows 2000/XP
// support USB chip: CH372/CH375
//

// 测试CH372或者CH375与计算机进行USB连接时的数据传输速度
// 在Windows XP SP2下, CH372A或者CH375A的下传速度约为341K字节/秒, 缓冲上传模式下上传速度约为447K字节/秒, 直接上传模式下上传速度约为372K字节/秒
// 不同的操作系统或者不同的单片机速度都会影响速度测试结果

// 本程序使用缓冲上传模式, 特点是, 由驱动程序自动接收上传数据保存在内部缓冲区中, 供应用程序随时读取, 传输速度快, 适用于数据流方式, 不宜用于WINDOWS 98/ME
// 在缓冲上传模式下, 应用程序调用CH375ReadData后将会立即返回, 如果内部缓冲区中没有数据则返回0长度数据, 可以随时调用CH375QueryBufUpload查询缓冲区中是否有数据

// 默认工作于直接上传模式, 需要应用程序调用CH375SetBufUpload启用缓冲上传模式
// 使用直接上传模式, 特点是, 只在应用程序需要时调用API上传数据, 上传数据容易同步, 适用于请求加应答方式
// 在直接上传模式下, 应用程序调用CH375ReadData后只在等接收到所需要的数据才会返回, 如果暂时没有上传数据那么应用程序将挂起, 除非设置读超时而超时返回


#include	<windows.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<conio.h>
#include	<winioctl.h>

#include	"CH375DLL.H"				// CH375的动态链接库

// 我们在计算机和单片机的应用程序之间约定下传数据的首字节是命令码
#define		TEST_START			0x20	// 测试过程开始
#define		TEST_DATA			0x21	// 测试数据正确性
#define		TEST_UPLOAD			0x22	// 测试上传数据块
#define		TEST_DOWNLOAD		0x23	// 测试下传数据块


//程序入口
void main ( )
{
	unsigned long	index=0;  // 0#设备
	unsigned char	mBuffer[4100];
	unsigned char	mReadBuf[100];
	unsigned long	i, mLength, mTestCount, mErrCnt, mStep, mTotal;
	double          speed;

	printf( "\nCH372/CH375 Bulk Data Test Program V1.2 ,   Copyright (C) W.ch 2004.12\n" );
	printf( "test data correctness and USB speed, use buffer upload mode\n" );

// 需要使用DLL则需要先加载
	printf( "*** Load DLL: CH375DLL.DLL \n" );
	if ( LoadLibrary( "CH375DLL.DLL" ) == NULL ) return;  // 加载DLL失败,可能未安装到系统中

	printf( "*** CH375OpenDevice: 0# \n" );
	if ( CH375OpenDevice( index ) == INVALID_HANDLE_VALUE ) return;  /* 使用之前必须打开设备 */

	CH375SetTimeout( index, 2000, 2000 );  // 设置USB数据读写超时,超过2000mS未完成读写将强制返回,避免一直等待下去

	CH375SetBufUpload( index, 1 );  // 使用缓冲上传模式, 通知驱动程序启用内部线程对USB上传数据进行缓冲

	mErrCnt=0;

	printf( "*** Step-0: notice MCU start test \n" );
	mBuffer[0]=TEST_START;  // 我们在计算机和单片机的应用程序之间约定下传数据的首字节是命令码
	mTotal=1;
	if ( CH375WriteData( index, &mBuffer, &mTotal ) )  // 发送成功
	{
		printf( "start test now\n" );
	}
	else {  // 写操作失败
		mErrCnt++;
		printf( "S0-T0 CH375WriteData return error, length=1\n" );
	}
	Sleep(200);
	mBuffer[0]=TEST_START;
	mTotal=1;
	if ( CH375WriteData( index, &mBuffer, &mTotal ) )  // 发送成功
	{
	}
	else {  // 写操作失败
		mErrCnt++;
		printf( "S0-T1 CH375WriteData return error, length=1\n" );
	}
	Sleep(200);

	CH375SetBufUpload( index, 1 );  // 这里调用是为了清空内部上传缓冲区,对于缓冲上传,应该在正式上传数据之前刷新USB接收缓冲区,否则缓冲区中可能会有之前的无效数据

	printf( "*** Step-1: test data correctness: 1000 times, random length and data\n" );
	for ( mTestCount=0; mTestCount<1000; ++mTestCount )  // 循环测试
	{
		mStep=mTestCount&0x03;
		switch( mStep )
		{
			case 0: memset( mBuffer, 0x00, 64 );  // 发送全00H/返回全FFH
					break;
			case 1: memset( mBuffer, 0xff, 64 );  // 发送全FFH/返回全00H
					break;
			default: for ( i=0; i<64; i++ ) mBuffer[i]=(unsigned char)(rand());  // 发送随机数/返回位反码
					break;
		}
		mBuffer[0]=TEST_DATA;
		mTotal=rand();  // 随机数长度
		mTotal=mTotal%100;
		if ( mTotal == 0 || mTotal > 64 ) mTotal=(mTotal&0x01)?64:63;
		printf( "Cnt=%4ld, Len=%2ld, Data: %02XH,%02XH,%02XH,%02XH...\xd", mTestCount, mTotal, mBuffer[0], mBuffer[1], mBuffer[2], mBuffer[3] );
		if ( kbhit() )
		{
			if ( getch() == 0x20 ) {  // 终止循环
				printf( "*** CH375CloseDevice by break: 0              \n" );
				CH375SetTimeout( index, 0xffffffff, 0xffffffff );  // 取消超时
				CH375SetBufUpload( index, 0 );  // 恢复直接上传模式
				CH375CloseDevice( index );
				exit(2);
				break;
			}
		}
		if ( CH375WriteData( index, &mBuffer, &mTotal ) )  // 发送成功
		{
RetryGetData:
			mLength = 64;
			if ( CH375ReadData( index, &mReadBuf, &mLength ) )  // 接收成功,实际是到缓冲区中取数据
			{	// 对于缓冲上传方式,CH375ReadData总是立即返回,所以要严格检查返回长度是否满足
				if ( mLength == 0 ) {  // 在缓冲区中无数据时有可能返回长度为0,只能等待一会后再查
					Sleep(20);  // 等待上传数据,或者做其它事
					goto RetryGetData;  // 尚未读入数据,继续等待
				}
				if ( mLength != mTotal || mLength==0 ) {
					mErrCnt++;
					printf( "S1-T%0ld-C%ld return length error: %ld (%ld)\n", mStep, mTestCount, mLength, mTotal );
				}
				else {
					for ( i=0; i<mLength; ++i ) {
						if ( (unsigned char)(mReadBuf[i]^0xff)!=mBuffer[i] ) {
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
	Sleep(100);

	printf( "*** Step-2: test speed of download data: 2048KB data \n" );
	for ( i=0; i<4096; i+=4 ) *(unsigned long *)(&mBuffer[i])=rand();  // 产生随机数以发送
	for ( i=0; i<4096; i+=64 ) mBuffer[i]=TEST_DOWNLOAD;  // 每64字节为一个数据包,首字节为命令码
	mTotal=0;
	mStep=GetTickCount();
	for ( mTestCount=0; mTestCount<500; ++mTestCount )  // 循环测试
	{
		*(unsigned long *)(&mBuffer[4])=mTestCount;
		mLength = 4096;
		if ( CH375WriteData( index, &mBuffer, &mLength ) )  // 发送成功
		{
			if ( mLength != 4096 ) printf( "S2-C%ld CH375WriteData actual length short %ld (4096)\n", mTestCount, mLength );
			mTotal +=mLength;  // 累计长度
		}
		else {  // 写操作失败
			mErrCnt++;
			printf( "S2-C%ld CH375WriteData return error\n", mTestCount );
		}
	}
	mLength=GetTickCount();
	mLength=mLength-mStep;
	speed=1000;
	if ( mLength !=0 ) speed=speed*mTotal/mLength;
	else speed=9999999;
	printf( "*** download speed = %7.1f Bytes/Sec, total=%ld bytes, time=%ld mS\n", speed, mTotal, mLength);
	Sleep(100);

	printf( "*** Step-3: test speed of upload data: 2048KB data\n" );
	mBuffer[0]=TEST_UPLOAD;
	mLength = 1;
	if ( CH375WriteData( index, &mBuffer, &mLength ) )  // 发送成功
	{
		mTotal=0;
		mStep=GetTickCount();
		for ( mTestCount=0; mTotal < 500 * 4096L; ++mTestCount )  // 循环测试
		{
			mLength = 4096;  // 对于缓冲上传方式,可以是0到4096之间的任意以64为倍数的长度,长度基本不影响速度;对于直接上传方式,可以是0到4096之间的任意以64为倍数的长度,长度大则速度快
			if ( CH375ReadData( index, &mBuffer, &mLength ) )  // 接收成功
			{  // 对于缓冲上传方式,CH375ReadData总是立即返回,所以要严格检查返回长度是否满足
				if ( mLength != 4096 ) {  // 读出数据长度不足所请求的长度,说明内部缓冲区不满,可以延时后再读
					Sleep(10);  // 等待上传数据,或者做其它事
					if ( mLength > 4096 ) printf( "S3-C%ld CH375ReadData actual length long %ld (4096)\n", mTestCount, mLength );
				}
				mTotal +=mLength;  // 累计长度
			}
			else {  // 读操作失败
				mErrCnt++;
				printf( "S3-C%ld CH375ReadData return error\n", mTestCount );
			}
		}
		mLength=GetTickCount();
		mLength=mLength-mStep;
		speed=1000;
		if ( mLength !=0 ) speed=speed*mTotal/mLength;
		else speed=9999999;
		printf( "*** upload speed = %7.1f Bytes/Sec, total=%ld bytes, time=%ld mS, %d\n", speed, mTotal, mLength, mTestCount);
	}
	else {  // 写操作失败
		mErrCnt++;
		printf( "S3 CH375WriteData return error\n" );
	}
	Sleep(100);

// 关闭CH375设备,如果不继续使用则必须关闭设备,就象读写完硬盘中的数据文件后要关闭一样
	printf( "*** Total error = %ld \n", mErrCnt );
	printf( "*** CH375CloseDevice: 0 \n" );
	CH375SetTimeout( index, 0xffffffff, 0xffffffff );  // 取消超时
	CH375SetBufUpload( index, 0 );  // 恢复直接上传模式
	CH375CloseDevice( index );

	printf( "\nExit.\n" );
	getch();
}
