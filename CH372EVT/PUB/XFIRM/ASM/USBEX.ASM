;CH372/CH375 USB device mode & external firmware
;by wch       2004.10,  2005.03
;$INCLUDE                 	 (reg52.h)
$INCLUDE                 	 (CH375INC.ASM)

STACK           EQU    50H             ;��ջ��ջ��(��������)
;
; ��Ҫ��������Ĳ���
CH375_CMD_PORT		EQU    0BDF1H         ;CH375����ڵĵ�ַ,��ַ������Զ�Ƭѡ
CH375_DAT_PORT		EQU    0BCF0H         ;CH375����ڵĵ�ַ,��ַ������Զ�Ƭѡ
;

CH375_CON_ACT		BIT    P1.4

CMD_BUFFER			DATA  38H				;�����������,������Լ�����������ΪCONST_CMD_LEN

VAR_SETUP_REQUEST	EQU   72H							;USB������
VAR_SETUP_LENGTH	EQU   73H							;�������ݳ���
VAR_SETUP_DESCR		EQU   74H							;������ƫ�Ƶ�ַ
VAR_SN_STRING		EQU   75H							;��Ʒ���к��ַ���������,ANSI�ַ���
VAR_USB_ADDRESS		EQU   76H
	
; ����λ��־
CH375_CON_FLAG  BIT    2FH.0	      ;375оƬ������ɱ�־
;
;****************************************************************************
;������
				ORG   0000H            ;��λ��Ƭ�����
				LJMP  START
				ORG   0003H            ;CH375�ж�
				LJMP  CH375_INTER

START:
; ���³�ʼ��ָ��Ǳ�Ҫ��,����Ƭ���ָ�ΪĬ��״̬
				CLR   EA               ;���ж�
				MOV   SP,#STACK        ;�ö�ջ��ֵ
				MOV   A,#0FFH
				MOV   P0,A
				MOV   P1,A             ;��˿�״̬
				MOV   P2,A
				MOV   P3,A
				CLR   A
				MOV   IE,A             ;���ж��������ƼĴ���
				MOV   IP,A             ;���ж����ȼ����ƼĴ���
				MOV   PSW,A            ;�����״̬��
				MOV   TCON,A           ;�嶨ʱ��/���������ƼĴ���
				MOV   TMOD,A           ;�嶨ʱ��/������������ʽ�Ĵ���
				MOV   PCON,A           ;���Դ���ƼĴ���
				MOV   R7,#0FFH
				CALL  DELAY_MS			;��ʱ�ȴ�CH375��ʼ�����
; ��ʼ��
				CALL  CH375_INIT       ;��ʼ��
				setb  EA
				ljmp  $	;������
;
; ����������CH375
; ����: ACC Ϊ������
; ���: DPTR Ϊ���ݿ�
; USE: ACC, DPTR
WR_CMD_TO_375:	MOV   DPTR,#CH375_CMD_PORT	;����ڵ�ַ
				MOVX  @DPTR,A				;����USB����ģʽ
				NOP							;���ʱ��Ƶ�ʵ���20MHz�������ָ����ʱ,����30MHzҪ���2��ָ��
				MOV   DPTR,#CH375_DAT_PORT	;���ݿڵ�ַ
				RET
;
; ��ʼ���ӳ���
; USE:   ACC, R7, DPTR
CH375_INIT:		MOV   A,#CMD_SET_USB_MODE	;����USB����ģʽ
				CALL  WR_CMD_TO_375			;���������
;				MOV   A,#02H				;����Ϊ���ù̼�ģʽ
				MOV   A,#01H				;����Ϊ���ù̼�ģʽ
				MOVX  @DPTR,A				;����Ϊʹ�����ù̼���USB�豸��ʽ
				NOP							;���ʱ��Ƶ�ʵ���16MHz�������ָ����ʱ
				NOP
CH375_INIT_WT:	MOVX  A,@DPTR				;���ز���״̬,Ҳ���Եȴ�20uS
				XRL   A,#CMD_RET_SUCCESS
				JNZ   CH375_INIT_WT			;�ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS
; ��������ָ�����������ж�
				CLR   IT0					;���ⲿ�ź�Ϊ�͵�ƽ����
				SETB  PX0					;�ø����ȼ�
				CLR   IE0					;���жϱ�־
				SETB  EX0					;����CH375�ж�
				RET
