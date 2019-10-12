; CH375/CH372/CH451 EVT
; U2(AT89C51) Program
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.12
;
;****************************************************************************
;
; 应用层定义
CONST_CMD_LEN	EQU   07H				; 命令块的长度
; 由于命令与数据都是通过数据下传管道(USB端点2的OUT)下传, 为了防止两者混淆,
; 我们可以在计算机应用程序与单片机程序之间约定, 命令块的长度总是7, 而数据块的长度肯定不是7, 例如64,32等
; 另外, 可以约定, 命令块的首字节是命令码, 等等
DEMO_CH451_CMD	EQU   56H              ; PC发送命令给CH451
DEMO_TEST_DATA	EQU   91H              ; 测试命令,将PC机发来的命令包的所有数据取反后返回
DEMO_UP_CLEAR	EQU   0A0H				; 连续上传数据块之前进行同步,实际是让单片机清除上传缓冲区的已有内容
DEMO_UP_DATA	EQU   0A1H				; 连续上传数据块,指定外部RAM缓冲区起始地址,不限长度
DEMO_DOWN_DATA	EQU   0A2H				; 连续下传数据块,指定外部RAM缓冲区起始地址,不限长度
;
STACK           EQU    0FH             ;堆栈区栈顶(向上延伸)
;
; CH375的命令代码定义
$INCLUDE			(..\..\MCU_IF1\MCS51ASM\CH375INC.ASM)
;
; 需要主程序定义的参数
CH375_CMD_PORT		EQU    0BDF1H         ;CH375命令口的地址,地址译码后自动片选
CH375_DAT_PORT		EQU    0BCF0H         ;CH375命令口的地址,地址译码后自动片选
;
; 有关CH451的定义
CH451_DCLK		BIT   P1.7             ;串行数据时钟,上升沿激活
CH451_DIN		BIT   P1.6             ;串行数据输出,接CH451的数据输入
CH451_LOAD		BIT   P1.5             ;串行命令加载,上升沿激活
CH451_DOUT		BIT   P3.3             ;INT1,键盘中断和键值数据输入,接CH451的数据输出
CH451_KEY		DATA  28H              ;存放键盘中断中读取的键值
LAST_KEY		DATA  29H              ;保存上次的键值
;
; 有关CH375的定义
CH451_CMD_H		DATA  30H				;PC机发给CH451的高4位命令,为0FFH则命令无效
CH451_CMD_L		DATA  31H				;PC机发给CH451的低8位命令
BUF_ADDR_L		DATA  32H				;保存用于传输的外部RAM或者内部RAM缓冲区的地址,低8位
BUF_ADDR_H		DATA  33H				;保存用于传输的外部RAM或者内部RAM缓冲区的地址,高8位
BUF_LEN_L		DATA  34H				;当前剩余的要传输的数据长度,低8位
BUF_LEN_H		DATA  35H				;当前剩余的要传输的数据长度,高8位
CMD_BUFFER		DATA  38H				;命令包缓冲区,本程序约定命令包长度为CONST_CMD_LEN
CMD_BUFFER_NOT	DATA  39H				;命令码的反码,用于校验命令包
CMD_BUFFER_ID	DATA  3AH				;命令包缓冲区次字节,缓冲区识别码,针对MCS51单片机定义: 1-专用功能寄存器SFR, 2-内部RAM, 3-外部RAM, 不过本程序实际只演示内部RAM
CMD_BUFFER_ADRL	DATA  3BH				;命令包缓冲区第3字节,缓冲区地址低8位
CMD_BUFFER_ADRH	DATA  3CH				;命令包缓冲区第4字节,缓冲区地址高8位
CMD_BUFFER_LENL	DATA  3DH				;命令包缓冲区第5字节,数据总长度低8位
CMD_BUFFER_LENH	DATA  3EH				;命令包缓冲区第6字节,数据总长度高8位
; 由于MCS-51单片机存取外部RAM的读写速度低于内部RAM, 并且需要用到DPTR, 所以读写速度较慢, 其程序可以参考本程序修改
;
; 定义位标志
FLAG_INT_WAIT	BIT    2FH.2          ;中断等待标志,1指示有中断数据正在CH375中等待发送
;
;
;****************************************************************************
;主程序,用于配合演示CH375和CH451功能
				ORG   0000H            ;复位后单片机入口
				LJMP  START
				ORG   0003H            ;CH375中断
				LJMP  CH375_INTER
				ORG   0013H            ;CH451中断,键盘中断后将通知PC机应用层
				LJMP  CH451_INTER
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
				MOV   R7,#0FFH
				CALL  DELAY_MS			;延时等待CH375初始化完成
