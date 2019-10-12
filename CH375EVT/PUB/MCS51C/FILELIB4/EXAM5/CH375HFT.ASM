; /* 2004.06.05
; ****************************************
; **  Copyright  (C)  W.ch  1999-2004   **
; **  Web:  http://www.winchiphead.com  **
; ****************************************
; **  USB Host File Interface for CH375 **
; **  TC2.0@PC, KC7.0@MCS51             **
; ****************************************
; */
; /* CH375 主机文件系统接口 */
; /* 支持: FAT12/FAT16/FAT32 */
; 
; /* MCS-51单片机ASM语言的U盘文件读写示例程序, 适用于89C52或者更大程序空间的单片机 */
; /* 该程序将U盘中的/C51/CH375HFT.C文件中的小写字母转成大写字母后, 写到新建的文件NEWFILE.TXT中,
;    如果找不到原文件CH375HFT.C, 那么该程序新建NEWFILE.TXT文件并写入提示信息
;    另外,本例还演示以字节为单位读取U盘中的/C51/CH375HFT.C文件, 读写速度较前面的扇区模式慢,
;    但是由于字节模式读写文件不需要文件数据缓冲区FILE_DATA_BUF, 所以总共只需要600字节的RAM,
;    适用于单片机硬件资源有限、数据量小并且读写速度要求不高的系统
; */
; /* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR复制", 所以速度较慢, 适用于所有MCS51单片机 */
;
; /* A51   CH375HFT.ASM */
; /* LX51  CH375HFT.OBJ , CH375HF6.LIB    如果将CH375HF6换成CH375HF4就可以禁止支持FAT32 */
; /* OHX51 CH375HFT */
;

LIB_CFG_VALUE			EQU     05H		; CH375程序库配置
; 位7: CH375的INT#引脚连接方式: 0查询方式,1中断方式 */
; 位5: 在写操作结束后是否延时: 0写后延时,1不延时 */
; 位3位2: 针对文件读写的多扇区数据的复制方式: 00外部子程序, 01单DPTR复制, 10双DPTR复制, 11单DPTR和P2+R0复制 */
; 位1位0: 针对磁盘读写的单扇区数据的复制方式: 00单DPTR复制, 01单DPTR复制, 10双DPTR复制, 11单DPTR和P2+R0复制 */

CH375_CMD_PORT_ADDR		EQU		0BDF1H	; CH375命令端口的I/O地址 */
CH375_DAT_PORT_ADDR		EQU		0BCF0H	; CH375数据端口的I/O地址 */

DISK_BASE_BUF_ADDR		EQU		0000H	; 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */
#define	DISK_BASE_BUF_LEN	512
FILE_DATA_BUF_ADDR		EQU		0200H	; 外部RAM的文件数据缓冲区的起始地址,缓冲区长度不小于一次读写的数据长度 */
#define	FILE_DATA_BUF_LEN	6800H
; 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */
; 如果准备使用双缓冲区交替读写,那么不要定义FILE_DATA_BUF_LEN,而是在参数中指定缓冲区起址,用CH375FileReadX代替CH375FileRead,用CH375FileWriteX代替CH375FileWrite */

#define	CH375_INT_WIRE		INT0
;CH375_INT_WIRE			EQU		INT0	; P3.2, INT0, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM		1		; 未调用CH375FileEnumer程序故禁止以节约代码 */
#define NO_DEFAULT_CH375_F_QUERY	1		; 未调用CH375FileQuery程序故禁止以节约代码 */

$INCLUDE		(..\CH375HF6.INC)			; ASM头文件,常用定义,如果不需要支持FAT32,那么请选用CH375HF4.INC */

; 应用程序段定义
MY_CODE					SEGMENT		CODE
MY_DATA					SEGMENT		DATA

;/* 在P1.4连接一个LED用于监控演示程序的进度,低电平LED亮,当U盘插入后亮 */
;sbit P1_4  = P1^4;
LED_OUT					EQU		P1.4
;#define LED_OUT_INIT( )		{ P1_4 = 1; }	/* P1.4 高电平 */
;#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 低电平驱动LED显示 */
;#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 低电平驱动LED显示 */
;sbit P1_5  = P1^5;
LED_RUN					EQU		P1.5
;/* 在P1.5连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘操作时亮 */
;#define LED_RUN_ACT( )		{ P1_5 = 0; }	/* P1.5 低电平驱动LED显示 */
;#define LED_RUN_INACT( )	{ P1_5 = 1; }	/* P1.5 低电平驱动LED显示 */
;sbit P1_6  = P1^6;
LED_WR					EQU		P1.6
;/* 在P1.6连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘写操作时亮 */
;#define LED_WR_ACT( )		{ P1_6 = 0; }	/* P1.6 低电平驱动LED显示 */
;#define LED_WR_INACT( )		{ P1_6 = 1; }	/* P1.6 低电平驱动LED显示 */

