; /* 2004.06.05
; ****************************************
; **  Copyright  (C)  W.ch  1999-2007   **
; **  Web:  http://www.winchiphead.com  **
; ****************************************
; **  USB Host File Module      @CH375  **
; ****************************************
; */
; /* U盘文件读写模块, 连接方式: 并口+查询 */
; /* MCS-51单片机ASM语言示例程序, 仅适用于V3.0A及以上版本的模块 */
; /* 因为使用U盘文件读写模块而不是使用U盘文件级子程序库,所以占用较少的单片机资源,可以使用89C51单片机测试 */
; 
;$include	(REG51.INC)
$include	(..\CH375HM.INC)

;   电路连接方式
;   单片机    模块
;    P0    =  D0-D7
;    RD    =  RD#
;    WR    =  WR#
;    ?     =  CS#   如果没有外部RAM,那么CS#=P26,如果有超过16KB的外部RAM,那么CS#=P27 & ! P26 & ...,所以CS#的片选地址为BXXXH
;    P20   =  A0
;    INT0  =  INT#  虽然连接到INT0,但是本程序只是查询模块的INT#的状态,所以可以用P1口等普通I/O引脚代替INT0

CH375HM_INDEX	XDATA	0BCF0H			;CH375模块的索引端口的I/O地址
CH375HM_DATA	XDATA	0BDF1H			;CH375模块的数据端口的I/O地址

CH375HM_INT		BIT		INT0			;假定CH375模块的INT#引脚连接到单片机的INT0引脚

LED_OUT			BIT		P1.4			;P1.4 低电平驱动LED显示,用于监控演示程序的进度

; 假定文件数据缓冲区: ExtRAM: 0000H-7FFFH
DATA_BUF		XDATA	00000H			;外部RAM的文件数据缓冲区,从该单元开始的缓冲区长度不小于一次读写的数据长度,最少为512字节

; 命令参数结构, 长度为20到60字节
mCmdParam		DATA	20H				;默认情况下该结构将占用64字节的RAM,可以修改MAX_PATH_LEN常量,当修改为32时,只占用32字节的RAM
SECTOR_COUNT	DATA	1DH				;文件占用扇区数
NEW_SIZE_HIGH	DATA	1EH				;文件长度的高字节
NEW_SIZE_LOW	DATA	1FH				;文件长度的低字节

				ORG		0000H
				LJMP	MAIN
;
; 程序数据起始地址
				ORG		0100H
STR_FILE_NAME1:	DB		'\C51\CH375HFT.C',00H	;文件名,该文件在C51子目录下
STR_FILE_DATA:	DB		'Note: ',0DH,0AH
				DB      '原本是打算将/C51/CH375HFT.C文件中的小写字母转成大写后写入新的文件,但是找不到这个文件',0DH,0AH,00H
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

; 执行命令
; 输入命令码和输入参数长度,返回操作状态码,输入参数和返回参数都在CMD_PARAM结构中
; 入口:   R7 命令码, R5 参数长度, 对于读写文件数据命令, DPTR 为外部RAM缓冲区起始地址
; 出口:   R7 状态码, 对于读写文件数据命令, DPTR 为数据终止地址
; 使用:   R0, R4, R5, R6, R7
ExecCommand:	MOV     R6,DPH				;对于读写文件数据命令,保存外部RAM缓冲区起始地址
				MOV     R4,DPL
				MOV  	DPTR,#CH375HM_INDEX
				MOV     A,#PARA_COMMAND_ADDR
				MOVX 	@DPTR,A				;索引地址为PARA_COMMAND_ADDR
				MOV  	DPTR,#CH375HM_DATA
				MOV  	A,R7
				MOVX 	@DPTR,A				;向索引地址PARA_COMMAND_ADDR写入命令码
				MOV  	A,R5
				MOV     R7,A				;后续参数的长度
				JZ      ExecCommand_W0		;没有参数
				MOV  	DPTR,#CH375HM_INDEX
				MOV     A,#PARA_BUFFER_ADDR
				MOVX 	@DPTR,A				;指向缓冲区
				MOV     R0,#mCmdParam		;指向输入参数的起始地址
				MOV  	DPTR,#CH375HM_DATA
