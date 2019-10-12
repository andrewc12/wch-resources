; /* 2004.06.05
; ****************************************
; **  Copyright  (C)  W.ch  1999-2004   **
; **  Web:  http://www.winchiphead.com  **
; ****************************************
; **  USB Host File Module      @CH375  **
; ****************************************
; */
; /* U盘文件读写模块, 连接方式: 3线制串口+查询 */
; /* MCS-51单片机ASM语言示例程序 */
; /* 因为使用U盘文件读写模块而不是使用U盘文件级子程序库,所以占用较少的单片机资源,可以使用89C51单片机测试 */
; /* 以字节为单位进行U盘文件读写,单片机的RAM只需要几十个字节,不需要外部RAM */
; 
;$include	(REG51.INC)
$include	(..\CH375HM.INC)

; 电路连接方式,只需要连接3根线,使用串口同步码启动操作
;   单片机    模块
;    TXD   =  SIN
;    RXD   =  SOUT
;             STA# 悬空或接高电平
;             INT# 接地或接低电平
;    GND   =  GND

LED_OUT			BIT		P1.4			;P1.4 低电平驱动LED显示,用于监控演示程序的进度

; 命令参数结构, 长度为20到60字节
mCmdParam		DATA	20H				;默认情况下该结构将占用64字节的RAM,可以修改MAX_PATH_LEN常量,当修改为32时,只占用32字节的RAM
NEW_SIZE_HIGH	DATA	1EH				;文件长度的高字节
NEW_SIZE_LOW	DATA	1FH				;文件长度的低字节

				ORG		0000H
				LJMP	MAIN
;
; 程序数据起始地址
				ORG		0100H
STR_FILE_NAME1:	DB		'\C51\CH375HFT.C',00H	;文件名,该文件在C51子目录下
STR_FILE_DATA1:	DB		'Note: ',0DH,0AH,'这个程序是以字节为单位进行U盘文件读写,',00H
STR_FILE_DATA2:	DB		'单片机只需要有几十字节的RAM就可以了',0DH,0AH,00
STR_FILE_NAME2:	DB		'\双击我吧.TXT',00H		;新文件名,在根目录下

; 从程序空间复制字符串到内部RAM缓冲区,源字符串必须以00H作为结束标志,长度不能超过255
; 入口:   DPTR 指向源字符串的起始地址, R0 目标缓冲区的起始地址
; 出口:   R7 返回字符串的长度(含字符串结束符00H)
; 使用:   DPTR, R0, R7
CopyString:		MOV		R7,#00H
CopyStringChar:	MOV		A,R7
				INC		R7
				MOVC	A,@A+DPTR
				MOV		@R0,A
				INC		R0
				JNZ		CopyStringChar		;不是字符串结束符00H,继续复制
				RET
;
; 延时100毫秒,不精确
; 使用:   R6, R7
mDelay100mS:	MOV  	R7,#0C8H
mDelay100mS_1:	MOV  	R6,#0C8H
mDelay100mS_2:	NOP
				NOP
				NOP
				DJNZ 	R6,mDelay100mS_2
				DJNZ 	R7,mDelay100mS_1
				RET  	

; 发送一个字节数据给CH375模块
; 入口:   ACC 准备发送的数据
mSendByte:		CLR     TI
				MOV     SBUF,A
				JNB     TI,$
				RET

; 从CH375模块接收一个字节数据
; 出口:   ACC 已经接收的数据
mRecvByte:		JNB     RI,$
				MOV     A,SBUF
				CLR     RI
				RET

