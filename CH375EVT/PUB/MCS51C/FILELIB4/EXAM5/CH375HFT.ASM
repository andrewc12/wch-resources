; /* 2004.06.05
; ****************************************
; **  Copyright  (C)  W.ch  1999-2004   **
; **  Web:  http://www.winchiphead.com  **
; ****************************************
; **  USB Host File Interface for CH375 **
; **  TC2.0@PC, KC7.0@MCS51             **
; ****************************************
; */
; /* CH375 �����ļ�ϵͳ�ӿ� */
; /* ֧��: FAT12/FAT16/FAT32 */
; 
; /* MCS-51��Ƭ��ASM���Ե�U���ļ���дʾ������, ������89C52���߸������ռ�ĵ�Ƭ�� */
; /* �ó���U���е�/C51/CH375HFT.C�ļ��е�Сд��ĸת�ɴ�д��ĸ��, д���½����ļ�NEWFILE.TXT��,
;    ����Ҳ���ԭ�ļ�CH375HFT.C, ��ô�ó����½�NEWFILE.TXT�ļ���д����ʾ��Ϣ
;    ����,��������ʾ���ֽ�Ϊ��λ��ȡU���е�/C51/CH375HFT.C�ļ�, ��д�ٶȽ�ǰ�������ģʽ��,
;    ���������ֽ�ģʽ��д�ļ�����Ҫ�ļ����ݻ�����FILE_DATA_BUF, �����ܹ�ֻ��Ҫ600�ֽڵ�RAM,
;    �����ڵ�Ƭ��Ӳ����Դ���ޡ�������С���Ҷ�д�ٶ�Ҫ�󲻸ߵ�ϵͳ
; */
; /* CH375��INT#���Ų��ò�ѯ��ʽ����, ���ݸ��Ʒ�ʽΪ"��DPTR����", �����ٶȽ���, ����������MCS51��Ƭ�� */
;
; /* A51   CH375HFT.ASM */
; /* LX51  CH375HFT.OBJ , CH375HF6.LIB    �����CH375HF6����CH375HF4�Ϳ��Խ�ֹ֧��FAT32 */
; /* OHX51 CH375HFT */
;

LIB_CFG_VALUE			EQU     05H		; CH375���������
; λ7: CH375��INT#�������ӷ�ʽ: 0��ѯ��ʽ,1�жϷ�ʽ */
; λ5: ��д�����������Ƿ���ʱ: 0д����ʱ,1����ʱ */
; λ3λ2: ����ļ���д�Ķ��������ݵĸ��Ʒ�ʽ: 00�ⲿ�ӳ���, 01��DPTR����, 10˫DPTR����, 11��DPTR��P2+R0���� */
; λ1λ0: ��Դ��̶�д�ĵ��������ݵĸ��Ʒ�ʽ: 00��DPTR����, 01��DPTR����, 10˫DPTR����, 11��DPTR��P2+R0���� */

CH375_CMD_PORT_ADDR		EQU		0BDF1H	; CH375����˿ڵ�I/O��ַ */
CH375_DAT_PORT_ADDR		EQU		0BCF0H	; CH375���ݶ˿ڵ�I/O��ַ */

DISK_BASE_BUF_ADDR		EQU		0000H	; �ⲿRAM�Ĵ������ݻ���������ʼ��ַ,�Ӹõ�Ԫ��ʼ�Ļ���������ΪSECTOR_SIZE */
#define	DISK_BASE_BUF_LEN	512
FILE_DATA_BUF_ADDR		EQU		0200H	; �ⲿRAM���ļ����ݻ���������ʼ��ַ,���������Ȳ�С��һ�ζ�д�����ݳ��� */
#define	FILE_DATA_BUF_LEN	6800H
; �ⲿRAM���ļ����ݻ�����,���������Ȳ�С��һ�ζ�д�����ݳ��� */
; ���׼��ʹ��˫�����������д,��ô��Ҫ����FILE_DATA_BUF_LEN,�����ڲ�����ָ����������ַ,��CH375FileReadX����CH375FileRead,��CH375FileWriteX����CH375FileWrite */

