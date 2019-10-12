/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Module      @CH375  **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* U���ļ���дģ��, ���ӷ�ʽ: 3���ƴ���+��ѯ+�¼��ж�֪ͨ */
/* MCS-51��Ƭ��C����ʾ������ */
/* ��Ϊʹ��U���ļ���дģ�������ʹ��U���ļ����ӳ����,����ռ�ý��ٵĵ�Ƭ����Դ,����ʹ��89C51��Ƭ������ */
/* ���ֽ�Ϊ��λ����U���ļ���д,��Ƭ����RAMֻ��Ҫ��ʮ���ֽ�,����Ҫ�ⲿRAM */
/* ������������ʾ��ADCģ���ɼ������ݱ��浽U���ļ�MY_ADC.TXT�� */

#include <reg51.h>
#include <absacc.h>
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN			32		/* ���·������,������б�ָܷ�����С���������Լ�·��������00H,CH375ģ��֧�ֵ����ֵ��62,��Сֵ��13 */
#include "..\CH375HM.H"

//#define ENABLE_AUTO_NOTICE		1		/* ����ģ���ڼ�⵽U�����ӻ��߶Ͽ���,�Զ�����״̬��֪ͨ����Ƭ�� */
#define U16						unsigned short

/* ��·���ӷ�ʽ,ֻ��Ҫ����3����,ʹ�ô���ͬ������������
   ��Ƭ��    ģ��
    TXD   =  SIN
    RXD   =  SOUT
             STA# ���ջ�Ӹߵ�ƽ
             INT# �ӵػ�ӵ͵�ƽ
    GND   =  GND
*/
sbit	P15					=	P1^5;

CMD_PARAM	idata	mCmdParam;			/* Ĭ������¸ýṹ��ռ��60�ֽڵ�RAM,�����޸�MAX_PATH_LEN����,���޸�Ϊ32ʱ,ֻռ��32�ֽڵ�RAM */
unsigned char		TempLength;			/* ��ʱ�������е����ݳ���,��ԭ�ļ��еڶ��ζ������ֽ��� */
unsigned char idata	TempBuffer[20];		/* ��ʱ������,��Ŵ�ԭ�ļ��ж��������� */

sbit	LED_OUT		=	P1^4;			/* P1.4 �͵�ƽ����LED��ʾ,���ڼ����ʾ����Ľ��� */

/* �Ժ���Ϊ��λ��ʱ,������24MHzʱ�� */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* ��24MHzʱ������ʱ500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* ��24MHzʱ������ʱ500uS */
	}
}

/* ����һ���ֽ����ݸ�CH375ģ�� */
void	mSendByte( unsigned char c )
{
	TI = 0;
	SBUF = c;
	while ( TI == 0 );
}

/* ��CH375ģ�����һ���ֽ����� */
unsigned char	mRecvByte( )
{
	unsigned char	c;
	while ( RI == 0 );
	c = SBUF;
	RI = 0;
	return( c );
}

/* ִ������ */
unsigned char	ExecCommand( unsigned char cmd, unsigned char len )
/* ����������������������,���ز���״̬��,��������ͷ��ز�������CMD_PARAM�ṹ�� */
{
	unsigned char		i, j, status;
	mSendByte( SER_SYNC_CODE1 );  /* ���ʹ���ͬ����֪ͨģ��,˵�������뿪ʼ����,����ʼִ������ */
	mSendByte( SER_SYNC_CODE2 );  /* ����������ͬ�������STA#���½��� */
/* ������������ͬ����Ӧ����������,���������,��ô���ʱ�䲻�ܳ���20mS,����������Ч */
	RI = 0;
	mSendByte( cmd );  /* д�������� */
	mSendByte( len );  /* д����������ĳ��� */
	if ( len ) {  /* �в��� */
		for ( i = 0; i != len; i ++ ) mSendByte( mCmdParam.Other.mBuffer[ i ] );  /* ����д����� */
	}
	while ( 1 ) {  /* �������ݴ���,ֱ��������ɲ��˳� */
		status = mRecvByte( );  /* �ȴ�ģ����ɲ��������ز���״̬ */
		if ( status == ERR_SUCCESS ) {  /* �����ɹ� */
			i = mRecvByte( );  /* ���ؽ�����ݵĳ��� */
			if ( i ) {  /* �н������ */
				j = 0;
				do {  /* ʹ��do+while�ṹ����Ϊ��Ч�ʸ���for */
					mCmdParam.Other.mBuffer[ j ] = mRecvByte( );  /* ���ս�����ݲ����浽�����ṹ�� */
					j ++;
				} while ( -- i );
			}
			break;  /* �����ɹ����� */
		}
		else if ( status == USB_INT_DISK_READ || status == USB_INT_DISK_WRITE || status == USB_INT_DISK_RETRY ) {  /* ���ڴ�U�̶����ݿ�,�������ݶ���,������U��д���ݿ�,��������д��,��д���ݿ�ʧ������ */
			break;  /* ������ֻʹ�����ֽ�Ϊ��λ���ļ���д�ӳ���,������������²����յ���״̬��,����ʧ�ܷ��� */
		}
		else {  /* ����ʧ�� */
			if ( status == ERR_DISK_DISCON || status == ERR_USB_CONNECT ) mDelaymS( 100 );  /* U�̸ո����ӻ��߶Ͽ�,Ӧ����ʱ��ʮ�����ٲ��� */
			break;  /* ����ʧ�ܷ��� */
		}
	}
	return( status );
}

