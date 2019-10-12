; CH372, CH375
; Interface Program for MCS-51
; ʹ�����ù̼���USB�豸��ʽ
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.09
;
;****************************************************************************
; �������л���
;
;CH375_CS_			BIT   P2.7            ;MCS-51��Ƭ����A15����,����CH375Ƭѡ,���ض���
;CH375_A0			BIT   P2.0            ;MCS-51��Ƭ����A8����,����CH375��ַ0,���ض���,Ҳ�����ⲿ74373�����A0��ַ
;CH375_INT_			BIT   P3.2            ;MCS-51��Ƭ����INT0����,����CH375�ж�����,���趨��
;CH375_WR_			BIT   P3.6            ;MCS-51��Ƭ����RD����,����CH375дѡͨ,���趨��
;CH375_RD_			BIT   P3.7            ;MCS-51��Ƭ����WR����,����CH375��ѡͨ,���趨��
;
; ������������ʱ��Ƶ�ʲ�����24MHz�ı�׼MCS-51��Ƭ��, �������24MHz��Ӧ���ʵ�������ʱ, �������24MHz������ʵ�������ʱ
;
; ��Ҫ��������Ĳ���
CH375_CMD_PORT		EQU    0BDF1H         ;CH375����ڵĵ�ַ,��ַ������Զ�Ƭѡ
CH375_DAT_PORT		EQU    0BCF0H         ;CH375����ڵĵ�ַ,��ַ������Զ�Ƭѡ
;
; �����Լ���USB�豸�ĳ���VID��4348H, ��ƷPID��5537H
;USB_VENDOR_ID_L	EQU    48H            ;����VENDOR ID�ĵ��ֽ�
;USB_VENDOR_ID_H	EQU    43H            ;����VENDOR ID�ĸ��ֽ�
;USB_PRODUCT_ID_L	EQU    37H            ;����PRODUCT ID�ĵ��ֽ�
;USB_PRODUCT_ID_H	EQU    55H            ;����PRODUCT ID�ĸ��ֽ�
;
RECV_LEN			DATA   3FH            ;�ս��յ������ݵĳ���
RECV_BUFFER			DATA   40H            ;���ݻ�����,���ڱ�����յ����´�����,����Ϊ0��64�ֽ�
;SEND_LEN			DATA   3EH            ;׼���´η��͵����ݳ���
;SEND_BUFFER		DATA   80H            ;׼���´η��͵����ݻ�����
; ����MCS-51��Ƭ����ȡ�ⲿRAM�Ķ�д�ٶȵ����ڲ�RAM, ������Ҫ�õ�DPTR, ���Զ�д�ٶȽ���, �������Բο��������޸�
;
; ����λ��־
FLAG_SEND_WAIT		BIT    20H.1          ;���͵ȴ���־,1ָʾ�����ݿ�����CH375�еȴ�����
FLAG_INT_WAIT		BIT    20H.2          ;�ж����ݵȴ����ͱ�־,1ָʾ���ж���������CH375�еȴ�����
;
;
$INCLUDE			(CH375INC.ASM)
;
;****************************************************************************
;
;����ӿ��ӳ���,�����ο�
;
;  1. CH375_INIT     ��ʼ���ӳ���
;       USE:   ACC, R7, DPTR
;
;  2. CH375_UPLOAD   �ϴ����ݿ��ӳ���(ͨ�������˵��ϴ�)
;       ENTRY: R0 ָ������׼���ϴ����ݵĻ�����, R7 ׼���ϴ������ݳ���, ��Ч��ֵ��0��64
;       USE:   ACC, R0, R7, DPTR
;
;  3. CH375_UP_INT   �ϴ��ж������ӳ���(ͨ���ж϶˵��ϴ�),��ѡ�ӳ���
;       ENTRY: R0 ָ������׼���ϴ����ݵĻ�����, R7 ׼���ϴ������ݳ���, ��Ч��ֵ��0��8
;       USE:   ACC, R0, R7, DPTR
;
;  4. CH375_INTER    �жϷ����ӳ���
;       USE:   ��ջ8�ֽ�
;
;****************************************************************************
;�����������
;				ORG   0000H					;��λ��Ƭ�����
;				LJMP  START
;				ORG   0003H					;CH375�ж�
;				LJMP  CH375_INTER
;
;START:			NOP
;				...
;				CALL  CH375_INIT			;��ʼ��
;				...
;				SETB  EA					;�����ж�
;				...
;WAIT_REQUEST:	NOP
;				SJMP  WAIT_REQUEST			;��ѭ��
;
;****************************************************************************
;
; ��ʼ���ӳ���
; USE:   ACC, R7, DPTR
CH375_INIT:		CLR   FLAG_SEND_WAIT		;�巢�͵ȴ���־,1ָʾ�����ݿ�����CH375�еȴ�����
				CLR   FLAG_INT_WAIT			;���ж����ݷ��͵ȴ���־,1ָʾ���ж���������CH375�еȴ�����
