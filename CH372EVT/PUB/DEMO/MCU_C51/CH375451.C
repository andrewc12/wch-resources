/*; CH375/CH372/CH451 EVT
; U2(AT89C51) Program
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.12
;
;****************************************************************************
*/

/* MCS-51单片机C语言的示例程序 */

#include <reg52.h>
#include <string.h>
#include "..\..\MCU_IF1\MCS51C\CH375INC.H"

typedef	struct	_COMMAND_PACKET	{	/* 自定义的命令包结构 */
	unsigned char	mCommandCode;		/* 命令请求码,见下面的定义 */
	unsigned char	mCommandCodeNot;	/* 命令码的反码,用于校验命令包 */
	union	{
		unsigned char	mParameter[5];	/* 参数 */
		struct {
			unsigned char	mBufferID;  /* 缓冲区识别码,本程序针对MCS51单片机定义: 1-专用功能寄存器SFR, 2-内部RAM, 3-外部RAM, 不过本程序实际只演示内部RAM */
			unsigned int	mBufferAddr;	/* 读写操作的起始地址,寻址范围是0000H-0FFFFH,低字节在前 */
			unsigned int	mLength;	/* 数据块总长度,低字节在前 */
		} buf;
	} u;
}	mCOMMAND_PACKET,	*mpCOMMAND_PACKET;

#define		CONST_CMD_LEN		0x07	/* 命令块的长度 */
/* 由于命令与数据都是通过数据下传管道(USB端点2的OUT)下传, 为了防止两者混淆,
 我们可以在计算机应用程序与单片机程序之间约定, 命令块的长度总是7, 而数据块的长度肯定不是7, 例如64,32等
 另外, 可以约定, 命令块的首字节是命令码, 等等
 本程序约定命令码: 80H-0FFH是通用命令,适用于各种应用
                   00H-7FH是专用命令,针对各种应用特别定义 */
/* 通用命令 */
#define		DEF_CMD_GET_INFORM		0x90	/* 获取下位机的说明信息,长度不超过64个字符,字符串以00H结束 */
#define		DEF_CMD_TEST_DATA		0x91	/* 测试命令,下位机将PC机发来的命令包的所有数据取反后返回 */
#define		DEF_CMD_CLEAR_UP		0xA0	/* 在上传数据块之前进行同步,实际是让下位机清除上传缓冲区的已有内容 */
#define		DEF_CMD_UP_DATA			0xA1	/* 从下位机的指定地址的缓冲区中读取数据块(上传数据块) */
#define		DEF_CMD_DOWN_DATA		0xA2	/* 向下位机的指定地址的缓冲区中写入数据块(下传数据块) */
/* 专用命令 */
#define		DEMO_CH451_CMD			0x56	/* PC发送命令给CH451,用于演示CH451的功能 */
/* 对于MCS51单片机在使用通用命令时,还需要指定缓冲区识别码 */
#define		ACCESS_MCS51_SFR		1		/* 读写51单片机的SFR */
#define		ACCESS_MCS51_IRAM		2		/* 读写51单片机的内部RAM */
#define		ACCESS_MCS51_XRAM		3		/* 读写51单片机的外部RAM */

unsigned char volatile xdata CH375_CMD_PORT _at_ 0xBDF1;		/* CH375命令端口的I/O地址 */
unsigned char volatile xdata CH375_DAT_PORT _at_ 0xBCF0;		/* CH375数据端口的I/O地址 */

/* 有关CH451的定义,演示板的连接方式 */
sbit   CH451_dclk=P1^7;      /* 串行数据时钟上升延激活 */
sbit   CH451_din=P1^6;	     /* 串行数据输出,接CH451的数据输入 */
sbit   CH451_load=P1^5;      /* 串行命令加载,上升延激活 */
sbit   CH451_dout=P3^3;      /* INT1,键盘中断和键值数据输入,接CH451的数据输出 */
unsigned char CH451_KEY;				/* 存放键盘中断中读取的键值 */
unsigned char LAST_KEY;					/* 保存上次的键值 */

