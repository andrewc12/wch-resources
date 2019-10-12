;CH372/CH375 USB device mode & external firmware
;by wch       2004.10,  2005.03
;$INCLUDE                 	 (reg52.h)
$INCLUDE                 	 (CH375INC.ASM)

STACK           EQU    50H             ;堆栈区栈顶(向上延伸)
;
; 需要主程序定义的参数
CH375_CMD_PORT		EQU    0BDF1H         ;CH375命令口的地址,地址译码后自动片选
CH375_DAT_PORT		EQU    0BCF0H         ;CH375命令口的地址,地址译码后自动片选
;

CH375_CON_ACT		BIT    P1.4

CMD_BUFFER			DATA  38H				;命令包缓冲区,本程序约定命令包长度为CONST_CMD_LEN

VAR_SETUP_REQUEST	EQU   72H							;USB请求码
VAR_SETUP_LENGTH	EQU   73H							;后续数据长度
VAR_SETUP_DESCR		EQU   74H							;描述符偏移地址
VAR_SN_STRING		EQU   75H							;产品序列号字符串缓冲区,ANSI字符串
VAR_USB_ADDRESS		EQU   76H
	
; 定义位标志
CH375_CON_FLAG  BIT    2FH.0	      ;375芯片配置完成标志
;
;****************************************************************************
;主程序
				ORG   0000H            ;复位后单片机入口
				LJMP  START
				ORG   0003H            ;CH375中断
				LJMP  CH375_INTER

START:
; 以下初始化指令不是必要的,将单片机恢复为默认状态
				CLR   EA               ;关中断
				MOV   SP,#STACK        ;置堆栈初值
				MOV   A,#0FFH
				MOV   P0,A
				MOV   P1,A             ;清端口状态
				MOV   P2,A
				MOV   P3,A
				CLR   A
				MOV   IE,A             ;清中断允许控制寄存器
				MOV   IP,A             ;清中断优先级控制寄存器
				MOV   PSW,A            ;清程序状态字
				MOV   TCON,A           ;清定时器/计数器控制寄存器
				MOV   TMOD,A           ;清定时器/计数器工作方式寄存器
				MOV   PCON,A           ;清电源控制寄存器
				MOV   R7,#0FFH
				CALL  DELAY_MS			;延时等待CH375初始化完成
; 初始化
				CALL  CH375_INIT       ;初始化
				setb  EA
				ljmp  $	;主程序
;
; 输出命令码给CH375
; 输入: ACC 为命令码
; 输出: DPTR 为数据口
; USE: ACC, DPTR
WR_CMD_TO_375:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOVX  @DPTR,A				;设置USB工作模式
				NOP							;如果时钟频率低于20MHz则无需该指令延时,高于30MHz要多加2条指令
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				RET
;
; 初始化子程序
; USE:   ACC, R7, DPTR
CH375_INIT:		MOV   A,#CMD_SET_USB_MODE	;设置USB工作模式
				CALL  WR_CMD_TO_375			;输出命令码
;				MOV   A,#02H				;设置为内置固件模式
				MOV   A,#01H				;设置为外置固件模式
				MOVX  @DPTR,A				;设置为使用内置固件的USB设备方式
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				NOP
CH375_INIT_WT:	MOVX  A,@DPTR				;返回操作状态,也可以等待20uS
				XRL   A,#CMD_RET_SUCCESS
				JNZ   CH375_INIT_WT			;等待操作成功,通常需要等待10uS-20uS
; 下述三条指令用于启用中断
				CLR   IT0					;置外部信号为低电平触发
				SETB  PX0					;置高优先级
				CLR   IE0					;清中断标志
				SETB  EX0					;允许CH375中断
				RET
