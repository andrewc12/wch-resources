/*
; CH375/CH372 Bulk Data Test
; U2(AT89C51) Program
; 本程序测试数据传输的正确性,可以用于长时间连续测试,对应的计算机端的测试程序为TEST.EXE
; 方法: 下传随机长度的随机数据包,被单片机接收并将数据按位取反后返回,最终由计算机程序接收后比较数据是否正确
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.09
*/

/* MCS-51单片机C语言的示例程序,用于其它单片机时一般只要修改前面几个接口子程序及硬件定义 */

#pragma NOAREGS
#include <reg52.h>
#include <string.h>
#include "..\..\MCU_IF2\C\CH375INC.H"	/* 头文件,在网上下载的CH372或者CH375评估板资料中有 */

unsigned char volatile xdata CH375_CMD_PORT _at_ 0xBDF1;		/* CH375命令端口的I/O地址 */
unsigned char volatile xdata CH375_DAT_PORT _at_ 0xBCF0;		/* CH375数据端口的I/O地址 */

/* 延时2微秒,不精确 */
void	delay2us( )
{
	unsigned char i;
	for ( i = 2; i != 0; i -- );  /* 根据单片机的时钟选择初值 */
}

/* 延时1微秒,不精确,因为MCS51单片机较慢所以实际上无需延时 */
//void	delay1us( )
//{
//	unsigned char i;
//	for ( i = 1; i != 0; i -- );
//}

/* 基本操作 */

void CH375_WR_CMD_PORT( unsigned char cmd ) {  /* 向CH375的命令端口写入命令,周期不小于4uS,如果单片机较快则延时 */
	delay2us();
	CH375_CMD_PORT=cmd;
/* ******************** 注释中是用普通I/O引脚模拟8位并口的时序,CH375_CS引脚是可选的,可以一直接GND强制片选
	CH375_D0_D7 = cmd;
	CH375_A0 = 1;  选择CH375的命令口
//	CH375_D0_D7_DIR = output;  对于标准双向I/O,请在此设置为输出方向
	CH375_RD = 1;  如果I/O默认电平是高电平,那么这是可选操作
	CH375_CS = 0;
	CH375_WR = 0;
//	CH375_CS = 0;  对于高速单片机,该指令用于延时,以便向CH375_WR产生宽度至少为80nS的低电平脉冲
	CH375_WR = 1;
	CH375_CS = 1;
	CH375_A0 = 0;
//	CH375_D0_D7_DIR = input;  对于标准双向I/O,请在此设置为输入方向
	CH375_D0_D7 = 0xFF;  对于准双向I/O,请在此设置输出全高电平
******************** */
	delay2us();
}

void CH375_WR_DAT_PORT( unsigned char dat ) {  /* 向CH375的数据端口写入数据,周期不小于1.5uS,如果单片机较快则延时 */
	CH375_DAT_PORT=dat;
/* ******************** 注释中是用普通I/O引脚模拟8位并口的时序
	CH375_D0_D7 = dat;
//	CH375_D0_D7_DIR = output;  对于标准双向I/O,请在此设置为输出方向
	CH375_CS = 0;
	CH375_WR = 0;
//	CH375_CS = 0;  对于高速单片机,该指令用于延时,以便向CH375_WR产生宽度至少为80nS的低电平脉冲
	CH375_WR = 1;
	CH375_CS = 1;
//	CH375_D0_D7_DIR = input;  对于标准双向I/O,请在此设置为输入方向
	CH375_D0_D7 = 0xFF;  对于准双向I/O,请在此设置输出全高电平
******************** */
//	delay1us();  /* 因为MCS51单片机较慢所以实际上无需延时 */
}

unsigned char CH375_RD_DAT_PORT( void ) {  /* 从CH375的数据端口读出数据,周期不小于1.5uS,如果单片机较快则延时 */
//	delay1us();  /* 因为MCS51单片机较慢所以实际上无需延时 */
/* ******************** 注释中是用普通I/O引脚模拟8位并口的时序
//	CH375_D0_D7_DIR = input;  对于标准双向I/O,请在此设置为输入方向
	CH375_D0_D7 = 0xFF;  对于准双向I/O,请在此设置输出全高电平,便于输入
	CH375_CS = 0;
	CH375_RD = 0;
//	CH375_CS = 0;  对于高速单片机,该指令用于延时,以便向CH375_RD产生宽度至少为80nS的低电平脉冲
	unsigned char dat = CH375_D0_D7;
	CH375_RD = 1;
	CH375_CS = 1;
	CH375_D0_D7 = 0xFF;  对于准双向I/O,请在此设置输出全高电平
	return( dat );
******************** */
	return( CH375_DAT_PORT );
}