; ����CH375�Ƿ���������,��ѡ����
;				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
;				MOV   A,#CMD_CHECK_EXIST
;				MOVX  @DPTR,A				;����CH375�Ƿ���������,��ѡ����
;				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
;				DEC   DPL					;���ݿڵ�ַ
;				MOV   A,#55H
;				MOVX  @DPTR,A				;д���������
;				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
;				CPL   A
;				MOV   R7,A					;ȡ������
;				MOVX  A,@DPTR				;��������Ӧ���ǲ�������ȡ��
;				XRL   A,R7
;				JZ    CH375_INIT_OK			;����ͨ��
;				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
;				MOV   A,#CMD_RESET_ALL
;				MOV   R7,#50H				;����ظ�������
;CH375_INIT_HR:	MOVX  @DPTR,A				;ִ��Ӳ����λ
;				DJNZ  R7,CH375_INIT_HR
;				MOV   R7,#0A0H				;160*256uS=40mS
;CH375_INIT_D2:	CLR   A
;CH375_INIT_D1:	DJNZ  ACC,CH375_INIT_D1		;��ʱ1uS
;				DJNZ  R7,CH375_INIT_D2		;��ʱ256uS
;				SJMP  CH375_INIT			;�ٴβ���
;CH375_INIT_OK:
; �����ⲿ�Զ����USB�豸VID��PID,��ѡ����,��ִ�и�������ʹ��Ĭ�ϵ�VID��PID
;				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
;				MOV   A,#CMD_SET_USB_ID
;				MOVX  @DPTR,A				;�����ⲿ�Զ����USB�豸VID��PID,��ѡ����
;				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
;				DEC   DPL					;���ݿڵ�ַ
;				MOV   A,#USB_VENDOR_ID_L
;				MOVX  @DPTR,A				;д�볧��ID�ĵ��ֽ�
;				MOV   A,#USB_VENDOR_ID_H
;				MOVX  @DPTR,A				;д�볧��ID�ĸ��ֽ�
;				MOV   A,#USB_PRODUCT_ID_L
;				MOVX  @DPTR,A				;д���ƷID�ĵ��ֽ�
;				MOV   A,#USB_PRODUCT_ID_H
;				MOVX  @DPTR,A				;д���ƷID�ĸ��ֽ�
;				NOP
; ����USB����ģʽ
				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_SET_USB_MODE
				MOVX  @DPTR,A				;����USB����ģʽ
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				DEC   DPL					;���ݿڵ�ַ
				MOV   A,#02H
				MOVX  @DPTR,A				;����Ϊʹ�����ù̼���USB�豸��ʽ
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
CH375_INIT_WT:	MOVX  A,@DPTR				;���ز���״̬
				XRL   A,#CMD_RET_SUCCESS
				JNZ   CH375_INIT_WT			;�ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS
; ��������ָ�����������ж�
				CLR   IT0					;���ⲿ�ź�Ϊ�͵�ƽ����
				SETB  PX0					;�ø����ȼ�
				CLR   IE0					;���жϱ�־
				SETB  EX0					;����CH375�ж�
				RET