;**************************************************************************************
CH375_EP2_DOWN:	MOV A,#CMD_RD_USB_DATA		;将下传的数据读出，释放端点
				CALL  WR_CMD_TO_375			;读命令发向CH375
				MOVX A,@DPTR				;取出后续数据长度
				MOV R4,A				;保存长度
				JZ   CH375_EP2_UP			;长度为0返回
				MOV R5,A
				MOV  R0,#CMD_BUFFER			;缓冲区首地址送R0
CH375_EP2_LOOP:			MOVX A,@DPTR
				MOV @R0,A
				INC R0	
				DJNZ R5,CH375_EP2_LOOP
; 演示回传
CH375_EP2_UP:	MOV  A,#CMD_WR_USB_DATA7	;演示回传数据
				CALL  WR_CMD_TO_375
				MOV  A,R4
				MOVX @DPTR,A
				JZ   CH375_EP2_UP_RET		;长度为0
				MOV  R0,#CMD_BUFFER			;缓冲区首地址送R0
CH375_EP2_UP_LOOP:	MOV  A,@R0
				MOVX @DPTR,A
				INC  R0
				DJNZ  R4,CH375_EP2_UP_LOOP
CH375_EP2_UP_RET:	RET
;***************************************************************************************							
				
; 中断服务子程序
; USE:   堆栈6字节, 工作寄存器组1的R0,R5,R6,R7
CH375_INTER:	PUSH  PSW					;现场保护
				PUSH  ACC
				PUSH  DPL
				PUSH  	DPH
				SETB  	RS0                   		;PSW.3,切换至工作寄存器组1
				MOV   A,#CMD_GET_STATUS
				CALL  WR_CMD_TO_375			;获取中断状态并取消中断请求
				MOVX 	 A,@DPTR				;返回操作状态
				ANL 	 A,#0FH				;屏蔽高4位
				CLR   	IE0					;清中断标志,对应于INT0中断
;根据中断状态进行分析处理
				CJNE  A,#USB_INT_EP2_OUT,CH375_INT_06	;批量端点下传成功
				LCALL CH375_EP2_DOWN		;接收下传的数据
				LJMP  CH375_INT_RET			; USB批量数据接收成功
CH375_INT_06:	CJNE  A,#USB_INT_EP2_IN,CH375_INT_07	;批量端点上传成功
				;USB批量数据发送成功
				LJMP CH375_INT_ULOCK			;解锁后中断返回
CH375_INT_07:
				CJNE  	A,#USB_INT_EP0_SETUP,CH375_INT_01
				LJMP  	CH375_SETUP_OK			;USB控制传输建立事务
CH375_INT_01:	CJNE  	A,#USB_INT_EP0_OUT,CH375_INT_02
				LJMP 	CH375_INT_ULOCK		;端点0下传,解锁后中断返回
CH375_INT_02:	CJNE  	A,#USB_INT_EP0_IN,CH375_INT_03
									;端点0数据上传
				MOV 	A,VAR_SETUP_REQUEST
				CJNE 	A,#DEF_USB_GET_DESCR,CH375_EP0_IN_01
									;描述符上传
				LCALL 	 CH375_DES_UPLOAD		;调用描述符上传
				LJMP     CH375_INT_ULOCK	;解锁后中断返回

CH375_EP0_IN_01:	CJNE 	A,#DEF_USB_SET_ADDRESS,CH375_EP0_IN_02
				MOV	    A,#CMD_SET_USB_ADDR		;设置地址,以便下一次事务用新的地址
				CALL  WR_CMD_TO_375
				MOV    A,VAR_USB_ADDRESS		;取出地址
				MOVX	@DPTR,A
				LJMP    CH375_INT_ULOCK
CH375_EP0_IN_02:	LJMP    CH375_INT_ULOCK	;解锁后中断返回
CH375_INT_03:	CJNE 	 A,#USB_INT_EP1_IN,CH375_INT_04
									;端点1数据上传,请在此加上处理代码
				LJMP 	CH375_INT_ULOCK		;解锁后中断返回