ExecCommand_WP:	MOV		A,@R0
				MOVX	@DPTR,A				;从索引地址PARA_BUFFER_ADDR开始,写入参数
				INC     R0
				DJNZ    R7,ExecCommand_WP
ExecCommand_W0:	MOV  	DPTR,#CH375HM_INDEX
				MOV     A,#PARA_CMD_LEN_ADDR
				MOVX    @DPTR,A				;索引地址PARA_CMD_LEN_ADDR
				MOV  	DPTR,#CH375HM_DATA
				MOV     A,R5
				ORL     A,#PARA_CMD_BIT_ACT	;最高位通知模块,说明命令包已经写入,请求开始执行命令
				MOVX    @DPTR,A				;向索引地址PARA_CMD_LEN_ADDR写入后续参数的长度
ExecCommand_Wait:

$IF 1
				JB		CH375HM_INT,$		;等待模块完成操作产生低电平中断,并返回操作状态
$ELSE
				MOV  	DPTR,#CH375HM_INDEX	;如果不需要扇区方式读写,那么可以查询模块的命令码单元代替查询模块INT#引脚
				MOV     A,#PARA_COMMAND_ADDR
				MOVX    @DPTR,A
				MOV  	DPTR,#CH375HM_DATA
				MOVX    A,@DPTR				;模块操作完成时该值会清0,仅适用于非扇区方式读写
				JNZ     ExecCommand_Wait	;等待中断
$ENDIF

;最佳检测方式是对模块的INT#信号进行下降沿边沿检测
				MOV  	DPTR,#CH375HM_INDEX
				MOV  	A,#PARA_STATUS_ADDR
				MOVX 	@DPTR,A				;写入索引地址PARA_STATUS_ADDR
				MOV  	DPTR,#CH375HM_DATA
				MOVX 	A,@DPTR				;从索引地址PARA_STATUS_ADDR读取中断状态
				MOV		R7,A				;操作状态码
				MOV  	DPTR,#CH375HM_INDEX
				MOV     A,#PARA_CMD_LEN_ADDR
				MOVX    @DPTR,A				;启动操作的单元地址
				MOV  	DPTR,#CH375HM_DATA
				MOV     A,#PARA_CMD_BIT_INACT
				MOVX    @DPTR,A				;中断应答,取消来自模块的中断请求
;因为模块在收到中断应答后3uS之内才撤消中断请求,所以,如果是查询INT#信号的低电平,那么在发出中断应答后3uS之内不应该再查询INT#信号的状态
;但是由于51单片机较慢,下面的处理时间已经超过3uS,所以不必另加延时等待模块撤消中断请求
				CJNE    R7,#ERR_SUCCESS,ExecCommand_J0	;不是操作成功的状态码
				MOV  	DPTR,#CH375HM_INDEX	;操作成功
				MOV		A,#PARA_STS_LEN_ADDR
				MOVX 	@DPTR,A
				MOV  	DPTR,#CH375HM_DATA
				MOVX 	A,@DPTR				;从索引地址PARA_STS_LEN_ADDR读取返回结果数据的长度
				MOV  	R7,A
				JZ      ExecCommand_R0		;没有结果数据
				MOV  	DPTR,#CH375HM_INDEX
				MOV     A,#PARA_BUFFER_ADDR
				MOVX 	@DPTR,A				;指向缓冲区
				MOV     R0,#mCmdParam		;指向输出参数的起始地址
				MOV  	DPTR,#CH375HM_DATA	;从索引地址PARA_BUFFER_ADDR开始,读取结果
ExecCommand_RP:	MOVX	A,@DPTR
				MOV		@R0,A
				INC		R0
				DJNZ 	R7,ExecCommand_RP