mCOMMAND_PACKET	CMD_PKT;				/* 命令包结构缓冲区 */
unsigned char data *CurrentRamAddr;		/* 进行数据块传输时保存被读写的缓冲区的起始地址 */
unsigned char CurrentRamLen;			/* 进行数据块传输时保存剩余长度 */
bit		FLAG_INT_WAIT;		/* 中断等待标志,1指示有中断数据正在CH375中等待发送 */
unsigned char CH451_CMD_H;	/* PC机发给CH451的高4位命令,为0FFH则命令无效 */
unsigned char CH451_CMD_L;  /* PC机发给CH451的低8位命令 */
unsigned char code InformString[16] = "CH375/CH451\x0";	/* 信息字符串 */

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

/* 将PC机的低字节在前的16位字数据转换为C51的高字节在前的数据 */
unsigned int	BIG_ENDIAN( unsigned int value )
{
	unsigned int  in, out;
	in = value;
	((unsigned char *)&out)[1] = ((unsigned char *)&in)[0];
	((unsigned char *)&out)[0] = ((unsigned char *)&in)[1];
	return( out );
}

/* CH375初始化子程序 */
void	CH375_Init( )
{
	unsigned char i;
	FLAG_INT_WAIT = 0;  /* 清发送中断等待标志 */
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

/* 加载上传数据 */
void	LoadUpData( unsigned char data *Buf, unsigned char Len )
{
	unsigned char i;
	CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* 向USB端点2的发送缓冲区写入数据块 */
	Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
	CH375_DAT_PORT = Len;  /* 首先写入后续数据长度 */
	for ( i=0; i<Len; i++ ) CH375_DAT_PORT = Buf[i];  /* 加载数据 */
}

/* CH375中断服务程序INT0,使用寄存器组1 */
void	mCH375Interrupt( ) interrupt 0 using 1
{
	unsigned char InterruptStatus;
	unsigned char length, c1, len1, len2, i;
#define		cmd_buf		((unsigned char data *)(&CMD_PKT))
	CH375_CMD_PORT = CMD_GET_STATUS;  /* 获取中断状态并取消中断请求 */
	Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
	InterruptStatus = CH375_DAT_PORT;  /* 获取中断状态 */
	IE0 = 0;  /* 清中断标志,对应于INT0中断 */
	if ( InterruptStatus == USB_INT_EP2_OUT ) {  /* 批量端点下传成功 */
		CH375_CMD_PORT = CMD_RD_USB_DATA;  /* 从当前USB中断的端点缓冲区读取数据块,并释放缓冲区 */
		Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
		length = CH375_DAT_PORT;  /* 首先读取后续数据长度 */
		if ( length == CONST_CMD_LEN ) {  /* 命令块长度总是CONST_CMD_LEN,分析并处理命令 */
/* 分析通过USB接收到的命令块,长度总是CONST_CMD_LEN,首字节为命令,其余为可选的参数,这种命令结构是由单片机和计算机应用层之间自行定义的 */
			for ( i=0; i<CONST_CMD_LEN; i++ ) cmd_buf[i] = CH375_DAT_PORT;  /* 接收命令包的数据 */
			if ( CMD_PKT.mCommandCode != (unsigned char)( ~ CMD_PKT.mCommandCodeNot ) ) return;  /* 命令包反码校验错误 */
			switch ( CMD_PKT.mCommandCode ) {  /* 分析命令码,switch可以用多个if/else代替 */
				case DEF_CMD_GET_INFORM:  /* 获取下位机的说明信息,长度不超过64个字符,字符串以00H结束 */
					CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* 向USB端点2的发送缓冲区写入数据块 */
					Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
					CH375_DAT_PORT = 16;  /* 首先写入后续数据长度 */
					for ( i=0; i<16; i++ ) CH375_DAT_PORT = InformString[i];  /* 加载数据 */
					break;
				case DEF_CMD_TEST_DATA:  /* 测试命令,下位机将PC机发来的命令包的所有数据取反后返回 */
					CH375_CMD_PORT = CMD_WR_USB_DATA7;  /* 向USB端点2的发送缓冲区写入数据块 */
					Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
					CH375_DAT_PORT = CONST_CMD_LEN;  /* 首先写入后续数据长度 */
					for ( i=0; i<CONST_CMD_LEN; i++ ) CH375_DAT_PORT = ~ cmd_buf[i];  /* 加载数据,数据取反后返回,由计算机应用程序测试数据是否正确 */
					break;
				case DEF_CMD_CLEAR_UP:  /* 在上传数据块之前进行同步,实际是让下位机清除上传缓冲区的已有内容 */
/* 连续上传数据块之前进行同步,实际是让单片机清除上传缓冲区的已有内容
; 如果上一次进行数据上传时,计算机提前结束上传,那么有可能在上传缓冲区中遗留有数据,所以在第二次上传前需要清除上传缓冲区 */
					CH375_CMD_PORT = CMD_SET_ENDP7;  /* 设置USB端点2的IN,也就是批量上传端点 */
					Delay2us( );  /* 如果时钟频率低于16MHz则无需该指令延时 */
					CH375_DAT_PORT = 0x0e;  /* 同步触发位不变,设置USB端点2的IN正忙,返回NAK */
					break;
				case DEF_CMD_UP_DATA:  /* 从下位机的指定地址的缓冲区中读取数据块(上传数据块) */
/* 连续上传数据块, 本程序实际只演示内部RAM */
/*					switch ( CMD_PKT.u.buf.mBufferID ) {
						case ACCESS_MCS51_SFR:  读写51单片机的SFR
						case ACCESS_MCS51_IRAM:  读写51单片机的内部RAM
						case ACCESS_MCS51_XRAM:  读写51单片机的外部RAM
					} */
					CurrentRamAddr = (unsigned char)BIG_ENDIAN( CMD_PKT.u.buf.mBufferAddr );  /* 起始地址,对于内部RAM只用低8位地址 */
					CurrentRamLen = (unsigned char)BIG_ENDIAN( CMD_PKT.u.buf.mLength );  /* 数据块长度,对于内部RAM总长度不可能超过256 */
					len1 = CurrentRamLen >= CH375_MAX_DATA_LEN ? CH375_MAX_DATA_LEN : CurrentRamLen;  /* 数据上传,准备第一组数据 */
					LoadUpData( CurrentRamAddr, len1);  /* 加载上传数据 */
					CurrentRamLen -= len1;
					CurrentRamAddr += len1;
					break;
				case DEF_CMD_DOWN_DATA:  /* 向下位机的指定地址的缓冲区中写入数据块(下传数据块) */
/* 连续下传数据块, 本程序实际只演示外部RAM */
					CurrentRamAddr = BIG_ENDIAN( CMD_PKT.u.buf.mBufferAddr );  /* 起始地址 */
					CurrentRamLen = BIG_ENDIAN( CMD_PKT.u.buf.mLength );  /* 数据块长度 */
					break;
				case DEMO_CH451_CMD:  /* PC发送命令给CH451,用于演示CH451的功能 */
/*; 为了防止在CH375中断服务程序打断主程序中的CH451_READ而执行CH451_WRITE产生错误
; 所以在此保存CH451的命令码由主程序在空闲时发给CH451 */
					CH451_CMD_L = CMD_PKT.u.mParameter[1];  /* 低8位命令 */
					CH451_CMD_H = CMD_PKT.u.mParameter[2];  /* 高4位命令 */
					break;
				default:
					break;
			}
		}
		else if ( length == 0 ) return;  /* 长度为0,没有数据则直接退出,在某些应用中也可以将长度0定义为一种特殊命令 */
		else {  /* 不是命令包 */
/* 接收下传的数据块,每组数据的长度不超过64字节,例如总长度150个字节,则第1组和第2组各是64字节,第3组是剩余长度22字节
; 为了将数据块与命令包区分开来,剩余长度不能等于CONST_CMD_LEN,可以用填充数据的办法避免相同 */
			if ( CMD_PKT.mCommandCode == DEF_CMD_DOWN_DATA ) {  /* 向下位机的指定地址的缓冲区中写入数据块(下传数据块) */
/* 接收下传的数据块,每组数据的长度不超过64字节,由于演示板中没有外部RAM,并且外部RAM的效率很低,所以用内部RAM示范 */
				CurrentRamLen -= length;
				while ( length-- ) {
					*CurrentRamAddr = CH375_DAT_PORT;
					CurrentRamAddr ++;
				}
			}
			else {  /* 未定义的命令 */
				while ( length -- ) c1 = CH375_DAT_PORT;  /* 丢弃数据 */
			}
		}
	}
	else if ( InterruptStatus == USB_INT_EP2_IN ) {  /* 批量数据发送成功 */
		if ( CMD_PKT.mCommandCode == DEF_CMD_UP_DATA ) {  /* 从下位机的指定地址的缓冲区中读取数据块(上传数据块) */
			len2 = CurrentRamLen >= CH375_MAX_DATA_LEN ? CH375_MAX_DATA_LEN : CurrentRamLen;  /* 数据上传,准备数据 */
			LoadUpData( CurrentRamAddr, len2 );  /* 加载上传数据 */
			CurrentRamLen -= len2;
			CurrentRamAddr += len2;
		}
/* 收到上传成功中断后,在退出之前必须解锁USB缓冲区,以便继续收发数据 */
		CH375_CMD_PORT = CMD_UNLOCK_USB;  /* 释放当前USB缓冲区 */
	}
	else if ( InterruptStatus == USB_INT_EP1_IN ) {  /* 中断数据发送成功 */
		CH375_CMD_PORT = CMD_UNLOCK_USB;  /* 释放当前USB缓冲区 */
		FLAG_INT_WAIT = 0;  /* 清发送中断等待标志,通知应用程序可以继续发送中断数据 */
	}
	else {  /* 内置固件的USB方式下不应该出现其它中断状态 */
	}
}

/* 上传中断数据子程序(通过中断端点上传),可选子程序 */
void	LoadIntData( unsigned char c1, unsigned char c2 )
{
	unsigned int i;
	for ( i=1000; i!=0; i-- ) {  /* 等待发送中断数据完成,因为PC机每隔1毫秒就读取中断数据,所以最多等待1毫秒 */
		if ( FLAG_INT_WAIT == 0 ) break;  /* 前次中断数据已经被PC机取走 */
	}
/* 如果未与计算机相连,或者计算机中的应用程序未运行,则会出现超时,否则不超过1毫秒中断数据就应该被PC机取走 */
	EX0 = 0;  /* 为了防止中途被中断而乱了顺序,必须先禁止中断 */
	CH375_CMD_PORT = CMD_WR_USB_DATA5;  /* 向USB端点1的发送缓冲区写入数据块 */
	Delay2us( );
	CH375_DAT_PORT = 2;  /* 后续数据长度 */
/* 虽然每次可以上传8个字节, 但是本程序中用于键盘中断, 所以每次只需要传输两个字节 */
	FLAG_INT_WAIT = 1;  /* 置中断等待标志 */
	CH375_DAT_PORT = c1;  /* 发送中断数据1 */
	CH375_DAT_PORT = c2;  /* 发送中断数据2 */
	EX0 = 1;  /* 允许中断 */
}

/* CH451初始化子程序 */
void	CH451_Init( )
{
	CH451_din=0;         /* 先低后高,选择4线输入 */
	CH451_din=1;
	IT1 =0;  /* 置外部信号为低电平触发 */
	IE1 =0;  /* 清中断标志 */
	EX1 =1;  /* 允许键盘中断 */
}

/* CH451输出命令子程序 */
/* 定义一无符号整型变量存储12字节的命令字 */
void	CH451_Write( unsigned int command )
{
  unsigned char i;
  EX1 = 0;  /* 禁止键盘中断 */
  CH451_load=0;  /* 命令开始 */
  for( i=0; i<12; i++ ) {  /* 送入12位数据,低位在前 */
    CH451_din = command & 1;
    CH451_dclk = 0;
    command >>= 1;
    CH451_dclk = 1;  /* 上升沿有效,产生时钟上升沿锁通知CH451输入位数据 */
  }
  CH451_load = 1;  /* 加载数据,产生加载上升沿通知CH451处理命令数据 */
  EX1 = 1;  /* 允许键盘中断 */
}

/* 读取CH451键值子程序 */
unsigned char CH451_Read( )
{
  unsigned char i;
  unsigned char command, keycode;
  EX1 = 0;  /* 禁止键盘中断 */
  command=0x07;  /* 输入读451命令字,只需要高4位,低8位可省去 */
  CH451_load=0;  /* 命令开始 */
  for( i=0; i<4; i++ ){  /* 送入4位数据,低位在前 */
    CH451_din = command & 1;
    CH451_dclk = 0;
    command >>= 1;
    CH451_dclk = 1;  /* 上升沿有效,产生时钟上升沿锁通知CH451输入位数据 */
  }
  CH451_load = 1;  /* 加载数据,产生加载上升沿通知CH451处理命令数据 */
  keycode=0;
  for( i=0; i<7; i++ ){  /* 输入键值,7位 */
    keycode<<=1;  /* 数据移入keycode,高位在前,低位在后 */
    keycode|=CH451_dout;  /* 从高到低读入451的数据 */
    CH451_dclk=0; /* 产生时钟下升沿通知CH451输出下一位 */
    CH451_dclk=1;
  }
  IE1=0;  /* 清中断标志,是读取时DOUT输出低电平产生的 */
  EX1=1;
  return( keycode );  /* 反回键值 */
}

/* CH451键盘中断服务程序INT1,使用寄存器组1 */
void	mCH451Interrupt( ) interrupt 2 using 1
{
  unsigned char i, command;
  command=0x07;  /* 输入读451命令字,只需要高4位,低8位可省去 */
  CH451_load=0;  /* 命令开始 */
  for( i=0; i<4; i++ ){  /* 送入4位数据,低位在前 */
    CH451_din = command & 1;
    CH451_dclk = 0;
    command >>= 1;
    CH451_dclk = 1;  /* 上升沿有效,产生时钟上升沿锁通知CH451输入位数据 */
  }
  CH451_load = 1;  /* 加载数据,产生加载上升沿通知CH451处理命令数据 */
  CH451_KEY=0;
  for( i=0; i<7; i++ ){  /* 输入键值,7位 */
    CH451_KEY<<=1;  /* 数据移入,高位在前,低位在后 */
    CH451_KEY|=CH451_dout;  /* 从高到低读入451的数据 */
    CH451_dclk=0; /* 产生时钟下升沿通知CH451输出下一位 */
    CH451_dclk=1;
  }
  IE1=0;  /* 清中断标志 */
}

/* 由单片机控制演示,等待按键,然后先左移原显示,再加上按键值 */
void	DEMO_CH451_ONLY( )
{
	unsigned char key;
	CH451_Write( 0x0f00 | 0x17 );  /* 向第1个数码管加载数据'H' */
	CH451_Write( 0x0e00 | 0x0e );  /* 向第1个数码管加载数据'E' */
	CH451_Write( 0x0d00 | 0x18 );  /* 向第1个数码管加载数据'L' */
	CH451_Write( 0x0c00 | 0x19 );  /* 向第1个数码管加载数据'P' */
	CH451_Write( 0x0b00 | 0x10 );  /* 向第1个数码管加载数据' ' */
	CH451_Write( 0x0a00 | 0x14 );  /* 向第1个数码管加载数据'[' */
	CH451_Write( 0x0900 | 0x88 );  /* 向第1个数码管加载数据'8.' */
	CH451_Write( 0x0800 | 0x15 );  /* 向第1个数码管加载数据']' */
	CH451_Write( 0x0600 | 0x30 );  /* 第3个和第4个数码管闪烁 */
	while ( 1 ) {  /* 以下演示为显示按键代码,并且左移 */
		if ( CH451_KEY != 0xff ) {  /* 检测到新的按键,先左移显示再显示键值 */
			key = CH451_KEY;
			CH451_KEY = 0xff;  /* 取按键值并清除原值 */
			CH451_Write( 0x0300 );  /* 左移一位 */
			CH451_Write( 0x0300 );  /* 左移一位 */
			key &= 0x3f;     /* 键值0-63 */
			CH451_Write( 0x0900 | ( key / 10 ) );  /* 第7个数码管显示键值的十位数 */
			CH451_Write( 0x0800 | ( key % 10 ) );  /* 第8个数码管显示键值的个位数 */
		}
	}
}

/* 由PC机通过USB演示CH451的功能,验证USB通讯 */
void	DEMO_USB( )
{
	unsigned int  ch451cmd;
	TR0 = 1;  /* 由定时器0自由计数值产生伪随机数 */
	CH451_CMD_H = 0xff;  /* 清除CH451的命令码 */
	while ( 1 ) {  /* 以下指令开始工作循环,等待PC机命令进行操作 */
		if ( CH451_CMD_H == 0xff ) {  /* 没有CH451的命令, 以下检测按键状态 */
/*    LAST_KEY和CH451_KEY两个变量,都是0FFH则没有键按下 */
/*    仅CH451_KEY为有效键值则是键刚按下, 两者都是有效键值并且相同则已经通知过计算机 */
			if ( LAST_KEY == 0xff ) {  /* 按键未按下或者已按下但是尚未通知过PC机 */
				if ( CH451_KEY != 0xff && CH451_KEY >= 0x40 ) {  /* 检测到新的键键,则置入有效的中断特征数据,以通知PC机应用层 */
					LAST_KEY = CH451_KEY;
/* 设定中断特征数据01H,通知PC机的应用层,按键已按下, 中断数据的次字节就是键码,所以PC机应用层直接获取键值,不必再读取 */
					LoadIntData( 1, LAST_KEY );  /* 上传中断数据,本程序用于键盘中断,所以每次只需要传输两字节 */
				}
			}
			else {	/* 按键已经按下并且已经通知过PC机,所以检测按键释放 */
				CH451_KEY = CH451_Read( );  /* 重新读取最近一次有效按键的键值 */
				if ( CH451_KEY != LAST_KEY ) {  /* 键值不同,按键已经释放或者新键按下,如果键码相同,说明按键仍然没有释放 */
/* 设定中断特征数据02H,通知PC机的应用层,按键已释放, 中断数据的次字节就是键码,所以PC机应用层直接获取键值 */
					LoadIntData( 2, LAST_KEY ); /* 上传中断数据 */
					LAST_KEY = 0xff;  /* 已经发送释放信号给PC */
				}
			}
		}
		else {  /* 在CH375中断服务程序中接收到PC机发给CH451的命令 */
/* PC发送命令给CH451,由于主程序中的CH451_READ可能会被CH375的高优先级中断,所以不宜在CH375中断服务程序中执行 */
			ch451cmd = ( (unsigned int)CH451_CMD_H << 8 ) + CH451_CMD_L;  /* 高4位命令及低8位命令 */
			CH451_CMD_H = 0xff;  /* 清除原命令码,防止重发 */
			CH451_Write( ch451cmd );  /* 向CH451发送命令 */
		}
	}
}

main( ) {
	Delay50ms( );	/* 延时等待CH375初始化完成,如果单片机由CH375提供复位信号则不必延时 */
	CH375_Init( );  /* 初始化CH375 */
	CH451_Init( );  /* 初始化CH451 */
/* 启动CH451的显示驱动和键盘扫描 */
	CH451_Write( 0x0403 );  /* 设置系统参数命令,使能显示驱动,使能键盘功能 */
	CH451_Write( 0x058C );  /* 设置显示参数,BCD译码方式,12级亮度 */
/* 由于在CH451启动键盘功能前的串行输出数据有可能产生键盘中断,所以需要再次取消中断 */
	IE1 = 0;
	CH451_KEY = 0xff;  /* 清除按键代码,在使能键盘前,CH451的DOUT输出串行数据 */
	LAST_KEY = 0xff;  /* 有可能被当作键盘中断请求INT1,所以要先清除 */
	EA = 1;  /* 允许中断 */
/* 如果P2的第5脚和第6脚之间插上短路子则由单片机控制演示,否则由PC机通过USB控制进行演示 */
	if ( T1 ) DEMO_USB( );  /* T1,P3.5为默认的高电平,由PC机通过USB演示 */
	else DEMO_CH451_ONLY( );  /* 由单片机控制演示,等待按键,然后先左移原显示,再加上按键值 */
}