#define	CH375_INT_WIRE		INT0
;CH375_INT_WIRE			EQU		INT0	; P3.2, INT0, CH375���ж���INT#����,����CH375��INT#����,���ڲ�ѯ�ж�״̬ */

#define NO_DEFAULT_CH375_F_ENUM		1		; δ����CH375FileEnumer����ʽ�ֹ�Խ�Լ���� */
#define NO_DEFAULT_CH375_F_QUERY	1		; δ����CH375FileQuery����ʽ�ֹ�Խ�Լ���� */

$INCLUDE		(..\CH375HF6.INC)			; ASMͷ�ļ�,���ö���,�������Ҫ֧��FAT32,��ô��ѡ��CH375HF4.INC */

; Ӧ�ó���ζ���
MY_CODE					SEGMENT		CODE
MY_DATA					SEGMENT		DATA

;/* ��P1.4����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,��U�̲������ */
;sbit P1_4  = P1^4;
LED_OUT					EQU		P1.4
;#define LED_OUT_INIT( )		{ P1_4 = 1; }	/* P1.4 �ߵ�ƽ */
;#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 �͵�ƽ����LED��ʾ */
;#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 �͵�ƽ����LED��ʾ */
;sbit P1_5  = P1^5;
LED_RUN					EQU		P1.5
;/* ��P1.5����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,����U�̲���ʱ�� */
;#define LED_RUN_ACT( )		{ P1_5 = 0; }	/* P1.5 �͵�ƽ����LED��ʾ */
;#define LED_RUN_INACT( )	{ P1_5 = 1; }	/* P1.5 �͵�ƽ����LED��ʾ */
;sbit P1_6  = P1^6;
LED_WR					EQU		P1.6
;/* ��P1.6����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,����U��д����ʱ�� */
;#define LED_WR_ACT( )		{ P1_6 = 0; }	/* P1.6 �͵�ƽ����LED��ʾ */
;#define LED_WR_INACT( )		{ P1_6 = 1; }	/* P1.6 �͵�ƽ����LED��ʾ */

; ��λ���ж�����
				CSEG	AT	0000H
				LJMP	ASM_MAIN		; ��Ƭ����λ������������
;				CSEG	AT	0003H		; INT0�ж�����
;				LJMP    InterruptHandle	; CH375���жϷ������,���ʹ��"�жϷ�ʽ"��������ж������޸�����

; Ӧ�ó������õ��ı���,ֻ�����С,��ָ����ַ,�����ӳ����Զ������ַ
	RSEG  MY_DATA
   SecCount:   DS   1
    NewSize:   DS   2

; Ӧ�ó������õ��ĳ���,ֻ�����С,��ָ����ַ,�����ӳ����Զ������ַ
	RSEG  MY_CODE
SOURCE_NAME:	DB  '\C51\CH375HFT.C',00H
INFO_MISS_FILE:	DB  '�Ҳ���/C51/CH375HFT.C�ļ�',0DH,0AH,00H
NEW_NAME:		DB  '\NEWFILE.TXT',00H
NEW_NAME_2:		DB  '\MY_FILE2.TXT',00H
DATA_FOR_FILE2:	DB  'OK! ����д�����ļ��е�',0DH,0AH,00H

; Ӧ�ó������,��ָ����ַ,�����ӳ����Զ������ַ
	RSEG  MY_CODE
;
; ��ʱ100����,����ȷ */
mDelay100mS:		MOV  	R7,#0C8H
mDelay100mS_next:	MOV  	R6,#0C8H
mDelay100mS_loop:	NOP
					NOP
					DJNZ 	R6,mDelay100mS_loop
					DJNZ 	R7,mDelay100mS_next
					RET  	
;
; ������ռ���ַ������Ƶ��ڲ�RAM�� */
; ���: R0 ָ���ڲ�RAM��Ŀ�ĵ�ַ, DPTR ָ�����ռ���ַ���Դ��ַ
_mCopyStringToIRAM:	CLR  	A
					MOVC 	A,@A+DPTR
					INC  	DPTR
					MOV  	@R0,A
					INC  	R0
					JNZ     _mCopyStringToIRAM
					RET  	
