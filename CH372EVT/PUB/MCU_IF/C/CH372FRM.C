/*
; 单片机通过CH372或者CH375连接到PC机进行USB通讯的框架程序
; 可以用include直接包含到应用系统的主程序中,或者添加到工程项目中
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; V1.0 @2004.09, V1.1 @2004.12
;****************************************************************************
*/

/* C语言, 适用用大多数单片机,包括MCS51以及其它内核的单片机 */
/* 如果单片机类型或者硬件资源不同时, 该程序应该根据需要进行局部修改 */

#include "CH375INC.H"

#ifdef __CX51__
#ifndef __C51__
#define __C51__		1
#endif
#endif

typedef unsigned char                 UINT8;
typedef unsigned short                UINT16;
typedef unsigned long                 UINT32;
#ifdef __C51__
typedef unsigned char  idata         *PUINT8;
typedef unsigned char volatile xdata  IOPORT;
#pragma NOAREGS
#include <reg52.h>
#else
typedef unsigned char                *PUINT8;
typedef unsigned char volatile        IOPORT;
#endif

#define DELAY_START_VALUE		1	  /* 根据单片机的时钟选择延时初值 */

/*#define MY_USB_VENDOR_ID		0x4348*/	/* 定义自己的USB设备的厂商ID */
/*#define MY_USB_PRODUCT_ID		0x5537*/	/* 定义自己的USB设备的产品ID */
/*#define ENABLE_USB_SUSPEND		1*/		/* 检查USB总线的挂起状态,便于进入低功耗模式 */

IOPORT		CH375_CMD_PORT _at_ 0xBDF1;		/* CH375命令端口的I/O地址,根据实际硬件电路进行修改 */
IOPORT		CH375_DAT_PORT _at_ 0xBCF0;		/* CH375数据端口的I/O地址,根据实际硬件电路进行修改 */

/* 延时1微秒,不精确,需要根据硬件实际情况调整 */
void	Delay1us( )
{
#if DELAY_START_VALUE != 0
	UINT8 i;
	for ( i=DELAY_START_VALUE; i!=0; i-- );
#endif
}

/* 延时2微秒,不精确,需要根据硬件实际情况调整 */
void	Delay2us( )
{
	UINT8 i;
	for ( i=DELAY_START_VALUE*2+1; i!=0; i-- );
}

/* 与CH372/CH375有关的基本I/O操作 */

void CH375_WR_CMD_PORT( UINT8 cmd ) {  /* 向CH375的命令端口写入命令,周期不小于4uS,如果单片机较快则延时 */
	Delay2us();
	CH375_CMD_PORT=cmd;
	Delay2us();
}

void CH375_WR_DAT_PORT( UINT8 dat ) {  /* 向CH375的数据端口写入数据,周期不小于1.5uS,如果单片机较快则延时 */
	CH375_DAT_PORT=dat;
	Delay1us();  /* 如果是MCS51单片机,因其较慢,所以实际上无需延时 */
}

UINT8 CH375_RD_DAT_PORT( void ) {  /* 从CH375的数据端口读出数据,周期不小于1.5uS,如果单片机较快则延时 */
	Delay1us();  /* 如果是MCS51单片机,因其较慢,所以实际上无需延时 */
	return( CH375_DAT_PORT );
}

/* CH375初始化子程序 */
void	CH375_Init( void ) {
	UINT8 i;

#ifdef MY_USB_VENDOR_ID
#ifdef MY_USB_PRODUCT_ID
/* 设置外部自定义的USB设备VID和PID,可选操作,不执行该命令则使用默认的VID和PID,
   如果设置使用自定义的ID,那么计算机端驱动程序INF安装文件中的"USB\VID_4348&PID_5537"需要进行类似修改 */
	CH375_WR_CMD_PORT( CMD_SET_USB_ID );  /* 设置外部自定义的USB设备VID和PID,可选操作 */
	CH375_WR_DAT_PORT( (UINT8)MY_USB_VENDOR_ID );  /* 写入厂商ID的低字节 */
	CH375_WR_DAT_PORT( (UINT8)(MY_USB_VENDOR_ID>>8) );  /* 写入厂商ID的高字节 */
	CH375_WR_DAT_PORT( (UINT8)MY_USB_PRODUCT_ID );  /* 写入产品ID的低字节 */
	CH375_WR_DAT_PORT( (UINT8)(MY_USB_PRODUCT_ID>>8) );  /* 写入产品ID的高字节 */
#endif
#endif

/* 设置USB工作模式, 必要操作 */
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );
	CH375_WR_DAT_PORT( 2 );  /* 设置为使用内置固件的USB设备方式 */
	for ( i=100; i!=0; i-- ) if ( CH375_RD_DAT_PORT( ) == CMD_RET_SUCCESS ) break;  /* 等待操作成功,通常需要等待10uS-20uS */
/*	if ( i == 0 ) { CH372/CH375芯片内部或者物理连接存在硬件错误 }; */

