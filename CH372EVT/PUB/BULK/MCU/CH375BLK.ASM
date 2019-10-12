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
;
;
; 需要主程序定义的参数
CH375_CMD_PORT		EQU    0BDF1H         ;CH375命令口的地址,地址译码后自动片选
CH375_DAT_PORT		EQU    0BCF0H         ;CH375命令口的地址,地址译码后自动片选
;
THIS_CMD_CODE		DATA   3EH            ;保存当前命令码
RECV_LEN			DATA   3FH            ;刚接收到的数据的长度
RECV_BUFFER			DATA   40H            ;数据缓冲区,用于保存接收到的下传数据,长度为0到64字节
; 由于MCS-51单片机存取外部RAM的读写速度低于内部RAM, 并且需要用到DPTR, 所以读写速度较慢, 其程序可以参考本程序修改
;
; 定义位标志
FLAG_RECV_OK		BIT    2FH.0          ;接收成功标志,1指示成功接收到数据块
FLAG_SEND_WAIT		BIT    2FH.1          ;发送等待标志,1指示有数据块正在CH375中等待发送
;
; CH375的命令代码定义
$INCLUDE			(..\..\MCU_IF1\MCS51ASM\CH375INC.ASM)
;
;****************************************************************************
;
; 应用层定义
;TEST_OTHER		EQU    00H				;其它自定义的命令码
TEST_START		EQU    20H				;测试过程开始
TEST_DATA		EQU    21H				;测试数据正确性
TEST_UPLOAD		EQU    22H				;测试上传数据块
TEST_DOWNLOAD	EQU    23H				;测试下传数据块
;
STACK           EQU    0FH             ;堆栈区栈顶(向上延伸)
;
; 有关CH451的定义
CH451_DCLK		BIT   P1.7             ;串行数据时钟,上升沿激活
CH451_DIN		BIT   P1.6             ;串行数据输出,接CH451的数据输入
CH451_LOAD		BIT   P1.5             ;串行命令加载,上升沿激活
CH451_DOUT		BIT   P3.3             ;INT1,键盘中断和键值数据输入,接CH451的数据输出
;
;****************************************************************************
;主程序,用于配合演示CH375
				ORG   0000H            ;复位后单片机入口
				LJMP  START
				ORG   0003H            ;CH375中断
				LJMP  CH375_INTER
;
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
				MOV   THIS_CMD_CODE,#00H ;清除命令码
				MOV   R7,#0FFH
				CALL  DELAY_MS			;延时等待CH375初始化完成
; 初始化
				CALL  CH375_INIT       ;初始化
				CALL  CH451_INIT       ;初始化
;				.....
;
; 启动CH451的显示驱动
				MOV   B,#04H           ;设置系统参数命令
				MOV   A,#01H           ;使能显示驱动
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#05H           ;设置显示参数
				MOV   A,#88H           ;BCD译码方式,8级亮度
				CALL  CH451_WRITE      ;发出命令
				MOV   R6,#08H
				MOV   R5,#08H
				MOV   R4,#01H
SHOW_INIT:		MOV   B,R5
				MOV   A,R4
				INC   R5
				INC   R4
				CALL  CH451_WRITE      ;显示字符
				DJNZ  R6,SHOW_INIT
;
				SETB  EA               ;允许中断
				SETB  TR0				;由定时器0自由计数值产生随机数
;
; 以下指令开始工作循环,等待PC机命令进行操作
WAIT_REQUEST:	NOP
				JNB   FLAG_RECV_OK,WAIT_REQUEST	;等待下传数据
				CLR   FLAG_RECV_OK
				MOV   A,RECV_LEN			;分析下传数据并准备应答
				JZ    WAIT_REQUEST			;长度为0,在本程序中没有意义
;				MOV   R0,#RECV_BUFFER		;刚接收到的数据块
;				MOV   R7,RECV_LEN			;刚接收到的数据长度
;				MOV   A,@R0					;我们在计算机和单片机的应用程序之间约定下传数据的首字节是命令码
;				CJNE  A,#TEST_OTHER,WAIT_REQUEST	;自定义的命令码,其它测试命令已经在中断服务程序中处理掉了
;				NOP							;其它命令,分析后准备应答
;				MOV   R0,#SEND_BUFFER		;指向准备上传的数据的缓冲区
;				MOV   R7,SEND_LEN			;准备上传的数据的长度
;				CALL  CH375_UPLOAD			;上传数据块子程序(通过批量端点上传)
				MOV   B,#03H           ;移位命令
				MOV   A,#00H           ;左移一位
				CALL  CH451_WRITE      ;发出命令使CH451显示左移一位
				SJMP  WAIT_REQUEST			;等待下一个请求
;
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
; 初始化子程序
CH451_INIT:		CLR   CH451_DIN        ;先低后高,输出上升沿通知CH451选择4线串行接口
				SETB  CH451_DCLK       ;置为默认的高电平
				SETB  CH451_DIN
				SETB  CH451_LOAD
				SETB  CH451_DOUT       ;置为输入
				CLR   EX1              ;禁止中断
				RET
