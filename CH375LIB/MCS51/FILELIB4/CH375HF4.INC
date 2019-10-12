; 2004.06.05, 2004.09.20, 2004.10.22, 2004.11.20, 2004.12.12, 2004.12.28, 2005.01.04, 2005.01.12, 2005.01.26, 2005.03.01, 2005.07.29, 2005.12.28, 2006.08.02, 2007.08.16, 2011.08.02
;****************************************
;**  Copyright  (C)  W.ch  1999-2007   **
;**  Web:  http://www.winchiphead.com  **
;****************************************
;**  USB Host File Interface for CH375 **
;**  TC2.0@PC, KC7.0@MCS51             **
;****************************************
;
; CH375 主机文件系统接口 */
; 支持: FAT12/FAT16/FAT32 */
; MCS51单片机, 查询方式或者中断方式, 支持小/中/大模式编译 */
; 支持3种数据存取方式: 普通的单DPTR切换方式, ATMEL/PHILIPS/SST等单片机的双DPTR切换方式, 用P2+R0指向端口而用DPTR指向外部RAM的方式 */
; U盘文件读写程序库的ASM语言头文件 */
; CH375HF4是CH375HF6.LIB的优化版，主要针对支持CH375B芯片，不支持外部子程序复制方式 */
;
CH375_LIB_VER		EQU		39H

; *********************************************************************************************************************
; 硬件特性

CH375_MAX_DATA_LEN	EQU     40H							;最大数据包的长度, 缓冲区的长度

; *********************************************************************************************************************
; 命令代码

CMD_GET_IC_VER		EQU     01H							;获取芯片及固件版本
; 输出: 版本号( 位7为1, 位6为0, 位5~位0为版本号 )
;           CH375B返回版本号的值为0B7H即版本号为37H

CMD_ENTER_SLEEP		EQU     03H							;进入睡眠状态

CMD_SET_USB_SPEED	EQU     04H							;设置USB总线速度, 在每次CMD_SET_USB_MODE设置USB工作模式时会自动恢复到12Mbps全速
; 输入: 总线速度代码
;           00H=12Mbps全速FullSpeed（默认值）, 01H=1.5Mbps（仅修改频率）, 02H=1.5Mbps低速LowSpeed
CMD_SET_SYS_FREQ	EQU     CMD_SET_USB_SPEED

CMD_RESET_ALL		EQU     05H							;执行硬件复位

CMD_CHECK_EXIST		EQU     06H							;测试工作状态
; 输入: 任意数据
; 输出: 输入数据的按位取反

CMD_GET_TOGGLE		EQU     0AH							;获取OUT事务的同步状态
; 输入: 数据1AH
; 输出: 同步状态
;           位4为1则OUT事务同步, 否则OUT事务不同步

CMD_CHK_SUSPEND		EQU     0BH							;设备方式: 设置检查USB总线挂起状态的方式
; 输入: 数据10H, 检查方式
;                    00H=不检查USB挂起, 04H=以50mS为间隔检查USB挂起, 05H=以10mS为间隔检查USB挂起

CMD_DELAY_100US		EQU     0FH							;并口方式: 延时100uS
; 输出: 延时期间输出0, 延时结束输出非0

CMD_SET_USB_ID		EQU     12H							;设备方式: 设置USB厂商VID和产品PID
; 输入: 厂商ID低字节, 厂商ID高字节, 产品ID低字节, 产品ID高字节

CMD_SET_USB_ADDR	EQU     13H							;设置USB地址
; 输入: 地址值

CMD_SET_USB_MODE	EQU     15H							;设置USB工作模式
; 输入: 模式代码
;       00H=未启用的设备方式, 01H=已启用的设备方式并且使用外部固件模式, 02H=已启用的设备方式并且使用内置固件模式
;       04H=未启用的主机方式, 05H=已启用的主机方式, 06H=已启用的主机方式并且自动产生SOF包, 07H=已启用的主机方式并且复位USB总线
; 输出: 操作状态( CMD_RET_SUCCESS或CMD_RET_ABORT, 其它值说明操作未完成 )

CMD_SET_ENDP2		EQU     18H							;设备方式: 设置USB端点0的接收器
; 输入: 工作方式
;           位7为1则位6为同步触发位, 否则同步触发位不变
;           位3~位0为事务响应方式:  0000-就绪ACK, 1110-正忙NAK, 1111-错误STALL

CMD_SET_ENDP3		EQU     19H							;设备方式: 设置USB端点0的发送器
; 输入: 工作方式
;           位7为1则位6为同步触发位, 否则同步触发位不变
;           位3~位0为事务响应方式:  0000~1000-就绪ACK, 1110-正忙NAK, 1111-错误STALL

CMD_SET_ENDP4		EQU     1AH							;设备方式: 设置USB端点1的接收器
; 输入: 工作方式
;           位7为1则位6为同步触发位, 否则同步触发位不变
;           位3~位0为事务响应方式:  0000-就绪ACK, 1110-正忙NAK, 1111-错误STALL

CMD_SET_ENDP5		EQU     1BH							;设备方式: 设置USB端点1的发送器
; 输入: 工作方式
;           位7为1则位6为同步触发位, 否则同步触发位不变
;           位3~位0为事务响应方式:  0000~1000-就绪ACK, 1110-正忙NAK, 1111-错误STALL

CMD_SET_ENDP6		EQU     1CH							;设置USB端点2/主机端点的接收器
; 输入: 工作方式
;           位7为1则位6为同步触发位, 否则同步触发位不变
;           位3~位0为事务响应方式:  0000-就绪ACK, 1101-就绪但不返回ACK, 1110-正忙NAK, 1111-错误STALL

CMD_SET_ENDP7		EQU     1DH							;设置USB端点2/主机端点的发送器
; 输入: 工作方式
;           位7为1则位6为同步触发位, 否则同步触发位不变
;           位3~位0为事务响应方式:  0000-就绪ACK, 1101-就绪但无须应答, 1110-正忙NAK, 1111-错误STALL

CMD_GET_STATUS		EQU     22H							;获取中断状态并取消中断请求
; 输出: 中断状态

CMD_UNLOCK_USB		EQU     23H							;设备方式: 释放当前USB缓冲区

CMD_RD_USB_DATA0	EQU     27H							;从当前USB中断的端点缓冲区读取数据块
; 输出: 长度, 数据流

CMD_RD_USB_DATA		EQU     28H							;从当前USB中断的端点缓冲区读取数据块, 并释放缓冲区, 相当于 CMD_RD_USB_DATA0 + CMD_UNLOCK_USB
; 输出: 长度, 数据流

CMD_WR_USB_DATA3	EQU     29H							;设备方式: 向USB端点0的发送缓冲区写入数据块
; 输入: 长度, 数据流

CMD_WR_USB_DATA5	EQU     2AH							;设备方式: 向USB端点1的发送缓冲区写入数据块
; 输入: 长度, 数据流

CMD_WR_USB_DATA7	EQU     2BH							;向USB端点2或者主机端点的发送缓冲区写入数据块
; 输入: 长度, 数据流

; **************************************************************************
; 以下命令用于USB主机方式, 只有CH375支持

CMD_SET_BAUDRATE	EQU     02H							;主机方式 & 串口方式: 设置串口通讯波特率
; 输入: 波特率分频系数, 波特率分频常数
; 输出: 操作状态( CMD_RET_SUCCESS或CMD_RET_ABORT, 其它值说明操作未完成 )

CMD_GET_MAX_LUN		EQU		0AH							;主机方式: 获取USB存储器最大逻辑单元号
; 输入: 数据38H
; 输出: 最大逻辑单元号

CMD_SET_RETRY		EQU     0BH							;主机方式: 设置USB事务操作的重试次数
; 输入: 数据25H, 重试次数
;                    位7为0则收到NAK时不重试, 位7为1位6为0则收到NAK时无限重试, 位7为1位6为1则收到NAK时最多重试2秒, 位5~位0为超时后的重试次数

