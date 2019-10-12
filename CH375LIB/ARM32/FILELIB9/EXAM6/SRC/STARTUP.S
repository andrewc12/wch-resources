;���³����ѡ��LPC2114�����׼� 2004.07
;
;**��        ��: lpc21xx���������룬�����쳣������ڡ���ʼ����ջ�Ĵ����

;�����ջ�Ĵ�С
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


;������ⲿ�����������
    IMPORT  FIQ_Exception                   ;�����ж��쳣��������
    IMPORT  __main                          ;C������������� 
    IMPORT  TargetResetInit                 ;Ŀ��������ʼ��
    IMPORT  StackUsr
    IMPORT  bottom_of_heap

;���ⲿʹ�õı����������
    EXPORT  Reset
    EXPORT __rt_div0
    EXPORT __user_initial_stackheap

    CODE32

    AREA    vectors,CODE,READONLY
        ENTRY
;�ж�������
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

;δ����ָ��
Undefined
        B       Undefined

;���ж�
SoftwareInterrupt
        B       SoftwareInterrupt

;ȡָ����ֹ
PrefetchAbort
        B       PrefetchAbort

;ȡ������ֹ
DataAbort
        B       DataAbort

;�����ж�
FIQ_Handler
        STMFD   SP!, {R0-R3, LR}
        BL      FIQ_Exception
        LDMFD   SP!, {R0-R3, LR}
        SUBS    PC,  LR,  #4

;********************************************************************************************************
;** ��������: ��ʼ����ջ
InitStack    
        MOV     R0, LR

;���ù���ģʽ��ջ
        MSR     CPSR_c, #0xd3
        LDR     SP, StackSvc
;�����ж�ģʽ��ջ
        MSR     CPSR_c, #0xd2
        LDR     SP, StackIrq
;���ÿ����ж�ģʽ��ջ
        MSR     CPSR_c, #0xd1
        LDR     SP, StackFiq
;������ֹģʽ��ջ
        MSR     CPSR_c, #0xd7
        LDR     SP, StackAbt
;����δ����ģʽ��ջ
        MSR     CPSR_c, #0xdb
        LDR     SP, StackUnd
;����ϵͳģʽ��ջ
        MSR     CPSR_c, #0xdf
        LDR     SP, =StackUsr

        MOV     PC, R0

;*********************************************************************************************************
;** ��������: ��λ���
ResetInit     
        BL      InitStack               ;��ʼ����ջ
        BL      TargetResetInit         ;Ŀ��������ʼ��                                       
        B       __main			;��ת��c�������

;*********************************************************************************************************
;** ��������: �⺯����ʼ���Ѻ�ջ������ɾ��
__user_initial_stackheap    
    LDR   r0,=bottom_of_heap
    MOV   pc,lr

;*********************************************************************************************************
;** ��������: ������������Ϊ0���������������ԭʼ��__rt_div0����Ŀ������С
__rt_div0
        B       __rt_div0

StackSvc           DCD     SvcStackSpace + (SVC_STACK_LEGTH - 1)* 4
StackIrq           DCD     IrqStackSpace + (IRQ_STACK_LEGTH - 1)* 4
StackFiq           DCD     FiqStackSpace + (FIQ_STACK_LEGTH - 1)* 4
StackAbt           DCD     AbtStackSpace + (ABT_STACK_LEGTH - 1)* 4
StackUnd           DCD     UndtStackSpace + (UND_STACK_LEGTH - 1)* 4

;*********************************************************************************************************
;** ��������: ֧��оƬ���ܹ���
    IF :DEF: EN_CRP
        IF  . >= 0x1fc
        INFO    1,"\nThe data at 0x000001fc must be 0x87654321.\nPlease delete some source before this line."
        ENDIF
CrpData
    WHILE . < 0x1fc
    NOP
    WEND
CrpData1
    DCD     0x87654321          ;/* ������Ϊ0x87654321ʱ���û����򱻱��� */
    ENDIF

;/* �����ջ�ռ� */
        AREA    MyStacks, DATA, NOINIT, ALIGN=2
SvcStackSpace      SPACE   SVC_STACK_LEGTH * 4  ;����ģʽ��ջ�ռ�
IrqStackSpace      SPACE   IRQ_STACK_LEGTH * 4  ;�ж�ģʽ��ջ�ռ�
FiqStackSpace      SPACE   FIQ_STACK_LEGTH * 4  ;�����ж�ģʽ��ջ�ռ�
AbtStackSpace      SPACE   ABT_STACK_LEGTH * 4  ;��ֹ��ģʽ��ջ�ռ�
UndtStackSpace     SPACE   UND_STACK_LEGTH * 4  ;δ����ģʽ��ջ
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

        EXPORT  $IRQ_Label                      ; ����ı��
        IMPORT  $IRQ_Exception_Function         ; ���õ��ⲿ���

$IRQ_Label
        SUB     LR, LR, #4                      ; ���㷵�ص�ַ
        STMFD   SP!, {R0-R3, R12, LR}           ; �������񻷾�
        MRS     R3, SPSR                        ; ����״̬
        STMFD   SP!, {R3}
        STMFD   SP, {LR}^                       ; �����û�״̬��SP,ע�ⲻ�ܻ�д
                                                ; �����д�����û���SP�����Ժ���Ҫ����SP
        SUB     SP, SP, #4

        MSR     CPSR_c, #(NoInt | SYS32Mode)    ; �л���ϵͳģʽ
       
        BL      $IRQ_Exception_Function         ; ����c���Ե��жϴ�������

        MSR     CPSR_c, #(NoInt | IRQ32Mode)    ; �л���irqģʽ
        LDMFD   SP, {LR}^                       ; �ָ��û�״̬��SP,ע�ⲻ�ܻ�д
                                                ; �����д�����û���SP�����Ժ���Ҫ����SP
        ADD     SP, SP, #4                      ; 
        LDMFD   SP!, {R3}
        MSR     SPSR_cxsf, R3

        LDMFD   SP!, {R0-R3, R12, PC}^          ;
    MEND

;/* ���������жϾ�����û�����ʵ������ı� */

Timer0_Handler  HANDLER Timer0

    END
;