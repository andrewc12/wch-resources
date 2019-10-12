;以下程序节选于LPC2114开发套件 2004.07
;
;**描        述: lpc21xx的启动代码，包含异常向量入口、初始化堆栈的代码等

;定义堆栈的大小
SVC_STACK_LEGTH         EQU         0
FIQ_STACK_LEGTH         EQU         0
IRQ_STACK_LEGTH         EQU         256
ABT_STACK_LEGTH         EQU         0
UND_STACK_LEGTH         EQU         0

NoInt       EQU 0x80

USR32Mode   EQU 0x10
SVC32Mode   EQU 0x13
SYS32Mode   EQU 0x1f
IRQ32Mode   EQU 0x12
FIQ32Mode   EQU 0x11

PINSEL2     EQU 0xE002C014


;引入的外部标号在这声明
    IMPORT  FIQ_Exception                   ;快速中断异常处理程序
    IMPORT  __main                          ;C语言主程序入口 
    IMPORT  TargetResetInit                 ;目标板基本初始化
    IMPORT  StackUsr
    IMPORT  bottom_of_heap

;给外部使用的标号在这声明
    EXPORT  Reset
    EXPORT __rt_div0
    EXPORT __user_initial_stackheap

    CODE32

    AREA    vectors,CODE,READONLY
        ENTRY
;中断向量表
Reset
        LDR     PC, ResetAddr
        LDR     PC, UndefinedAddr
        LDR     PC, SWI_Addr
        LDR     PC, PrefetchAddr
        LDR     PC, DataAbortAddr
        DCD     0xb9205f80
        LDR     PC, [PC, #-0xff0]
        LDR     PC, FIQ_Addr

ResetAddr           DCD     ResetInit
UndefinedAddr       DCD     Undefined
SWI_Addr            DCD     SoftwareInterrupt
PrefetchAddr        DCD     PrefetchAbort
DataAbortAddr       DCD     DataAbort
Nouse               DCD     0
IRQ_Addr            DCD     0
FIQ_Addr            DCD     FIQ_Handler

;未定义指令
Undefined
        B       Undefined

;软中断
SoftwareInterrupt
        B       SoftwareInterrupt

;取指令中止
PrefetchAbort
        B       PrefetchAbort

;取数据中止
DataAbort
        B       DataAbort

;快速中断
FIQ_Handler
        STMFD   SP!, {R0-R3, LR}
        BL      FIQ_Exception
        LDMFD   SP!, {R0-R3, LR}
        SUBS    PC,  LR,  #4

;********************************************************************************************************
;** 功能描述: 初始化堆栈
InitStack    
        MOV     R0, LR

;设置管理模式堆栈
        MSR     CPSR_c, #0xd3
        LDR     SP, StackSvc
;设置中断模式堆栈
        MSR     CPSR_c, #0xd2
        LDR     SP, StackIrq
;设置快速中断模式堆栈
        MSR     CPSR_c, #0xd1
        LDR     SP, StackFiq
;设置中止模式堆栈
        MSR     CPSR_c, #0xd7
        LDR     SP, StackAbt
;设置未定义模式堆栈
        MSR     CPSR_c, #0xdb
        LDR     SP, StackUnd
;设置系统模式堆栈
        MSR     CPSR_c, #0xdf
        LDR     SP, =StackUsr

        MOV     PC, R0

;*********************************************************************************************************
;** 功能描述: 复位入口
ResetInit     
        BL      InitStack               ;初始化堆栈
        BL      TargetResetInit         ;目标板基本初始化                                       
        B       __main			;跳转到c语言入口

;*********************************************************************************************************
;** 功能描述: 库函数初始化堆和栈，不能删除
__user_initial_stackheap    
    LDR   r0,=bottom_of_heap
    MOV   pc,lr

;*********************************************************************************************************
;** 功能描述: 整数除法除数为0错误处理函数，替代原始的__rt_div0减少目标代码大小
__rt_div0
        B       __rt_div0

StackSvc           DCD     SvcStackSpace + (SVC_STACK_LEGTH - 1)* 4
StackIrq           DCD     IrqStackSpace + (IRQ_STACK_LEGTH - 1)* 4
StackFiq           DCD     FiqStackSpace + (FIQ_STACK_LEGTH - 1)* 4
StackAbt           DCD     AbtStackSpace + (ABT_STACK_LEGTH - 1)* 4
StackUnd           DCD     UndtStackSpace + (UND_STACK_LEGTH - 1)* 4

;*********************************************************************************************************
;** 功能描述: 支持芯片加密功能
    IF :DEF: EN_CRP
        IF  . >= 0x1fc
        INFO    1,"\nThe data at 0x000001fc must be 0x87654321.\nPlease delete some source before this line."
        ENDIF
CrpData
    WHILE . < 0x1fc
    NOP
    WEND
CrpData1
    DCD     0x87654321          ;/* 当此数为0x87654321时，用户程序被保护 */
    ENDIF

;/* 分配堆栈空间 */
        AREA    MyStacks, DATA, NOINIT, ALIGN=2
SvcStackSpace      SPACE   SVC_STACK_LEGTH * 4  ;管理模式堆栈空间
IrqStackSpace      SPACE   IRQ_STACK_LEGTH * 4  ;中断模式堆栈空间
FiqStackSpace      SPACE   FIQ_STACK_LEGTH * 4  ;快速中断模式堆栈空间
AbtStackSpace      SPACE   ABT_STACK_LEGTH * 4  ;中止义模式堆栈空间
UndtStackSpace     SPACE   UND_STACK_LEGTH * 4  ;未定义模式堆栈
    END

;********************************************************************************************************/
NoInt       EQU 0x80
USR32Mode   EQU 0x10
SVC32Mode   EQU 0x13
SYS32Mode   EQU 0x1f
IRQ32Mode   EQU 0x12
FIQ32Mode   EQU 0x11

    CODE32

    AREA    IRQ,CODE,READONLY

    MACRO
$IRQ_Label HANDLER $IRQ_Exception_Function

        EXPORT  $IRQ_Label                      ; 输出的标号
        IMPORT  $IRQ_Exception_Function         ; 引用的外部标号

$IRQ_Label
        SUB     LR, LR, #4                      ; 计算返回地址
        STMFD   SP!, {R0-R3, R12, LR}           ; 保存任务环境
        MRS     R3, SPSR                        ; 保存状态
        STMFD   SP!, {R3}
        STMFD   SP, {LR}^                       ; 保存用户状态的SP,注意不能回写
                                                ; 如果回写的是用户的SP，所以后面要调整SP
        SUB     SP, SP, #4

        MSR     CPSR_c, #(NoInt | SYS32Mode)    ; 切换到系统模式
       
        BL      $IRQ_Exception_Function         ; 调用c语言的中断处理程序

        MSR     CPSR_c, #(NoInt | IRQ32Mode)    ; 切换回irq模式
        LDMFD   SP, {LR}^                       ; 恢复用户状态的SP,注意不能回写
                                                ; 如果回写的是用户的SP，所以后面要调整SP
        ADD     SP, SP, #4                      ; 
        LDMFD   SP!, {R3}
        MSR     SPSR_cxsf, R3

        LDMFD   SP!, {R0-R3, R12, PC}^          ;
    MEND

;/* 以下添加中断句柄，用户根据实际情况改变 */

Timer0_Handler  HANDLER Timer0

    END
;