CMD_SET_DISK_LUN	EQU     0BH							;主机方式: 设置USB存储器的当前逻辑单元号
; 输入: 数据34H, 新的当前逻辑单元号(00H-0FH)

CMD_SET_PKT_P_SEC	EQU     0BH							;主机方式: 设置USB存储器的每扇区数据包总数
; 输入: 数据39H, 新的每扇区数据包总数(08H,10H,20H,40H)

CMD_TEST_CONNECT	EQU     16H							;主机方式: 检查USB设备连接状态
; 输出: 状态( USB_INT_CONNECT或USB_INT_DISCONNECT或USB_INT_USB_READY, 其它值说明操作未完成 )

CMD_ABORT_NAK		EQU     17H							;主机方式: 放弃当前NAK的重试
;
CMD_CLR_STALL		EQU     41H							;主机方式: 控制传输-清除端点错误
; 输入: 端点号
; 输出中断

CMD_SET_ADDRESS		EQU     45H							;主机方式: 控制传输-设置USB地址
; 输入: 地址值
; 输出中断

CMD_GET_DESCR		EQU     46H							;主机方式: 控制传输-获取描述符
; 输入: 描述符类型
; 输出中断

CMD_SET_CONFIG		EQU     49H							;主机方式: 控制传输-设置USB配置
; 输入: 配置值
; 输出中断

CMD_AUTO_SETUP		EQU     4DH							;主机方式: 自动配置USB设备
; 输出中断

CMD_ISSUE_TKN_X		EQU     4EH							;主机方式: 发出同步令牌, 执行事务, 该命令可代替 CMD_SET_ENDP6/CMD_SET_ENDP7 + CMD_ISSUE_TOKEN
; 输入: 同步标志, 事务属性
;           同步标志的位7为主机端点IN的同步触发位, 位6为主机端点OUT的同步触发位, 位5~位0必须为0
;           事务属性的低4位是令牌, 高4位是端点号
; 输出中断

CMD_ISSUE_TOKEN		EQU     4FH							;主机方式: 发出令牌, 执行事务
; 输入: 事务属性
;           低4位是令牌, 高4位是端点号
; 输出中断

CMD_DISK_BOC_CMD	EQU     50H							;主机方式: 对USB存储器执行BulkOnly传输协议的命令
; 输出中断

CMD_DISK_INIT		EQU     51H							;主机方式: 初始化USB存储器
; 输出中断

CMD_DISK_RESET		EQU     52H							;主机方式: 控制传输-复位USB存储器
; 输出中断

CMD_DISK_SIZE		EQU     53H							;主机方式: 获取USB存储器的容量
; 输出中断

CMD_DISK_READ		EQU     54H							;主机方式: 从USB存储器读数据块(以扇区为单位)
; 输入: LBA扇区地址(总长度32位, 低字节在前), 扇区数(01H~FFH)
; 输出中断

CMD_DISK_RD_GO		EQU     55H							;主机方式: 继续执行USB存储器的读操作
; 输出中断

CMD_DISK_WRITE		EQU     56H							;主机方式: 向USB存储器写数据块(以扇区为单位)
; 输入: LBA扇区地址(总长度32位, 低字节在前), 扇区数(01H~FFH)
; 输出中断

CMD_DISK_WR_GO		EQU     57H							;主机方式: 继续执行USB存储器的写操作
; 输出中断

CMD_DISK_INQUIRY	EQU     58H							;主机方式: 查询USB存储器特性
; 输出中断

CMD_DISK_READY		EQU     59H							;主机方式: 检查USB存储器就绪
; 输出中断

CMD_DISK_R_SENSE	EQU     5AH							;主机方式: 检查USB存储器错误
; 输出中断

CMD_DISK_MAX_LUN	EQU     5DH							;主机方式: 控制传输-获取USB存储器最大逻辑单元号
; 输出中断

; *********************************************************************************************************************
; 操作状态

CMD_RET_SUCCESS		EQU     51H							;命令操作成功
CMD_RET_ABORT		EQU     5FH							;命令操作失败

; *********************************************************************************************************************
; USB中断状态

; 以下状态代码为特殊事件中断, 如果通过CMD_CHK_SUSPEND启用USB总线挂起检查, 那么必须处理USB总线挂起和睡眠唤醒的中断状态
USB_INT_USB_SUSPEND	EQU     05H							;USB总线挂起事件
USB_INT_WAKE_UP		EQU     06H							;从睡眠中被唤醒事件

; 以下状态代码0XH用于USB设备方式
;   内置固件模式下只需要处理: USB_INT_EP1_OUT, USB_INT_EP1_IN, USB_INT_EP2_OUT, USB_INT_EP2_IN
;   位7-位4为0000
;   位3-位2指示当前事务, 00=OUT, 10=IN, 11=SETUP
;   位1-位0指示当前端点, 00=端点0, 01=端点1, 10=端点2, 11=USB总线复位
USB_INT_EP0_SETUP	EQU     0CH							;USB端点0的SETUP
USB_INT_EP0_OUT		EQU     00H							;USB端点0的OUT
USB_INT_EP0_IN		EQU     08H							;USB端点0的IN
USB_INT_EP1_OUT		EQU     01H							;USB端点1的OUT
USB_INT_EP1_IN		EQU     09H							;USB端点1的IN
USB_INT_EP2_OUT		EQU     02H							;USB端点2的OUT
USB_INT_EP2_IN		EQU     0AH							;USB端点2的IN
;USB_INT_BUS_RESET	EQU     0000XX11B					;USB总线复位
USB_INT_BUS_RESET1	EQU     03H							;USB总线复位
USB_INT_BUS_RESET2	EQU     07H							;USB总线复位
USB_INT_BUS_RESET3	EQU     0BH							;USB总线复位
USB_INT_BUS_RESET4	EQU     0FH							;USB总线复位

; 以下状态代码2XH-3XH用于USB主机方式的通讯失败代码, 仅CH375支持
;   位7-位6为00
;   位5为1
;   位4指示当前接收的数据包是否同步
;   位3-位0指示导致通讯失败时USB设备的应答: 0010=ACK, 1010=NAK, 1110=STALL, 0011=DATA0, 1011=DATA1, XX00=超时
;USB_INT_RET_ACK	EQU     001X0010B					;错误:对于IN事务返回ACK
;USB_INT_RET_NAK	EQU     001X1010B					;错误:返回NAK
;USB_INT_RET_STALL	EQU     001X1110B					;错误:返回STALL
;USB_INT_RET_DATA0	EQU     001X0011B					;错误:对于OUT/SETUP事务返回DATA0
;USB_INT_RET_DATA1	EQU     001X1011B					;错误:对于OUT/SETUP事务返回DATA1
;USB_INT_RET_TOUT	EQU     001XXX00B					;错误:返回超时
;USB_INT_RET_TOGX	EQU     0010X011B					;错误:对于IN事务返回数据不同步
;USB_INT_RET_PID	EQU     001XXXXXB					;错误:未定义

; 以下状态代码1XH用于USB主机方式的操作状态代码, 仅CH375支持

USB_INT_SUCCESS		EQU     14H							;USB事务或者传输操作成功
USB_INT_CONNECT		EQU     15H							;检测到USB设备连接事件
USB_INT_DISCONNECT	EQU     16H							;检测到USB设备断开事件
USB_INT_BUF_OVER	EQU     17H							;USB控制传输的数据太多, 缓冲区溢出
USB_INT_USB_READY	EQU     18H							;USB设备已经被初始化（已分配USB地址）
USB_INT_DISK_READ	EQU     1DH							;USB存储器读数据块, 请求数据读出
USB_INT_DISK_WRITE	EQU     1EH							;USB存储器写数据块, 请求数据写入
USB_INT_DISK_ERR	EQU     1FH							;USB存储器操作失败