;
; �ϴ����ݿ��ӳ���(ͨ�������˵��ϴ�)
; ENTRY: R0 ָ������׼���ϴ����ݵĻ�����, R7 ׼���ϴ������ݳ���, ��Ч��ֵ��0��64
; USE:   ACC, R0, R7, DPTR
CH375_UPLOAD:	NOP							;Ϊ�˷�ֹ���������û�ж�ȡ���ݶ����µ�Ƭ��һֱ�ڴ˵ȴ�,���Լ��㳬ʱ,�ο�CH375_UP_INT�ӳ���
				JB    FLAG_SEND_WAIT,CH375_UPLOAD	;������ڵȴ�����(ǰ�η�����δ���)������ȴ�,��������Ӧ��ȡ�����ݵ�ʱ��δȡ���������϶�Ϊ��ʱ,���������ȴ�
				CLR   EX0					;Ϊ�˷�ֹ��;���ж϶�����˳��,�����Ƚ�ֹ�ж�
				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_WR_USB_DATA7
				MOVX  @DPTR,A				;��USB�˵�2�ķ��ͻ�����д�����ݿ�
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				DEC   DPL					;���ݿڵ�ַ
				MOV   A,R7
				MOVX  @DPTR,A				;����д��������ݳ���
CH375_UPLOAD_1:	MOV   A,@R0
				INC   R0
				MOVX  @DPTR,A				;��������
				DJNZ  R7,CH375_UPLOAD_1		;������������ֱ������
				SETB  FLAG_SEND_WAIT		;�õȴ����ͱ�־
				SETB  EX0					;�����ж�
				RET
;
; �ϴ��ж������ӳ���(ͨ���ж϶˵��ϴ�),��ѡ�ӳ���
; ENTRY: R0 ָ������׼���ϴ����ݵĻ�����, R7 ׼���ϴ������ݳ���, ��Ч��ֵ��0��8
; USE:   ACC, R0, R7, DPTR
CH375_UP_INT:	MOV   A,#0FFH
CH375_UP_INT_W:	JNB   FLAG_INT_WAIT,CH375_UP_INT_G	;ǰ���ж������Ѿ���PC��ȡ�������
				NOP							;�ȴ������ж�����
				NOP
				NOP							;���ȴ�2����,��12MHzʱ�Ӽ���
				NOP
				DJNZ  ACC,CH375_UP_INT_W	;�����ȴ�,��2�����ʱ,���齫��ʱ����Ϊ�ϴ��ֵ,���������������Ӧ�ñ�ȡ�ߵ�ʱ���2��
; ���δ����������,���߼�����е�Ӧ�ó���δ����,�����ֳ�ʱ,���򲻳���1�����ж����ݾ�Ӧ�ñ�PC��ȡ��
CH375_UP_INT_G:	CLR   EX0					;Ϊ�˷�ֹ��;���ж϶�����˳��,�����Ƚ�ֹ�ж�
				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_WR_USB_DATA5
				MOVX  @DPTR,A				;��USB�˵�1�ķ��ͻ�����д�����ݿ�
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				DEC   DPL					;���ݿڵ�ַ
				MOV   A,R7
				MOVX  @DPTR,A				;����д��������ݳ���
CH375_UP_INT_1:	MOV   A,@R0
				INC   R0
				MOVX  @DPTR,A				;�����ж�����
				DJNZ  R7,CH375_UP_INT_1		;������������ֱ������
				SETB  FLAG_INT_WAIT			;���жϵȴ���־
				SETB  EX0					;�����ж�
				RET
;
; �жϷ����ӳ���
; USE:   ��ջ8�ֽ�
CH375_INTER:	PUSH  PSW					;�ֳ�����
				PUSH  ACC
				PUSH  DPL
				PUSH  DPH
				PUSH  01H					;R1
				PUSH  02H					;R2
				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_GET_STATUS
				MOVX  @DPTR,A				;��ȡ�ж�״̬��ȡ���ж�����
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				MOV   DPTR,#CH375_DAT_PORT	;���ݿڵ�ַ
				MOVX  A,@DPTR				;���ز���״̬
				CLR   IE0					;���жϱ�־,��Ӧ��INT0�ж�
				CJNE  A,#USB_INT_EP2_OUT,CH375_INT_1	;�����˵��´��ɹ�,���յ�����
				LJMP  CH375_DOWN_OK			;USB�������ݽ��ճɹ�,USB�˵�2��OUT