;
; ������״̬,���������ͣ��,ʵ��Ӧ�ó�����Ҫ��������
_mStopIfError:		MOV  	A,R7
					JNZ     ERROR_FOUND			; ����
					RET							; �����ɹ��򷵻�
ERROR_FOUND:		CLR     LED_OUT				; LED��˸��ʾ
					LCALL	mDelay100mS
					SETB    LED_OUT
					LCALL	mDelay100mS
					SJMP 	ERROR_FOUND
;
; Ӧ�ó���������, ������ʾ
ASM_MAIN:			MOV     SP,#0CFH			; ��ջ
					CLR  	LED_OUT				; ������LED��һ����ʾ����
					LCALL	mDelay100mS
					SETB 	LED_OUT
					LCALL	CH375LibInit		; ��ʼ��CH375������CH375оƬ
					LCALL	_mStopIfError
; ��ѭ��, �ȴ�U�̲��벢����, ʵ���ϵ�Ƭ��������������, ����Ҫ��дU��ʱ�ٲ�ѯU��״̬
WAIT_DISK_IN:		MOV  	A,CH375DiskStatus	; ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̲��� */
					XRL  	A,#DISK_CONNECT
					JZ   	DISK_IN_NOW			; U���Ѿ�����
					LCALL	xQueryInterrupt		; �õ��ý�����U�̲�κ�Ż᷵��,���ϣ����������,��ô��ʹ��CH375DiskConnect����
					SJMP 	WAIT_DISK_IN
DISK_IN_NOW:		CLR  	LED_OUT				; LED��
					LCALL	mDelay100mS			; ��ʱ,��ѡ����,�е�USB�洢����Ҫ��ʮ�������ʱ */
					LCALL	mDelay100mS
; ���U���Ƿ�׼����,�����U�̲���Ҫ��һ��,����ĳЩU�̱���Ҫִ����һ�����ܹ���
					MOV		SecCount,#05H
IS_DISK_READY:		LCALL	mDelay100mS
					LCALL   CH375DiskReady		; ��ѯ�����Ƿ�׼����
					MOV     A,R7
					XRL     A,#ERR_SUCCESS
					JZ		DISK_READY_OK		;׼������
					DJNZ    SecCount,IS_DISK_READY	; �����ȴ�,�е�U�����Ƿ���δ׼����,�������Ա�����
DISK_READY_OK:		NOP
					CLR     LED_RUN				; ��ʼ����U��
; ��ԭ�ļ�\C51\CH375HFT.C
					MOV  	R0,#LOW (mCmdParam)	; �������,mCmdParam.Open.mPathName
					MOV  	DPTR,#SOURCE_NAME	; ָ��ԭ�ļ���,���ļ���C51��Ŀ¼��
					LCALL	_mCopyStringToIRAM	; �����ļ�����Ϊ�����ӳ���Ĳ���
					LCALL	CH375FileOpen		; ���ļ�
					MOV  	A,R7
					XRL  	A,#ERR_MISS_DIR
					JZ   	DIR_NOT_FOUND		; C51��Ŀ¼������
					MOV  	A,R7
					XRL  	A,#ERR_MISS_FILE
					JZ      FILE_NOT_FOUND		; CH375HFT.C�ļ�������
					LCALL	_mStopIfError
					SJMP    FOUND_SOURCE_FILE	; �ҵ��ļ�\C51\CH375HFT.C
; û���ҵ�ԭ�ļ�
DIR_NOT_FOUND:
FILE_NOT_FOUND:		MOV     R7,#00H
					MOV     DPTR,#FILE_DATA_BUF	; ���ļ�����������Ĭ�ϵ���ʾ��Ϣ��Ϊ���ļ�������
COPY_INFO_CHAR:		PUSH    DPL
					PUSH    DPH
					MOV     DPTR,#INFO_MISS_FILE	; ��û���ҵ�ԭ�ļ�ʱ�Ĵ�����ʾ��Ϣ
					MOV     A,R7
					MOVC 	A,@A+DPTR
					POP     DPH
					POP     DPL
					MOVX    @DPTR,A				; ����ʾ��Ϣ��Ϊ���ļ�������
					INC     DPTR
					JZ      COPY_INFO_END
					INC     R7
					CJNE    R7,#0FFH,COPY_INFO_CHAR	; �ļ��ַ�����
COPY_INFO_END:		MOV  	NewSize,#00H
					MOV  	NewSize+01H,R7		; ���ļ��ĳ���
					MOV  	SecCount,#01H		; ���ļ�ֻռ��1������, (NewSize+511)/512
					LJMP 	CREATE_NEW_FILE
; �ҵ�ԭ�ļ�
FOUND_SOURCE_FILE:	;LCALL	CH375FileQuery		; ��ѯ����Ϣ,��������,����ʱ��,���Ե�
					;LCALL	_mStopIfError
					MOV     A,CH375vFileSize	; ԭ�ļ��ĳ���
					JNZ     SOURCE_FILE_LONG	; ������ʾ���õ�62256ֻ��32K�ֽ�,����CH375�ӳ�����512�ֽ�,����ֻ��ȡ������63������
					MOV     A,CH375vFileSize+01H
					JNZ     SOURCE_FILE_LONG	; ����RAM����������,��������һ�ζ�д�����ĳ��Ȳ�����32256�ֽ�
					MOV     A,CH375vFileSize+02H
					CLR     C
					SUBB    A,#HIGH (FILE_DATA_BUF_LEN)
					JC      SOURCE_FILE_SHORT	; С��RAM�������ĳ���
					JNZ     SOURCE_FILE_LONG	; ����RAM�������ĳ���
					MOV     A,CH375vFileSize+03H
					CLR     C
					SUBB    A,#LOW (FILE_DATA_BUF_LEN)
					JC      SOURCE_FILE_SHORT	; һ�ζ�д�����ĳ���δ����RAM�������ĳ���
SOURCE_FILE_LONG:	MOV  	SecCount,#(FILE_DATA_BUF_LEN/512)	; ����ΪRAM�ļ���д�������ĳ���
					MOV  	NewSize,#HIGH (FILE_DATA_BUF_LEN)
					MOV  	NewSize+01H,#LOW (FILE_DATA_BUF_LEN)
					SJMP 	READ_SOURCE_FILE
SOURCE_FILE_SHORT:	MOV  	A,CH375vFileSize+03H	; ���ԭ�ļ���С,��ôʹ��ԭ����
					MOV     NewSize+01H,A		; NewSize=CH375vFileSize
					ADD  	A,#0FFH
					MOV  	A,CH375vFileSize+02H
					MOV     NewSize,A			; SecCount = (NewSize+511)/512
					ADDC 	A,#01H
					MOV  	R6,A
					CLR  	A
					ADDC 	A,CH375vFileSize+01H
					RRC     A
					MOV     A,R6
					RRC     A
					MOV  	SecCount,A			; �������ļ��ĳ��ȶ�Ӧ��������,��Ϊ��д��������Ϊ��λ��
; ��ԭ�ļ��ж�ȡ���ݵ�FILE_DATA_BUF��,������Ϊ��λ���ж�д,�����ٶȽϿ�,�������������ϴ�����
READ_SOURCE_FILE:	MOV  	R0,#LOW (mCmdParam)	; ������Ҫָ���������ݵĳ���,������Ϊ��λ,mCmdParam.Read.mSectorCount
					MOV  	@R0,SecCount		; ָ����ȡ������, ���������Ƕ�ȡȫ������,�������63��������ֻ��ȡ63������
					PUSH	CH375vFileSize+03H
					PUSH	CH375vFileSize+02H
					PUSH	CH375vFileSize+01H
					PUSH	CH375vFileSize
					MOV		A,CH375vFileSize+03H	;Ĭ�������,��������ʽ��ȡ����ʱ,�޷������ļ�β������1�������Ĳ���,���Ա�����ʱ�Ӵ��ļ������Զ�ȡβ����ͷ
					ADD		A,#0FFH				;CH375vFileSize+511
					MOV		CH375vFileSize+03H,A
					MOV		A,CH375vFileSize+02H
					ADDC	A,#01H
					MOV		CH375vFileSize+02H,A
					MOV		A,CH375vFileSize+01H
					ADDC	A,#00H
					MOV		CH375vFileSize+01H,A
					MOV		A,CH375vFileSize
					ADDC	A,#00H
					MOV		CH375vFileSize,A
					LCALL	CH375FileRead		; ���ļ���ȡ����
					LCALL	_mStopIfError		; ����ļ����ݵĸ��Ʒ�ʽΪ"�ⲿ�ӳ���",��ô��_xWriteToExtBuf�ӳ����������,�����Զ�����FILE_DATA_BUF��
					POP		CH375vFileSize		; �ָ�ԭ�ļ�����
					POP		CH375vFileSize+01H
					POP		CH375vFileSize+02H
					POP		CH375vFileSize+03H
					MOV  	R0,#LOW (mCmdParam)	; mCmdParam.Close.mUpdateLen
					MOV     @R0,#00H
					LCALL	CH375FileClose		; �ر��ļ�, �������Ҫ�ر��ļ�
					LCALL	_mStopIfError