; 执行命令
; 输入命令码和输入参数长度,返回操作状态码,输入参数和返回参数都在CMD_PARAM结构中
; 入口:   R7 命令码, R5 参数长度
; 出口:   R7 状态码, R5 返回结果的长度
; 使用:   R0, R5, R6, R7
ExecCommand:	MOV     A,#SER_SYNC_CODE1	;发送串口同步码通知模块,说明命令码开始发送,请求开始执行命令
				LCALL   mSendByte
				MOV     A,#SER_SYNC_CODE2	;用两个串口同步码代替STA#的下降沿
				LCALL   mSendByte			;上面两个串口同步码应该连续发送,如果不连续,那么间隔时间不能超过20mS,否则命令无效
				CLR     RI
				MOV     A,R7
				LCALL   mSendByte			;写入命令码
				MOV     A,R5
				LCALL   mSendByte			;写入后续参数的长度
				MOV     A,R5
				JZ      ExecCommand_Wait	;没有参数
				MOV     R0,#mCmdParam		;指向输入参数的起始地址
ExecCmdParam:	MOV     A,@R0
				LCALL   mSendByte			;依次写入参数
				INC     R0
				DJNZ    R5,ExecCmdParam
ExecCommand_Wait:							;处理数据传输,直到操作完成才退出
				LCALL   mRecvByte			;等待模块完成操作并返回操作状态
				MOV     R7,A				;状态码
				XRL     A,#ERR_SUCCESS
				JNZ		ExecCmdStatus1		;不是操作成功状态,需要进一步分析
				LCALL   mRecvByte			;操作成功,则返回结果数据的长度
				MOV     R5,A				;结果数据的长度
				JZ      ExecCmdRet			;没有结果数据,操作成功返回
				MOV     R6,A
				MOV     R0,#mCmdParam		;指向输出参数的起始地址
ExecCmdResult:	LCALL   mRecvByte			;接收结果数据并保存到参数结构中
				MOV     @R0,A
				INC     R0
				DJNZ    R6,ExecCmdResult
				SJMP    ExecCmdRet			;有结果数据,操作成功返回
ExecCmdStatus1:	CJNE	R7,#USB_INT_DISK_READ,ExecCmdStatus2	;正在从U盘读数据块,请求数据读出
				SJMP    ExecCmdRet			;本程序只使用以字节为单位的文件读写子程序,所以正常情况下不会收到该状态码,当作操作失败返回
ExecCmdStatus2:	CJNE	R7,#USB_INT_DISK_WRITE,ExecCmdStatus3	;正在向U盘写数据块,请求数据写入
				SJMP    ExecCmdRet			;本程序只使用以字节为单位的文件读写子程序,所以正常情况下不会收到该状态码,当作操作失败返回
ExecCmdStatus3:	CJNE	R7,#USB_INT_DISK_RETRY,ExecCmdStatus4	;读写数据块失败重试
				SJMP    ExecCmdRet			;本程序只使用以字节为单位的文件读写子程序,所以正常情况下不会收到该状态码,当作操作失败返回
ExecCmdStatus4:	CJNE    R7,#ERR_USB_CONNECT,ExecCmdStatus5
				LCALL   mDelay100mS			;U盘刚刚连接或者断开,应该延时几十毫秒再操作
				MOV     R7,#ERR_USB_CONNECT
ExecCmdStatus5:	SJMP    ExecCmdRet			;操作失败
ExecCmdRet:		RET
; END OF ExecCommand

; 检查操作状态,如果错误则停机
; 输入:   R7 为操作状态码
mStopIfError:	MOV		A,R7
				JNZ		mStopIfError_LED	;状态码是错误
				RET
mStopIfError_LED:							;LED闪烁
				CLR		A
				MOV		C,LED_OUT
				MOV		ACC.0,C
				XRL		A,#01H
				MOV		C,ACC.0
				MOV		LED_OUT,C
				LCALL	mDelay100mS
				SJMP	mStopIfError_LED
;
; 主程序
MAIN:			CLR		A
				MOV		PSW,A
				MOV		IE,A
				MOV		SP,#60H
				MOV		A,#0FFH
				MOV		P0,A
				MOV		P1,A
				MOV		P2,A
				MOV		P3,A
				CLR  	LED_OUT				;开机后LED亮一下以示工作
				LCALL	mDelay100mS			;延时100毫秒,CH375模块上电后需要100毫秒左右的复位时间
				LCALL	mDelay100mS
				SETB 	LED_OUT