ExecCommand_R0:	LJMP	ExecCommand_Ret
ExecCommand_J0:	CJNE    R7,#USB_INT_DISK_READ,ExecCommand_J1
				MOV  	DPTR,#CH375HM_INDEX	;正在从U盘读数据块,请求数据读出
				MOV		A,#PARA_BUFFER_ADDR
				MOVX 	@DPTR,A				;指向缓冲区
				MOV  	R7,#040H			;一次读取64个字节
RD_SEC_BYTE:	MOV     DPTR,#CH375HM_DATA
				MOVX	A,@DPTR				;依次输入64字节的数据
				MOV     DPH,R6
				MOV     DPL,R4				;缓冲区地址
				MOVX    @DPTR,A				;读取的数据保存到外部缓冲区
				INC     DPTR
				MOV     R6,DPH
				MOV     R4,DPL
				DJNZ    R7,RD_SEC_BYTE		;从索引地址0到63依次读出64字节的数据
				MOV  	DPTR,#CH375HM_INDEX
				MOV     A,#PARA_CMD_LEN_ADDR
				MOVX    @DPTR,A				;启动操作的单元地址
				MOV  	DPTR,#CH375HM_DATA
				MOV     A,#PARA_CMD_BIT_ACT
				MOVX    @DPTR,A				;通知模块继续,说明64字节数据已经读取完成
				LJMP    ExecCommand_Wait
ExecCommand_J1:	CJNE    R7,#USB_INT_DISK_WRITE,ExecCommand_J2
				MOV  	DPTR,#CH375HM_INDEX	;正在向U盘写数据块,请求数据写入
				MOV		A,#PARA_BUFFER_ADDR
				MOVX 	@DPTR,A				;指向缓冲区
				MOV  	R7,#040H			;一次写入64个字节
WR_SEC_BYTE:	MOV     DPH,R6
				MOV     DPL,R4				;缓冲区地址
				MOVX    A,@DPTR				;写入的数据来自外部缓冲区
				INC     DPTR
				MOV     R6,DPH
				MOV     R4,DPL
				MOV     DPTR,#CH375HM_DATA
				MOVX    @DPTR,A				;依次输出64字节的数据
				DJNZ    R7,WR_SEC_BYTE		;向索引地址0到63依次写入64字节的数据
				MOV  	DPTR,#CH375HM_INDEX
				MOV     A,#PARA_CMD_LEN_ADDR
				MOVX    @DPTR,A				;启动操作的单元地址
				MOV  	DPTR,#CH375HM_DATA
				MOV     A,#PARA_CMD_BIT_ACT
				MOVX    @DPTR,A				;通知模块继续,说明64字节数据已经写入完成
				LJMP	ExecCommand_Wait
ExecCommand_J2:	CJNE    R7,#USB_INT_DISK_RETRY,ExecCommand_J3
				MOV  	DPTR,#CH375HM_INDEX	;读写数据块失败重试,应该向回修改缓冲区指针
				MOV		A,#PARA_BUFFER_ADDR
				MOVX 	@DPTR,A				;指向缓冲区
				MOV		DPTR,#CH375HM_DATA
				MOVX	A,@DPTR				;大端模式下为回改指针字节数的高8位,如果是小端模式那么接收到的是回改指针字节数的低8位
				MOV		R7,A
				MOVX	A,@DPTR				;大端模式下为回改指针字节数的低8位,如果是小端模式那么接收到的是回改指针字节数的高8位
				CLR		C
				XCH		A,R4
				SUBB	A,R4
				MOV		R4,A				;这是大端模式下的回改指针
				MOV		A,R6
				SUBB	A,R7
				MOV		R6,A
				MOV  	DPTR,#CH375HM_INDEX
				MOV     A,#PARA_CMD_LEN_ADDR
				MOVX    @DPTR,A				;启动操作的单元地址
				MOV  	DPTR,#CH375HM_DATA
				MOV     A,#PARA_CMD_BIT_ACT
				MOVX    @DPTR,A				;通知模块继续,说明重试状态码已经处理完成
				LJMP	ExecCommand_Wait