; 初始化
				CALL  CH375_INIT       ;初始化
				CALL  CH451_INIT       ;初始化
;				.....
; 启动CH451的显示驱动和键盘扫描
				MOV   B,#04H           ;设置系统参数命令
				MOV   A,#03H           ;禁止看门狗,使能键盘功能,使能显示驱动
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#05H           ;设置显示参数
				MOV   A,#8CH           ;BCD译码方式,12级亮度
				CALL  CH451_WRITE      ;发出命令
				MOV   CH451_KEY,#0FFH  ;清除按键代码,在使能键盘前,CH451的DOUT输出串行数据
				MOV   LAST_KEY,#0FFH   ;有可能被当作键盘中断请求INT1,所以要先清除
				MOV   CH451_CMD_H,#0FFH	;清除CH451的命令码
; 由于在CH451启动键盘功能前的串行输出数据有可能产生键盘中断,所以需要再次取消中断
				CLR   IE1
				SETB  EA               ;允许中断
;				...
;
; 如果P2的第5脚和第6脚之间插上短路子则由单片机控制演示,否则由PC机通过USB控制进行演示
				JB    P3.5,WAIT_USB_REQ ;T1,由PC机通过USB演示
;
; 由单片机控制演示,等待按键,然后先左移原显示,再加上按键值
				MOV   B,#0FH
				MOV   A,#17H           ;向第1个数码管加载数据'H'
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#0EH
				MOV   A,#0EH           ;向第2个数码管加载数据'E'
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#0DH
				MOV   A,#18H           ;向第3个数码管加载数据'L'
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#0CH
				MOV   A,#19H           ;向第4个数码管加载数据'P'
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#0BH
				MOV   A,#10H           ;向第5个数码管加载数据' '
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#0AH
				MOV   A,#14H           ;向第6个数码管加载数据'['
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#09H
				MOV   A,#88H           ;向第7个数码管加载数据'8.'
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#08H
				MOV   A,#15H           ;向第8个数码管加载数据']'
				CALL  CH451_WRITE      ;发出命令
				MOV   B,#06H           ;闪烁控制
				MOV   A,#30H           ;第3个和第4个数码管闪烁
				CALL  CH451_WRITE      ;发出命令
;
; 以下演示为显示按键代码,并且左移
WAIT_KEY:		MOV   A,#0FFH
				XCH   A,CH451_KEY      ;取按键值并清除原值
				MOV   R2,A
				XRL   A,#0FFH
				JZ    WAIT_KEY         ;没有按键
; 检测到新按键则先左移显示再显示键值
				MOV   A,#00H           ;左移一位
				MOV   B,#03H           ;移位命令
				CALL  CH451_WRITE      ;发出命令
				MOV   A,#00H           ;左移一位
				MOV   B,#03H           ;移位命令
				CALL  CH451_WRITE      ;发出命令
				MOV   A,R2
				ANL   A,#3FH           ;键值0-63
				MOV   B,#0AH
				DIV   AB               ;A为键值的十位数
				MOV   B,#09H           ;向第7个数码管加载数据
				CALL  CH451_WRITE      ;发出命令
				MOV   A,R2
				ANL   A,#3FH           ;键值0-63
				MOV   B,#0AH
				DIV   AB
				MOV   A,B              ;A为键值的个位数
				MOV   B,#08H           ;向第8个数码管加载数据
				CALL  CH451_WRITE      ;发出命令
				SJMP  WAIT_KEY
;
; 以下指令开始工作循环,等待PC机命令进行操作
WAIT_USB_REQ:	NOP
				NOP
				NOP						;其它事务
				NOP

				MOV   A,CH451_CMD_H	   ;高4位命令
				CJNE  A,#0FFH,IS_CH451_CMD	;在CH375中断服务程序中接收到PC机发给CH451的命令