;**************************************************************************************
CH375_EP2_DOWN:	MOV A,#CMD_RD_USB_DATA		;���´������ݶ������ͷŶ˵�
				CALL  WR_CMD_TO_375			;�������CH375
				MOVX A,@DPTR				;ȡ���������ݳ���
				MOV R4,A				;���泤��
				JZ   CH375_EP2_UP			;����Ϊ0����
				MOV R5,A
				MOV  R0,#CMD_BUFFER			;�������׵�ַ��R0
CH375_EP2_LOOP:			MOVX A,@DPTR
				MOV @R0,A
				INC R0	
				DJNZ R5,CH375_EP2_LOOP
; ��ʾ�ش�
CH375_EP2_UP:	MOV  A,#CMD_WR_USB_DATA7	;��ʾ�ش�����
				CALL  WR_CMD_TO_375
				MOV  A,R4
				MOVX @DPTR,A
				JZ   CH375_EP2_UP_RET		;����Ϊ0
				MOV  R0,#CMD_BUFFER			;�������׵�ַ��R0
CH375_EP2_UP_LOOP:	MOV  A,@R0
				MOVX @DPTR,A
				INC  R0
				DJNZ  R4,CH375_EP2_UP_LOOP
CH375_EP2_UP_RET:	RET
;***************************************************************************************							
				
; �жϷ����ӳ���
; USE:   ��ջ6�ֽ�, �����Ĵ�����1��R0,R5,R6,R7
CH375_INTER:	PUSH  PSW					;�ֳ�����
				PUSH  ACC
				PUSH  DPL
				PUSH  	DPH
				SETB  	RS0                   		;PSW.3,�л��������Ĵ�����1
				MOV   A,#CMD_GET_STATUS
				CALL  WR_CMD_TO_375			;��ȡ�ж�״̬��ȡ���ж�����
				MOVX 	 A,@DPTR				;���ز���״̬
				ANL 	 A,#0FH				;���θ�4λ
				CLR   	IE0					;���жϱ�־,��Ӧ��INT0�ж�
;�����ж�״̬���з�������
				CJNE  A,#USB_INT_EP2_OUT,CH375_INT_06	;�����˵��´��ɹ�
				LCALL CH375_EP2_DOWN		;�����´�������
				LJMP  CH375_INT_RET			; USB�������ݽ��ճɹ�
CH375_INT_06:	CJNE  A,#USB_INT_EP2_IN,CH375_INT_07	;�����˵��ϴ��ɹ�
				;USB�������ݷ��ͳɹ�
				LJMP CH375_INT_ULOCK			;�������жϷ���
CH375_INT_07:
				CJNE  	A,#USB_INT_EP0_SETUP,CH375_INT_01
				LJMP  	CH375_SETUP_OK			;USB���ƴ��佨������
CH375_INT_01:	CJNE  	A,#USB_INT_EP0_OUT,CH375_INT_02
				LJMP 	CH375_INT_ULOCK		;�˵�0�´�,�������жϷ���
CH375_INT_02:	CJNE  	A,#USB_INT_EP0_IN,CH375_INT_03
									;�˵�0�����ϴ�
				MOV 	A,VAR_SETUP_REQUEST
				CJNE 	A,#DEF_USB_GET_DESCR,CH375_EP0_IN_01
									;�������ϴ�
				LCALL 	 CH375_DES_UPLOAD		;�����������ϴ�
				LJMP     CH375_INT_ULOCK	;�������жϷ���

CH375_EP0_IN_01:	CJNE 	A,#DEF_USB_SET_ADDRESS,CH375_EP0_IN_02
				MOV	    A,#CMD_SET_USB_ADDR		;���õ�ַ,�Ա���һ���������µĵ�ַ
				CALL  WR_CMD_TO_375
				MOV    A,VAR_USB_ADDRESS		;ȡ����ַ
				MOVX	@DPTR,A
				LJMP    CH375_INT_ULOCK
CH375_EP0_IN_02:	LJMP    CH375_INT_ULOCK	;�������жϷ���
CH375_INT_03:	CJNE 	 A,#USB_INT_EP1_IN,CH375_INT_04
									;�˵�1�����ϴ�,���ڴ˼��ϴ�������
				LJMP 	CH375_INT_ULOCK		;�������жϷ���

CH375_INT_04:	CJNE  A,#USB_INT_EP1_OUT,CH375_INT_05
				 					;�˵�1�����´�
				MOV A,#CMD_RD_USB_DATA		;���´������ݶ������ͷŶ˵�
				CALL  WR_CMD_TO_375			;�������CH375
				MOVX A,@DPTR				;
				JZ   CH375_INT_RET			;����Ϊ0����
				MOV  R5,A