ExecCommand_J3:								;R7为操作完成状态,操作出现错误
				CJNE    R7,#ERR_DISK_DISCON,ExecCommand_J4	;检查U盘断开事件
				SJMP    ExecCommandEv
ExecCommand_J4:	CJNE    R7,#ERR_USB_CONNECT,ExecCommand_J5	;检查U盘插入事件
ExecCommandEv:	LCALL	mDelay100mS			;U盘刚刚连接或者断开,应该延时几十毫秒再操作
				JB		CH375HM_INT,ExecCommand_Ret	;没有中断则返回
				LJMP    ExecCommand_Wait	;如果仍然有中断请求说明之前的中断是U盘插拔通知中断,现在再处理命令完成中断
ExecCommand_J5:	SJMP    ExecCommand_Ret
ExecCommand_Ret:
;				JNB		CH375HM_INT,$		;等待模块的中断撤消
				MOV     DPH,R6				;对于读写文件数据命令
				MOV     DPL,R4				;从DPTR返回新的缓冲区地址
				RET
; END OF ExecCommand

; 检查操作状态,如果错误则停机,应该替换为实际的处理措施
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
				LCALL	mDelay100mS
				LCALL	mDelay100mS
				SETB 	LED_OUT
;其它电路初始化
;初始化完成
				NOP
MAIN_LOOP:									;主循环
;可以在打算读写U盘时再查询,没有必要一直连续不停地查询,可以让单片机做其它事,没事可做就延时等待一会再查询
				MOV		R5,#00H				;没有命令参数
				MOV		R7,#CMD_QueryStatus	;使用查询方式看U盘是否连接
				LCALL	ExecCommand			;查询当前模块的状态
				LCALL	mStopIfError		;错误则停机
				MOV		A,mCmdParam+1		;Status.mDiskStatus
				XRL		A,#DISK_CONNECT
				JZ		MAIN_CONNECT		;U盘已经连接
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
;没有原文件则自行产生数据
				MOV		DPTR,#DATA_BUF		;因为没有找到源文件,所以自行产生数据写入新文件
				MOV		R5,#00H
MAIN_COPY_STR:	PUSH	DPH
				PUSH    DPL
				MOV		DPTR,#STR_FILE_DATA
				MOV		A,R5
				INC		R5
				MOVC	A,@A+DPTR			;从程序空间复制字符串作为新文件的内容
				POP     DPL
				POP     DPH
				MOVX	@DPTR,A
				INC     DPTR
				JNZ		MAIN_COPY_STR		;未到字符串结束符00H
MAIN_FILL_STR:	MOV		A,#24H				;'$'
				MOVX	@DPTR,A
				INC		DPTR
				MOV		A,DPH
				CJNE	A,#HIGH (DATA_BUF) + 2,MAIN_FILL_STR	;填充满512字节(一个扇区)
				MOV		NEW_SIZE_HIGH,#02H
				MOV		NEW_SIZE_LOW,#00H	;新文件的长度,512字节
				MOV		SECTOR_COUNT,#01H	;新文件的长度只有一个扇区,计算文件的扇区数,因为读写是以扇区为单位的
				LJMP	MAIN_CREATE_NEW		;创建新文件
MAIN_OPEN_J2:	LCALL	mStopIfError		;源文件成功打开
;查询当前文件的信息并计算长度
				MOV		R5,#00H
				MOV		R7,#CMD_FileQuery	;查询当前文件的信息,没有输入参数
				LCALL	ExecCommand
				LCALL	mStopIfError
				MOV		NEW_SIZE_HIGH,#10H
				MOV		NEW_SIZE_LOW,#00H	;新文件的长度不超过4096,实际上模块本身不限制长度
				MOV		R0,#mCmdParam
				MOV		A,@R0				;Modify.mFileSize.31-24,高字节在前
				JNZ		MAIN_QUERY_J1		;文件太大
				INC		R0
				MOV		A,@R0				;Modify.mFileSize.23-16
				JNZ		MAIN_QUERY_J1		;文件太大
				INC		R0
				MOV		A,@R0				;Modify.mFileSize.15-8
				CLR		C
				SUBB	A,#10H				;本示例程序将文件长度限制在4096
				JNC		MAIN_QUERY_J1		;文件长度超过4096
				MOV		A,@R0				;Modify.mFileSize.15-8
				MOV		NEW_SIZE_HIGH,A
				INC		R0
				MOV		A,@R0				;Modify.mFileSize.7-0
				MOV		NEW_SIZE_LOW,A		;新文件的长度