; 复位和中断向量
				CSEG	AT	0000H
				LJMP	ASM_MAIN		; 单片机复位后的主程序入口
;				CSEG	AT	0003H		; INT0中断向量
;				LJMP    InterruptHandle	; CH375的中断服务程序,如果使用"中断方式"则需根据中断引脚修改向量

; 应用程序中用到的变量,只定义大小,不指定地址,由链接程序自动分配地址
	RSEG  MY_DATA
   SecCount:   DS   1
    NewSize:   DS   2

; 应用程序中用到的常量,只定义大小,不指定地址,由链接程序自动分配地址
	RSEG  MY_CODE
SOURCE_NAME:	DB  '\C51\CH375HFT.C',00H
INFO_MISS_FILE:	DB  '找不到/C51/CH375HFT.C文件',0DH,0AH,00H
NEW_NAME:		DB  '\NEWFILE.TXT',00H
NEW_NAME_2:		DB  '\MY_FILE2.TXT',00H
DATA_FOR_FILE2:	DB  'OK! 这是写到新文件中的',0DH,0AH,00H

; 应用程序代码,不指定地址,由链接程序自动分配地址
	RSEG  MY_CODE
;
; 延时100毫秒,不精确 */
mDelay100mS:		MOV  	R7,#0C8H
mDelay100mS_next:	MOV  	R6,#0C8H
mDelay100mS_loop:	NOP
					NOP
					DJNZ 	R6,mDelay100mS_loop
					DJNZ 	R7,mDelay100mS_next
					RET  	
;
; 将程序空间的字符串复制到内部RAM中 */
; 入口: R0 指向内部RAM的目的地址, DPTR 指向程序空间的字符串源地址
_mCopyStringToIRAM:	CLR  	A
					MOVC 	A,@A+DPTR
					INC  	DPTR
					MOV  	@R0,A
					INC  	R0
					JNZ     _mCopyStringToIRAM
					RET  	
;
; 检查操作状态,如果错误则停机,实际应用程序需要另作处理
_mStopIfError:		MOV  	A,R7
					JNZ     ERROR_FOUND			; 错误
					RET							; 操作成功则返回
ERROR_FOUND:		CLR     LED_OUT				; LED闪烁显示
					LCALL	mDelay100mS
					SETB    LED_OUT
					LCALL	mDelay100mS
					SJMP 	ERROR_FOUND
;
; 应用程序主程序, 仅作演示
ASM_MAIN:			MOV     SP,#0CFH			; 堆栈
					CLR  	LED_OUT				; 开机后LED亮一下以示工作
					LCALL	mDelay100mS
					SETB 	LED_OUT
					LCALL	CH375LibInit		; 初始化CH375程序库和CH375芯片
					LCALL	_mStopIfError
; 主循环, 等待U盘插入并处理, 实际上单片机可以做其它事, 当需要读写U盘时再查询U盘状态
WAIT_DISK_IN:		MOV  	A,CH375DiskStatus	; 查询CH375中断并更新中断状态,等待U盘插入 */
					XRL  	A,#DISK_CONNECT
					JZ   	DISK_IN_NOW			; U盘已经插入
					LCALL	xQueryInterrupt		; 该调用仅在有U盘插拔后才会返回,如果希望立即返回,那么请使用CH375DiskConnect代替
					SJMP 	WAIT_DISK_IN
DISK_IN_NOW:		CLR  	LED_OUT				; LED亮
					LCALL	mDelay100mS			; 延时,可选操作,有的USB存储器需要几十毫秒的延时 */
					LCALL	mDelay100mS
; 检查U盘是否准备好,大多数U盘不需要这一步,但是某些U盘必须要执行这一步才能工作
					MOV		SecCount,#05H
IS_DISK_READY:		LCALL	mDelay100mS
					LCALL   CH375DiskReady		; 查询磁盘是否准备好
					MOV     A,R7
					XRL     A,#ERR_SUCCESS
					JZ		DISK_READY_OK		;准备好了
					DJNZ    SecCount,IS_DISK_READY	; 继续等待,有的U盘总是返回未准备好,不过可以被忽略