; *********************************************************************************************************************
; 常用USB定义

; USB的包标识PID, 主机方式可能用到
DEF_USB_PID_NULL	EQU     00H							;保留PID, 未定义
DEF_USB_PID_SOF		EQU     05H
DEF_USB_PID_SETUP	EQU     0DH
DEF_USB_PID_IN		EQU     09H
DEF_USB_PID_OUT		EQU     01H
DEF_USB_PID_ACK		EQU     02H
DEF_USB_PID_NAK		EQU     0AH
DEF_USB_PID_STALL	EQU     0EH
DEF_USB_PID_DATA0	EQU     03H
DEF_USB_PID_DATA1	EQU     0BH
DEF_USB_PID_PRE		EQU     0CH

; USB请求类型, 外置固件模式可能用到
DEF_USB_REQ_READ	EQU     80H							;控制读操作
DEF_USB_REQ_WRITE	EQU     00H							;控制写操作
DEF_USB_REQ_TYPE	EQU     60H							;控制请求类型
DEF_USB_REQ_STAND	EQU     00H							;标准请求
DEF_USB_REQ_CLASS	EQU     20H							;设备类请求
DEF_USB_REQ_VENDOR	EQU     40H							;厂商请求
DEF_USB_REQ_RESERVE	EQU     60H							;保留请求

; USB标准设备请求, RequestType的位6位5=00(Standard), 主机方式可能用到
DEF_USB_CLR_FEATURE	EQU     01H
DEF_USB_SET_FEATURE	EQU     03H
DEF_USB_GET_STATUS	EQU     00H
DEF_USB_SET_ADDRESS	EQU     05H
DEF_USB_GET_DESCR	EQU     06H
DEF_USB_SET_DESCR	EQU     07H
DEF_USB_GET_CONFIG	EQU     08H
DEF_USB_SET_CONFIG	EQU     09H
DEF_USB_GET_INTERF	EQU     0AH
DEF_USB_SET_INTERF	EQU     0BH
DEF_USB_SYNC_FRAME	EQU     0CH

; *********************************************************************************************************************

; 返回的操作状态码 */
; 以下是中断状态码,适用于CH375IntStatus
; 以下是事件通知状态码,检测到当前U盘已经连接或者已经断开,可以作为最终状态码 */
ERR_USB_CONNECT		EQU		015H	; 检测到USB设备连接事件,磁盘已经连接 */
ERR_USB_DISCON		EQU		016H	; 检测到USB设备断开事件,磁盘已经断开 */
; 以下是阶段性的状态码,用于请求数据交换,不是最终的操作状态 */
;USB_INT_DISK_READ	EQU		01DH	; USB存储器读数据块,只用于CMD_FileRead命令,请求数据读出 */
;USB_INT_DISK_WRITE	EQU		01EH	; USB存储器写数据块,只用于CMD_FileWrite命令,请求数据写入 */
ERR_USB_DISK_ERR	EQU		01FH	; USB存储器操作失败,在初始化时可能是USB存储器不支持,在读写操作中可能是磁盘损坏或者已经断开 */
; 最终状态码 */
ERR_SUCCESS			EQU		000H	; 操作成功 */
ERR_CH375_ERROR		EQU		081H	; CH375硬件错误,可能需要复位CH375 */
ERR_DISK_DISCON		EQU		082H	; 检测到USB设备连接事件,磁盘已经断开,或者磁盘尚未连接 */
ERR_STATUS_ERR		EQU		083H	; 磁盘状态错误,可能正在连接或者断开磁盘 */
ERR_MBR_ERROR		EQU		091H	; 磁盘的主引导记录无效,可能磁盘尚未分区或者尚未格式化 */
ERR_TYPE_ERROR		EQU		092H	; 磁盘分区类型不支持,只支持FAT12/FAT16/BigDOS/FAT32,需要由磁盘管理工具重新分区 */
ERR_BPB_ERROR		EQU		0A1H	; 磁盘尚未格式化,或者参数错误,需要由WINDOWS采用默认参数重新格式化 */
ERR_TOO_LARGE		EQU		0A2H	; 磁盘非正常格式化并且容量大于4GB,或者容量大于250GB,需要由WINDOWS采用默认参数重新格式化 */
ERR_FAT_ERROR		EQU		0A3H	; 磁盘的文件系统不支持,只支持FAT12/FAT16/FAT32,需要由WINDOWS采用默认参数重新格式化 */
ERR_DISK_FULL		EQU		0B1H	; 磁盘文件太满,剩余空间太少或者已经没有,需要磁盘整理 */
ERR_FDT_OVER		EQU		0B2H	; 目录内文件太多,没有空闲的目录项,FAT12/FAT16根目录下的文件数应该少于500个,需要磁盘整理 */
ERR_MISS_DIR		EQU		0B3H	; 指定路径的某个子目录没有找到,可能是目录名称错误 */
ERR_FILE_CLOSE		EQU		0B4H	; 文件已经关闭,如果需要使用,应该重新打开文件 */
ERR_OPEN_DIR		EQU		041H	; 指定路径的目录被打开 */
ERR_MISS_FILE		EQU		042H	; 指定路径的文件没有找到,可能是文件名称错误 */
ERR_FOUND_NAME		EQU		043H	; 搜索到与通配符相匹配的文件名,文件名及其完整路径在命令缓冲区中,如果需要使用,应该打开该文件 */
; 其余错误代码未定义 */

; ********************************************************************************************************************* */
; 磁盘及文件状态,适用于CH375DiskStatus */
DISK_UNKNOWN		EQU		000H	; 尚未初始化,未知状态 */
DISK_DISCONNECT		EQU		001H	; 磁盘没有连接或者已经断开 */
DISK_CONNECT		EQU		002H	; 磁盘已经连接,但是尚未初始化或者无法识别该磁盘 */
DISK_MOUNTED		EQU		003H	; 磁盘已经初始化成功,但是尚未分析文件系统或者文件系统不支持 */
DISK_READY			EQU		010H	; 已经分析磁盘的文件系统并且能够支持 */
DISK_OPEN_ROOT		EQU		012H	; 已经打开根目录,扇区模式,只能以扇区为单位读写目录的内容,使用后必须关闭,注意FAT12/FAT16根目录是固定长度 */
DISK_OPEN_DIR		EQU		013H	; 已经打开子目录,扇区模式,只能以扇区为单位读写目录的内容 */
DISK_OPEN_FILE		EQU		014H	; 已经打开文件,扇区模式,可以以扇区为单位进行数据读写 */
DISK_OPEN_FILE_B	EQU		015H	; 已经打开文件,字节模式,可以以字节为单位进行数据读写 */

; ********************************************************************************************************************* */
; FAT类型标志,适用于CMD_PARAM.Query.mDiskFat */
DISK_FS_UNKNOWN		EQU		0		; 未知的文件系统 */
DISK_FAT12			EQU		1		; FAT12文件系统 */
DISK_FAT16			EQU		2		; FAT16文件系统 */
DISK_FAT32			EQU		3		; FAT32文件系统 */

; ********************************************************************************************************************* */
; 文件属性,适用于CMD_PARAM.Modify.mFileAttr */
ATTR_READ_ONLY		EQU		001H	; 文件为只读属性 */
ATTR_HIDDEN			EQU		002H	; 文件为隐含属性 */
ATTR_SYSTEM			EQU		004H	; 文件为系统属性 */
ATTR_VOLUME_ID		EQU		008H	; 卷标 */
ATTR_DIRECTORY		EQU		010H	; 子目录 */
ATTR_ARCHIVE		EQU		020H	; 文件为存档属性 */
;ATTR_LONG_NAME			( ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID )
; 文件属性 unsigned char */
; bit0 bit1 bit2 bit3 bit4 bit5 bit6 bit7 */
;  只   隐   系   卷   目   存   未定义   */
;  读   藏   统   标   录   档            */
; 文件时间 unsigned short,适用于CMD_PARAM.Modify.mFileTime */
; Time = (Hour<<11) + (Minute<<5) + (Second>>1) */
; 文件日期 unsigned short,适用于CMD_PARAM.Modify.mFileDate */
; Date = ((Year-1980)<<9) + (Month<<5) + Day */