; 没有CH451的命令, 以下检测按键状态
;    LAST_KEY和CH451_KEY两个变量,都是0FFH则没有键按下
;    仅CH451_KEY为有效键值则是键刚按下, 两者都是有效键值并且相同则已经通知过计算机
				MOV   A,LAST_KEY
				XRL   A,#0FFH
				JNZ   IS_KEY_RELEASE	;按键已经按下并且已经通知过PC机,所以去检测按键释放
				MOV   A,CH451_KEY
				XRL   A,#0FFH
				JZ    WAIT_USB_REQ		;没有键按下,主循环
				MOV   A,CH451_KEY
				CLR   C
				SUBB  A,#40H
				JC    WAIT_USB_REQ		;键码无效
; 检测到新的键键,则置入有效的中断特征数据,以通知PC机应用层
				MOV   A,CH451_KEY
				MOV   LAST_KEY,A
				MOV   R0,#01H          ;设定中断特征数据01H,通知PC机的应用层,按键已按下
				MOV   R7,LAST_KEY		;中断数据的次字节就是键码,所以PC机应用层直接获取键值,不必再读取
				CALL  CH375_UP_INT2		;上传中断数据,本程序用于键盘中断,所以每次只需要传输两字节
				LJMP  WAIT_USB_REQ		;主循环
IS_KEY_RELEASE:	NOP
				CALL  CH451_READ       ;重新读取最近一次有效按键的键值
				MOV   CH451_KEY,A		;保存键值
				XRL   A,LAST_KEY
				JZ    WAIT_USB_REQ     ;键码相同,按键仍然没有释放
				MOV   R0,#02H          ;设定中断特征数据02H,通知PC机的应用层,按键已释放
				MOV   R7,LAST_KEY		;中断数据的次字节就是键码,所以PC机应用层直接获取键值
				MOV   LAST_KEY,#0FFH   ;将要发送释放信号给PC
				CALL  CH375_UP_INT2		;上传中断数据
				LJMP  WAIT_USB_REQ		;主循环

; PC发送命令给CH451,由于主程序中的CH451_READ可能会被CH375的高优先级中断,所以不宜在CH375中断服务程序中执行
IS_CH451_CMD:	NOP
				MOV   B,CH451_CMD_H		;高4位命令
				MOV   A,CH451_CMD_L		;低8位命令
				MOV   CH451_CMD_H,#0FFH	;清除原命令码,防止重发
				CALL  CH451_WRITE      ;向CH451发送命令
				LJMP  WAIT_USB_REQ		;主循环
;
;****************************************************************************
;
; 初始化子程序
; USE:   ACC, R7, DPTR
CH375_INIT:		CLR   FLAG_INT_WAIT			;清中断等待标志,1指示有中断数据正在CH375中等待发送
; 测试CH375是否正常工作,可选操作
;				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
;				MOV   A,#CMD_CHECK_EXIST
;				MOVX  @DPTR,A				;测试CH375是否正常工作,可选操作
;				NOP							;如果时钟频率低于16MHz则无需该指令延时
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
;CH375_UPLOAD:	JB    FLAG_SEND_WAIT,CH375_UPLOAD	;如果正在等待发送(前次发送尚未完成)则继续等待
;				CLR   EX0					;为了防止中途被中断而乱了顺序,必须先禁止中断
;				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
;				MOV   A,#CMD_WR_USB_DATA7
;				MOVX  @DPTR,A				;向USB端点2的发送缓冲区写入数据块
;				NOP							;如果时钟频率低于16MHz则无需该指令延时
;				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
;				MOV   A,R7
;				MOVX  @DPTR,A				;首先写入后续数据长度
;CH375_UPLOAD_1:	MOV   A,@R0
;				INC   R0
;				MOVX  @DPTR,A				;发送数据
;				DJNZ  R7,CH375_UPLOAD_1		;继续发送数据直至结束
;				SETB  FLAG_SEND_WAIT		;置等待发送标志
;				SETB  EX0					;允许中断
;				RET
;
; 上传中断数据子程序(通过中断端点上传),可选子程序
; 虽然每次可以上传8个字节, 但是本程序中用于键盘中断, 所以每次只需要传输两个字节
; ENTRY: R0 中断数据首字节, R7 中断数据首字节
; USE:   ACC, R0, R7, DPTR
CH375_UP_INT2:	MOV   A,#0FFH
CH375_UP_INT2W:	JNB   FLAG_INT_WAIT,CH375_UP_INT2G	;前次中断数据已经被PC机取走则继续
				NOP							;等待发送中断数据
				NOP
				NOP							;最多等待1毫秒,以24MHz时钟计算
				NOP
				DJNZ  ACC,CH375_UP_INT2W	;继续等待,满1毫秒后超时