; ��FILE_DATA_BUF�е�ԭ�ļ��ַ�ת��Ϊ��д
					MOV     DPTR,#FILE_DATA_BUF
					MOV     R6,#00H
					MOV     R7,#00H				; ����
CONVERT_CHAR_NEXT:	CLR     C
					MOV  	A,R7
					SUBB 	A,NewSize+01H
					MOV  	A,R6
					SUBB 	A,NewSize
					JNC  	CONVERT_CHAR_END	; ԭ�ļ��ַ��Ѿ��������
					MOVX 	A,@DPTR
					MOV  	R5,A
					CLR  	C
					SUBB 	A,#061H
					JC   	CONVERT_CHAR_SKIP	; ����Сд�ַ�
					MOV  	A,R5
					SETB 	C
					SUBB 	A,#07AH
					JNC  	CONVERT_CHAR_SKIP	; ����Сд�ַ�
					MOV  	A,R5
					ANL  	A,#0DFH
					MOVX 	@DPTR,A				; ��ԭСд�ַ�ת��Ϊ��д��д��FILE_DATA_BUF��
CONVERT_CHAR_SKIP:	INC     DPTR				; ָ�򻺳����е���һ���ַ�
					INC  	R7
					CJNE 	R7,#00H,CONVERT_CHAR_NEXT	; ����
					INC  	R6
					SJMP 	CONVERT_CHAR_NEXT
CONVERT_CHAR_END:

; �ڸ�Ŀ¼�´������ļ�NEWFILE.TXT,������Ϊ��λ���ж�д,�����ٶȽϿ�,�������������ϴ�����
CREATE_NEW_FILE:	CLR     LED_WR				; д����
					MOV  	R0,#LOW (mCmdParam)	; �������,mCmdParam.Create.mPathName
					MOV  	DPTR,#NEW_NAME		; ָ�����ļ���
					LCALL	_mCopyStringToIRAM	; �����ļ�����Ϊ�����ӳ���Ĳ���
					LCALL	CH375FileCreate		; �½��ļ�����,����ļ��Ѿ���������ɾ�������½�
					LCALL	_mStopIfError
					MOV  	R0,#LOW (mCmdParam)	; д����Ҫָ��д�����ݵĳ���,������Ϊ��λ,mCmdParam.Write.mSectorCount
					MOV  	@R0,SecCount		; д����������������
					LCALL	CH375FileWrite		; ���ļ�д������
					LCALL	_mStopIfError		; ����ļ����ݵĸ��Ʒ�ʽΪ"�ⲿ�ӳ���",��ô��_xReadFromExtBuf�ӳ���д������,�����Զ���FILE_DATA_BUF�е�����д��
					MOV     A,#0FFH
					MOV  	R0,#LOW (mCmdParam+08H)	; �������mCmdParam.Modify.mFileAttr: �µ��ļ�����,Ϊ0FFH���޸�
					MOV  	@R0,A
					MOV  	R0,#LOW (mCmdParam+06H)	; �������mCmdParam.Modify.mFileTime: �µ��ļ�ʱ��,Ϊ0FFFFH���޸�,ʹ���½��ļ�������Ĭ��ʱ��
					MOV  	@R0,A
					INC  	R0
					MOV  	@R0,A
					MOV  	R0,#LOW (mCmdParam+04H)	; �������mCmdParam.Modify.mFileDate: �µ��ļ�����: 2004.05.18 */
					MOV  	@R0,#030H			; ���ڲ��������� Date = ((Year-1980)<<9) + (Month<<5) + Day
					INC  	R0
					MOV  	@R0,#0B2H
					MOV  	R0,#LOW (mCmdParam)	; �������mCmdParam.Modify.mFileSize: ���ԭ�ļ���С,��ô�µ��ļ�������ԭ�ļ�һ����,����RAM����
					MOV     @R0,#00H
					INC     R0
					MOV     @R0,#00H			; ����ļ����ȴ���64KB,��ô�õ�ԪӦ��д���ļ����ȵĸ�23-16λ
					INC     R0
					MOV  	@R0,NewSize
					INC     R0
					MOV  	@R0,NewSize+01H		; ָ���µ��ļ�����
					LCALL	CH375FileModify		; �޸ĵ�ǰ�ļ�����Ϣ,�޸����ںͳ���
					LCALL	_mStopIfError
					MOV  	R0,#LOW (mCmdParam)	; �������, mCmdParam.Close.mUpdateLen
					MOV  	@R0,#00H			; ��Ҫ�Զ������ļ�����,����Զ�����,��ô�ó�������512�ı���
					LCALL	CH375FileClose		; �ر��ļ�
					LCALL	_mStopIfError