;其它电路初始化
;设置与CH375模块通讯的串口
				MOV     SCON,#50H
				MOV     PCON,#80H
				MOV     TMOD,#20H
				MOV     TH1,#0E6H			;24MHz晶振, 4800bps
				SETB    TR1
;初始化完成
				NOP
MAIN_LOOP:									;主循环
;可以在打算读写U盘时再查询,没有必要一直连续不停地查询,可以让单片机做其它事,没事可做就延时等待一会再查询
				MOV		R5,#00H				;没有命令参数
				MOV		R7,#CMD_QueryStatus	;使用查询方式看U盘是否连接
				LCALL	ExecCommand			;查询当前模块的状态
				LCALL	mStopIfError		;错误则停机
				MOV		A,mCmdParam+1		;Status.mDiskStatus
				CLR     C
				SUBB	A,#DISK_CONNECT
				JNC		MAIN_CONNECT		;U盘已经连接
				LCALL	mDelay100mS			;可以让单片机做其它事,没事可做就延时等待一会再查询
				LCALL	mDelay100mS
				SJMP	MAIN_LOOP
MAIN_CONNECT:	LCALL	mDelay100mS			;U盘已经连接,延时,可选操作,有的USB存储器需要几十毫秒的延时
				LCALL	mDelay100mS
				CLR  	LED_OUT				;LED亮说明U盘连接
; 检查U盘是否准备好,大多数U盘不需要这一步,但是某些U盘必须要执行这一步才能工作
				MOV		R3,#05H
WAIT_READY:		LCALL	mDelay100mS
				MOV     R5,#00H
				MOV     R7,#CMD_DiskReady
				LCALL   ExecCommand			;查询磁盘是否准备好
				MOV     A,R7
				JZ		DISK_IS_READY
				DJNZ	R3,WAIT_READY		;U盘尚未准备好
DISK_IS_READY:
;读取原文件
				MOV		DPTR,#STR_FILE_NAME1
				MOV		R0,#mCmdParam		;Open.mPathName
				LCALL	CopyString			;复制文件名
				MOV		A,R7
				MOV		R5,A				;参数长度为文件名长度
				MOV		R7,#CMD_FileOpen	;打开文件
				LCALL	ExecCommand			;执行打开文件操作
				CJNE	R7,#ERR_MISS_DIR,MAIN_OPEN_J1
				SJMP	MAIN_OPEN_MISS		;ERR_MISS_DIR说明没有找到C51子目录
MAIN_OPEN_J1:	CJNE	R7,#ERR_MISS_FILE,MAIN_OPEN_J2
MAIN_OPEN_MISS:								;ERR_MISS_FILE说明没有找到文件
				LJMP	MAIN_CREATE_NEW		;创建新文件
MAIN_OPEN_J2:	LCALL	mStopIfError		;源文件成功打开
;以字节为单位读取原文件
READ_FILE_BYTE:	MOV		R2,#10H				;请求读出16字节数据, 单次读写的长度不能超过 sizeof( mCmdParam.ByteWrite.mByteBuffer )
				MOV		mCmdParam+0,R2		;ByteRead.mByteCount
				MOV		R5,#01H				;只有一个输入参数
				MOV		R7,#CMD_ByteRead	;从文件以字节为单位读取数据块
				LCALL	ExecCommand			;从文件读取数据,如果文件比较大,一次读不完,可以再用命令CMD_ByteRead继续读取,文件指针自动向后移动
				LCALL	mStopIfError
; 在mCmdParam+0单元是实际读出的数据长度,从mCmdParam+1单元开始是读出的数据块
				MOV     A,mCmdParam+0		;ByteRead.mByteCount
				JZ      READ_FILE_END		;实际读出的数据长度小于请求读出的长度则说明文件结束
				MOV     R7,A
				MOV     R0,#mCmdParam+1		;ByteRead.mByteBuffer,从mCmdParam+1单元开始是读出的数据块