CH375_INT_1:	CJNE  A,#USB_INT_EP2_IN,CH375_INT_2	;�����˵��ϴ��ɹ�,�������ݳɹ�
				LJMP  CH375_UP_OK			;USB�������ݷ��ͳɹ�,USB�˵�2��IN
CH375_INT_2:	CJNE  A,#USB_INT_EP1_IN,CH375_INT_3	;�ж϶˵��ϴ��ɹ�,�����ж����ݳɹ�
				LJMP  CH375_UP_INT_OK		;USB�ж����ݷ��ͳɹ�,USB�˵�1��IN
CH375_INT_3:	CJNE  A,#USB_INT_EP1_OUT,CH375_INT_4	;�����˵��´��ɹ�,���յ���������
				LJMP  CH375_DN_AUX_OK		;USB�������ݽ��ճɹ�,USB�˵�1��OUT
CH375_INT_4:	;�����Ҫ��USB���߹���ʱʹCH375����͹���ģʽ,��ô��Ӧ�ô���USB_INT_USB_SUSPEND��USB_INT_WAKE_UP�ж�״̬
				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;�ͷŵ�ǰUSB������
				SJMP  CH375_INT_RET
CH375_INT_RET:	POP   02H					;R2
				POP   01H					;R1
				POP   DPH
				POP   DPL
				POP   ACC
				POP   PSW					;�ָ��Ĵ���
				RETI						;�жϷ���
; USB�������ݽ��ճɹ�
CH375_DOWN_OK:	MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_RD_USB_DATA
				MOVX  @DPTR,A				;�ӵ�ǰUSB�жϵĶ˵㻺������ȡ���ݿ�,���ͷŻ�����
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				MOV   DPTR,#CH375_DAT_PORT	;���ݿڵ�ַ
				MOVX  A,@DPTR				;���ȶ�ȡ�������ݳ���
				MOV   R2,A
				JZ    CH375_INT_RET			;����Ϊ0,û��������ֱ���˳�
				MOV   RECV_LEN,A
				MOV   R1,#RECV_BUFFER		;���ջ�����
CH375_INT_RECV:	MOVX  A,@DPTR				;��������
				MOV   @R1,A
				INC   R1
				DJNZ  R2,CH375_INT_RECV		;������������ֱ������
;				CALL  PROCESS				;�����´����ݲ�׼��Ӧ��
				LJMP  CH375_INT_RET
; USB�������ݷ��ͳɹ�
CH375_UP_OK:	NOP
;				MOV   A,SEND_LEN
;				JZ    CH375_INT_UNLK		;ʣ�೤��Ϊ0,û��������Ҫ����,�������˳�
;				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
;				MOV   A,#CMD_WR_USB_DATA7
;				MOVX  @DPTR,A				;��USB�˵�2�ķ��ͻ�����д�����ݿ�
;				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
;				MOV   DPTR,#CH375_DAT_PORT	;���ݿڵ�ַ
;				MOV   A,SEND_LEN
;				MOVX  @DPTR,A				;����д��������ݳ���
;				MOV   R1,#SEND_BUFFER		;���ͻ�����
;CH375_INT_SEND:	MOV   A,@R1
;				MOVX  @DPTR,A				;��������
;				INC   R1
;				DJNZ  SEND_LEN,CH375_INT_SEND	;������������ֱ������
CH375_INT_UNLK:	MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;�ͷŵ�ǰUSB������
				CLR   FLAG_SEND_WAIT		;�������
				LJMP  CH375_INT_RET
; USB�ж����ݷ��ͳɹ�, �ж϶˵�������ڵ�Ƭ����ʱ֪ͨ�������,������δ�õ�
CH375_UP_INT_OK:
				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;�ͷŵ�ǰUSB������
				CLR   FLAG_INT_WAIT			;�ж������Ѿ����������
				LJMP  CH375_INT_RET
; USB�������ݽ��ճɹ�,�����˵�������ڼ��������Ƭ���˷��Ͱ�,������δ�õ�,�������ݵĽ��տ��Բο������˵�
CH375_DN_AUX_OK:
				MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOV   A,#CMD_UNLOCK_USB
				MOVX  @DPTR,A				;�ͷŵ�ǰUSB������
				LJMP  CH375_INT_RET
;
;****************************************************************************
;