CH375_INT_04:	CJNE  A,#USB_INT_EP1_OUT,CH375_INT_05
				 					;端点1数据下传
				MOV A,#CMD_RD_USB_DATA		;将下传的数据读出，释放端点
				CALL  WR_CMD_TO_375			;读命令发向CH375
				MOVX A,@DPTR				;
				JZ   CH375_INT_RET			;长度为0返回
				MOV  R5,A
CH375_USB_OUT1_LOOP:		MOVX A,@DPTR				;读出数据释放缓冲区
				NOP							;保存到缓冲区中
				DJNZ  R5,CH375_USB_OUT1_LOOP		;数据未读完继续
				LJMP 	CH375_INT_RET		;中断返回
CH375_INT_05:	ANL  A,#03H
				CJNE A,#03H,CH375_INT_09
				LJMP  CH375_USB_REST			;跳转到USB复位
CH375_INT_09:	SJMP  CH375_INT_ULOCK		;命令不支持错误返回

CH375_INT_ULOCK:	;解锁后中断返回
				MOV 	A,#CMD_UNLOCK_USB			;解锁端点
				CALL  WR_CMD_TO_375
CH375_INT_RET:	POP   DPH
				POP   DPL
				POP   ACC
				POP   PSW					;恢复寄存器
				RETI

CH375_USB_REST:	;USB总线复位
				SETB CH375_CON_ACT				;清配置值等，CH372 会自动清USB 地址和同步触发标志
				CLR CH375_CON_FLAG				;清单片机中各种标志	
				MOV VAR_SETUP_REQUEST,#0FFH
				LJMP CH375_INT_ULOCK
;*****************************************************************************************
;***********************************USB SETUP***********************************************
CH375_SETUP_RET_ERR:
				MOV A,#CMD_SET_ENDP3			;设置端点1
				CALL  WR_CMD_TO_375
				MOV A,#0FH			;设置状态为STALL
				MOVX @DPTR,A
				LJMP CH375_INT_RET			;错误的控制传输

CH375_SETUP_RET:
				LJMP CH375_INT_RET			;跳转到中断返回
CH375_SETUP_OK:	MOV  A,#CMD_RD_USB_DATA			;
				CALL  WR_CMD_TO_375			;发出读数据命令
				MOVX A,@DPTR				;读出长度
				MOV  R6,A
				MOV  R7,A				;长度放入R7
				MOV  R0,#CMD_BUFFER			;命令缓冲区首地址放入间接寻址
CH375_SETUP_REC_CMD:		MOVX A,@DPTR				;读出数据
				MOV @R0,A				;数据放入命令缓冲区
				INC R0
				DJNZ  R7,CH375_SETUP_REC_CMD  		;长度不为0继续
				CJNE R6,#8H,CH375_INT_RET		;长度不为8跳出不与理睬
				MOV  R0,#CMD_BUFFER
				MOV  A,CMD_BUFFER+6			;取出控制传输的长度
				ANL  A,#7FH				;长度不超过128个
				MOV VAR_SETUP_LENGTH,A			;存储长度
				MOV A,CMD_BUFFER			;判断USB请求
				JB  ACC.6,CH375_USB_REQ_FACT		;USB厂商请求
				JB  ACC.5,CH375_USB_REQ_CLASS		;跳转到USB类请求
				ANL  A,#0E0H
				CJNE A,#80H,CH375_REQUEST_OTHER1
				LJMP  CH375_REQUEST_STANDTRAD		;USB标准请求
CH375_REQUEST_OTHER1:		CJNE A,#00H,CH375_REQUEST_OTHER
				LJMP  CH375_REQUEST_STANDTRAD		;USB标准请求
CH375_REQUEST_OTHER:		LJMP  CH375_SETUP_RET_ERR		;其余命令不支持
CH375_USB_REQ_FACT:		LJMP 	CH375_SETUP_RET			;厂商请求
CH375_USB_REQ_CLASS:	LJMP 	CH375_SETUP_RET			;
;***************************************************************************************
;******************************USB STANDTRAD  REQUEST   STAR  **************************