; 如果未与计算机相连,或者计算机中的应用程序未运行,则会出现超时,否则不超过1毫秒中断数据就应该被PC机取走
CH375_UP_INT2G:	CLR   EX0					;为了防止中途被中断而乱了顺序,必须先禁止中断
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA5
				MOVX  @DPTR,A				;向USB端点1的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,#02H
				MOVX  @DPTR,A				;首先写入后续数据长度
				MOV   A,R0
				MOVX  @DPTR,A				;发送中断数据1
				MOV   A,R7
				MOVX  @DPTR,A				;发送中断数据2
				SETB  FLAG_INT_WAIT			;置中断等待标志
				SETB  EX0					;允许中断
				RET
;
; 上传中断数据子程序(通过中断端点上传),可选子程序
; ENTRY: R0 指向存放了准备上传数据的缓冲区, R7 准备上传的数据长度, 有效数值是0到8
; USE:   ACC, R0, R7, DPTR
;CH375_UP_INT:	CLR   EX0					;为了防止中途被中断而乱了顺序,必须先禁止中断
;				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
;				MOV   A,#CMD_WR_USB_DATA5
;				MOVX  @DPTR,A				;向USB端点1的发送缓冲区写入数据块
;				NOP							;如果时钟频率低于16MHz则无需该指令延时
;				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
;				MOV   A,R7
;				MOVX  @DPTR,A				;首先写入后续数据长度
;CH375_UP_INT_1:	MOV   A,@R0
;				INC   R0
;				MOVX  @DPTR,A				;发送中断数据
;				DJNZ  R7,CH375_UP_INT_1		;继续发送数据直至结束
;				SETB  EX0					;允许中断
;				RET
;
; 中断服务子程序
; USE:   堆栈6字节, 工作寄存器组1的R0,R5,R6,R7
CH375_INTER:	PUSH  PSW					;现场保护
				PUSH  ACC
				PUSH  DPL
				PUSH  DPH
				SETB  RS0                   ;PSW.3,切换至工作寄存器组1
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_GET_STATUS
				MOVX  @DPTR,A				;获取中断状态并取消中断请求
				NOP							;如果时钟频率低于16MHz则无需该指令延时,高于30MHz要多加2条指令
				NOP
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
CH375_INT_RET:	POP   DPH
				POP   DPL
				POP   ACC
				POP   PSW					;恢复寄存器
				RETI						;中断返回

; USB批量数据接收成功
CH375_DOWN_OK:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_RD_USB_DATA
				MOVX  @DPTR,A				;从当前USB中断的端点缓冲区读取数据块,并释放缓冲区
				NOP							;如果时钟频率低于16MHz则无需该指令延时,高于30MHz要多加2条指令
				NOP
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOVX  A,@DPTR				;首先读取后续数据长度
				MOV   R7,A
				JZ    CH375_INT_RET			;长度为0,没有数据则直接退出,在某些应用中也可以将长度0定义为一种特殊命令
				CJNE  A,#CONST_CMD_LEN,CH375_INT_DOWN	;不是命令包
				LJMP  CH375_INT_CMD			;命令块长度总是CONST_CMD_LEN
; 接收下传的数据块,每组数据的长度不超过64字节,例如总长度150个字节,则第1组和第2组各是64字节,第3组是剩余长度22字节
; 为了将数据块与命令包区分开来,剩余长度不能等于CONST_CMD_LEN,可以用填充数据的办法避免相同
CH375_INT_DOWN:	LJMP  CH375_INT_DN1			;接收数据块