MAIN_QUERY_J1:	MOV		A,NEW_SIZE_LOW
				ADD		A,#0FFH
				MOV		A,NEW_SIZE_HIGH
				ADDC	A,#01H
				CLR		C
				RRC		A					;每个扇区是512字节,所以高8位除以2
				MOV		SECTOR_COUNT,A		;计算文件的扇区数,因为读写是以扇区为单位的
				JNZ		MAIN_QUERY_J3
				INC		SECTOR_COUNT		;至少一个扇区
MAIN_QUERY_J3:
;读取原文件
				MOV		A,SECTOR_COUNT
				MOV		mCmdParam+0,A		;Read.mSectorCount
				MOV		R5,#01H				;只有一个输入参数
				MOV		R7,#CMD_FileRead	;从文件读取数据
				MOV		DPTR,#DATA_BUF		;对于读写文件数据的操作,指定外部RAM缓冲区,存放数据的缓冲区的起始地址
				LCALL	ExecCommand			;从文件读取数据,如果文件比较大,一次读不完,可以再用命令CMD_FileRead继续读取,文件指针自动向后移动
				LCALL	mStopIfError
				MOV		A,#00H
				MOV		mCmdParam+0,A		;Close.mUpdateLen
				MOV		R5,#01H				;只有一个输入参数
				MOV		R7,#CMD_FileClose	;关闭文件
				LCALL	ExecCommand			;关闭文件
				LCALL	mStopIfError
;将源文件中的小写字符转换为大写,准备写入新文件
				MOV  	DPTR,#DATA_BUF
				MOV		R7,NEW_SIZE_HIGH
				MOV		R6,NEW_SIZE_LOW
MAIN_CHAR_CASE:	CJNE	R6,#00H,MAIN_CHAR_CNT1
				CJNE	R7,#00H,MAIN_CHAR_CNT2
				SJMP	MAIN_CHAR_END
MAIN_CHAR_CNT2:	DEC		R7
MAIN_CHAR_CNT1:	DEC		R6					;计数
				MOVX 	A,@DPTR
				MOV		R5,A
				CLR  	C
				SUBB 	A,#61H				;'a'
				JC   	MAIN_CHAR_SKIP		;不是小写字母
				MOV  	A,R5
				CLR		C
				SUBB 	A,#7BH				;'z'+1
				JNC  	MAIN_CHAR_SKIP		;不是小写字母
				MOV		A,R5
				ANL		A,#0DFH				;小写转大写字母
				MOVX	@DPTR,A
MAIN_CHAR_SKIP:	INC		DPTR
				SJMP	MAIN_CHAR_CASE
MAIN_CHAR_END:	NOP							;已经将小写字母转大写
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
				MOV		A,SECTOR_COUNT
				MOV		mCmdParam+0,A		;Write.mSectorCount
				MOV		R5,#01H				;只有一个输入参数
				MOV		R7,#CMD_FileWrite	;向文件写入数据
				MOV		DPTR,#DATA_BUF		;对于读写文件数据的操作,指定外部RAM缓冲区,存放数据的缓冲区的起始地址
				LCALL	ExecCommand			;向文件写入数据,如果文件比较大,一次写不完,可以再用命令CMD_FileWrite继续写入,文件指针自动向后移动
				LCALL	mStopIfError
				MOV		A,#01H				;自动计算文件长度,长度总是512的倍数
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