; ********************************************************************************************************************* */
; 文件名,适用于CMD_PARAM.?.mPathName */
PATH_WILDCARD_CHAR	EQU		02AH	; 路径名的通配符 '*' */
PATH_SEPAR_CHAR1	EQU		05CH	; 路径名的分隔符 '\' */
PATH_SEPAR_CHAR2	EQU		02FH	; 路径名的分隔符 '/' */
#ifndef MAX_PATH_LEN
MAX_PATH_LEN		EQU		30		; 最大路径长度,含所有斜杠分隔符和小数点间隔符以及路径结束符00H */
#endif
#ifndef MAX_BYTE_IO
MAX_BYTE_IO			EQU		( MAX_PATH_LEN - 1 )	; 以字节为单位单次读写文件时的最大长度,超过该长度可以分多次读写 */
#endif

; 外部命令参数结构, 用于在调用CH375程序库中的子程序时提供参数
;typedef union _CMD_PARAM {
;	struct {
;		UINT8	mBuffer[ MAX_PATH_LEN ];
;	} Other;
;	struct {
;		UINT32	mReserved;
;		UINT32	mTotalSector;			/* 返回: 当前逻辑盘的总扇区数 */
;		UINT32	mFreeSector;			/* 返回: 当前逻辑盘的剩余扇区数 */
;		UINT8	mDiskFat;				/* 返回: 当前逻辑盘的FAT类型 */
;	} Query;							/* CMD_DiskQuery, 查询磁盘信息 */
;	struct {
;		UINT8	mPathName[ MAX_PATH_LEN ];	/* 输入参数: 路径: [盘符,冒号,斜杠,目录名或者文件名及扩展名...,结束符00H], 其中盘符和冒号可以省略, 例如"C:\DIR1.EXT\DIR2\FILENAME.EXT",00H */
;	} Open;								/* CMD_FileOpen, 打开文件 */
;	struct {
;		UINT8	mPathName[ MAX_PATH_LEN ];	/* 输入参数: 路径: [盘符,冒号,斜杠,目录名或者文件名及扩展名(含通配符*)...,枚举序号], 其中盘符和冒号可以省略, 例如"C:\DIR1.EXT\DIR2\FILE*",00H */
;	} Enumer;							/* CMD_FileEnumer, 枚举文件,返回文件名 */
;	struct {
;		UINT8	mUpdateLen;				/* 输入参数: 是否允许更新长度: 0禁止,1允许 */
;	} Close;							/* CMD_FileClose, 关闭当前文件 */
;	struct {
;		UINT8	mPathName[ MAX_PATH_LEN ];	/* 输入参数: 路径: [盘符,冒号,斜杠,目录名或者文件名及扩展名...,结束符00H], 其中盘符和冒号可以省略, 例如"C:\DIR1.EXT\DIR2\FILENAME.EXT",00H */
;	} Create;							/* CMD_FileCreate, 新建文件并打开,如果文件已经存在则先删除后再新建 */
;	struct {
;		UINT8	mPathName[ MAX_PATH_LEN ];	/* 输入参数: 路径: [盘符,冒号,斜杠,目录名或者文件名及扩展名...,结束符00H], 其中盘符和冒号可以省略, 例如"C:\DIR1.EXT\DIR2\FILENAME.EXT",00H */
;	} Erase;							/* CMD_FileErase, 删除文件并关闭 */
;	struct {
;		UINT32	mFileSize;				/* 输入参数: 新的文件长度,为0FFFFFFFFH则不修改, 返回: 原长度 */
;		UINT16	mFileDate;				/* 输入参数: 新的文件日期,为0FFFFH则不修改, 返回: 原日期 */
;		UINT16	mFileTime;				/* 输入参数: 新的文件时间,为0FFFFH则不修改, 返回: 原时间 */
;		UINT8	mFileAttr;				/* 输入参数: 新的文件属性,为0FFH则不修改, 返回: 原属性 */
;	} Modify;							/* CMD_FileQuery, 查询当前文件的信息; CMD_FileModify, 查询或者修改当前文件的信息 */
;	struct {
;		UINT32	mSectorOffset;			/* 输入参数: 扇区偏移,0则移动到文件头,0FFFFFFFFH则移动到文件尾, 返回: 当前文件指针对应的绝对线性扇区号, 0FFFFFFFFH则已到文件尾 */
;	} Locate;							/* CMD_FileLocate, 移动当前文件指针 */
;	struct {
;		UINT8	mSectorCount;			/* 输入参数: 读取扇区数, 返回: 实际读取扇区数 */
;	} Read;								/* CMD_FileRead, 从当前文件读取数据 */
;	struct {
;		UINT8	mSectorCount;			/* 输入参数: 写入扇区数, 返回: 实际写入扇区数 */
;	} Write;							/* CMD_FileWrite, 向当前文件写入数据 */
;	struct {
;		UINT8	mSectorCount;			/* 输入参数: 读取扇区数, 返回: 实际读取扇区数 */
;		UINT8	mReserved[7];
;		PUINT8X	mDataBuffer;			/* 输入参数: 缓冲区起始地址, 返回: 缓冲区当前地址 */
;	} ReadX;							/* CMD_FileReadX, 从当前文件读取数据到指定缓冲区 */
;	struct {
;		UINT8	mSectorCount;			/* 输入参数: 写入扇区数, 返回: 实际写入扇区数 */
;		UINT8	mReserved[7];
;		PUINT8X	mDataBuffer;			/* 输入参数: 缓冲区起始地址, 返回: 缓冲区当前地址 */
;	} WriteX;							/* CMD_FileWriteX, 向当前文件写入指定缓冲区的数据 */
;	struct {
;		UINT32	mDiskSizeSec;			/* 返回: 整个物理磁盘的总扇区数 */
;	} DiskSize;							/* CMD_DiskSize, 查询磁盘容量 */
;	struct {
;		UINT32	mByteOffset;			/* 输入参数: 以字节为单位的偏移量, 以字节为单位的文件指针, 返回: 当前文件指针对应的绝对线性扇区号, 0FFFFFFFFH则已到文件尾 */
;	} ByteLocate;						/* CMD_ByteLocate, 以字节为单位移动当前文件指针 */
;	struct {
;		UINT8	mByteCount;				/* 输入参数: 准备读取的字节数,不得大于MAX_BYTE_IO, 返回: 实际读出的字节数 */
;		UINT8	mByteBuffer[ MAX_BYTE_IO ];	/* 返回: 读出的数据块 */
;	} ByteRead;							/* CMD_ByteRead, 以字节为单位从当前文件读取数据块 */
;	struct {
;		UINT8	mByteCount;				/* 输入参数: 准备写入的字节数,不得大于MAX_BYTE_IO, 返回: 实际写入的字节数 */
;		UINT8	mByteBuffer[ MAX_BYTE_IO ];	/* 输入参数: 准备写入的数据块 */
;	} ByteWrite;						/* CMD_ByteWrite, 以字节为单位向当前文件写入数据块 */
;	struct {
;		UINT8	mSaveVariable;			/* 输入参数: 为0则恢复单个U盘的变量,为0x80则恢复多个U盘的变量,其它值则备份/保存变量 */
;		UINT8	mReserved[3];
;		PUINT8X	mBuffer;				/* 输入参数: 指向子程序库的变量的备份缓冲区,长度不小于80个字节 */
;	} SaveVariable;						/* CMD_SaveVariable, 备份/保存/恢复子程序库的变量 */
;	union {
;		struct {
;			UINT32	mCBW_Sig;
;			UINT32	mCBW_Tag;
;			UINT8	mCBW_DataLen;		/* 输入: 数据传输长度,有效值是0到255 */
;			UINT8	mCBW_DataLen1;
;			UINT8	mCBW_DataLen2;
;			UINT8	mCBW_DataLen3;
;			UINT8	mCBW_Flag;			/* 输入: 传输方向等标志 */
;			UINT8	mCBW_LUN;
;			UINT8	mCBW_CB_Len;		/* 输入: 命令块的长度,有效值是1到16 */
;			UINT8	mCBW_CB_Buf[6];		/* 输入: 命令块,该缓冲区最多为16个字节 */
;		} mCBW;							/* BulkOnly协议的命令块, 输入CBW结构 */
;		struct {
;			UINT32	mCSW_Sig;
;			UINT32	mCSW_Tag;
;			UINT32	mCSW_Residue;		/* 返回: 剩余数据长度 */
;			UINT8	mCSW_Status;		/* 返回: 命令执行结果状态 */
;			UINT8	mReserved;
;		} mCSW;							/* BulkOnly协议的命令状态块, 输出CSW结构 */
;	} BOC;								/* CMD_BulkOnlyCmd, 执行基于BulkOnly协议的命令, 如果有数据传输那么数据在pDISK_BASE_BUF中 */
;} CMD_PARAM;

