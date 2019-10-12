/*
; CH375/CH372 Bulk Data Test
; U2(AT89C51) Program
; 本程序测试数据传输的正确性, 并通过收发大数据块来测试传输速度,
; 本程序适用于时钟频率不高于24MHz的标准MCS-51单片机, 如果高于24MHz则应该适当增加延时, 如果低于24MHz则可以适当减少延时
; 只要单片机允许, 尽量使用较高的时钟, 例如将原12MHz晶振换为24MHz,
; 因为MCS51单片机自身速度较慢,读写外部RAM需要2个周期以及附加的循环指令,即使24MHz时钟也无法超过500KB/S
; 为了减少单片机自身对速度测试的影响,本程序中速度测试部分在单片机中断服务程序中完成
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.09
*/

/* MCS-51单片机C语言的示例程序 */

#include <reg52.h>
#include <string.h>
#include "..\..\MCU_IF1\MCS51C\CH375INC.H"

/* #define USE_MY_USB_ID	YES */
#define MY_USB_VENDOR_ID	0x1234		/* 厂商ID */
#define MY_USB_DEVICE_ID	0x5678		/* 设备ID */

unsigned char volatile xdata CH375_CMD_PORT _at_ 0xBDF1;		/* CH375命令端口的I/O地址 */
unsigned char volatile xdata CH375_DAT_PORT _at_ 0xBCF0;		/* CH375数据端口的I/O地址 */

unsigned char THIS_CMD_CODE;  /* 保存当前命令码 */
unsigned char RECV_LEN;       /* 刚接收到的数据的长度 */
unsigned char RECV_BUFFER[ CH375_MAX_DATA_LEN ];  /* 数据缓冲区,用于保存接收到的下传数据,长度为0到64字节 */
/* 由于MCS-51单片机存取外部RAM的读写速度低于内部RAM, 并且需要用到DPTR, 所以读写速度较慢, 其程序可以参考本程序修改 */

/* 定义位标志 */
bit bdata FLAG_RECV_OK;       /* 接收成功标志,1指示成功接收到数据块 */
bit bdata FLAG_SEND_WAIT;     /* 发送等待标志,1指示有数据块正在CH375中等待发送 */

/* 应用层定义 */
/* TEST_OTHER		EQU    00H				;其它自定义的命令码 */
#define TEST_START		0x20  /* 测试过程开始 */
#define TEST_DATA		0x21  /* 测试数据正确性 */
#define TEST_UPLOAD		0x22  /* 测试上传数据块 */
#define TEST_DOWNLOAD	0x23  /* 测试下传数据块 */

/* 有关CH451的定义,演示板的连接方式,该程序没有用到键盘 */
sbit   CH451_dclk=P1^7;      /* 串行数据时钟上升延激活 */
sbit   CH451_din=P1^6;	     /* 串行数据输出,接CH451的数据输入 */
sbit   CH451_load=P1^5;      /* 串行命令加载,上升延激活 */

/* 延时2微秒,不精确 */
void	Delay2us( )
{
	unsigned char i;
#define DELAY_START_VALUE	1  /* 根据单片机的时钟选择初值,20MHz以下为0,30MHz以上为2 */
	for ( i=DELAY_START_VALUE; i!=0; i-- );
}

/* 延时50毫秒,不精确 */
void	Delay50ms( )
{
	unsigned char i, j;
	for ( i=200; i!=0; i-- ) for ( j=250; j!=0; j-- );
}

/* CH451初始化子程序 */
void	CH451_Init( )
{
  CH451_din=0;         /* 先低后高,选择4线输入 */
  CH451_din=1;
}

/* CH451输出命令子程序 */
/* 定义一无符号整型变量存储12字节的命令字 */
void	CH451_Write( unsigned int command )
{
  unsigned char i;
  CH451_load=0;  /* 命令开始 */
  for( i=0; i<12; i++ ) {  /* 送入12位数据,低位在前 */
    CH451_din = command & 1;
    CH451_dclk = 0;
    command >>= 1;
    CH451_dclk = 1;  /* 上升沿有效 */
  }
  CH451_load = 1;  /* 加载数据 */
}