DISK_READY_OK:		NOP
					CLR     LED_RUN				; 开始操作U盘
; 打开原文件\C51\CH375HFT.C
					MOV  	R0,#LOW (mCmdParam)	; 命令参数,mCmdParam.Open.mPathName
					MOV  	DPTR,#SOURCE_NAME	; 指向原文件名,该文件在C51子目录下
					LCALL	_mCopyStringToIRAM	; 复制文件名作为调用子程序的参数
					LCALL	CH375FileOpen		; 打开文件
					MOV  	A,R7
					XRL  	A,#ERR_MISS_DIR
					JZ   	DIR_NOT_FOUND		; C51子目录不存在
					MOV  	A,R7
					XRL  	A,#ERR_MISS_FILE
					JZ      FILE_NOT_FOUND		; CH375HFT.C文件不存在
					LCALL	_mStopIfError
					SJMP    FOUND_SOURCE_FILE	; 找到文件\C51\CH375HFT.C
; 没有找到原文件
DIR_NOT_FOUND:
FILE_NOT_FOUND:		MOV     R7,#00H
					MOV     DPTR,#FILE_DATA_BUF	; 向文件缓冲区复制默认的提示信息作为新文件的内容
COPY_INFO_CHAR:		PUSH    DPL
					PUSH    DPH
					MOV     DPTR,#INFO_MISS_FILE	; 在没有找到原文件时的错误提示信息
					MOV     A,R7
					MOVC 	A,@A+DPTR
					POP     DPH
					POP     DPL
					MOVX    @DPTR,A				; 将提示信息作为新文件的内容
					INC     DPTR
					JZ      COPY_INFO_END
					INC     R7
					CJNE    R7,#0FFH,COPY_INFO_CHAR	; 文件字符计数
COPY_INFO_END:		MOV  	NewSize,#00H
					MOV  	NewSize+01H,R7		; 新文件的长度
					MOV  	SecCount,#01H		; 新文件只占用1个扇区, (NewSize+511)/512
					LJMP 	CREATE_NEW_FILE
; 找到原文件
FOUND_SOURCE_FILE:	;LCALL	CH375FileQuery		; 查询其信息,包括长度,日期时间,属性等
					;LCALL	_mStopIfError
					MOV     A,CH375vFileSize	; 原文件的长度
					JNZ     SOURCE_FILE_LONG	; 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以只读取不超过63个扇区
					MOV     A,CH375vFileSize+01H
					JNZ     SOURCE_FILE_LONG	; 由于RAM缓冲区有限,所以限制一次读写操作的长度不超过32256字节
					MOV     A,CH375vFileSize+02H
					CLR     C
					SUBB    A,#HIGH (FILE_DATA_BUF_LEN)
					JC      SOURCE_FILE_SHORT	; 小于RAM缓冲区的长度
					JNZ     SOURCE_FILE_LONG	; 大于RAM缓冲区的长度
					MOV     A,CH375vFileSize+03H
					CLR     C
					SUBB    A,#LOW (FILE_DATA_BUF_LEN)
					JC      SOURCE_FILE_SHORT	; 一次读写操作的长度未操作RAM缓冲区的长度
SOURCE_FILE_LONG:	MOV  	SecCount,#(FILE_DATA_BUF_LEN/512)	; 限制为RAM文件读写缓冲区的长度
					MOV  	NewSize,#HIGH (FILE_DATA_BUF_LEN)
					MOV  	NewSize+01H,#LOW (FILE_DATA_BUF_LEN)
					SJMP 	READ_SOURCE_FILE
SOURCE_FILE_SHORT:	MOV  	A,CH375vFileSize+03H	; 如果原文件较小,那么使用原长度
					MOV     NewSize+01H,A		; NewSize=CH375vFileSize
					ADD  	A,#0FFH
					MOV  	A,CH375vFileSize+02H
					MOV     NewSize,A			; SecCount = (NewSize+511)/512
					ADDC 	A,#01H
					MOV  	R6,A
					CLR  	A
					ADDC 	A,CH375vFileSize+01H
					RRC     A
					MOV     A,R6
					RRC     A
					MOV  	SecCount,A			; 计算新文件的长度对应的扇区数,因为读写是以扇区为单位的
