; CH372, CH375
; Interface Program for MCS-51
; 使用内置固件的USB设备方式
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.09
;
;****************************************************************************
; 程序运行环境
;
;CH375_CS_			BIT   P2.7            ;MCS-51单片机的A15引脚,用于CH375片选,不必定义
;CH375_A0			BIT   P2.0            ;MCS-51单片机的A8引脚,用于CH375地址0,不必定义,也可用外部74373锁存的A0地址
;CH375_INT_			BIT   P3.2            ;MCS-51单片机的INT0引脚,用于CH375中断请求,无需定义
;CH375_WR_			BIT   P3.6            ;MCS-51单片机的RD引脚,用于CH375写选通,无需定义
;CH375_RD_			BIT   P3.7            ;MCS-51单片机的WR引脚,用于CH375读选通,无需定义
;
; 本程序适用于时钟频率不高于24MHz的标准MCS-51单片机, 如果高于24MHz则应该适当增加延时, 如果低于24MHz则可以适当减少延时
;
; 需要主程序定义的参数
CH375_CMD_PORT		EQU    0BDF1H         ;CH375命令口的地址,地址译码后自动片选
CH375_DAT_PORT		EQU    0BCF0H         ;CH375命令口的地址,地址译码后自动片选
;
; 定义自己的USB设备的厂商VID是4348H, 产品PID是5537H
;USB_VENDOR_ID_L	EQU    48H            ;定义VENDOR ID的低字节
;USB_VENDOR_ID_H	EQU    43H            ;定义VENDOR ID的高字节
;USB_PRODUCT_ID_L	EQU    37H            ;定义PRODUCT ID的低字节
;USB_PRODUCT_ID_H	EQU    55H            ;定义PRODUCT ID的高字节
;
RECV_LEN			DATA   3FH            ;刚接收到的数据的长度
RECV_BUFFER			DATA   40H            ;数据缓冲区,用于保存接收到的下传数据,长度为0到64字节
;SEND_LEN			DATA   3EH            ;准备下次发送的数据长度
;SEND_BUFFER		DATA   80H            ;准备下次发送的数据缓冲区
; 由于MCS-51单片机存取外部RAM的读写速度低于内部RAM, 并且需要用到DPTR, 所以读写速度较慢, 其程序可以参考本程序修改
;
; 定义位标志
FLAG_SEND_WAIT		BIT    20H.1          ;发送等待标志,1指示有数据块正在CH375中等待发送
FLAG_INT_WAIT		BIT    20H.2          ;中断数据等待发送标志,1指示有中断数据正在CH375中等待发送
;
;
$INCLUDE			(CH375INC.ASM)
;
;****************************************************************************
;
;对外接口子程序,仅供参考
;
;  1. CH375_INIT     初始化子程序
;       USE:   ACC, R7, DPTR
;
;  2. CH375_UPLOAD   上传数据块子程序(通过批量端点上传)
;       ENTRY: R0 指向存放了准备上传数据的缓冲区, R7 准备上传的数据长度, 有效数值是0到64
;       USE:   ACC, R0, R7, DPTR
;
;  3. CH375_UP_INT   上传中断数据子程序(通过中断端点上传),可选子程序
;       ENTRY: R0 指向存放了准备上传数据的缓冲区, R7 准备上传的数据长度, 有效数值是0到8
;       USE:   ACC, R0, R7, DPTR
;
;  4. CH375_INTER    中断服务子程序
;       USE:   堆栈8字节
;
;****************************************************************************
;主程序的例子
;				ORG   0000H					;复位后单片机入口
;				LJMP  START
;				ORG   0003H					;CH375中断
;				LJMP  CH375_INTER
;
;START:			NOP
;				...
;				CALL  CH375_INIT			;初始化
;				...
;				SETB  EA					;允许中断
;				...
;WAIT_REQUEST:	NOP
;				SJMP  WAIT_REQUEST			;主循环
;
;****************************************************************************
;
; 初始化子程序
; USE:   ACC, R7, DPTR
CH375_INIT:		CLR   FLAG_SEND_WAIT		;清发送等待标志,1指示有数据块正在CH375中等待发送
				CLR   FLAG_INT_WAIT			;清中断数据发送等待标志,1指示有中断数据正在CH375中等待发送