; 分析通过USB接收到的命令块,长度总是CONST_CMD_LEN,首字节为命令,其余为可选的参数
CH375_INT_CMD:	MOV   R0,#CMD_BUFFER		;命令包缓冲区
CH375_INT_CMD0:	MOVX  A,@DPTR				;接收命令包的数据
				MOV   @R0,A
				INC   R0
				DJNZ  R7,CH375_INT_CMD0		;继续接收命令包数据直至结束
; 分析通过USB接收到的命令块,长度总是CONST_CMD_LEN,首字节为命令,其余为可选的参数
; 这种首字节为命令的结构是由单片机和计算机应用层之间自行定义的
				MOV   A,CMD_BUFFER			;取命令码分析
				CPL   A
				XRL   A,CMD_BUFFER_NOT
				JNZ   CH375_INT_RET			;命令包反码校验错误
				MOV   A,CMD_BUFFER			;命令码
				CJNE  A,#DEMO_UP_CLEAR,CH375_INT_CMD1
; 连续上传数据块之前进行同步,实际是让单片机清除上传缓冲区的已有内容
; 如果上一次进行数据上传时,计算机提前结束上传,那么有可能在上传缓冲区中遗留有数据,所以在第二次上传前需要清除上传缓冲区
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_SET_ENDP7
				MOVX  @DPTR,A				;设置USB端点2的IN,也就是批量上传端点
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,#0EH				;同步触发位不变
				MOVX  @DPTR,A				;设置USB端点2的IN正忙,返回NAK,也就是缓冲区中没有数据
				LJMP  CH375_INT_RET

CH375_INT_CMD1:	CJNE  A,#DEMO_DOWN_DATA,CH375_INT_CMD2
; 连续下传数据块
; 假定事先约定, 该命令在第3字节和第4字节指定下面将下传的数据应该存放在外部RAM的起始地址
				MOV   A,CMD_BUFFER_ADRL
				MOV   BUF_ADDR_L,A			;保存用于下传的外部RAM缓冲区的地址
				MOV   A,CMD_BUFFER_ADRH
				MOV   BUF_ADDR_H,A
				MOV   A,CMD_BUFFER_LENL
				MOV   BUF_LEN_L,A			;假定事先约定第5和第6字节是将要下传的数据长度
				MOV   A,CMD_BUFFER_LENH
				MOV   BUF_LEN_H,A
				LJMP  CH375_INT_RET

CH375_INT_CMD2:	CJNE  A,#DEMO_UP_DATA,CH375_INT_CMD3
; 连续上传数据块
; 假定事先约定, 该命令在第3字节和第4字节指定下面将上传的数据应该来自外部RAM的起始地址
				MOV   A,CMD_BUFFER_ADRL
				MOV   BUF_ADDR_L,A			;保存用于上传的外部RAM缓冲区的地址
				MOV   A,CMD_BUFFER_ADRH
				MOV   BUF_ADDR_H,A
				MOV   A,CMD_BUFFER_LENL
				MOV   BUF_LEN_L,A			;假定事先约定第5和第6字节是将要下传的数据长度
				MOV   A,CMD_BUFFER_LENH
				MOV   BUF_LEN_H,A
; 数据上传,准备第一组数据,每组数据的长度不超过64字节
				CLR   C
				MOV   A,BUF_LEN_L
				SUBB  A,#40H				;检查剩余长度
				MOV   R5,A
				MOV   A,BUF_LEN_H
				SUBB  A,#00H
				MOV   R6,A
				JC    CH375_UP_SHORT		;剩余长度不足一个数据包的最大长度64
				MOV   R7,#40H
				MOV   BUF_LEN_L,R5
				MOV   BUF_LEN_H,R6
				SJMP  CH375_UP_FIRST
CH375_UP_SHORT:	MOV   R7,BUF_LEN_L
				MOV   BUF_LEN_L,#00H
				MOV   BUF_LEN_H,#00H
CH375_UP_FIRST:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA7
				MOVX  @DPTR,A				;向USB端点2的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,R7					;每次最多上传64个字节的数据
				MOVX  @DPTR,A				;首先写入后续数据长度
