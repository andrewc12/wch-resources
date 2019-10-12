; Define for CH372 & CH375
; Website:  http://wch.cn    http://winchiphead.com
; Email:    tech@wch.cn      tech@winchiphead.com
; Author:   W.ch 2003.09
; V3.0 for CH372B, CH375B

; *********************************************************************************************************************
; Ӳ������

CH375_MAX_DATA_LEN	EQU     40H							;������ݰ��ĳ���, �������ĳ���

; *********************************************************************************************************************
; �������

CMD_GET_IC_VER		EQU     01H							;��ȡоƬ���̼��汾
; ���: �汾��( λ7Ϊ1, λ6Ϊ0, λ5~λ0Ϊ�汾�� )
;           CH375B���ذ汾�ŵ�ֵΪ0B7H���汾��Ϊ37H

CMD_ENTER_SLEEP		EQU     03H							;����˯��״̬

CMD_SET_USB_SPEED	EQU     04H							;����USB�����ٶ�, ��ÿ��CMD_SET_USB_MODE����USB����ģʽʱ���Զ��ָ���12Mbpsȫ��
; ����: �����ٶȴ���
;           00H=12Mbpsȫ��FullSpeed��Ĭ��ֵ��, 01H=1.5Mbps�����޸�Ƶ�ʣ�, 02H=1.5Mbps����LowSpeed
CMD_SET_SYS_FREQ	EQU     CMD_SET_USB_SPEED

CMD_RESET_ALL		EQU     05H							;ִ��Ӳ����λ

CMD_CHECK_EXIST		EQU     06H							;���Թ���״̬
; ����: ��������
; ���: �������ݵİ�λȡ��

CMD_GET_TOGGLE		EQU     0AH							;��ȡOUT�����ͬ��״̬
; ����: ����1AH
; ���: ͬ��״̬
;           λ4Ϊ1��OUT����ͬ��, ����OUT����ͬ��

CMD_CHK_SUSPEND		EQU     0BH							;�豸��ʽ: ���ü��USB���߹���״̬�ķ�ʽ
; ����: ����10H, ��鷽ʽ
;                    00H=�����USB����, 04H=��50mSΪ������USB����, 05H=��10mSΪ������USB����

CMD_DELAY_100US		EQU     0FH							;���ڷ�ʽ: ��ʱ100uS
; ���: ��ʱ�ڼ����0, ��ʱ���������0

CMD_SET_USB_ID		EQU     12H							;�豸��ʽ: ����USB����VID�Ͳ�ƷPID
; ����: ����ID���ֽ�, ����ID���ֽ�, ��ƷID���ֽ�, ��ƷID���ֽ�

CMD_SET_USB_ADDR	EQU     13H							;����USB��ַ
; ����: ��ֵַ

CMD_SET_USB_MODE	EQU     15H							;����USB����ģʽ
; ����: ģʽ����
;       00H=δ���õ��豸��ʽ, 01H=�����õ��豸��ʽ����ʹ���ⲿ�̼�ģʽ, 02H=�����õ��豸��ʽ����ʹ�����ù̼�ģʽ
;       04H=δ���õ�������ʽ, 05H=�����õ�������ʽ, 06H=�����õ�������ʽ�����Զ�����SOF��, 07H=�����õ�������ʽ���Ҹ�λUSB����
; ���: ����״̬( CMD_RET_SUCCESS��CMD_RET_ABORT, ����ֵ˵������δ��� )

CMD_SET_ENDP2		EQU     18H							;�豸��ʽ: ����USB�˵�0�Ľ�����
; ����: ������ʽ
;           λ7Ϊ1��λ6Ϊͬ������λ, ����ͬ������λ����
;           λ3~λ0Ϊ������Ӧ��ʽ:  0000-����ACK, 1110-��æNAK, 1111-����STALL

CMD_SET_ENDP3		EQU     19H							;�豸��ʽ: ����USB�˵�0�ķ�����
; ����: ������ʽ
;           λ7Ϊ1��λ6Ϊͬ������λ, ����ͬ������λ����
;           λ3~λ0Ϊ������Ӧ��ʽ:  0000~1000-����ACK, 1110-��æNAK, 1111-����STALL

CMD_SET_ENDP4		EQU     1AH							;�豸��ʽ: ����USB�˵�1�Ľ�����
; ����: ������ʽ
;           λ7Ϊ1��λ6Ϊͬ������λ, ����ͬ������λ����
;           λ3~λ0Ϊ������Ӧ��ʽ:  0000-����ACK, 1110-��æNAK, 1111-����STALL