;
;
; 输出命令子程序
; ENTRY: B-ACC 待写的12位数据, 低8位在ACC中, 高4位在B的低4位中
; USE:   ACC, B, R7
CH451_WRITE:	CLR   CH451_LOAD       ;命令开始,此命令可以放在后面
				MOV   R7,#08H          ;将ACC中8位送出
CH451_WRITE_8:	RRC   A                ;低位在前,高位在后
				CLR   CH451_DCLK
				MOV   CH451_DIN,C      ;送出一位数据
				SETB  CH451_DCLK       ;产生时钟上升沿通知CH451输入位数据
				DJNZ  R7,CH451_WRITE_8 ;位数据未完继续
				MOV   A,B
				MOV   R7,#04H          ;将B中4位送出
CH451_WRITE_4:	RRC   A                ;低位在前,高位在后
				CLR   CH451_DCLK
				MOV   CH451_DIN,C      ;送出一位数据
				SETB  CH451_DCLK       ;产生时钟上升沿通知CH451输入位数据
				DJNZ  R7,CH451_WRITE_4 ;位数据未完继续
				SETB  CH451_LOAD       ;产生加载上升沿通知CH451处理命令数据
				RET
;
;
;****************************************************************************
;
; 初始化子程序
; USE:   ACC, R7, DPTR
CH375_INIT:		CLR   FLAG_RECV_OK			;清接收成功标志,1指示成功接收到数据块
				CLR   FLAG_SEND_WAIT		;清发送等待标志,1指示有数据块正在CH375中等待发送
; 测试CH375是否正常工作,可选操作
;				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
;				MOV   A,#CMD_CHECK_EXIST
;				MOVX  @DPTR,A				;测试CH375是否正常工作,可选操作
;				NOP							;如果时钟频率低于16MHz则无需该指令延时,高于30MHz要多加2条指令
;				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
;				MOV   A,#55H
;				MOVX  @DPTR,A				;写入测试数据
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
;				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
;				MOV   A,#USB_VENDOR_ID_L
;				MOVX  @DPTR,A				;写入厂商ID的低字节
;				MOV   A,#USB_VENDOR_ID_H
;				MOVX  @DPTR,A				;写入厂商ID的高字节
;				MOV   A,#USB_DEVICE_ID_L
;				MOVX  @DPTR,A				;写入设备ID的低字节
;				MOV   A,#USB_DEVICE_ID_H
;				MOVX  @DPTR,A				;写入设备ID的高字节
;				NOP
; 设置USB工作模式
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_SET_USB_MODE
				MOVX  @DPTR,A				;设置USB工作模式
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,#02H
				MOVX  @DPTR,A				;设置为使用内置固件的USB设备方式
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
CH375_UPLOAD:	JB    FLAG_SEND_WAIT,CH375_UPLOAD	;如果正在等待发送(前次发送尚未完成)则继续等待
				CLR   EX0					;为了防止中途被中断而乱了顺序,必须先禁止中断
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA7
				MOVX  @DPTR,A				;向USB端点2的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
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
CH375_UP_INT:	CLR   EX0					;为了防止中途被中断而乱了顺序,必须先禁止中断
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA5
				MOVX  @DPTR,A				;向USB端点1的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,R7
				MOVX  @DPTR,A				;首先写入后续数据长度
CH375_UP_INT_1:	MOV   A,@R0
				INC   R0
				MOVX  @DPTR,A				;发送中断数据
				DJNZ  R7,CH375_UP_INT_1		;继续发送数据直至结束
				SETB  EX0					;允许中断
				RET
;
; 中断服务子程序
; USE:   堆栈8字节
CH375_INTER:	PUSH  PSW					;现场保护
				PUSH  ACC
				PUSH  DPL
				PUSH  DPH
;				PUSH  01H					;R1
;				PUSH  02H					;R2
;主程序中未使用R1/R2,所以不必入栈保护,从而提高速度
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_GET_STATUS
				MOVX  @DPTR,A				;获取中断状态并取消中断请求
				NOP							;如果时钟频率低于16MHz则无需该指令延时,高于30MHz要多加2条指令
				NOP
; 单片机向写CH375写入命令码后,应该要等2uS才能读取数据,所以时钟频率高于16MHz时需要指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOVX  A,@DPTR				;返回操作状态
				CLR   IE0					;清中断标志,对应于INT0中断
				CJNE  A,#USB_INT_EP2_OUT,CH375_INT_1	;批量端点下传成功
				LJMP  CH375_DOWN_OK			; USB批量数据接收成功
CH375_INT_1:	CJNE  A,#USB_INT_EP2_IN,CH375_INT_2	;批量端点上传成功
				LJMP  CH375_UP_OK			; USB批量数据发送成功