; 下面是从MCS51单片机的内部RAM读取数据并上传,速度是外部RAM快3倍
				MOV   R0,BUF_ADDR_L			;由于演示板中没有外部RAM,并且外部RAM的效率很低,所以用内部RAM示范
CH375_INT_UP0:	MOV   A,@R0					;从内部RAM缓冲区取出准备上传的数据
				INC   R0					;在12MHz时钟是这种方法每复制1字节大概需要6uS
				MOVX  @DPTR,A				;将数据写入批量端点的发送缓冲区
				DJNZ  R7,CH375_INT_UP0		;继续写入数据直至结束
				LJMP  CH375_INT_RET
; 下面从MCS51单片机的外部RAM读取数据并上传,如果采用较高时钟并且具有双DPTR的单片机则速度可以快些
;CH375_INT_UP0:	MOV   DPL,BUF_ADDR_L		;由于MCS51单片机读取外部RAM的效率很低
;				MOV   DPH,BUF_ADDR_H		;所以在12MHz时钟是这种方法每复制1字节大概需要18uS
;				MOVX  A,@DPTR				;从外部RAM缓冲区取出准备上传的数据
;				INC   DPTR					;缓冲区地址增量
;				MOV   BUF_ADDR_L,DPL
;				MOV   BUF_ADDR_H,DPH
;				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
;				MOVX  @DPTR,A				;将数据写入批量端点的发送缓冲区
;				DJNZ  R7,CH375_INT_UP0		;继续写入数据直至结束
;				LJMP  CH375_INT_RET

CH375_INT_CMD3:	CJNE  A,#DEMO_TEST_DATA,CH375_INT_CMD4
; 测试数据传输命令,将PC机发来的当前命令包的所有数据按位取反后返回
				MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA7
				MOVX  @DPTR,A				;向批量端点的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,#CONST_CMD_LEN
				MOV   R7,A
				MOVX  @DPTR,A				;首先写入后续数据长度,也就是命令包的长度
				MOV   R0,#CMD_BUFFER		;命令包缓冲区
CH375_TEST_DAT:	MOV   A,@R0
				CPL   A                     ;数据按位取反
				MOVX  @DPTR,A
				INC   R0
				DJNZ  R7,CH375_TEST_DAT     ;将命令包数据按位取反写入批量端点的发送缓冲区
				MOV   BUF_LEN_L,#00H		;剩余数据长度为0,只发送一组数据
				MOV   BUF_LEN_H,#00H
				LJMP  CH375_INT_RET

CH375_INT_CMD4:	CJNE  A,#DEMO_CH451_CMD,CH375_INT_CMD5
; PC发送命令给CH451
				MOV   A,CMD_BUFFER_ADRH
				MOV   CH451_CMD_H,A         ;高4位命令
				MOV   A,CMD_BUFFER_ADRL     ;低8位命令
				MOV   CH451_CMD_L,A
; 为了防止在CH375中断服务程序打断主程序中的CH451_READ而执行CH451_WRITE产生错误
; 所以在此保存CH451的命令码由主程序在空闲时发给CH451
				LJMP  CH375_INT_RET

CH375_INT_CMD5:	NOP							;未定义的命令码,不支持,放弃
				LJMP  CH375_INT_RET

; 接收下传的数据块,每组数据的长度不超过64字节
CH375_INT_DN1:
; 下面是向MCS51单片机的内部RAM保存下传的数据,速度是外部RAM快3倍
				MOV   R0,BUF_ADDR_L			;由于演示板中没有外部RAM,并且外部RAM的效率很低,所以用内部RAM示范
				MOV   R0,#40H				;为了防止做测试时误操作到数据区域00H-3FH,临时指定专用缓冲区
CH375_INT_DN0:	MOVX  A,@DPTR				;从批量端点的接收缓冲区读取数据
				MOV   @R0,A					;向内部RAM缓冲区保存接收到的下传数据
				INC   R0					;在12MHz时钟是这种方法每复制1字节大概需要6uS
				DJNZ  R7,CH375_INT_DN0		;继续取出数据直至结束
				LJMP  CH375_INT_RET