CMD_SET_ENDP5		EQU     1BH							;�豸��ʽ: ����USB�˵�1�ķ�����
; ����: ������ʽ
;           λ7Ϊ1��λ6Ϊͬ������λ, ����ͬ������λ����
;           λ3~λ0Ϊ������Ӧ��ʽ:  0000~1000-����ACK, 1110-��æNAK, 1111-����STALL

CMD_SET_ENDP6		EQU     1CH							;����USB�˵�2/�����˵�Ľ�����
; ����: ������ʽ
;           λ7Ϊ1��λ6Ϊͬ������λ, ����ͬ������λ����
;           λ3~λ0Ϊ������Ӧ��ʽ:  0000-����ACK, 1101-������������ACK, 1110-��æNAK, 1111-����STALL

CMD_SET_ENDP7		EQU     1DH							;����USB�˵�2/�����˵�ķ�����
; ����: ������ʽ
;           λ7Ϊ1��λ6Ϊͬ������λ, ����ͬ������λ����
;           λ3~λ0Ϊ������Ӧ��ʽ:  0000-����ACK, 1101-����������Ӧ��, 1110-��æNAK, 1111-����STALL

CMD_GET_STATUS		EQU     22H							;��ȡ�ж�״̬��ȡ���ж�����
; ���: �ж�״̬

CMD_UNLOCK_USB		EQU     23H							;�豸��ʽ: �ͷŵ�ǰUSB������

CMD_RD_USB_DATA0	EQU     27H							;�ӵ�ǰUSB�жϵĶ˵㻺������ȡ���ݿ�
; ���: ����, ������

CMD_RD_USB_DATA		EQU     28H							;�ӵ�ǰUSB�жϵĶ˵㻺������ȡ���ݿ�, ���ͷŻ�����, �൱�� CMD_RD_USB_DATA0 + CMD_UNLOCK_USB
; ���: ����, ������

CMD_WR_USB_DATA3	EQU     29H							;�豸��ʽ: ��USB�˵�0�ķ��ͻ�����д�����ݿ�
; ����: ����, ������

CMD_WR_USB_DATA5	EQU     2AH							;�豸��ʽ: ��USB�˵�1�ķ��ͻ�����д�����ݿ�
; ����: ����, ������

CMD_WR_USB_DATA7	EQU     2BH							;��USB�˵�2���������˵�ķ��ͻ�����д�����ݿ�
; ����: ����, ������

; **************************************************************************
; ������������USB������ʽ, ֻ��CH375֧��

CMD_SET_BAUDRATE	EQU     02H							;������ʽ & ���ڷ�ʽ: ���ô���ͨѶ������
; ����: �����ʷ�Ƶϵ��, �����ʷ�Ƶ����
; ���: ����״̬( CMD_RET_SUCCESS��CMD_RET_ABORT, ����ֵ˵������δ��� )

CMD_GET_DEV_RATE	EQU		0AH							;������ʽ: ��ȡ��ǰ���ӵ�USB�豸��������������
; ����: ����07H
; ���: ������������
;                    λ4Ϊ1����1.5Mbps����USB�豸, ������12Mbpsȫ��USB�豸

CMD_GET_MAX_LUN		EQU		0AH							;������ʽ: ��ȡUSB�洢������߼���Ԫ��
; ����: ����38H
; ���: ����߼���Ԫ��

CMD_SET_RETRY		EQU     0BH							;������ʽ: ����USB������������Դ���
; ����: ����25H, ���Դ���
;                    λ7Ϊ0���յ�NAKʱ������, λ7Ϊ1λ6Ϊ0���յ�NAKʱ��������, λ7Ϊ1λ6Ϊ1���յ�NAKʱ�������2��, λ5~λ0Ϊ��ʱ������Դ���

CMD_SET_DISK_LUN	EQU     0BH							;������ʽ: ����USB�洢���ĵ�ǰ�߼���Ԫ��
; ����: ����34H, �µĵ�ǰ�߼���Ԫ��(00H-0FH)

CMD_SET_PKT_P_SEC	EQU     0BH							;������ʽ: ����USB�洢����ÿ�������ݰ�����
; ����: ����39H, �µ�ÿ�������ݰ�����(08H,10H,20H,40H)