#define EN_DISK_WRITE			1
#define EN_DISK_FAT12			1
#define EN_DISK_FAT32			1		;使用CH375HF4.LIB时请禁止该常量定义
#define EN_BYTE_ACCESS			1
;#define EN_SAVE_VARIABLE		1
;#define EXT_BLK_INTERFACE		1
#define EN_SEC_SIZE_AUTO		1

; ********************************************************************************************************************* */
; 段定义
CH375LIB_CODE			SEGMENT		CODE
CH375LIB_IDATA			SEGMENT		IDATA

; ********************************************************************************************************************* */
; 子程序库中提供的变量 */
	EXTRN	DATA (CH375IntStatus)	;CH375操作的中断状态
	EXTRN	DATA (CH375DiskStatus)	;磁盘及文件状态
	EXTRN	DATA (CH375LibConfig)	;CH375程序库配置,下行说明
; 位7: CH375的INT#引脚连接方式: 0查询方式,1中断方式 */
; 位6: 该位为1且CH375Version2为1则芯片为CH375B */
; 位5: 在写操作结束后是否延时: 0写后延时,1不延时 */
; 位4: 在添加数据后是否自动更新文件长度: 0不更新,1自动更新 */
; 位3位2: 针对文件读写的多扇区数据的复制方式: 00单DPTR复制, 01单DPTR复制, 10双DPTR复制, 11单DPTR和P2+R0复制 */
; 位1位0: 针对磁盘读写的单扇区数据的复制方式: 00单DPTR复制, 01单DPTR复制, 10双DPTR复制, 11单DPTR和P2+R0复制 */

; 如果CH375的INT#引脚连接到单片机的中断输入引脚并且准备使用中断方式,那么LIB_CFG_INT_EN定义为1,否则定义为0由单片机查询INT#引脚 */
; 由于MCS51单片机复制外部RAM中的数据时比较慢,所以CH375的程序库提供几种优化速度的方式,在LIB_CFG_FILE_IO和LIB_CFG_DISK_IO中定义:
;   方式0:
;   方式1:"单DPTR复制", 最常规的数据复制方式, 使用一个DPTR来回切换, 每传输一个字节需要16个机器周期, 速度最慢, 适用于所有MCS51单片机,
;   方式2:"双DPTR复制", 针对特定硬件的数据复制方式, 使用两个DPTR, 每传输一个字节需要8.5个机器周期, 速度较快, 适用于ATMEL/PHILIPS/SST等具有双DPTR的单片机,
;   方式3:"单DPTR和P2+R0复制", 用P2+R0指向CH375的I/O端口并且用DPTR指向外部RAM进行数据复制, 每传输一个字节需要6.25个机器周期, 速度最快,
;                       适用于所有标准的MCS51单片机, 但是某些单片机在启用内置的外部RAM时会关闭P2+R0的功能, 所以可能不适用,
;   对于文件数据读写,也就是应用程序调用CH375FileReadX和CH375FileWriteX子程序时:
;     在方式1,2,3下,应用程序每次调用CH375FileReadX和CH375FileWriteX时,CH375的程序库都会从指定缓冲区的起始地址开始读写数据,
;     例如: 某文件长度为1K(占用2个扇区), 如果调用CH375FileReadX时读1K(指定mCmdParam.Read.mSectorCount为2), 那么1K数据全读到指定缓冲区中,
;           如果缓冲区较小只有0.5K, 那么分两次读取, 第一次调用CH375FileReadX时读0.5K, 处理完这0.5K数据后再调用CH375FileReadX读下一个0.5K并处理

	EXTRN	DATA (CH375vDiskFat)	;逻辑盘的FAT标志:1=FAT12,2=FAT16,3=FAT32
	EXTRN	DATA (CH375vSecPerClus)	;逻辑盘的每簇扇区数
	EXTRN	DATA (CH375vStartCluster)	;当前文件或者目录的起始簇号,UINT32
	EXTRN	DATA (CH375vFileSize)	;当前文件的长度,UINT32
	EXTRN	DATA (CH375vCurrentOffset)	;当前文件指针,当前读写位置的字节偏移

; FAT数据区中文件目录信息 */
;typedef struct _FAT_DIR_INFO {
;	UINT8	DIR_Name[11];				/* 00H,文件名,共11字节,不足处填空格 */
;	UINT8	DIR_Attr;					/* 0BH,文件属性,参考前面的说明 */
;	UINT8	DIR_NTRes;					/* 0CH */
;	UINT8	DIR_CrtTimeTenth;			/* 0DH,文件创建的时间,以0.1秒单位计数 */
;	UINT16	DIR_CrtTime;				/* 0EH,文件创建的时间 */
;	UINT16	DIR_CrtDate;				/* 10H,文件创建的日期 */
;	UINT16	DIR_LstAccDate;				/* 12H,最近一次存取操作的日期 */
;	UINT16	DIR_FstClusHI;				/* 14H */
;	UINT16	DIR_WrtTime;				/* 16H,文件修改时间,参考前面的宏MAKE_FILE_TIME */
;	UINT16	DIR_WrtDate;				/* 18H,文件修改日期,参考前面的宏MAKE_FILE_DATA */
;	UINT16	DIR_FstClusLO;				/* 1AH */
;	UINT32	DIR_FileSize;				/* 1CH,文件长度 */
;} FAT_DIR_INFO;							/* 20H */

	EXTRN	BIT		(CH375Version2)		;芯片版本:0-CH375,1-CH375A/B
	EXTRN	XDATA	(CH375vDataStart)	;逻辑盘的数据区域的起始LBA,UINT32
	EXTRN	DATA	(CH375vFdtLba)		;当前FDT所在的LBA地址,UINT32
	EXTRN	DATA	(CH375vFdtOffset)	;当前FDT在扇区内的偏移地址,UINT16
	EXTRN	XDATA	(CH375vDiskRoot)	;对于FAT16盘为根目录占用扇区数,对于FAT32盘为根目录起始簇号,UINT32
#ifdef EN_SEC_SIZE_AUTO
	EXTRN	XDATA	(CH375vSectorSize)	;磁盘的扇区大小,UINT16