; �ڸ�Ŀ¼�´����ڶ������ļ�MY_FILE2.TXT,���ֽ�Ϊ��λ���ж�д,�����ٶȽ���,������RAM���١���������С�����
					MOV  	R0,#LOW (mCmdParam)	; �������,mCmdParam.Create.mPathName
					MOV  	DPTR,#NEW_NAME_2	; ָ�����ļ���
					LCALL	_mCopyStringToIRAM	; �����ļ�����Ϊ�����ӳ���Ĳ���
					LCALL	CH375FileCreate		; �½��ļ�����,����ļ��Ѿ���������ɾ�������½�
					LCALL	_mStopIfError
; ���ֽ�Ϊ��λд�뼸ʮ���ֽڵ�����
					MOV  	R0,#LOW (mCmdParam+01H)	; �������,mCmdParam.ByteWrite.mByteBuffer
					MOV  	DPTR,#DATA_FOR_FILE2	; ָ��׼��д�����ļ��е�����
					LCALL	_mCopyStringToIRAM	; ��׼��д�����ļ��е����ݸ��Ƶ������ṹ��
					MOV  	R0,#LOW (mCmdParam)	; д����Ҫָ��д�����ݵĳ���,������Ϊ��λ,mCmdParam.ByteWrite.mByteCount
					MOV  	@R0,#18H			; ����д����ֽ���24,���ܴ���MAX_BYTE_IO
					LCALL	CH375ByteWrite		; ���ļ�д������,�������������Ҫ����д��,��ô�����ٵ���CH375ByteWrite,���Ÿղŵ�д��
					LCALL	_mStopIfError		; ���ֽ�Ϊ��λд�ļ�����Ҫʹ��FILE_DATA_BUF������,�����븴�Ʒ�ʽ�޹�
					MOV  	R0,#LOW (mCmdParam)	; �������, mCmdParam.Close.mUpdateLen
					MOV  	@R0,#01H			; ���ֽ�Ϊ��λд���ļ���,Ӧ���Զ������ļ�����
					LCALL	CH375FileClose		; �ر��ļ�
					LCALL	_mStopIfError

					SETB    LED_WR
; ��ʾ�������, �ȴ�U�̰γ�
					SETB 	LED_RUN
WAIT_DISK_OUT:		MOV  	A,CH375DiskStatus	; ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̰γ�
					XRL  	A,#01H
					JZ   	DISK_OUT_NOW
					LCALL	xQueryInterrupt
					SJMP 	WAIT_DISK_OUT
DISK_OUT_NOW:		SETB 	LED_OUT				; LED��
					LCALL	mDelay100mS
					LCALL	mDelay100mS
					LJMP 	WAIT_DISK_IN		; ��ѭ��
;
END