;*************************************************************************************
;SELECT REQUEST
CH375_REQUEST_STANDTRAD:	MOV A,CMD_BUFFER+1			;USB标准请求
				MOV VAR_SETUP_REQUEST,A			;保存请求
				CJNE A,#DEF_USB_GET_STATUS,CH375_REQ_SEL_01				
				LJMP  CH375_USB_GET_STATUS
CH375_REQ_SEL_01:
				CJNE A,#DEF_USB_CLR_FEATURE,CH375_REQ_SEL_02				
				LJMP CH375_USB_CLR_FEATURE
CH375_REQ_SEL_02:
				CJNE A,#DEF_USB_SET_ADDRESS,CH375_REQ_SEL_03				
				LJMP CH375_USB_SET_ADDRES
CH375_REQ_SEL_03:
				CJNE A,#DEF_USB_GET_DESCR,CH375_REQ_SEL_04					;
				LJMP CH375_USB_GET_DESCR
CH375_REQ_SEL_04:
				CJNE A,#DEF_USB_GET_CONFIG,CH375_REQ_SEL_05				
				LJMP CH375_USB_GET_CONFIG
CH375_REQ_SEL_05:
				CJNE A,#DEF_USB_SET_CONFIG,CH375_REQ_SEL_06				
				LJMP  CH375_USB_SET_CONFIG
CH375_REQ_SEL_06:
				CJNE A,#DEF_USB_GET_INTERF,CH375_REQ_SEL_07			
				LJMP CH375_USB_GET_INTERF

CH375_REQ_SEL_07:		LJMP CH375_SETUP_RET_ERR


;*************************************************************************************************
;USB GET STATUS   得到状态
CH375_USB_GET_STATUS:		MOV 	@R0,#00H
				INC 	R0
				MOV    @R0,#00H
				LCALL    CH375_EP0_UPLOAD		;调用端点0上传
				LJMP   	 CH375_SETUP_RET		;中断返回
CH375_SETUP_RET_ERR_1:		LJMP    CH375_SETUP_RET_ERR
;************************************************************************************************
;USB  CLR  FEATURE  清除特性
CH375_USB_CLR_FEATURE:         	MOV   	A,CMD_BUFFER		;取请求码
				ANL  	A,#1FH
				CJNE  	A,#02H,CH375_SETUP_RET_ERR_1	;不是端点则不支持
				MOV  	A,CMD_BUFFER+4

				CJNE    A,#82H,CH375_USB_CLR_FEA_0
											;端点2上传
				MOV     A,#CMD_SET_ENDP7				;清除端点2上传
				CALL  WR_CMD_TO_375
				MOV     A,#8EH
				MOVX 	@DPTR,A
				LCALL CH375_EP0_IN_ZERO		;上传0长读的信息包
				LJMP   CH375_SETUP_RET
						;
CH375_USB_CLR_FEA_0:		CJNE    A,#02H,CH375_USB_CLR_FEA_1
				MOV     A,#CMD_SET_ENDP6	;清除端点2下传
				CALL  WR_CMD_TO_375
				MOV     A,#80H
				MOVX @DPTR,A
				LCALL CH375_EP0_IN_ZERO		;上传0长读的信息包							;端点2下传
				LJMP   CH375_SETUP_RET

CH375_USB_CLR_FEA_1:				CJNE    A,#81H,CH375_USB_CLR_FEA_2
				MOV     A,#CMD_SET_ENDP5	;清除端点1上传
				CALL  WR_CMD_TO_375
				MOV     A,#8EH							;端点1上传
				MOVX @DPTR,A
				LCALL CH375_EP0_IN_ZERO		;上传0长读的信息包
				LJMP   CH375_SETUP_RET