CH375_USB_OUT1_LOOP:		MOVX A,@DPTR				;���������ͷŻ�����
				NOP							;���浽��������
				DJNZ  R5,CH375_USB_OUT1_LOOP		;����δ�������
				LJMP 	CH375_INT_RET		;�жϷ���
CH375_INT_05:	ANL  A,#03H
				CJNE A,#03H,CH375_INT_09
				LJMP  CH375_USB_REST			;��ת��USB��λ
CH375_INT_09:	SJMP  CH375_INT_ULOCK		;���֧�ִ��󷵻�

CH375_INT_ULOCK:	;�������жϷ���
				MOV 	A,#CMD_UNLOCK_USB			;�����˵�
				CALL  WR_CMD_TO_375
CH375_INT_RET:	POP   DPH
				POP   DPL
				POP   ACC
				POP   PSW					;�ָ��Ĵ���
				RETI

CH375_USB_REST:	;USB���߸�λ
				SETB CH375_CON_ACT				;������ֵ�ȣ�CH372 ���Զ���USB ��ַ��ͬ��������־
				CLR CH375_CON_FLAG				;�嵥Ƭ���и��ֱ�־	
				MOV VAR_SETUP_REQUEST,#0FFH
				LJMP CH375_INT_ULOCK
;*****************************************************************************************
;***********************************USB SETUP***********************************************
CH375_SETUP_RET_ERR:
				MOV A,#CMD_SET_ENDP3			;���ö˵�1
				CALL  WR_CMD_TO_375
				MOV A,#0FH			;����״̬ΪSTALL
				MOVX @DPTR,A
				LJMP CH375_INT_RET			;����Ŀ��ƴ���

CH375_SETUP_RET:
				LJMP CH375_INT_RET			;��ת���жϷ���
CH375_SETUP_OK:	MOV  A,#CMD_RD_USB_DATA			;
				CALL  WR_CMD_TO_375			;��������������
				MOVX A,@DPTR				;��������
				MOV  R6,A
				MOV  R7,A				;���ȷ���R7
				MOV  R0,#CMD_BUFFER			;��������׵�ַ������Ѱַ
CH375_SETUP_REC_CMD:		MOVX A,@DPTR				;��������
				MOV @R0,A				;���ݷ����������
				INC R0
				DJNZ  R7,CH375_SETUP_REC_CMD  		;���Ȳ�Ϊ0����
				CJNE R6,#8H,CH375_INT_RET		;���Ȳ�Ϊ8������������
				MOV  R0,#CMD_BUFFER
				MOV  A,CMD_BUFFER+6			;ȡ�����ƴ���ĳ���
				ANL  A,#7FH				;���Ȳ�����128��
				MOV VAR_SETUP_LENGTH,A			;�洢����
				MOV A,CMD_BUFFER			;�ж�USB����
				JB  ACC.6,CH375_USB_REQ_FACT		;USB��������
				JB  ACC.5,CH375_USB_REQ_CLASS		;��ת��USB������
				ANL  A,#0E0H
				CJNE A,#80H,CH375_REQUEST_OTHER1
				LJMP  CH375_REQUEST_STANDTRAD		;USB��׼����
CH375_REQUEST_OTHER1:		CJNE A,#00H,CH375_REQUEST_OTHER
				LJMP  CH375_REQUEST_STANDTRAD		;USB��׼����
CH375_REQUEST_OTHER:		LJMP  CH375_SETUP_RET_ERR		;�������֧��
CH375_USB_REQ_FACT:		LJMP 	CH375_SETUP_RET			;��������
CH375_USB_REQ_CLASS:	LJMP 	CH375_SETUP_RET			;
;***************************************************************************************
;******************************USB STANDTRAD  REQUEST   STAR  **************************

;*************************************************************************************
;SELECT REQUEST
CH375_REQUEST_STANDTRAD:	MOV A,CMD_BUFFER+1			;USB��׼����
				MOV VAR_SETUP_REQUEST,A			;��������
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
;USB GET STATUS   �õ�״̬
CH375_USB_GET_STATUS:		MOV 	@R0,#00H
				INC 	R0
				MOV    @R0,#00H
				LCALL    CH375_EP0_UPLOAD		;���ö˵�0�ϴ�
				LJMP   	 CH375_SETUP_RET		;�жϷ���
CH375_SETUP_RET_ERR_1:		LJMP    CH375_SETUP_RET_ERR
;************************************************************************************************
;USB  CLR  FEATURE  �������
CH375_USB_CLR_FEATURE:         	MOV   	A,CMD_BUFFER		;ȡ������
				ANL  	A,#1FH
				CJNE  	A,#02H,CH375_SETUP_RET_ERR_1	;���Ƕ˵���֧��
				MOV  	A,CMD_BUFFER+4

				CJNE    A,#82H,CH375_USB_CLR_FEA_0
											;�˵�2�ϴ�
				MOV     A,#CMD_SET_ENDP7				;����˵�2�ϴ�
				CALL  WR_CMD_TO_375
				MOV     A,#8EH
				MOVX 	@DPTR,A
				LCALL CH375_EP0_IN_ZERO		;�ϴ�0��������Ϣ��
				LJMP   CH375_SETUP_RET
						;
CH375_USB_CLR_FEA_0:		CJNE    A,#02H,CH375_USB_CLR_FEA_1
				MOV     A,#CMD_SET_ENDP6	;����˵�2�´�
				CALL  WR_CMD_TO_375
				MOV     A,#80H
				MOVX @DPTR,A
				LCALL CH375_EP0_IN_ZERO		;�ϴ�0��������Ϣ��							;�˵�2�´�
				LJMP   CH375_SETUP_RET

CH375_USB_CLR_FEA_1:				CJNE    A,#81H,CH375_USB_CLR_FEA_2
				MOV     A,#CMD_SET_ENDP5	;����˵�1�ϴ�
				CALL  WR_CMD_TO_375
				MOV     A,#8EH							;�˵�1�ϴ�
				MOVX @DPTR,A
				LCALL CH375_EP0_IN_ZERO		;�ϴ�0��������Ϣ��
				LJMP   CH375_SETUP_RET

CH375_USB_CLR_FEA_2:				CJNE    A,#01H,CH375_USB_CLR_FEA_3
				MOV     A,#CMD_SET_ENDP4	;����˵�1�´�
				CALL  WR_CMD_TO_375
				MOV     A,#80H							;�˵�1�´�
				MOVX @DPTR,A
				LCALL CH375_EP0_IN_ZERO		;�ϴ�0��������Ϣ��
				LJMP	 CH375_SETUP_RET
CH375_USB_CLR_FEA_3:
				LJMP   CH375_SETUP_RET_ERR	;;��֧��
;****************************************************************************************
;USB  AET ADDRESS
CH375_USB_SET_ADDRES:		MOV A,CMD_BUFFER+2			;ȡ����ַ
				MOV VAR_USB_ADDRESS,A			;�ݴ��ַ
				LCALL CH375_EP0_IN_ZERO		;�ϴ�0��������Ϣ��
				LJMP CH375_SETUP_RET			;���˳���������״̬����ܷ��ı�

;*****************************************************************************************
;USB  GET  DESCR ���������
CH375_USB_GET_DESCR: 		MOV  A,CMD_BUFFER+3
				CJNE A,#1H,CH375_GET_DESCR_1	;��ȡ�豸������
				MOV  A,#00H
				MOV  VAR_SETUP_DESCR,A		;������ƫ����=0
				LCALL CH375_DES_UPLOAD
				LJMP CH375_SETUP_RET
CH375_GET_DESCR_1:
				CJNE A,#2H,CH375_GET_DESCR_2
								;��ȡ����������
				MOV  A,#USB_CFG_DESCR_TAB-USB_DEV_DESCR_TAB
				MOV  VAR_SETUP_DESCR,A		;������ƫ����
				LCALL CH375_DES_UPLOAD
				LJMP CH375_SETUP_RET
CH375_GET_DESCR_2:
CH375_GET_DESCR_3: 		LJMP   CH375_SETUP_RET_ERR	;��֧���˳�

;*******************************************************************************************
;USB  GET CONFIG
CH375_USB_GET_CONFIG:		MOV  @R1,#01H			;����ֵΪ01H������������ָ����
				JB   CH375_CON_FLAG,CH375_CON_CON
				MOV  @R1,#00H			;CH375û�����÷���00
CH375_CON_CON:			LCALL CH375_EP0_UPLOAD		;���ö˵�0�ϴ�
				LJMP  CH375_SETUP_RET