/* 延时50毫秒,不精确 */
void	Delay50ms( )
{
	unsigned char i, j;
	for ( i=200; i!=0; i-- ) for ( j=250; j!=0; j-- );
}

/* CH375初始化子程序 */
void	CH375_Init( )
{
	unsigned char i;
/* 测试CH375是否正常工作,可选操作,通常不需要 */
	CH375_WR_CMD_PORT( CMD_CHECK_EXIST );  /* 测试CH375是否正常工作 */
	CH375_WR_DAT_PORT( 0x55 );  /* 写入测试数据 */
	i = ~ 0x55;  /* 返回数据应该是测试数据取反 */
	if ( CH375_RD_DAT_PORT( ) != i ) {  /* CH375不正常 */
		for ( i=80; i!=0; i-- ) {
			CH375_WR_CMD_PORT( CMD_RESET_ALL );  /* 多次重复发命令,执行硬件复位 */
			CH375_RD_DAT_PORT( );
		}
		CH375_WR_CMD_PORT( 0 );
		Delay50ms( );  /* 延时50ms */
	}
/* 设置USB工作模式, 必要操作 */
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );
	CH375_WR_DAT_PORT( 2 );  /* 设置为使用内置固件的USB设备方式 */
	for ( i=100; i!=0; i-- ) {  /* 等待操作成功,通常需要等待10uS-20uS */
		if ( CH375_RD_DAT_PORT( ) == CMD_RET_SUCCESS ) break;
	}
/*	if ( i==0 ) { CH372/CH375存在硬件错误 }; */
/* 下述启用中断,假定CH375连接在INT0 */
	IT0 = 0;  /* 置外部信号为低电平触发 */
	IE0 = 0;  /* 清中断标志 */
	EX0 = 1;  /* 允许CH375中断 */
}

/* CH375中断服务程序,使用寄存器组1 */
void	mCh375Interrupt( ) interrupt 0 using 1
{
	unsigned char InterruptStatus;
	unsigned char i, length;
	unsigned char data buffer[ 64 ];
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 获取中断状态并取消中断请求 */
	InterruptStatus = CH375_RD_DAT_PORT( );  /* 获取中断状态 */
	switch ( InterruptStatus ) {  /* 分析中断状态处理 */
		case USB_INT_EP2_OUT: {  /* 批量端点下传成功 */
			CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* 从当前USB中断的端点缓冲区读取数据块,并释放缓冲区 */
			length = CH375_RD_DAT_PORT( );  /* 首先读取后续数据长度 */
			for ( i = 0; i < length; i ++ ) buffer[ i ] = CH375_RD_DAT_PORT( );  /* 接收数据包 */
			/* 测试数据正确性,将接收到的命令包数据取反后返回给PC机 */
			CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* 向USB端点2的发送缓冲区写入数据块 */
			CH375_WR_DAT_PORT( length );  /* 首先写入后续数据长度,回传刚接收到的数据长度 */
			for ( i = 0; i < length; i ++ ) CH375_WR_DAT_PORT( ~ buffer[ i ] );  /* 数据取反后返回,由计算机应用程序测试数据是否正确 */
			break;
		}
		case USB_INT_EP2_IN: {  /* 批量数据发送成功 */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* 释放当前USB缓冲区 */
			break;
		}
		default: {  /* 其它中断,未用到,解锁后退出即可 */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* 释放当前USB缓冲区 */
			break;
		}
	}
}

main( ) {
	Delay50ms( );	/* 延时等待CH375初始化完成,如果单片机由CH375提供复位信号则不必延时 */
	CH375_Init( );  /* 初始化CH375 */
	EA = 1;  /* 允许中断 */
	while ( 1 );  /* 以下指令开始工作循环,等待PC机命令进行操作 */
}