/* CH375初始化子程序 */
void	CH375_Init( )
{
	unsigned char i;
	FLAG_RECV_OK=0;  /* 清接收成功标志,1指示成功接收到数据块 */
	FLAG_SEND_WAIT=0;  /* 清发送等待标志,1指示有数据块正在CH375中等待发送 */
/* 测试CH375是否正常工作,可选操作,通常不需要 */
#ifdef TEST_CH375_FIRST
	CH375_CMD_PORT = CMD_CHECK_EXIST;  /* 测试CH375是否正常工作 */
	Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
	CH375_DAT_PORT = 0x55;  /* 写入测试数据 */
	Delay2us( );
	i = ~ 0x55;  /* 返回数据应该是测试数据取反 */
	if ( CH375_DAT_PORT != i ) {  /* CH375不正常 */
		for ( i=80; i!=0; i-- ) {
			CH375_CMD_PORT = CMD_RESET_ALL;  /* 多次重复发命令,执行硬件复位 */
			Delay2us( );
		}
		CH375_CMD_PORT = 0;
		Delay50ms( );  /* 延时50ms */
	}
#endif
#ifdef USE_MY_USB_ID
/* 设置外部自定义的USB设备VID和PID,可选操作,不执行该命令则使用默认的VID和PID */
	CH375_CMD_PORT = CMD_SET_USB_ID;  /* 设置外部自定义的USB设备VID和PID,可选操作 */
	Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
	CH375_DAT_PORT = (unsigned char)MY_USB_VENDOR_ID;  /* 写入厂商ID的低字节 */
	CH375_DAT_PORT = (unsigned char)(MY_USB_VENDOR_ID>>8);  /* 写入厂商ID的高字节 */
	CH375_DAT_PORT = (unsigned char)MY_USB_DEVICE_ID;  /* 写入设备ID的低字节 */
	CH375_DAT_PORT = (unsigned char)(MY_USB_DEVICE_ID>>8);  /* 写入设备ID的高字节 */
	Delay2us( );
#endif
/* 设置USB工作模式, 必要操作 */
	CH375_CMD_PORT = CMD_SET_USB_MODE;
	Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
	CH375_DAT_PORT = 2;  /* 设置为使用内置固件的USB设备方式 */
	for ( i=100; i!=0; i-- ) {  /* 等待操作成功,通常需要等待10uS-20uS */
		if ( CH375_DAT_PORT==CMD_RET_SUCCESS ) break;
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
	unsigned char length, c1;
	unsigned char data *cmd_buf;
	unsigned char data *ret_buf;
	CH375_CMD_PORT = CMD_GET_STATUS;  /* 获取中断状态并取消中断请求 */
	Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
	InterruptStatus = CH375_DAT_PORT;  /* 获取中断状态 */
	IE0 = 0;  /* 清中断标志,对应于INT0中断 */
	if ( InterruptStatus == USB_INT_EP2_OUT ) {  /* 批量端点下传成功 */
		CH375_CMD_PORT = CMD_RD_USB_DATA;  /* 从当前USB中断的端点缓冲区读取数据块,并释放缓冲区 */
		Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
		length = CH375_DAT_PORT;  /* 首先读取后续数据长度 */
		if ( length != 0 ) {  /* 如果长度为0则不处理 */
			THIS_CMD_CODE = CH375_DAT_PORT;  /* 保存当前命令码,因为我们测试程序与PC机应用程序约定首字节为命令码 */
			if ( THIS_CMD_CODE == TEST_DOWNLOAD ) {  /* 测试下传速度 */
				while ( --length != 0 )  /* 先减1以去掉首字节后 */
					c1 = CH375_DAT_PORT;  /* 接收数据,为了测试速度,数据舍弃,24MHz的MCS51每读取一个字节需要2uS */
			}
			else {  /* 不是测试下传速度的命令,先接收完命令包再分析 */
				RECV_LEN = length;  /* 命令包的数据长度 */
				cmd_buf = RECV_BUFFER;  /* 接收缓冲区 */
				*cmd_buf = THIS_CMD_CODE;
				while ( --length != 0 ) {  /* 先减1以去掉首字节后 */
					cmd_buf++;
					*cmd_buf = CH375_DAT_PORT;
				}
/* 以上程序C语言要每读一个字节可能要十多个机器周期,如果用汇编语言只要4个机器周期
					mov  a,length
					jz   skip_get
					mov  r7,a
					mov  dptr,#CH375_DAT_PORT
get_next_byte:		movx a,@dptr    数据直接放弃
					djnz r7,get_next_byte
skip_get:			nop
*/
				if ( THIS_CMD_CODE == TEST_UPLOAD ) {  /* 测试上传速度 */
					CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* 向USB端点2的发送缓冲区写入数据块 */
					Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
					length = CH375_MAX_DATA_LEN;
					CH375_DAT_PORT = length;  /* 首先写入后续数据长度 */
					do {
						CH375_DAT_PORT = TL0;  /* 发送伪随机数数据,为了测试速度,数据无效,24MHz的MCS51每写出一个字节需要2uS */
					} while ( --length != 0 );
				}
				else if ( THIS_CMD_CODE == TEST_START ) {  /* 测试过程开始 */
/* 由于上一次测试数据上传速度时可能在上传缓冲区中遗留有数据, 所以在第二次测试前需要清除上传缓冲区 */
					CH375_CMD_PORT = CMD_SET_ENDP7;  /* 设置USB端点2的IN */
					Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
					CH375_DAT_PORT = 0x0e;  /* 同步触发位不变,设置USB端点2的IN正忙,返回NAK */
					FLAG_SEND_WAIT = 0;  /* 清除发送等待标志,通知应用程序可以继续发送数据 */
				}
				else if ( THIS_CMD_CODE == TEST_DATA ) {  /* 测试数据正确性,将接收到的命令包数据取反后返回给PC机 */
					CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* 向USB端点2的发送缓冲区写入数据块 */
					Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
					ret_buf = RECV_BUFFER;  /* 接收缓冲区 */
					length = RECV_LEN;  /* 刚接收到的数据长度 */
					CH375_DAT_PORT = length;  /* 首先写入后续数据长度 */
					if ( length ) {
						do {
							CH375_DAT_PORT = ~ *ret_buf;  /* 数据取反后返回,由计算机应用程序测试数据是否正确 */
							ret_buf++;
						} while ( --length != 0 );
					}
				}
				else {  /* 其它命令,尚未定义 */
					FLAG_RECV_OK = 1;  /* 其它命令,设置接收成功标志,通知应用程序取走数据再分析 */
				}
			}
		}
	}
	else if ( InterruptStatus == USB_INT_EP2_IN ) {  /* 批量数据发送成功 */
		if ( THIS_CMD_CODE == TEST_UPLOAD ) {  /* 测试上传速度,继续准备上传数据 */
			CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* 向USB端点2的发送缓冲区写入数据块 */
			Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
			length = CH375_MAX_DATA_LEN;
			CH375_DAT_PORT = length;  /* 首先写入后续数据长度 */
			do {
				CH375_DAT_PORT = TL0;  /* 发送伪随机数数据,为了测试速度,数据无效,24MHz的MCS51每写出一个字节需要2uS */
			} while ( --length != 0 );
		}
		CH375_CMD_PORT = CMD_UNLOCK_USB;  /* 释放当前USB缓冲区 */
		FLAG_SEND_WAIT = 1;  /* 清除发送等待标志,通知应用程序可以继续发送数据 */
	}
	else if ( InterruptStatus == USB_INT_EP1_IN ) {  /* 中断数据发送成功 */
		CH375_CMD_PORT = CMD_UNLOCK_USB;  /* 释放当前USB缓冲区 */
	}
	else {  /* 内置固件的USB方式下不应该出现其它中断状态 */
	}
}

main( ) {
	unsigned char i;
	Delay50ms( );	/* 延时等待CH375初始化完成,如果单片机由CH375提供复位信号则不必延时 */
	CH375_Init( );  /* 初始化CH375 */
	CH451_Init( );  /* 初始化CH451 */
/* 启动CH451的显示驱动 */
	CH451_Write( 0x0401 );  /* 设置系统参数命令,使能显示驱动 */
	CH451_Write( 0x0588 );  /* 设置显示参数,BCD译码方式,8级亮度 */
	for ( i=0; i<8; ++i ) CH451_Write( (unsigned int)i << 8 | 0x0800 | i );  /* 依次显示12345678 */
	EA = 1;  /* 允许中断 */
	TR0 = 1;  /* 由定时器0自由计数值产生伪随机数 */
	while ( 1 ) {  /* 以下指令开始工作循环,等待PC机命令进行操作 */
		if ( FLAG_RECV_OK ) {  /* 收到未定义的命令 */
			CH451_Write( 0x0300 );  /* 发出命令使CH451显示左移一位,提示收到未定义命令 */
		}
	}
}