; 测试CH375是否正常工作,可选操作
;				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
;				MOV   A,#CMD_CHECK_EXIST
;				MOVX  @DPTR,A				;测试CH375是否正常工作,可选操作
;				NOP							;如果时钟频率低于16MHz则无需该指令延时
;				DEC   DPL					;数据口地址
;				MOV   A,#55H
;				MOVX  @DPTR,A				;写入测试数据
;				NOP							;如果时钟频率低于16MHz则无需该指令延时
;				CPL   A
;				MOV   R7,A					;取反数据
;				MOVX  A,@DPTR				;返回数据应该是测试数据取反
;				XRL   A,R7
;				JZ    CH375_INIT_OK			;测试通过
;				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
;				MOV   A,#CMD_RESET_ALL
;				MOV   R7,#50H				;多次重复发命令
;CH375_INIT_HR:	MOVX  @DPTR,A				;执行硬件复位
;				DJNZ  R7,CH375_INIT_HR
;				MOV   R7,#0A0H				;160*256uS=40mS
;CH375_INIT_D2:	CLR   A
;CH375_INIT_D1:	DJNZ  ACC,CH375_INIT_D1		;延时1uS
;				DJNZ  R7,CH375_INIT_D2		;延时256uS
;				SJMP  CH375_INIT			;再次测试
;CH375_INIT_OK:
; 设置外部自定义的USB设备VID和PID,可选操作,不执行该命令则使用默认的VID和PID
;				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
;				MOV   A,#CMD_SET_USB_ID
;				MOVX  @DPTR,A				;设置外部自定义的USB设备VID和PID,可选操作
;				NOP							;如果时钟频率低于16MHz则无需该指令延时
;				DEC   DPL					;数据口地址
;				MOV   A,#USB_VENDOR_ID_L
;				MOVX  @DPTR,A				;写入厂商ID的低字节
;				MOV   A,#USB_VENDOR_ID_H
;				MOVX  @DPTR,A				;写入厂商ID的高字节
;				MOV   A,#USB_PRODUCT_ID_L
;				MOVX  @DPTR,A				;写入产品ID的低字节
;				MOV   A,#USB_PRODUCT_ID_H
;				MOVX  @DPTR,A				;写入产品ID的高字节
;				NOP
; 设置USB工作模式
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_SET_USB_MODE
				MOVX  @DPTR,A				;设置USB工作模式
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				DEC   DPL					;数据口地址
				MOV   A,#02H
				MOVX  @DPTR,A				;设置为使用内置固件的USB设备方式
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				NOP							;如果时钟频率低于16MHz则无需该指令延时
CH375_INIT_WT:	MOVX  A,@DPTR				;返回操作状态
				XRL   A,#CMD_RET_SUCCESS
				JNZ   CH375_INIT_WT			;等待操作成功,通常需要等待10uS-20uS
; 下述三条指令用于启用中断
				CLR   IT0					;置外部信号为低电平触发
				SETB  PX0					;置高优先级
				CLR   IE0					;清中断标志
				SETB  EX0					;允许CH375中断
				RET
;
; 上传数据块子程序(通过批量端点上传)
; ENTRY: R0 指向存放了准备上传数据的缓冲区, R7 准备上传的数据长度, 有效数值是0到64
; USE:   ACC, R0, R7, DPTR
CH375_UPLOAD:	NOP							;为了防止计算机程序没有读取数据而导致单片机一直在此等待,可以计算超时,参考CH375_UP_INT子程序
				JB    FLAG_SEND_WAIT,CH375_UPLOAD	;如果正在等待发送(前次发送尚未完成)则继续等待,超过正常应该取走数据的时间未取走数据则认定为超时,可以跳出等待
				CLR   EX0					;为了防止中途被中断而乱了顺序,必须先禁止中断
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA7
				MOVX  @DPTR,A				;向USB端点2的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				DEC   DPL					;数据口地址
				MOV   A,R7
				MOVX  @DPTR,A				;首先写入后续数据长度
CH375_UPLOAD_1:	MOV   A,@R0
				INC   R0
				MOVX  @DPTR,A				;发送数据
				DJNZ  R7,CH375_UPLOAD_1		;继续发送数据直至结束
				SETB  FLAG_SEND_WAIT		;置等待发送标志
				SETB  EX0					;允许中断
				RET
;
; 上传中断数据子程序(通过中断端点上传),可选子程序
; ENTRY: R0 指向存放了准备上传数据的缓冲区, R7 准备上传的数据长度, 有效数值是0到8
; USE:   ACC, R0, R7, DPTR
CH375_UP_INT:	MOV   A,#0FFH
CH375_UP_INT_W:	JNB   FLAG_INT_WAIT,CH375_UP_INT_G	;前次中断数据已经被PC机取走则继续
				NOP							;等待发送中断数据
				NOP
				NOP							;最多等待2毫秒,以12MHz时钟计算
				NOP
				DJNZ  ACC,CH375_UP_INT_W	;继续等待,满2毫秒后超时,建议将超时设置为较大的值,可以是正常情况下应该被取走的时间的2倍
; 如果未与计算机相连,或者计算机中的应用程序未运行,则会出现超时,否则不超过1毫秒中断数据就应该被PC机取走
CH375_UP_INT_G:	CLR   EX0					;为了防止中途被中断而乱了顺序,必须先禁止中断
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA5
				MOVX  @DPTR,A				;向USB端点1的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				DEC   DPL					;数据口地址
				MOV   A,R7
				MOVX  @DPTR,A				;首先写入后续数据长度