GET_READ_BYTE:	MOV     A,@R0
;				MOV     ?,A					;处理刚读出的数据
				INC     R0
				DJNZ    R7,GET_READ_BYTE
				MOV     A,mCmdParam+0		;实际读出的数据长度
				CLR     C
				SUBB    A,R2
				JC      READ_FILE_END		;实际读出的数据长度小于请求读出的长度则说明文件结束
				SJMP    READ_FILE_BYTE		;文件未结束,继续读出数据
READ_FILE_END:	MOV		A,#00H
				MOV		mCmdParam+0,A		;Close.mUpdateLen
				MOV		R5,#01H				;只有一个输入参数
				MOV		R7,#CMD_FileClose	;关闭文件
				LCALL	ExecCommand			;关闭文件
				LCALL	mStopIfError
;产生新文件
MAIN_CREATE_NEW:
				MOV		DPTR,#STR_FILE_NAME2
				MOV		R0,#mCmdParam		;Create.mPathName
				LCALL	CopyString			;复制文件名
				MOV		A,R7
				MOV		R5,A				;参数长度为文件名长度
				MOV		R7,#CMD_FileCreate
				LCALL	ExecCommand			;新建文件并打开,如果文件已经存在则先删除后再新建
				LCALL	mStopIfError
;以字节为单位写入第一组数据块
				MOV		DPTR,#STR_FILE_DATA1
				MOV		R0,#mCmdParam+1		;ByteWrite.mByteBuffer
				LCALL	CopyString			;复制数据块
				MOV     A,R7
				MOV		mCmdParam+0,A		;ByteWrite.mByteCount
				INC     A
				MOV		R5,A				;输入参数的长度为写入数据块的长度加一个长度单元
				MOV		R7,#CMD_ByteWrite	;以字节为单位向文件写入数据
				LCALL	ExecCommand			;向文件写入数据,如果文件比较大,一次写不完,可以再用命令CMD_ByteWrite继续写入,文件指针自动向后移动
				LCALL	mStopIfError
;以字节为单位写入第二组数据块
				MOV		DPTR,#STR_FILE_DATA2
				MOV		R0,#mCmdParam+1		;ByteWrite.mByteBuffer
				LCALL	CopyString			;复制数据块
				MOV     A,R7
				MOV		mCmdParam+0,A		;ByteWrite.mByteCount
				INC     A
				MOV		R5,A				;输入参数的长度为写入数据块的长度加一个长度单元
				MOV		R7,#CMD_ByteWrite	;以字节为单位向文件写入数据
				LCALL	ExecCommand			;向文件写入数据,如果文件比较大,一次写不完,可以再用命令CMD_ByteWrite继续写入,文件指针自动向后移动
				LCALL	mStopIfError
				MOV		A,#01H				;请求模块自动计算文件长度
				MOV		mCmdParam+0,A		;Close.mUpdateLen
				MOV		R5,#01H				;只有一个输入参数
				MOV		R7,#CMD_FileClose	;关闭文件
				LCALL	ExecCommand			;关闭文件
				LCALL	mStopIfError
;等待U盘断开,仅作演示,实际应用中不必考虑U盘是否断开
MAIN_TAKE_OUT:	MOV		R5,#00H				;没有命令参数
				MOV		R7,#CMD_QueryStatus	;使用查询方式看U盘是否断开
				LCALL	ExecCommand			;查询当前模块的状态
				LCALL	mStopIfError		;错误则停机
				MOV		A,mCmdParam+1		;Status.mDiskStatus
				XRL		A,#DISK_DISCONNECT
				JZ		MAIN_DISCONNECT		;U盘已经断开
				LCALL	mDelay100mS			;延时等待一会再查询
				LCALL	mDelay100mS
				SJMP	MAIN_TAKE_OUT
MAIN_DISCONNECT:	SETB	LED_OUT			;LED灭
				LJMP	MAIN_LOOP			;等待下一个U盘连接
; END main
;
END