CH375_INT_2:	CJNE  A,#USB_INT_EP1_IN,CH375_INT_3	;中断端点
				LJMP  CH375_UP_INT_OK		; USB中断数据发送成功
CH375_INT_3:	SJMP  CH375_INT_RET
CH375_INT_RET:
;				POP   02H					;R2
;				POP   01H					;R1
				POP   DPH
				POP   DPL
				POP   ACC
				POP   PSW					;恢复寄存器
				RETI						;中断返回
;
CH375_DOWN_OK:	; USB批量数据接收成功
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_RD_USB_DATA
				MOVX  @DPTR,A				;从当前USB中断的端点缓冲区读取数据块,并释放缓冲区
				NOP							;如果时钟频率低于16MHz则无需该指令延时,高于30MHz要多加一条指令
				NOP
; 单片机向写CH375写入命令码后,应该要等2uS才能读取数据,所以时钟频率高于16MHz时需要指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOVX  A,@DPTR				;首先读取后续数据长度
				MOV   R2,A
				JZ    CH375_INT_RET			;长度为0,没有数据则直接退出
				MOVX  A,@DPTR				;接收数据
				MOV   THIS_CMD_CODE,A		;保存当前命令码
				CJNE  A,#TEST_DOWNLOAD,IS_USB_CMD0
				SJMP  USB_CMD2_NEXT			;测试下传速度
USB_CMD2_RECV:	MOVX  A,@DPTR				;接收数据,为了测试速度,数据舍弃,24MHz的MCS51每读取一个字节需要2uS
USB_CMD2_NEXT:	DJNZ  R2,USB_CMD2_RECV		;继续接收数据直至结束
				SJMP  CH375_INT_RET
IS_USB_CMD0:	MOV   R1,#RECV_BUFFER		;接收缓冲区
				MOV   RECV_LEN,R2
				SJMP  CH375_INT_RECV0
CH375_INT_RECV:	MOVX  A,@DPTR				;接收数据
CH375_INT_RECV0:	MOV   @R1,A
				INC   R1
				DJNZ  R2,CH375_INT_RECV		;继续接收数据直至结束
				MOV   A,RECV_BUFFER			;首字节是命令码
				CJNE  A,#TEST_UPLOAD,IS_USB_CMD3
USB_CMD3_NEXT:	;测试上传速度
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA7
				MOVX  @DPTR,A				;向USB端点2的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,#40H
				MOVX  @DPTR,A				;首先写入后续数据长度
				MOV   R7,A
				MOV   A,TL0					;随机数
USB_CMD3_SEND:	MOVX  @DPTR,A				;发送数据,为了测试速度,数据无效,24MHz的MCS51每写出一个字节需要2uS
				DJNZ  R7,USB_CMD3_SEND		;继续发送数据直至结束
				SJMP  CH375_INT_UNLK
IS_USB_CMD3:	CJNE  A,#TEST_START,IS_USB_CMD1
				NOP							;测试过程开始
; 由于上一次测试数据上传速度时可能在上传缓冲区中遗留有数据, 所以在第二次测试前需要清除上传缓冲区
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_SET_ENDP7
				MOVX  @DPTR,A				;设置USB端点2的IN
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,#0EH				;同步触发位不变
				MOVX  @DPTR,A				;设置USB端点2的IN正忙,返回NAK
				CLR   FLAG_SEND_WAIT		;清除发送等待标志,通知应用程序可以继续发送数据
				LJMP  CH375_INT_RET
IS_USB_CMD1:	CJNE  A,#TEST_DATA,IS_USB_CMD9
				NOP							;测试数据正确性
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA7
				MOVX  @DPTR,A				;向USB端点2的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,RECV_LEN
				MOVX  @DPTR,A				;首先写入后续数据长度
				MOV   R2,A					;刚接收到的数据长度
				MOV   R1,#RECV_BUFFER		;刚接收到的数据块
USB_CMD1_NEXT:	MOV   A,@R1
				CPL   A						;数据取反后返回,由计算机应用程序测试数据是否正确
				MOVX  @DPTR,A				;发送数据
				INC   R1
				DJNZ  R2,USB_CMD1_NEXT
				LJMP  CH375_INT_RET
IS_USB_CMD9:	SETB  FLAG_RECV_OK			;其它命令,设置接收成功标志,通知应用程序取走数据再分析
				LJMP  CH375_INT_RET
;
CH375_UP_OK:	; USB批量数据发送成功
				MOV   A,THIS_CMD_CODE
				CJNE  A,#TEST_UPLOAD,CH375_INT_UNLK
				SJMP  USB_CMD3_NEXT			;测试上传速度,继续准备上传数据
CH375_INT_UNLK:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;释放当前USB缓冲区
				CLR   FLAG_SEND_WAIT		;清除发送等待标志,通知应用程序可以继续发送数据
				LJMP  CH375_INT_RET
;
CH375_UP_INT_OK:	; USB中断数据发送成功
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;释放当前USB缓冲区
				LJMP  CH375_INT_RET
;
;
END
;