; 下面从MCS51单片机的外部RAM读取数据并上传,如果采用较高时钟并且具有双DPTR的单片机则速度可以快些
;CH375_INT_DN0:	MOV   DPTR,#CH375_DAT_PORT	;数据口地址
;				MOVX  A,@DPTR				;从批量端点的接收缓冲区读取数据
;				MOV   DPL,BUF_ADDR_L		;由于MCS51单片机读取外部RAM的效率很低
;				MOV   DPH,BUF_ADDR_H		;所以在12MHz时钟是这种方法每复制1字节大概需要18uS
;				MOVX  @DPTR,A				;向外部RAM缓冲区保存接收到的下传数据
;				INC   DPTR					;缓冲区地址增量
;				MOV   BUF_ADDR_L,DPL
;				MOV   BUF_ADDR_H,DPH
;				DJNZ  R7,CH375_INT_DN0		;继续取出数据直至结束
;				LJMP  CH375_INT_RET

; USB批量数据发送成功
CH375_UP_OK:
; 假定在命令包中指定了总共需要上传的数据长度,则根据该长度上传数据
				MOV   A,BUF_LEN_L			;检查剩余长度
				ORL   A,BUF_LEN_H
				JZ    CH375_INT_UNLK		;剩余数据长度为0,没有数据需要发送,解锁后直接退出
				CLR   C
				MOV   A,BUF_LEN_L
				SUBB  A,#40H				;检查剩余长度
				MOV   R5,A
				MOV   A,BUF_LEN_H
				SUBB  A,#00H
				MOV   R6,A
				JC    CH375_UP_END			;剩余长度不足一个数据包的最大长度64
				MOV   R7,#40H
				MOV   BUF_LEN_L,R5
				MOV   BUF_LEN_H,R6
				SJMP  CH375_UP_NEXT
CH375_UP_END:	MOV   R7,BUF_LEN_L
				MOV   BUF_LEN_L,#00H
				MOV   BUF_LEN_H,#00H
CH375_UP_NEXT:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_WR_USB_DATA7
				MOVX  @DPTR,A				;向USB端点2的发送缓冲区写入数据块
				NOP							;如果时钟频率低于16MHz则无需该指令延时
				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
				MOV   A,R7					;每次最多上传64个字节的数据
				MOVX  @DPTR,A				;首先写入后续数据长度
; 下面是从MCS51单片机的内部RAM读取数据并上传,速度是外部RAM快3倍
				MOV   R0,BUF_ADDR_L			;由于演示板中没有外部RAM,并且外部RAM的效率很低,所以用内部RAM示范
CH375_INT_UP1:	MOV   A,@R0					;从内部RAM缓冲区取出准备上传的数据
				INC   R0					;在12MHz时钟是这种方法每复制1字节大概需要6uS
				MOVX  @DPTR,A				;将数据写入批量端点的发送缓冲区
				DJNZ  R7,CH375_INT_UP1		;继续写入数据直至结束
				LJMP  CH375_INT_UNLK
; 下面从MCS51单片机的外部RAM读取数据并上传,如果采用较高时钟并且具有双DPTR的单片机则速度可以快些
;CH375_INT_UP1:	MOV   DPL,BUF_ADDR_L		;由于MCS51单片机读取外部RAM的效率很低
;				MOV   DPH,BUF_ADDR_H		;所以在12MHz时钟是这种方法每复制1字节大概需要18uS
;				MOVX  A,@DPTR				;从外部RAM缓冲区取出准备上传的数据
;				INC   DPTR					;缓冲区地址增量
;				MOV   BUF_ADDR_L,DPL
;				MOV   BUF_ADDR_H,DPH
;				MOV   DPTR,#CH375_DAT_PORT	;数据口地址
;				MOVX  @DPTR,A				;将数据写入批量端点的发送缓冲区
;				DJNZ  R7,CH375_INT_UP1		;继续写入数据直至结束
;				LJMP  CH375_INT_UNLK

; 收到上传成功中断后,在退出之前必须解锁USB缓冲区,以便继续收发数据
CH375_INT_UNLK:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;释放当前USB缓冲区
				LJMP  CH375_INT_RET