CH375_USB_CLR_FEA_2:				CJNE    A,#01H,CH375_USB_CLR_FEA_3
				MOV     A,#CMD_SET_ENDP4	;清除端点1下传
				CALL  WR_CMD_TO_375
				MOV     A,#80H							;端点1下传
				MOVX @DPTR,A
				LCALL CH375_EP0_IN_ZERO		;上传0长读的信息包
				LJMP	 CH375_SETUP_RET
CH375_USB_CLR_FEA_3:
				LJMP   CH375_SETUP_RET_ERR	;;不支持
;****************************************************************************************
;USB  AET ADDRESS
CH375_USB_SET_ADDRES:		MOV A,CMD_BUFFER+2			;取出地址
				MOV VAR_USB_ADDRESS,A			;暂存地址
				LCALL CH375_EP0_IN_ZERO		;上传0长读的信息包
				LJMP CH375_SETUP_RET			;先退出，到发出状态后才能反改变

;*****************************************************************************************
;USB  GET  DESCR 获得描述符
CH375_USB_GET_DESCR: 		MOV  A,CMD_BUFFER+3
				CJNE A,#1H,CH375_GET_DESCR_1	;获取设备描述符
				MOV  A,#00H
				MOV  VAR_SETUP_DESCR,A		;描述符偏移量=0
				LCALL CH375_DES_UPLOAD
				LJMP CH375_SETUP_RET
CH375_GET_DESCR_1:
				CJNE A,#2H,CH375_GET_DESCR_2
								;获取配置描述符
				MOV  A,#USB_CFG_DESCR_TAB-USB_DEV_DESCR_TAB
				MOV  VAR_SETUP_DESCR,A		;描述符偏移量
				LCALL CH375_DES_UPLOAD
				LJMP CH375_SETUP_RET
CH375_GET_DESCR_2:
CH375_GET_DESCR_3: 		LJMP   CH375_SETUP_RET_ERR	;不支持退出

;*******************************************************************************************
;USB  GET CONFIG
CH375_USB_GET_CONFIG:		MOV  @R1,#01H			;配置值为01H，在描述符里指定的
				JB   CH375_CON_FLAG,CH375_CON_CON
				MOV  @R1,#00H			;CH375没有配置返回00
CH375_CON_CON:			LCALL CH375_EP0_UPLOAD		;调用端点0上传
				LJMP  CH375_SETUP_RET

;********************************************************************************************
;USB SET  CONFIG
CH375_USB_SET_CONFIG:	MOV  A,CMD_BUFFER+2
				CLR  CH375_CON_FLAG		;取消配置
				SETB  CH375_CON_ACT
				JZ    CH375_USB_SET_CFG1
				SETB  CH375_CON_FLAG		;设置配置
				CLR   CH375_CON_ACT		;输出低电平指示完成标志
CH375_USB_SET_CFG1:
				LCALL CH375_EP0_IN_ZERO		;上传0长读的信息包
				LJMP  CH375_SETUP_RET
;*********************************************************************************************
;USB  GET INTER
CH375_USB_GET_INTERF:	  MOV   @R1,#01H			;返回一个接口
				LCALL CH375_EP0_UPLOAD		;因为在这里我们指定一个接口
				LJMP  CH375_SETUP_RET
;*********************************************************************************************
CH375_EP0_UPLOAD:		MOV  A,VAR_SETUP_LENGTH		;取出长度
				MOV R5,#08H
				CLR C
				SUBB A,R5
				MOV R5,VAR_SETUP_LENGTH		;上传长度小于8
				JC  CH375_EP0_UP_LEN
				MOV  VAR_SETUP_LENGTH,A		;重新给出长度
				MOV R5,#08H
CH375_EP0_UP_LEN:
				MOV R0,#CMD_BUFFER		;上传缓冲区送R0
				MOV A,#CMD_WR_USB_DATA3		;命令口送入，写端点0
				CALL  WR_CMD_TO_375
				MOV  A,R5			;首先上传长度
				MOVX @DPTR,A			;数据长度送375