;********************************************************************************************
;USB SET  CONFIG
CH375_USB_SET_CONFIG:	MOV  A,CMD_BUFFER+2
				CLR  CH375_CON_FLAG		;ȡ������
				SETB  CH375_CON_ACT
				JZ    CH375_USB_SET_CFG1
				SETB  CH375_CON_FLAG		;��������
				CLR   CH375_CON_ACT		;����͵�ƽָʾ��ɱ�־
CH375_USB_SET_CFG1:
				LCALL CH375_EP0_IN_ZERO		;�ϴ�0��������Ϣ��
				LJMP  CH375_SETUP_RET
;*********************************************************************************************
;USB  GET INTER
CH375_USB_GET_INTERF:	  MOV   @R1,#01H			;����һ���ӿ�
				LCALL CH375_EP0_UPLOAD		;��Ϊ����������ָ��һ���ӿ�
				LJMP  CH375_SETUP_RET
;*********************************************************************************************
CH375_EP0_UPLOAD:		MOV  A,VAR_SETUP_LENGTH		;ȡ������
				MOV R5,#08H
				CLR C
				SUBB A,R5
				MOV R5,VAR_SETUP_LENGTH		;�ϴ�����С��8
				JC  CH375_EP0_UP_LEN
				MOV  VAR_SETUP_LENGTH,A		;���¸�������
				MOV R5,#08H
CH375_EP0_UP_LEN:
				MOV R0,#CMD_BUFFER		;�ϴ���������R0
				MOV A,#CMD_WR_USB_DATA3		;��������룬д�˵�0
				CALL  WR_CMD_TO_375
				MOV  A,R5			;�����ϴ�����
				MOVX @DPTR,A			;���ݳ�����375
CH375_EP0_UP_LOOP2:		MOV A,@R0			;����ȡ����			;
				MOVX @DPTR,A			;��������CH375
				INC R0				;��������ַ��һ
				DJNZ  R5,CH375_EP0_UP_LOOP2 	;���������������
CH375_EP0_UP_LOOPRET:
				RET
;********************************************************************************************
CH375_DES_UPLOAD:		MOV R0,#CMD_BUFFER		;�������׵�ַ�ͼ��Ѱַ
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
				MOV A,#CMD_WR_USB_DATA3  ;�ϴ�0���ȵ�������Ϣ��
				CALL  WR_CMD_TO_375
				MOV A,#00H
				MOVX @DPTR,A
				RET
;********************************************************************************************


; �Ժ���Ϊ��λ��ʱ
; ENTRY: R7 ��ʱ������
; USE:   ACC, R7
DELAY_MS:		MOV   A,#0FAH				;250*(1*6+2)*0.5=1000uS
DELAY_1MS:		NOP
				NOP
				NOP
				NOP
				NOP
				NOP
				DJNZ  ACC,DELAY_1MS			;ÿ������ʱ1����
				DJNZ  R7,DELAY_MS
				RET
;
;*********************************************************************************************************************
;
CH375_TAB_TAB:			MOVC A,@A+DPTR
				RET

USB_DEV_DESCR_TAB:	DB   12H							;USB�豸������
					DB   01H
					DB   10H
					DB   01H
					DB   0FFH							;04H, DeviceClass
					DB   80H							;05H, DeviceSubClass
					DB   37H							;06H, DeviceProtocol
					DB   08H							;�˵�0��������8�ֽ�
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
					
USB_CFG_DESCR_TAB:	DB   09H							;USB����������
					DB   02H
					DB   27H							;02H, TotalLength-L
					DB   00H
					DB   01H
					DB   01H							;����ֵ
					DB   00H
					DB   80H							;07H, Attributes
					DB   40H
					DB   09H							;USB�ӿ�������
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
					DB   82H							;�˵�82H����/�ϴ�
					DB   02H							;�����˵�
					DB   40H							;��������32�ֽ�
					DB   00H
					DB   00H
					DB   07H
					DB   05H
					DB   02H							;�˵�02H����/�´�
					DB   02H							;�����˵�
					DB   40H							;��������32�ֽ�
					DB   00H
					DB   00H
					DB   07H
					DB   05H
					DB   81H							;�˵�81H����/�ϴ�
					DB   03H							;�ж϶˵�
					DB   08H							;��������8�ֽ�
					DB   00H
					DB   01H							;��ѯʱ��1����
					DB   07H
					DB   05H
					DB   01H							;�˵�01H����/�´�
					DB   02H							;�����˵�
					DB   08H							;��������8�ֽ�
					DB   00H
					DB   00H
END
;