/* ������״̬,�����������ʾ������벢ͣ�� */
void	mStopIfError( unsigned char iError )
{
	unsigned char	led;
	if ( iError == ERR_SUCCESS ) return;  /* �����ɹ� */
/*	printf( "Error: %02X\n", (unsigned short)iError );*/  /* ��ʾ���� */
	led=0;
	while ( 1 ) {
		LED_OUT = led&1;  /* LED��˸ */
		mDelaymS( 100 );
		led^=1;
	}
}

main( ) {
	unsigned char	i, month, hour;
	unsigned short	year, date, adc, len;
	unsigned char	*name;
	LED_OUT = 0;  /* ������LED��һ����ʾ���� */
	mDelaymS( 100 );  /* ��ʱ100����,CH375ģ���ϵ����Ҫ100�������ҵĸ�λʱ�� */
	mDelaymS( 100 );
	LED_OUT = 1;
/* ������CH375ģ��ͨѶ�Ĵ��� */
	SCON = 0x50;
	PCON = 0x80;
	TMOD = 0x20;
	TH1 = 0xE6;  /* 24MHz����, 4800bps */
	TR1 = 1;
/* ����4800bps����,����������������޸�Ϊ9600bps */
	mCmdParam.BaudRate.mDivisor = 18432000/32/9600;  /* �������: ͨѶ�����ʳ���,�ٶ�ģ��ľ���X2��Ƶ��Ϊ18.432MHz */
	i = ExecCommand( CMD_BaudRate, 1 );  /* ���ô���ͨѶ������ */
	mStopIfError( i );
	TH1 = 0xF3;  /* 24MHz����, ���������ڵ�ͨѶ�����ʵ�����9600bps */
	mDelaymS( 5 );  /* ��ʱ5����,ȷ��CH375ģ���л������趨��ͨѶ������ */

#ifdef ENABLE_AUTO_NOTICE  /* Ҫ��ģ���ڼ�⵽U�����ӻ��߶Ͽ���,�Զ�����״̬��֪ͨ����Ƭ�� */
	mCmdParam.Setup.mSetup = 0x01;  /* �������: ģ������ֵ,λ0Ϊ1�����ʱ��ѯU������״̬���Զ��ж�֪ͨ */
	i = ExecCommand( CMD_SetupModule, 1 );  /* ����ģ������ */
	mStopIfError( i );
#endif

/*	printf( "Start\n" );*/
	while ( 1 ) {  /* ��ѭ�� */
/*		printf( "Wait\n" );*/

#ifdef ENABLE_AUTO_NOTICE  /* ����ģ���ڼ�⵽U�����ӻ��߶Ͽ���,�Զ�����״̬��֪ͨ����Ƭ�� */
		while ( 1 ) {  /* �ȴ�ģ����¼�֪ͨ */
			if ( RI == 1 ) {  /* ��ѯ�Ƿ��յ�ģ����¼�֪ͨ,Ҳ�����ô��ڽ����жϴ��� */
				i = mRecvByte( );  /* ��⵽U�����ӻ��߶Ͽ���,�Զ�����״̬��֪ͨ����Ƭ�� */
				if ( i == ERR_USB_CONNECT ) {  /* �¼�֪ͨ��U���Ѿ����� */
/*					printf( "Disk Connected\n" );*/
					break;
				}
				else if ( i == ERR_DISK_DISCON ) {  /* �¼�֪ͨ��U���Ѿ��Ͽ� */
/*					printf( "Disk Disconnected\n" );*/
				}
			}
			mDelaymS( 100 );  /* �����ڴ����дU��ʱ�ٲ�ѯ,û�б�Ҫһֱ������ͣ�ز�ѯ,�����õ�Ƭ����������,û�¿�������ʱ�ȴ�һ���ٲ�ѯ */
		}
#else
		while ( 1 ) {  /* ʹ�ò�ѯ��ʽ��U���Ƿ����� */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* ��ѯ��ǰģ���״̬ */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus >= DISK_CONNECT ) break;  /* U���Ѿ����� */
			mDelaymS( 100 );  /* �����ڴ����дU��ʱ�ٲ�ѯ,û�б�Ҫһֱ������ͣ�ز�ѯ,�����õ�Ƭ����������,û�¿�������ʱ�ȴ�һ���ٲ�ѯ */
		}
#endif

		mDelaymS( 200 );  /* ��ʱ,��ѡ����,�е�USB�洢����Ҫ��ʮ�������ʱ */
		LED_OUT = 0;  /* LED�� */
/* ���U���Ƿ�׼����,�����U�̲���Ҫ��һ��,����ĳЩU�̱���Ҫִ����һ�����ܹ��� */
		for ( i = 0; i < 5; i ++ ) {
			mDelaymS( 100 );
//			printf( "Ready ?\n" );
			if ( ExecCommand( CMD_DiskReady, 0 ) == ERR_SUCCESS ) break;  /* ��ѯ�����Ƿ�׼���� */
		}
/* ��ADCȡ�����ݱ��浽�ļ���,���ȴ������ļ�,����ļ�������,���½�һ�� */
		name = "/MY_ADC.TXT";  /* �ļ���,б��˵���ǴӸ�Ŀ¼��ʼ */
/*		printf( "Open\n" );*/
		strcpy( mCmdParam.Open.mPathName, name );  /* ԭ�ļ��� */
		i = ExecCommand( CMD_FileOpen, MAX_PATH_LEN );  /* ���ļ�,���������Ϊ���ֵ,ʡ���ټ���������� */
		if ( i == ERR_MISS_FILE ) {  /* ERR_MISS_FILE˵��û���ҵ��ļ�,�����½�һ�� */
/*			printf( "Create\n" );*/
			strcpy( mCmdParam.Create.mPathName, name );  /* ���ļ���,�ڸ�Ŀ¼�� */
			i = ExecCommand( CMD_FileCreate, MAX_PATH_LEN );  /* �½��ļ�����,����ļ��Ѿ���������ɾ�������½� */
			mStopIfError( i );
		}
		else {  /* �ҵ��ļ�,˵���ļ��Ѵ���,��Ϊ�����㸲��ԭ����,�����ƶ��ļ�ָ�뵽ĩβ,�Ա�׷������ */
			mStopIfError( i );
			mCmdParam.ByteLocate.mByteOffset = 0xFFFFFFFF;  /* �ƶ����ļ�β,������CMD_FileOpen���ļ���,����׷�����ݵ��Ѵ��ļ���ĩβ */
			i = ExecCommand( CMD_ByteLocate, 4 );  /* ���ֽ�Ϊ��λ�ƶ��ļ�ָ�� */
			mStopIfError( i );
		}
/*		printf( "Write or append data\n" );*/
		for ( hour = 8; hour != 18; hour ++  ) {  /* ��ѭ����ʽ����10������ */
			TR0=1;  /* �ö�ʱ��0�ļ���ֵ����ADC���� */
			year = 2004; month = 5;  /* �ٶ���2004��5�� */
			date = TL1 & 0x1F;  /* ��Ϊ���԰���û��ʵʱʱ��оƬ,�����ö�ʱ��1�ļ������������ʾ */
/*			adc = get_adc_data( ); */
			adc = ( (U16)TH0 << 8 ) | TL0;  /* ��Ϊ���԰���û��ADC,�����ö�ʱ��0�ļ�������ADC������ʾ */
			len = sprintf( mCmdParam.ByteWrite.mByteBuffer, "%04d.%02d.%02d.%02d ADC=%u\xd\xa", year, (U16)month, date, (U16)hour, adc );  /* �����������ݸ�ʽΪһ���ַ��� */
			mCmdParam.ByteWrite.mByteCount = (unsigned char)len;  /* ָ������д����ֽ���,���ܳ���MAX_BYTE_IO,�������û������ֶ��д�� */
			i = ExecCommand( CMD_ByteWrite, (unsigned char)(len+1) );  /* ���ֽ�Ϊ��λ���ļ�д������ */
			mStopIfError( i );
		}
//		i = ExecCommand( CMD_QueryStatus, 0 );  /* ��ѯ��ǰģ���״̬ */
//		mStopIfError( i );
//		FileSize = mCmdParam.Status.mFileSize;  /* ����: ��ǰ�ļ��ĳ��� */
//		CurrentFilePoint = mCmdParam.Status.mCurrentOffset;  /* ����: ��ǰ�ļ�ָ��,��ǰ��дλ�õ��ֽ�ƫ�� */
//
//		mCmdParam.ByteWrite.mByteCount = 0;  /* ָ��д��0�ֽ�,����ˢ���ļ��ĳ���,ע������ֽ�����Ϊ0��ôCMD_ByteWriteֻ����д�����ݶ����޸��ļ����� */
//		ExecCommand( CMD_ByteWrite, 1 );  /* ���ֽ�Ϊ��λ���ļ�д������,��Ϊ��0�ֽ�д��,����ֻ���ڸ����ļ��ĳ���,���׶���д�����ݺ�,���������ְ취�����ļ����� */
		strcpy( mCmdParam.ByteWrite.mByteBuffer, "�����ADC���ݵ��˽���\xd\xa" );
		len = strlen( mCmdParam.ByteWrite.mByteBuffer );  /* �����ַ������� */
		mCmdParam.ByteWrite.mByteCount = len;  /* ��ԭ�ļ��е�20���ֽڵ��������ӵ����ļ���ĩβ */
		i = ExecCommand( CMD_ByteWrite, len+1 );  /* ���ֽ�Ϊ��λ���ļ�д������ */
		mStopIfError( i );
/*		printf( "Close\n" );*/
		mCmdParam.Close.mUpdateLen = 1;  /* �Զ������ļ�����,�����ֽ�Ϊ��λ���ļ�д�����ݺ�,���û����0���ȵ�CMD_ByteWrite�����ļ�����,��ô�����ڹر��ļ�ʱ��ģ���Զ������ļ����� */
		i = ExecCommand( CMD_FileClose, 1 );  /* �ر��ļ�,�����ֽ�Ϊ��λ���ļ�д��(׷��)���ݺ�,�����������ļ���ر��ļ� */
		mStopIfError( i );

/* �ȴ�U�̶Ͽ� */
/*		printf( "Take_out\n" );*/
#ifdef ENABLE_AUTO_NOTICE  /* ����ģ���ڼ�⵽U�����ӻ��߶Ͽ���,�Զ�����״̬��֪ͨ����Ƭ�� */
		while ( 1 ) {  /* �ȴ�ģ����¼�֪ͨ */
			if ( RI == 1 ) {  /* ��ѯ�Ƿ��յ�ģ����¼�֪ͨ,Ҳ�����ô��ڽ����жϴ��� */
				i = mRecvByte( );  /* ��⵽U�����ӻ��߶Ͽ���,�Զ�����״̬��֪ͨ����Ƭ�� */
				if ( i == ERR_USB_CONNECT ) {  /* �¼�֪ͨ��U���Ѿ����� */
/*					printf( "Disk Connected\n" );*/
				}
				else if ( i == ERR_DISK_DISCON ) {  /* �¼�֪ͨ��U���Ѿ��Ͽ� */
/*					printf( "Disk Disconnected\n" );*/
					break;
				}
			}
			mDelaymS( 100 );  /* �����ڴ����дU��ʱ�ٲ�ѯ,û�б�Ҫһֱ������ͣ�ز�ѯ,�����õ�Ƭ����������,û�¿�������ʱ�ȴ�һ���ٲ�ѯ */
		}
#else
		while ( 1 ) {  /* ʹ�ò�ѯ��ʽ��U���Ƿ�Ͽ� */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* ��ѯ��ǰģ���״̬ */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus <= DISK_DISCONNECT ) break;  /* U���Ѿ��Ͽ� */
			mDelaymS( 100 );  /* û�б�Ҫһֱ������ͣ�ز�ѯ,�����õ�Ƭ����������,û�¿�������ʱ�ȴ�һ���ٲ�ѯ */
		}
#endif
		LED_OUT = 1;  /* LED�� */
	}
}