#ifdef ENABLE_USB_SUSPEND
/* 设置检查USB总线挂起状态,便于在USB总线挂起时使CH375也进入低功耗状态 */
	CH375_WR_CMD_PORT( CMD_CHK_SUSPEND );  /* 设置检查USB总线挂起状态的方式 */
	CH375_WR_DAT_PORT( 0x10 );
	CH375_WR_DAT_PORT( 0x04 );  /* 以50mS为间隔检查USB挂起 */
#endif

/* 下面启用USB中断,CH375的INT#引脚可以连接到单片机的中断引脚,中断为低电平有效或者下降沿有效,
   如果不使用中断,那么也可以用查询方式,由单片机程序查询CH375的INT#引脚为低电平则说明CH375请求中断 */
#ifdef __C51__
	IT0 = 0;  /* 置外部信号为低电平触发 */
	IE0 = 0;  /* 清中断标志 */
	EX0 = 1;  /* 允许CH375中断,假定CH375的INT#引脚连接到单片机的INT0 */
#endif
}

UINT8	UsbLength;							/* USB数据缓冲区中数据的长度 */
UINT8	UsbBuffer[ CH375_MAX_DATA_LEN ];	/* USB数据缓冲区 */

/* CH375中断服务程序,假定CH375的INT#引脚连接到单片机的INT0,使用寄存器组1 */
void	mCH375Interrupt( void ) interrupt 0 using 1 {
	UINT8			IntStatus;
	UINT8			cnt;
	PUINT8			buf;
	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* 获取中断状态并取消中断请求 */
	IntStatus = CH375_RD_DAT_PORT( );  /* 获取中断状态 */
/*	IE0 = 0;  清中断标志,与单片机硬件有关,对应于INT0中断 */
	switch( IntStatus ) {  /* 分析中断状态 */
		case USB_INT_EP2_OUT:  /* 批量端点下传成功,接收到数据 */
			CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* 从当前USB中断的端点缓冲区读取数据块,并释放缓冲区 */
/* 如果使用CMD_RD_USB_DATA0命令码,那么读取数据块后不会自动释放缓冲区,需要CMD_UNLOCK_USB命令后才释放 */
			UsbLength = cnt = CH375_RD_DAT_PORT( );  /* 首先读取后续数据长度 */
			if ( cnt ) {  /* 接收到数据放到缓冲区中 */
				buf = UsbBuffer;  /* 指向缓冲区 */
				do {
					*buf = CH375_RD_DAT_PORT( );  /* 接收命令包的数据 */
					buf ++;
				} while ( -- cnt );
			}
			else break;  /* 长度为0,没有数据,在某些应用中也可以将长度0定义为一种特殊命令 */
/* 分析接收到的数据并处理,此处略去,下面演示回传数据 */
			CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* 向USB端点2的发送缓冲区写入数据块 */
			cnt = UsbLength;
			CH375_WR_DAT_PORT( cnt );  /* 首先写入后续数据长度 */
			if ( cnt ) {  /* 将缓冲区中的数据发出 */
				buf = UsbBuffer;  /* 指向缓冲区 */
				do {
					CH375_WR_DAT_PORT( *buf );  /* 写入数据到CH375 */
					buf ++;
				} while ( -- cnt );
			}
			break;
		case USB_INT_EP2_IN:  /* 批量端点上传成功,数据发送成功 */
/* 如果还有数据需要接着发送,可在此通过CMD_WR_USB_DATA7命令写入,参考前面的回传 */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* 释放当前USB缓冲区,收到上传成功中断后,必须解锁USB缓冲区,以便继续收发 */
			break;
		case USB_INT_EP1_IN:  /* 中断端点上传成功,中断数据发送成功 */
/* 中断端点可以用于单片机及时通知计算机端,本程序未用到 */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* 释放当前USB缓冲区 */
			break;
		case USB_INT_EP1_OUT:  /* 辅助端点下传成功,接收到辅助数据 */
/* 辅助端点可以用于计算机端向单片机端发送包,本程序未用到,辅助数据的接收可以参考批量端点 */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* 释放当前USB缓冲区 */
			break;
#ifdef ENABLE_USB_SUSPEND
/* 默认情况下不会出现这些中断状态,只有当执行相应的命令后才会出现这些状态,根据需要处理 */
		case USB_INT_USB_SUSPEND:  /* USB总线挂起事件,只有当执行过CMD_CHK_SUSPEND命令后才可能出现该中断状态 */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* 释放当前USB缓冲区 */
			CH375_WR_CMD_PORT( CMD_ENTER_SLEEP );  /* 这是可选操作,进入低功耗睡眠挂起状态 */
			break;
		case USB_INT_WAKE_UP:  /* 从睡眠中被唤醒事件,只有当执行过CMD_ENTER_SLEEP命令后才可能出现该中断状态 */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* 释放当前USB缓冲区 */
			break;
#endif
		default:
/* 内置固件的USB方式下不应该出现其它中断状态,如果启用USB总线挂起检查,那么还要处理USB总线挂起和睡眠唤醒事件 */
			CH375_WR_CMD_PORT( CMD_UNLOCK_USB );  /* 释放当前USB缓冲区 */
			break;
	}
}

/*
main( void ) {
	CH375_Init( );
	EA = 1;
	while ( 1 );
}
*/