CH375_EP0_UP_LOOP2:		MOV A,@R0			;数据取出，			;
				MOVX @DPTR,A			;数据送入CH375
				INC R0				;缓冲区地址加一
				DJNZ  R5,CH375_EP0_UP_LOOP2 	;缓冲区不满则继续
CH375_EP0_UP_LOOPRET:
				RET
;********************************************************************************************
CH375_DES_UPLOAD:		MOV R0,#CMD_BUFFER		;缓冲区首地址送间接寻址
				MOV R5,#08H
				MOV DPTR,#USB_DEV_DESCR_TAB	;
CH375_DES_UPLOOP:
				MOV  A,VAR_SETUP_DESCR
				LCALL CH375_TAB_TAB
				MOV @R0,A
				INC VAR_SETUP_DESCR
				INC R0
				DJNZ R5,CH375_DES_UPLOOP
				LCALL CH375_EP0_UPLOAD
				RET
;RERUEST OVER

CH375_EP0_IN_ZERO:
				MOV A,#CMD_WR_USB_DATA3  ;上传0长度的联络信息包
				CALL  WR_CMD_TO_375
				MOV A,#00H
				MOVX @DPTR,A
				RET
;********************************************************************************************


; 以毫秒为单位延时
; ENTRY: R7 延时毫秒数
; USE:   ACC, R7
DELAY_MS:		MOV   A,#0FAH				;250*(1*6+2)*0.5=1000uS
DELAY_1MS:		NOP
				NOP
				NOP
				NOP
				NOP
				NOP
				DJNZ  ACC,DELAY_1MS			;每周期延时1毫秒
				DJNZ  R7,DELAY_MS
				RET
;
;*********************************************************************************************************************
;
CH375_TAB_TAB:			MOVC A,@A+DPTR
				RET

USB_DEV_DESCR_TAB:	DB   12H							;USB设备描述符
					DB   01H
					DB   10H
					DB   01H
					DB   0FFH							;04H, DeviceClass
					DB   80H							;05H, DeviceSubClass
					DB   37H							;06H, DeviceProtocol
					DB   08H							;端点0最大包长度8字节
					DB   48H				;08H, VendorId-L
					DB   43H				;09H, VendorId-H
					DB   37H				;0AH, ProductId-L
					DB   55H				;0BH, ProductId-H
					DB   00H							;0CH, ReleaseId-L
					DB   01H							;0DH, ReleaseId-H
					DB   00H
					DB   00H
					DB   00H							;10H, IndexSerialNumber
					DB   01H
					
USB_CFG_DESCR_TAB:	DB   09H							;USB配置描述符
					DB   02H
					DB   27H							;02H, TotalLength-L
					DB   00H
					DB   01H
					DB   01H							;配置值
					DB   00H
					DB   80H							;07H, Attributes
					DB   40H
					DB   09H							;USB接口描述符
					DB   04H
					DB   00H
					DB   00H
					DB   03H						;0DH, NumberEndpoints
					DB   0FFH							;0EH, InterfaceClass
					DB   80H							;0FH, InterfaceSubClass
					DB   37H							;10H, InterfaceProtocol
					DB   00H
					DB   07H
					DB   05H
					DB   82H							;端点82H发送/上传
					DB   02H							;批量端点
					DB   40H							;最大包长度32字节
					DB   00H
					DB   00H
					DB   07H
					DB   05H
					DB   02H							;端点02H接收/下传
					DB   02H							;批量端点
					DB   40H							;最大包长度32字节
					DB   00H
					DB   00H
					DB   07H
					DB   05H
					DB   81H							;端点81H发送/上传
					DB   03H							;中断端点
					DB   08H							;最大包长度8字节
					DB   00H
					DB   01H							;轮询时间1毫秒
					DB   07H
					DB   05H
					DB   01H							;端点01H接收/下传
					DB   02H							;批量端点
					DB   08H							;最大包长度8字节
					DB   00H
					DB   00H
END
;