#else
#define	CH375vSectorSize	512			;磁盘的扇区大小
#endif
	EXTRN	DATA	(pDISK_BASE_BUF)	;指向外部RAM的磁盘数据缓冲区,缓冲区长度不小于CH375vSectorSize,由应用程序初始化,UINT16,PUINT8X

	EXTRN	CODE	(CH375ReadBlock)	; 从磁盘读取多个扇区的数据到外部接口交换区 */
#ifdef EN_DISK_WRITE
	EXTRN	CODE	(CH375WriteBlock)	; 将外部接口交换区的多个扇区的数据块写入磁盘 */
#endif

#ifndef LIB_CFG_VALUE
#define	LIB_CFG_VALUE			05H		; CH375程序库配置值 */
#endif

; ********************************************************************************************************************* */
; 子程序库中提供的子程序, 操作完成后返回状态码在R7中 */
; 下述子程序中, 文件操作子程序CH375File*和磁盘查询子程序CH375DiskQuery都可能会用到磁盘数据缓冲区pDISK_BASE_BUF,
;   并且有可能在pDISK_BASE_BUF中保存了磁盘信息, 所以必须保证pDISK_BASE_BUF不被用于其它用途,
;   如果RAM较少, 要将pDISK_BASE_BUF临时用于其它用途, 那么在临时用完后必须调用CH375DirtyBuffer清除磁盘缓冲区
	EXTRN	CODE (CH375GetVer)			; 获取当前子程序库的版本号 */
	EXTRN	CODE (CH375Reset)			; 复位CH375 */
	EXTRN	CODE (CH375Init)			; 初始化CH375 */
	EXTRN	CODE (CH375DiskConnect)		; 检查磁盘是否连接 */
	EXTRN	CODE (CH375DiskReady)		; 查询磁盘是否准备好 */
	EXTRN	CODE (CH375DirtyBuffer)		; 清除磁盘缓冲区 */
	EXTRN	CODE (CH375FileOpen)		; 打开文件或者枚举文件 */
	EXTRN	CODE (CH375FileClose)		; 关闭当前文件 */
#ifdef EN_DISK_WRITE
	EXTRN	CODE (CH375FileErase)		; 删除文件并关闭 */
	EXTRN	CODE (CH375FileCreate)		; 新建文件并打开,如果文件已经存在则先删除后再新建 */
#endif
	EXTRN	CODE (CH375FileModify)		; 查询或者修改当前文件的信息 */
	EXTRN	CODE (CH375FileLocate)		; 移动当前文件指针 */
	EXTRN	CODE (CH375FileReadX)		; 从当前文件读取数据到指定缓冲区 */
#ifdef EN_DISK_WRITE
	EXTRN	CODE (CH375FileWriteX)		; 向当前文件写入指定缓冲区的数据 */
#endif
#ifdef EN_BYTE_ACCESS
	EXTRN	CODE (CH375ByteLocate)		; 以字节为单位移动当前文件指针 */
	EXTRN	CODE (CH375ByteRead)		; 以字节为单位从当前位置读取数据块 */
#ifdef EN_DISK_WRITE
	EXTRN	CODE (CH375ByteWrite)		; 以字节为单位向当前位置写入数据块 */
#endif
#endif
	EXTRN	CODE (CH375DiskSize)		; 查询磁盘容量 */
	EXTRN	CODE (CH375DiskQuery)		; 查询磁盘信息 */
#ifdef EN_SAVE_VARIABLE
	EXTRN	CODE (CH375SaveVariable)	; 备份/保存/恢复子程序库的变量,用于子程序库在多个CH375芯片之间进行切换 */
#endif
	EXTRN	CODE (CH375BulkOnlyCmd)		; 执行基于BulkOnly协议的命令 */
	EXTRN	CODE (CH375sDiskReady)		; 查询磁盘是否准备好,支持CH375S */

; 该头文件可以为CH375子程序库分配必要的I/O及内存资源,并产生必要的与硬件有关的目标代码,
; 如果该文件是被工程项目的多个源程序包含作为头文件,那么应该只允许一个头文件分配资源和产生代码,
; 除此之外的头文件应该被事先定义CH375HF_NO_CODE,从而禁止该头文件产生重复的目标代码,例如:
; #define	CH375HF_NO_CODE		1
; $include	(CH375HF?.INC)

#ifdef CH375HF_NO_CODE

	EXTRN	IDATA (mCmdParam)			; 命令参数 */
	EXTRN	XDATA (CH375_CMD_PORT)		; CH375命令端口的I/O地址 */
	EXTRN	XDATA (CH375_DAT_PORT)		; CH375数据端口的I/O地址 */
#ifdef DISK_BASE_BUF_LEN
	EXTRN	XDATA (DISK_BASE_BUF)		; 外部RAM的磁盘数据缓冲区,缓冲区长度为一个扇区的长度 */
#endif
#ifdef FILE_DATA_BUF_LEN
	EXTRN	XDATA (FILE_DATA_BUF)		; 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */
	EXTRN	CODE (CH375FileRead)		; 从当前文件读取数据 */
#ifdef EN_DISK_WRITE
	EXTRN	CODE (CH375FileWrite)		; 向当前文件写入数据 */
#endif
#endif
#ifndef NO_DEFAULT_CH375_F_ENUM
	EXTRN	CODE (CH375FileEnumer)		; 枚举文件 */
#endif
#ifndef NO_DEFAULT_CH375_F_QUERY
	EXTRN	CODE (CH375FileQuery)		; 查询当前文件的信息 */
#endif
	EXTRN	CODE (xQueryInterrupt)		; 外部定义的被CH375程序库调用的子程序,查询CH375中断并更新中断状态 */
	EXTRN	CODE (xDelay100uS)			; 外部定义的被CH375程序库调用的子程序,延时100uS */
#ifdef EN_DISK_WRITE
	EXTRN	CODE (xDelayAfterWrite)		; 外部定义的被CH375程序库调用的子程序,写操作后延时 */
#endif
	EXTRN	CODE (xFileNameEnumer)		; 外部定义的被CH375程序库调用的子程序,文件名枚举回调子程序 */
	EXTRN	CODE (CH375LibInit)			; 初始化CH375程序库和CH375芯片

#else

; ********************************************************************************************************************* */
; 外部定义的被CH375程序库调用的变量
	PUBLIC	mCmdParam					; 命令参数 */
	PUBLIC	CH375_CMD_PORT				; CH375命令端口的I/O地址 */
	PUBLIC	CH375_DAT_PORT				; CH375数据端口的I/O地址 */
#ifdef DISK_BASE_BUF_LEN
	PUBLIC	DISK_BASE_BUF				; 外部RAM的磁盘数据缓冲区,缓冲区长度为一个扇区的长度 */
#endif
#ifdef FILE_DATA_BUF_LEN
	PUBLIC	FILE_DATA_BUF				; 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */
#endif

; ********************************************************************************************************************* */
; 变量定义
	RSEG  CH375LIB_IDATA
 mCmdParam:   DS   MAX_PATH_LEN			; 命令参数 */

	XSEG  AT  CH375_CMD_PORT_ADDR
 CH375_CMD_PORT:   DS   1				; CH375命令端口的I/O地址 */

	XSEG  AT  CH375_DAT_PORT_ADDR
 CH375_DAT_PORT:   DS   1				; CH375数据端口的I/O地址 */

#ifdef DISK_BASE_BUF_LEN
	XSEG  AT  DISK_BASE_BUF_ADDR
  DISK_BASE_BUF:   DS   DISK_BASE_BUF_LEN	; 外部RAM的磁盘数据缓冲区,缓冲区长度为一个扇区的长度 */
#endif

#ifdef FILE_DATA_BUF_LEN
	XSEG  AT  FILE_DATA_BUF_ADDR
  FILE_DATA_BUF:   DS   FILE_DATA_BUF_LEN	; 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */
#endif

; ********************************************************************************************************************* */
; 默认的被CH375程序库调用的子程序定义
; 以下程序可以根据需要修改, 相关详细内容可以参考C程序头文件CH375HF6.H
	RSEG  CH375LIB_CODE

#ifdef FILE_DATA_BUF_LEN
	PUBLIC	CH375FileRead				; 从当前文件读取数据 */
; 从当前文件读取数据
CH375FileRead:		MOV		R0,#mCmdParam+8			;mCmdParam.ReadX.mDataBuffer
					MOV		@R0,#HIGH FILE_DATA_BUF	;指向文件数据缓冲区
					INC		R0
					MOV		@R0,#LOW FILE_DATA_BUF
					LJMP	CH375FileReadX
#ifdef EN_DISK_WRITE
	PUBLIC	CH375FileWrite				; 向当前文件写入数据 */
; 向当前文件写入数据
CH375FileWrite:		MOV		R0,#mCmdParam+8			;mCmdParam.WriteX.mDataBuffer
					MOV		@R0,#HIGH FILE_DATA_BUF	;指向文件数据缓冲区
					INC		R0
					MOV		@R0,#LOW FILE_DATA_BUF
					LJMP	CH375FileWriteX
#endif
#endif
;
#ifndef NO_DEFAULT_CH375_F_ENUM
; 在应用程序中定义NO_DEFAULT_CH375_F_ENUM可以禁止默认的枚举文件程序,然后用自行编写的程序代替它 */
	PUBLIC	CH375FileEnumer				; 枚举文件 */
; 枚举文件
CH375FileEnumer:	LCALL	CH375FileOpen
					CJNE	R7,#ERR_FOUND_NAME,CH375FileEnumer_0
					MOV		R7,#ERR_SUCCESS		; 操作成功 */
CH375FileEnumer_0:	RET
#endif
;
#ifndef NO_DEFAULT_CH375_F_QUERY
; 在应用程序中定义NO_DEFAULT_CH375_F_QUERY可以禁止默认的查询当前文件的信息程序,然后用自行编写的程序代替它 */
	PUBLIC	CH375FileQuery				; 查询当前文件的信息 */
; 查询当前文件的信息
CH375FileQuery:		MOV		R0,#mCmdParam
					MOV		R7,#10
CH375FileQuery_1:	MOV		@R0,#0FFH	; 输入参数全部无效,仅查询不修改 */
					INC		R0
					DJNZ	R7,CH375FileQuery_1
					LJMP	CH375FileModify
#endif
;
#ifndef NO_DEFAULT_CH375_INT
; 在应用程序中定义NO_DEFAULT_CH375_INT可以禁止默认的中断处理程序,然后用自行编写的程序代替它 */
	PUBLIC	xQueryInterrupt				; 外部定义的被CH375程序库调用的子程序,查询CH375中断并更新中断状态 */
; CH375中断服务程序,由CH375的INT#的低电平或者下降沿触发单片机中断
;xQueryInterrupt:	MOV		A,CH375IntStatus		; 该子程序用于"中断方式",查询中断状态,等待硬件中断
;					JZ		xQueryInterrupt			; 子程序库调用该子程序之前CH375IntStatus=0,硬件中断后,由中断服务程序置为非0的实际中断状态后返回
;					RET
InterruptHandle:	PUSH    PSW						; 中断服务程序,如果使用"中断方式",请在中断向量处执行一个跳转到此
					PUSH    ACC
					PUSH    DPL
					PUSH    DPH
					CALL    xQueryInterrupt_1
					POP     DPH
					POP     DPL
					POP     ACC
					POP     PSW
					RETI
; 查询CH375中断并更新中断状态
#ifdef CH375_INT_WIRE
xQueryInterrupt:	JB   	CH375_INT_WIRE,$		; 该子程序用于"查询方式",查询CH375的INT#引脚,如果CH375的中断引脚输出高电平则等待
#else
xQueryInterrupt:	MOV  	DPTR,#CH375_CMD_PORT	; 该子程序用于"查询方式",查询CH375的INT#引脚,如果CH375的中断引脚输出高电平则等待
					MOVX  	A,@DPTR					; 对于CH375B芯片还可以查询命令端口的位7
					JB   	ACC.7,xQueryInterrupt	; 查询CH375B的命令端口的位7为1说明中断引脚输出高电平则等待
#endif
xQueryInterrupt_1:	MOV  	DPTR,#CH375_CMD_PORT
					MOV  	A,#CMD_GET_STATUS
					MOVX 	@DPTR,A					; 获取当前中断状态 */
					INC     DPTR
					INC     DPTR					; 操作无意义,用于至少延时2uS */
					INC     DPTR
					MOV  	DPTR,#CH375_DAT_PORT
					MOVX 	A,@DPTR					; 获取中断状态 */
					MOV  	CH375IntStatus,A
					CJNE 	A,#ERR_USB_DISCON,QUERY_INT_DISCON
					MOV  	CH375DiskStatus,#DISK_DISCONNECT	; 检测到USB设备断开事件 */
					RET
QUERY_INT_DISCON:	CJNE 	A,#ERR_USB_CONNECT,QUERY_INT_CONNECT
					MOV  	CH375DiskStatus,#DISK_CONNECT	; 检测到USB设备连接事件 */
					RET
QUERY_INT_CONNECT:	RET
#endif

#ifndef NO_DEFAULT_DELAY_100US
; 在应用程序中定义NO_DEFAULT_DELAY_100US可以禁止默认的延时100uS子程序,然后用自行编写的程序代替它 */
	PUBLIC	xDelay100uS					; 外部定义的被CH375程序库调用的子程序,延时100uS */
; 延时100uS
xDelay100uS:		MOV		R7,#150
xDelay100uS_1:		DJNZ	R7,xDelay100uS_1	; 延时100uS,2x333nS@36MHz&12T */
					RET
#endif
;
#ifdef EN_DISK_WRITE
#ifndef NO_DEFAULT_DELAY_WRITE
; 在应用程序中定义NO_DEFAULT_DELAY_WRITE可以禁止默认的写操作后延时程序,然后用自行编写的程序代替它 */
	PUBLIC	xDelayAfterWrite			; 外部定义的被CH375程序库调用的子程序,写操作后延时 */
; 写操作后延时
xDelayAfterWrite:	MOV		R7,#250
xDelayAfterWrite_1:	DJNZ	R7,xDelayAfterWrite_1	; 延时200uS左右 */
					RET
#endif
#endif
;
#ifndef NO_DEFAULT_FILE_ENUMER
; 在应用程序中定义NO_DEFAULT_FILE_ENUMER可以禁止默认的文件名枚举回调程序,然后用自行编写的程序代替它 */
	PUBLIC	xFileNameEnumer				; 外部定义的被CH375程序库调用的子程序,文件名枚举回调子程序 */
; 文件名枚举回调子程序
; 如果指定枚举序号CH375vFileSize为0xFFFFFFFF后调用FileOpen，那么每搜索到一个文件FileOpen都会调用本回调程序，
; 回调程序xFileNameEnumer返回后，FileOpen递减CH375vFileSize并继续枚举直到搜索不到文件或者目录。建议做法是，
; 在调用FileOpen之前定义一个全局变量为0，当FileOpen回调本程序后，本程序由CH375vFdtOffset得到结构FAT_DIR_INFO，
; 分析结构中的DIR_Attr以及DIR_Name判断是否为所需文件名或者目录名，记录相关信息，并将全局变量计数增量，
; 当FileOpen返回后，判断返回值如果是ERR_MISS_FILE或ERR_FOUND_NAME都视为操作成功，全局变量为搜索到的有效文件数。
; 如果在本回调程序xFileNameEnumer中将CH375vFileSize置为1，那么可以通知FileOpen提前结束搜索。以下是回调程序例子
xFileNameEnumer:	MOV		A,pDISK_BASE_BUF+1	; 参考CH375HF6.H中的程序
#if 0
					ADD		A,CH375vFdtOffset+1
					MOV		DPL,A
					MOV		R7,A
					MOV		A,pDISK_BASE_BUF
					ADDC	A,CH375vFdtOffset		; 当前FDT的起始地址
					MOV		DPH,A					; DPTR指向文件名,pFileDir -> DIR_Name[0]
					MOV		R6,A
					MOVX	A,@DPTR
					MOV		R4,A					; 文件名首字符
					INC		DPTR
					MOVX	A,@DPTR
					MOV		R5,A					; 文件名次字符