CH375_UP_INT_1:	MOV   A,@R0
				INC   R0
				MOVX  @DPTR,A				;发送中断数据
				DJNZ  R7,CH375_UP_INT_1		;继续发送数据直至结束
				SETB  FLAG_INT_WAIT			;置中断等待标志
				SETB  EX0					;允许中断
				RET
;
; 中断服务子程序
; USE:   堆栈8字节
CH375_INTER:	PUSH  PSW					;现场保护
				PUSH  ACC
				PUSH  DPL
				PUSH  DPH
				PUSH  01H					;R1
				PUSH  02H					;R2
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_GET_STATUS
				MOVX  @DPTR,A				;获取中断状态并取消中断请求
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOVX  A,@DPTR				;返回操作状态
				CLR   IE0					;清中断标志,对应于INT0中断
				CJNE  A,#USB_INT_EP2_OUT,CH375_INT_1	;批量端点下传成功,接收到数据
				LJMP  CH375_DOWN_OK			;USB批量数据接收成功,USB端点2的OUT
CH375_INT_1:	CJNE  A,#USB_INT_EP2_IN,CH375_INT_2	;批量端点上传成功,发送数据成功
				LJMP  CH375_UP_OK			;USB批量数据发送成功,USB端点2的IN
CH375_INT_2:	CJNE  A,#USB_INT_EP1_IN,CH375_INT_3	;中断端点上传成功,发送中断数据成功
				LJMP  CH375_UP_INT_OK		;USB中断数据发送成功,USB端点1的IN
CH375_INT_3:	CJNE  A,#USB_INT_EP1_OUT,CH375_INT_4	;辅助端点下传成功,接收到辅助数据
				LJMP  CH375_DN_AUX_OK		;USB辅助数据接收成功,USB端点1的OUT
CH375_INT_4:	;如果需要在USB总线挂起时使CH375进入低功耗模式,那么还应该处理USB_INT_USB_SUSPEND和USB_INT_WAKE_UP中断状态
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;释放当前USB缓冲区
				SJMP  CH375_INT_RET
CH375_INT_RET:	POP   02H					;R2
				POP   01H					;R1
				POP   DPH
				POP   DPL
				POP   ACC
				POP   PSW					;恢复寄存器
				RETI						;中断返回
; USB批量数据接收成功
CH375_DOWN_OK:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_RD_USB_DATA
				MOVX  @DPTR,A				;从当前USB中断的端点缓冲区读取数据块,并释放缓冲区
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOVX  A,@DPTR				;首先读取后续数据长度
				MOV   R2,A
				JZ    CH375_INT_RET			;长度为0,没有数据则直接退出
				MOV   RECV_LEN,A
				MOV   R1,#RECV_BUFFER		;接收缓冲区
CH375_INT_RECV:	MOVX  A,@DPTR				;接收数据
				MOV   @R1,A
				INC   R1
				DJNZ  R2,CH375_INT_RECV		;继续接收数据直至结束
;				CALL  PROCESS				;分析下传数据并准备应答
				LJMP  CH375_INT_RET
; USB批量数据发送成功
CH375_UP_OK:	NOP
;				MOV   A,SEND_LEN
;				JZ    CH375_INT_UNLK		;剩余长度为0,没有数据需要发送,解锁后退出
;				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
;				MOV   A,#CMD_WR_USB_DATA7
;				MOVX  @DPTR,A				;向USB端点2的发送缓冲区写入数据块
;				NOP							;如果时钟频率低于16MHz则无需该指令延时
;				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
;				MOV   A,SEND_LEN
;				MOVX  @DPTR,A				;首先写入后续数据长度
;				MOV   R1,#SEND_BUFFER		;发送缓冲区
;CH375_INT_SEND:	MOV   A,@R1
;				MOVX  @DPTR,A				;发送数据
;				INC   R1
;				DJNZ  SEND_LEN,CH375_INT_SEND	;继续发送数据直至结束
CH375_INT_UNLK:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;释放当前USB缓冲区
				CLR   FLAG_SEND_WAIT		;发送完成
				LJMP  CH375_INT_RET
; USB中断数据发送成功, 中断端点可以用于单片机及时通知计算机端,本程序未用到
CH375_UP_INT_OK:
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;释放当前USB缓冲区
				CLR   FLAG_INT_WAIT			;中断数据已经被发送完成
				LJMP  CH375_INT_RET
; USB辅助数据接收成功,辅助端点可以用于计算机端向单片机端发送包,本程序未用到,辅助数据的接收可以参考批量端点
CH375_DN_AUX_OK:
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;释放当前USB缓冲区
				LJMP  CH375_INT_RET
;
;****************************************************************************
;
