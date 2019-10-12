#include <reg52.h>
#include <stdio.h>

#define	USER_KEY_IN				INT1	/* P3.3, INT1, 默认为高电平, 当用户按键后为低电平, 本程序仅作查询 */

/* 以下定义的详细说明请看CH375HF4.H文件 */
#define LIB_CFG_DISK_IO			1		/* 磁盘读写的数据的复制方式,1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_FILE_IO			1		/* 文件读写的数据的复制方式,0为"外部子程序",1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_INT_EN			0		/* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375命令端口的I/O地址 */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375数据端口的I/O地址 */
/* 只使用单片机内置的1KB外部RAM: 0000H-01FFH 为磁盘读写缓冲区, 以字节为单位读写文件不需要文件数据读写缓冲区FILE_DATA_BUF */
#define	DISK_BASE_BUF_ADDR		0x0000	/* 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */

#define CH375_INT_WIRE			INT0	/* P3.2, INT0, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM		1		/* 未调用CH375FileEnumer程序故禁止以节约代码 */
#define NO_DEFAULT_CH375_F_QUERY	1		/* 未调用CH375FileQuery程序故禁止以节约代码 */

#include "..\CH375HF6.H"				/* 如果不需要支持FAT32,那么请选用CH375HF4.H */

#ifdef __C51__
#pragma NOAREGS
#endif

/* 在P1.4连接一个LED用于监控演示程序的进度,低电平LED亮,正常工作时亮,按键时灭 */
sbit P1_4  = P1^4;
#define LED_OUT_INIT( )		{ P1_4 = 1; }	/* P1.4 高电平 */
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 低电平驱动LED显示 */
sbit P1_5  = P1^5;
/* 在P1.5连接一个LED用于监控演示程序的进度,低电平LED亮,当U盘插入后亮 */
#define LED_UDISK_IN( )		{ P1_5 = 0; }	/* P1.5 低电平驱动LED显示 */
#define LED_UDISK_OUT( )	{ P1_5 = 1; }	/* P1.5 低电平驱动LED显示 */
sbit P1_6  = P1^6;
/* 在P1.6连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘写操作时亮 */
#define LED_WR_NOW( )		{ P1_6 = 0; }	/* P1.6 低电平驱动LED显示 */
#define LED_NOT_WR( )		{ P1_6 = 1; }	/* P1.6 低电平驱动LED显示 */
sbit P1_7  = P1^7;
/* 在P1.7连接一个LED用于监控当前USB模式,亮为USB设备方式,灭为USB主机方式 */
#define LED_DEVICE( )		{ P1_7 = 0; }	/* P1.7 低电平驱动LED显示 */
#define LED_HOST( )			{ P1_7 = 1; }	/* P1.7 低电平驱动LED显示 */

/* 公用子程序 */
void	mDelaymS( UINT8 delay );
UINT8	IsKeyPress( void );
void	device( void );
void	host( void );