; 从原文件中读取数据到FILE_DATA_BUF中,以扇区为单位进行读写,所以速度较快,适用于数据量较大的情况
READ_SOURCE_FILE:	MOV  	R0,#LOW (mCmdParam)	; 读操作要指定读出数据的长度,以扇区为单位,mCmdParam.Read.mSectorCount
					MOV  	@R0,SecCount		; 指定读取扇区数, 本例子中是读取全部数据,如果超过63个扇区则只读取63个扇区
					PUSH	CH375vFileSize+03H
					PUSH	CH375vFileSize+02H
					PUSH	CH375vFileSize+01H
					PUSH	CH375vFileSize
					MOV		A,CH375vFileSize+03H	;默认情况下,以扇区方式读取数据时,无法读出文件尾部不足1个扇区的部分,所以必须临时加大文件长度以读取尾部零头
					ADD		A,#0FFH				;CH375vFileSize+511
					MOV		CH375vFileSize+03H,A
					MOV		A,CH375vFileSize+02H
					ADDC	A,#01H
					MOV		CH375vFileSize+02H,A
					MOV		A,CH375vFileSize+01H
					ADDC	A,#00H
					MOV		CH375vFileSize+01H,A
					MOV		A,CH375vFileSize
					ADDC	A,#00H
					MOV		CH375vFileSize,A
					LCALL	CH375FileRead		; 从文件读取数据
					LCALL	_mStopIfError		; 如果文件数据的复制方式为"外部子程序",那么由_xWriteToExtBuf子程序读出数据,否则自动读到FILE_DATA_BUF中
					POP		CH375vFileSize		; 恢复原文件长度
					POP		CH375vFileSize+01H
					POP		CH375vFileSize+02H
					POP		CH375vFileSize+03H
					MOV  	R0,#LOW (mCmdParam)	; mCmdParam.Close.mUpdateLen
					MOV     @R0,#00H
					LCALL	CH375FileClose		; 关闭文件, 用完后需要关闭文件
					LCALL	_mStopIfError
; 将FILE_DATA_BUF中的原文件字符转换为大写
					MOV     DPTR,#FILE_DATA_BUF
					MOV     R6,#00H
					MOV     R7,#00H				; 计数
CONVERT_CHAR_NEXT:	CLR     C
					MOV  	A,R7
					SUBB 	A,NewSize+01H
					MOV  	A,R6
					SUBB 	A,NewSize
					JNC  	CONVERT_CHAR_END	; 原文件字符已经处理完毕
					MOVX 	A,@DPTR
					MOV  	R5,A
					CLR  	C
					SUBB 	A,#061H
					JC   	CONVERT_CHAR_SKIP	; 不是小写字符
					MOV  	A,R5
					SETB 	C
					SUBB 	A,#07AH
					JNC  	CONVERT_CHAR_SKIP	; 不是小写字符
					MOV  	A,R5
					ANL  	A,#0DFH
					MOVX 	@DPTR,A				; 将原小写字符转换为大写后写回FILE_DATA_BUF中
CONVERT_CHAR_SKIP:	INC     DPTR				; 指向缓冲区中的下一个字符
					INC  	R7
					CJNE 	R7,#00H,CONVERT_CHAR_NEXT	; 计数
					INC  	R6
					SJMP 	CONVERT_CHAR_NEXT
CONVERT_CHAR_END:

; 在根目录下创建新文件NEWFILE.TXT,以扇区为单位进行读写,所以速度较快,适用于数据量较大的情况
CREATE_NEW_FILE:	CLR     LED_WR				; 写操作
					MOV  	R0,#LOW (mCmdParam)	; 命令参数,mCmdParam.Create.mPathName
					MOV  	DPTR,#NEW_NAME		; 指向新文件名
					LCALL	_mCopyStringToIRAM	; 复制文件名作为调用子程序的参数
					LCALL	CH375FileCreate		; 新建文件并打开,如果文件已经存在则先删除后再新建
					LCALL	_mStopIfError
					MOV  	R0,#LOW (mCmdParam)	; 写操作要指定写入数据的长度,以扇区为单位,mCmdParam.Write.mSectorCount
					MOV  	@R0,SecCount		; 写入所有扇区的数据
					LCALL	CH375FileWrite		; 向文件写入数据
					LCALL	_mStopIfError		; 如果文件数据的复制方式为"外部子程序",那么由_xReadFromExtBuf子程序写入数据,否则自动将FILE_DATA_BUF中的数据写入
					MOV     A,#0FFH
					MOV  	R0,#LOW (mCmdParam+08H)	; 输入参数mCmdParam.Modify.mFileAttr: 新的文件属性,为0FFH则不修改
					MOV  	@R0,A
					MOV  	R0,#LOW (mCmdParam+06H)	; 输入参数mCmdParam.Modify.mFileTime: 新的文件时间,为0FFFFH则不修改,使用新建文件产生的默认时间
					MOV  	@R0,A
					INC  	R0
					MOV  	@R0,A
					MOV  	R0,#LOW (mCmdParam+04H)	; 输入参数mCmdParam.Modify.mFileDate: 新的文件日期: 2004.05.18 */
					MOV  	@R0,#030H			; 日期产生方法是 Date = ((Year-1980)<<9) + (Month<<5) + Day
					INC  	R0
					MOV  	@R0,#0B2H
					MOV  	R0,#LOW (mCmdParam)	; 输入参数mCmdParam.Modify.mFileSize: 如果原文件较小,那么新的文件长度与原文件一样长,否则被RAM所限
					MOV     @R0,#00H
					INC     R0
					MOV     @R0,#00H			; 如果文件长度大于64KB,那么该单元应该写入文件长度的高23-16位
					INC     R0
					MOV  	@R0,NewSize
					INC     R0
					MOV  	@R0,NewSize+01H		; 指定新的文件长度
					LCALL	CH375FileModify		; 修改当前文件的信息,修改日期和长度
					LCALL	_mStopIfError
					MOV  	R0,#LOW (mCmdParam)	; 命令参数, mCmdParam.Close.mUpdateLen
					MOV  	@R0,#00H			; 不要自动计算文件长度,如果自动计算,那么该长度总是512的倍数
					LCALL	CH375FileClose		; 关闭文件
					LCALL	_mStopIfError

; 在根目录下创建第二个新文件MY_FILE2.TXT,以字节为单位进行读写,所以速度较慢,适用于RAM较少、数据量较小的情况
					MOV  	R0,#LOW (mCmdParam)	; 命令参数,mCmdParam.Create.mPathName
					MOV  	DPTR,#NEW_NAME_2	; 指向新文件名
					LCALL	_mCopyStringToIRAM	; 复制文件名作为调用子程序的参数
					LCALL	CH375FileCreate		; 新建文件并打开,如果文件已经存在则先删除后再新建
					LCALL	_mStopIfError
; 以字节为单位写入几十个字节的数据
					MOV  	R0,#LOW (mCmdParam+01H)	; 命令参数,mCmdParam.ByteWrite.mByteBuffer
					MOV  	DPTR,#DATA_FOR_FILE2	; 指向准备写入新文件中的数据
					LCALL	_mCopyStringToIRAM	; 将准备写入新文件中的数据复制到参数结构中
					MOV  	R0,#LOW (mCmdParam)	; 写操作要指定写入数据的长度,以扇区为单位,mCmdParam.ByteWrite.mByteCount
					MOV  	@R0,#18H			; 本次写入的字节数24,不能大于MAX_BYTE_IO
					LCALL	CH375ByteWrite		; 向文件写入数据,如果还有数据需要继续写入,那么可以再调用CH375ByteWrite,接着刚才的写入
					LCALL	_mStopIfError		; 以字节为单位写文件不需要使用FILE_DATA_BUF缓冲区,所以与复制方式无关
					MOV  	R0,#LOW (mCmdParam)	; 命令参数, mCmdParam.Close.mUpdateLen
					MOV  	@R0,#01H			; 以字节为单位写入文件后,应该自动计算文件长度
					LCALL	CH375FileClose		; 关闭文件
					LCALL	_mStopIfError

					SETB    LED_WR
; 演示处理完毕, 等待U盘拔出
					SETB 	LED_RUN
WAIT_DISK_OUT:		MOV  	A,CH375DiskStatus	; 查询CH375中断并更新中断状态,等待U盘拔出
					XRL  	A,#01H
					JZ   	DISK_OUT_NOW
					LCALL	xQueryInterrupt
					SJMP 	WAIT_DISK_OUT
DISK_OUT_NOW:		SETB 	LED_OUT				; LED灭
					LCALL	mDelay100mS
					LCALL	mDelay100mS
					LJMP 	WAIT_DISK_IN		; 主循环
;
END