CMD_TEST_CONNECT	EQU     16H							;������ʽ: ���USB�豸����״̬
; ���: ״̬( USB_INT_CONNECT��USB_INT_DISCONNECT��USB_INT_USB_READY, ����ֵ˵������δ��� )

CMD_ABORT_NAK		EQU     17H							;������ʽ: ������ǰNAK������
;
CMD_CLR_STALL		EQU     41H							;������ʽ: ���ƴ���-����˵����
; ����: �˵��
; ����ж�

CMD_SET_ADDRESS		EQU     45H							;������ʽ: ���ƴ���-����USB��ַ
; ����: ��ֵַ
; ����ж�

CMD_GET_DESCR		EQU     46H							;������ʽ: ���ƴ���-��ȡ������
; ����: ����������
; ����ж�

CMD_SET_CONFIG		EQU     49H							;������ʽ: ���ƴ���-����USB����
; ����: ����ֵ
; ����ж�

CMD_AUTO_SETUP		EQU     4DH							;������ʽ: �Զ�����USB�豸
; ����ж�

CMD_ISSUE_TKN_X		EQU     4EH							;������ʽ: ����ͬ������, ִ������, ������ɴ��� CMD_SET_ENDP6/CMD_SET_ENDP7 + CMD_ISSUE_TOKEN
; ����: ͬ����־, ��������
;           ͬ����־��λ7Ϊ�����˵�IN��ͬ������λ, λ6Ϊ�����˵�OUT��ͬ������λ, λ5~λ0����Ϊ0
;           �������Եĵ�4λ������, ��4λ�Ƕ˵��
; ����ж�

CMD_ISSUE_TOKEN		EQU     4FH							;������ʽ: ��������, ִ������
; ����: ��������
;           ��4λ������, ��4λ�Ƕ˵��
; ����ж�

CMD_DISK_BOC_CMD	EQU     50H							;������ʽ: ��USB�洢��ִ��BulkOnly����Э�������
; ����ж�

CMD_DISK_INIT		EQU     51H							;������ʽ: ��ʼ��USB�洢��
; ����ж�

CMD_DISK_RESET		EQU     52H							;������ʽ: ���ƴ���-��λUSB�洢��
; ����ж�

CMD_DISK_SIZE		EQU     53H							;������ʽ: ��ȡUSB�洢��������
; ����ж�

CMD_DISK_READ		EQU     54H							;������ʽ: ��USB�洢�������ݿ�(������Ϊ��λ)
; ����: LBA������ַ(�ܳ���32λ, ���ֽ���ǰ), ������(01H~FFH)
; ����ж�

CMD_DISK_RD_GO		EQU     55H							;������ʽ: ����ִ��USB�洢���Ķ�����
; ����ж�

CMD_DISK_WRITE		EQU     56H							;������ʽ: ��USB�洢��д���ݿ�(������Ϊ��λ)
; ����: LBA������ַ(�ܳ���32λ, ���ֽ���ǰ), ������(01H~FFH)
; ����ж�

CMD_DISK_WR_GO		EQU     57H							;������ʽ: ����ִ��USB�洢����д����
; ����ж�

CMD_DISK_INQUIRY	EQU     58H							;������ʽ: ��ѯUSB�洢������
; ����ж�

CMD_DISK_READY		EQU     59H							;������ʽ: ���USB�洢������
; ����ж�

CMD_DISK_R_SENSE	EQU     5AH							;������ʽ: ���USB�洢������
; ����ж�

CMD_DISK_MAX_LUN	EQU     5DH							;������ʽ: ���ƴ���-��ȡUSB�洢������߼���Ԫ��
; ����ж�

; *********************************************************************************************************************
; ����״̬

CMD_RET_SUCCESS		EQU     51H							;��������ɹ�
CMD_RET_ABORT		EQU     5FH							;�������ʧ��

; *********************************************************************************************************************
; USB�ж�״̬

; ����״̬����Ϊ�����¼��ж�, ���ͨ��CMD_CHK_SUSPEND����USB���߹�����, ��ô���봦��USB���߹����˯�߻��ѵ��ж�״̬
USB_INT_USB_SUSPEND	EQU     05H							;USB���߹����¼�
USB_INT_WAKE_UP		EQU     06H							;��˯���б������¼�

; ����״̬����0XH����USB�豸��ʽ
;   ���ù̼�ģʽ��ֻ��Ҫ����: USB_INT_EP1_OUT, USB_INT_EP1_IN, USB_INT_EP2_OUT, USB_INT_EP2_IN
;   λ7-λ4Ϊ0000
;   λ3-λ2ָʾ��ǰ����, 00=OUT, 10=IN, 11=SETUP
;   λ1-λ0ָʾ��ǰ�˵�, 00=�˵�0, 01=�˵�1, 10=�˵�2, 11=USB���߸�λ
USB_INT_EP0_SETUP	EQU     0CH							;USB�˵�0��SETUP
USB_INT_EP0_OUT		EQU     00H							;USB�˵�0��OUT
USB_INT_EP0_IN		EQU     08H							;USB�˵�0��IN
USB_INT_EP1_OUT		EQU     01H							;USB�˵�1��OUT
USB_INT_EP1_IN		EQU     09H							;USB�˵�1��IN
USB_INT_EP2_OUT		EQU     02H							;USB�˵�2��OUT
USB_INT_EP2_IN		EQU     0AH							;USB�˵�2��IN
;USB_INT_BUS_RESET	EQU     0000XX11B					;USB���߸�λ
USB_INT_BUS_RESET1	EQU     03H							;USB���߸�λ
USB_INT_BUS_RESET2	EQU     07H							;USB���߸�λ
USB_INT_BUS_RESET3	EQU     0BH							;USB���߸�λ
USB_INT_BUS_RESET4	EQU     0FH							;USB���߸�λ

; ����״̬����2XH-3XH����USB������ʽ��ͨѶʧ�ܴ���, ��CH375֧��
;   λ7-λ6Ϊ00
;   λ5Ϊ1
;   λ4ָʾ��ǰ���յ����ݰ��Ƿ�ͬ��
;   λ3-λ0ָʾ����ͨѶʧ��ʱUSB�豸��Ӧ��: 0010=ACK, 1010=NAK, 1110=STALL, 0011=DATA0, 1011=DATA1, XX00=��ʱ
;USB_INT_RET_ACK	EQU     001X0010B					;����:����IN���񷵻�ACK
;USB_INT_RET_NAK	EQU     001X1010B					;����:����NAK
;USB_INT_RET_STALL	EQU     001X1110B					;����:����STALL
;USB_INT_RET_DATA0	EQU     001X0011B					;����:����OUT/SETUP���񷵻�DATA0
;USB_INT_RET_DATA1	EQU     001X1011B					;����:����OUT/SETUP���񷵻�DATA1
;USB_INT_RET_TOUT	EQU     001XXX00B					;����:���س�ʱ
;USB_INT_RET_TOGX	EQU     0010X011B					;����:����IN���񷵻����ݲ�ͬ��
;USB_INT_RET_PID	EQU     001XXXXXB					;����:δ����

; ����״̬����1XH����USB������ʽ�Ĳ���״̬����, ��CH375֧��

USB_INT_SUCCESS		EQU     14H							;USB������ߴ�������ɹ�
USB_INT_CONNECT		EQU     15H							;��⵽USB�豸�����¼�
USB_INT_DISCONNECT	EQU     16H							;��⵽USB�豸�Ͽ��¼�
USB_INT_BUF_OVER	EQU     17H							;USB���ƴ��������̫��, ���������
USB_INT_USB_READY	EQU     18H							;USB�豸�Ѿ�����ʼ�����ѷ���USB��ַ��
USB_INT_DISK_READ	EQU     1DH							;USB�洢�������ݿ�, �������ݶ���
USB_INT_DISK_WRITE	EQU     1EH							;USB�洢��д���ݿ�, ��������д��
USB_INT_DISK_ERR	EQU     1FH							;USB�洢������ʧ��

; *********************************************************************************************************************
; ����USB����

; USB�İ���ʶPID, ������ʽ�����õ�
DEF_USB_PID_NULL	EQU     00H							;����PID, δ����
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

; USB��������, ���ù̼�ģʽ�����õ�
DEF_USB_REQ_READ	EQU     80H							;���ƶ�����
DEF_USB_REQ_WRITE	EQU     00H							;����д����
DEF_USB_REQ_TYPE	EQU     60H							;������������
DEF_USB_REQ_STAND	EQU     00H							;��׼����
DEF_USB_REQ_CLASS	EQU     20H							;�豸������
DEF_USB_REQ_VENDOR	EQU     40H							;��������
DEF_USB_REQ_RESERVE	EQU     60H							;��������

; USB��׼�豸����, RequestType��λ6λ5=00(Standard), ������ʽ�����õ�
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