; USB中断数据发送成功
CH375_UP_INT_OK:	MOV   DPTR,#CH375_CMD_PORT	;命令口地址
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;释放当前USB缓冲区
				CLR   FLAG_INT_WAIT			;清中断等待标志,通知应用程序可以继续发送中断数据
				LJMP  CH375_INT_RET
;
;****************************************************************************
;
; 初始化子程序
CH451_INIT:		CLR   CH451_DIN        ;先低后高,输出上升沿通知CH451选择4线串行接口
				SETB  CH451_DCLK       ;置为默认的高电平
				SETB  CH451_DIN
				SETB  CH451_LOAD
				SETB  CH451_DOUT       ;置为输入
; 下述三条指令用于键盘中断
				CLR   IT1              ;置外部信号为低电平触发
				CLR   IE1              ;清中断标志
				CLR   PX1              ;置低优先级
				SETB  EX1              ;允许键盘中断
				RET
;
;
; 输出命令子程序
; ENTRY: B-ACC 待写的12位数据, 低8位在ACC中, 高4位在B的低4位中
; USE:   ACC, B, R7
CH451_WRITE:	CLR   EX1              ;禁止键盘中断
				CLR   CH451_LOAD       ;命令开始,此命令可以放在后面
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
				SETB  EX1              ;允许键盘中断
				RET
;
;
; 输入键值子程序
; EXIT:  ACC 键值数据
; USE:   ACC, R7
CH451_READ:		CLR   EX1              ;禁止键盘中断
				CLR   CH451_LOAD       ;命令开始
				MOV   A,#07H           ;读取键值命令的高4位0111B
				MOV   R7,#04H          ;忽略12位命令的低8位
CH451_READ_4:	RRC   A                ;低位在前,高位在后
				CLR   CH451_DCLK
				MOV   CH451_DIN,C      ;送出一位数据
				SETB  CH451_DCLK       ;产生时钟上升沿锁通知CH451输入位数据
				DJNZ  R7,CH451_READ_4  ;位数据未完继续
				SETB  CH451_LOAD       ;产生加载上升沿通知CH451处理命令数据
				CLR   A                ;先清除键值单元以便移位
				MOV   R7,#07H          ;读入7位键值
CH451_READ_7:	MOV   C,CH451_DOUT     ;读入一位数据
				CLR   CH451_DCLK       ;产生时钟下升沿通知CH451输出下一位
				RLC   A                ;数据移入ACC,高位在前,低位在后
				SETB  CH451_DCLK
				DJNZ  R7,CH451_READ_7  ;位数据未完继续
				CLR   IE1              ;清中断标志,读操作过程中有低电平脉冲
				SETB  EX1              ;允许键盘中断
				RET
;
;
; 键盘中断子程序
; USE:   堆栈6字节
CH451_INTER:	PUSH  PSW              ;现场保护
				PUSH  ACC
				CLR   CH451_LOAD       ;命令开始
				MOV   A,#0F7H          ;读键值命令,忽略12位命令的低8位,高4位用作结束标志
CH451_INTER_4:	SETB  C                ;在高位添0以检测位数据结束
				RRC   A                ;低位在前,高位在后
				CLR   CH451_DCLK
				MOV   CH451_DIN,C      ;送出一位数据
				SETB  CH451_DCLK       ;产生时钟上升沿锁通知CH451输入位数据
				CJNE  A,#0FFH,CH451_INTER_4 ;位数据未完继续,共4位,完成后为0FFH
				SETB  CH451_LOAD       ;产生加载上升沿通知CH451处理命令数据
				MOV   A,#0FCH          ;该数据用以检测位数据结束
CH451_INTER_7:	MOV   C,CH451_DOUT     ;读入一位数据
				CLR   CH451_DCLK       ;产生时钟下升沿通知CH451输出下一位
				RLC   A                ;数据移入ACC,高位在前,低位在后
				SETB  CH451_DCLK
				JC    CH451_INTER_7    ;位数据未完继续,共7位,完成后才移出0
				MOV   CH451_KEY,A      ;保存键值
				POP   ACC
				POP   PSW
				CLR   IE1              ;清中断标志,该指令需根据实际情况作修改
				RETI                   ;中断返回
;
;*********************************************************************************************************************
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
;*********************************************************************************************************************
;
END
;