;					......
					MOV		A,R7
					ADD		A,#11
					MOV		DPL,A
					MOV		A,R6
					ADDC	A,#0
					MOV		DPH,A					; DPTR指向文件属性,pFileDir -> DIR_Attr
					MOVX	A,@DPTR
;					JB		ACC.4,是目录
;					JNB		ACC.4,是文件
#endif
					RET
#endif
;
#ifdef EXT_BLK_INTERFACE
#ifdef FILE_DATA_BUF_LEN
	PUBLIC	_xWriteToExtBuf				; 外部定义的被CH375程序库调用的子程序,从CH375读取文件数据到外部缓冲区 */
; 该子程序由CH375的子程序库调用,用于从CH375读取文件数据到外部缓冲区,被CH375FileRead调用 */
_xWriteToExtBuf:	RET								;如果文件读写的数据的复制方式不是"外部子程序",则不会调用该子程序
;
#ifdef EN_DISK_WRITE
	PUBLIC	_xReadFromExtBuf			; 外部定义的被CH375程序库调用的子程序,从外部缓冲区读取文件数据到CH375 */
; 该子程序由CH375的子程序库调用,用于从外部缓冲区读取文件数据到CH375,被CH375FileWrite调用 */
_xReadFromExtBuf:	RET								;如果文件读写的数据的复制方式不是"外部子程序",则不会调用该子程序
#endif
#endif
#endif
;
; ********************************************************************************************************************* */
; 供主程序调用的子程序
	PUBLIC	CH375LibInit
; 初始化CH375程序库和CH375芯片, 操作完成后返回状态码在R7中, 操作成功返回0
CH375LibInit:		MOV  	A,SP
					SETB  	C
					SUBB 	A,#0F0H					; CH375子程序库最多可能有7级子程序,查询方式最多需要14字节的堆栈空间,建议提供余量
					JC  	CH375LibInit_SP			; 单片机的堆栈空间不够
					MOV  	R7,#0FCH				; 当前子程序库的版本号太低则返回错误
					RET
CH375LibInit_SP:	MOV  	CH375LibConfig,#LIB_CFG_VALUE	; CH375程序库配置值
					LCALL	CH375GetVer				; 获取当前子程序库的版本号
					MOV  	A,R7
					CLR  	C
					SUBB 	A,#CH375_LIB_VER
					JNC  	CH375LibInit_GOON
					MOV  	R7,#0FFH				; 当前子程序库的版本号太低则返回错误
					RET
CH375LibInit_GOON:
#ifdef DISK_BASE_BUF_LEN
					MOV  	pDISK_BASE_BUF,#HIGH DISK_BASE_BUF	; 指向外部RAM的磁盘数据缓冲区
					MOV  	pDISK_BASE_BUF+1,#LOW DISK_BASE_BUF
#endif
					LCALL	CH375Init				; 初始化CH375
					RET
;
; ********************************************************************************************************************* */
; 以下被CH375程序库用到的子程序,来自C51的程序库C51S.LIB
; 如果没有下面这些子程序(将其注释掉), 那么链接方法是: LX51  CH375HFT.OBJ , CH375HF6.LIB, C51S.LIB
; 如果有下面这些子程序, 那么链接方法是: LX51  CH375HFT.OBJ , CH375HF6.LIB
; 建议, 如果应用程序也用到C51S.LIB中的子程序, 那么将下面这些内容注释掉, 防止链接时出现同名冲突
	PUBLIC	?C?ULCMP
?C?ULCMP:			MOV   A,R3
					SUBB  A,R7
					MOV   B,A
					MOV   A,R2
					SUBB  A,R6
					ORL   B,A
					MOV   A,R1
					SUBB  A,R5
					ORL   B,A
					MOV   A,R0
					SUBB  A,R4
					ORL   A,B
					RET
	PUBLIC	?C?ULSHR
?C?ULSHR:			MOV   A,R0
					JZ    ?C?ULSHR_RET
?C?ULSHR_LOOP:		MOV   A,R4
					CLR   C
					RRC   A
					MOV   R4,A
					MOV   A,R5
					RRC   A
					MOV   R5,A
					MOV   A,R6
					RRC   A
					MOV   R6,A
					MOV   A,R7
					RRC   A
					MOV   R7,A
					DJNZ  R0,?C?ULSHR_LOOP
?C?ULSHR_RET:		RET
	PUBLIC	?C?LSHL
?C?LSHL:			MOV   A,R0
					JZ    ?C?LSHL_RET
?C?LSHL_LOOP:		MOV   A,R7
					CLR   C
					RLC   A
					MOV   R7,A
					MOV   A,R6
					RLC   A
					MOV   R6,A
					MOV   A,R5
					RLC   A
					MOV   R5,A
					MOV   A,R4
					RLC   A
					MOV   R4,A
					DJNZ  R0,?C?LSHL_LOOP
?C?LSHL_RET:		RET
	PUBLIC	?C?LLDIDATA
?C?LLDIDATA:		MOV   A,@R0
					MOV   R4,A
					INC   R0
					MOV   A,@R0
					MOV   R5,A
					INC   R0
					MOV   A,@R0
					MOV   R6,A
					INC   R0
					MOV   A,@R0
					MOV   R7,A
					RET
	PUBLIC	?C?LLDIDATA0
?C?LLDIDATA0:		MOV   A,@R0
					MOV   R3,A
					INC   R0
					MOV   A,@R0
					MOV   R1,A
					INC   R0
					MOV   A,@R0
					MOV   R2,A
					INC   R0
					MOV   A,@R0
					XCH   A,R3
					MOV   R0,A
					RET
	PUBLIC	?C?LSTIDATA
?C?LSTIDATA:		MOV   A,R4
					MOV   @R0,A
					INC   R0
					MOV   A,R5
					MOV   @R0,A
					INC   R0
					MOV   A,R6
					MOV   @R0,A
					INC   R0
					MOV   A,R7
					MOV   @R0,A
					RET
	PUBLIC	?C?LSTXDATA
?C?LSTXDATA:		MOV   A,R4
					MOVX  @DPTR,A
					INC   DPTR
					MOV   A,R5
					MOVX  @DPTR,A
					INC   DPTR
					MOV   A,R6
					MOVX  @DPTR,A
					INC   DPTR
					MOV   A,R7
					MOVX  @DPTR,A
					RET
	PUBLIC	?C?LSTKIDATA
?C?LSTKIDATA:		POP   DPH
					POP   DPL
					CLR   A
					MOVC  A,@A+DPTR
					MOV   @R0,A
					INC   R0
					MOV   A,#01H
					MOVC  A,@A+DPTR
					MOV   @R0,A
					INC   R0
					MOV   A,#02H
					MOVC  A,@A+DPTR
					MOV   @R0,A
					INC   R0
					MOV   A,#03H
					MOVC  A,@A+DPTR
					MOV   @R0,A
					MOV   A,#04H
					JMP   @A+DPTR
; 以上是被CH375程序库用到的子程序,来自C51的程序库C51S.LIB,如果去掉这些子程序,那么链接时要加上C51S.LIB
;
#endif
;
