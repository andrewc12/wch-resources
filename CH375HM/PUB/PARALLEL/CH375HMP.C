/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2007   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Module      @CH375  **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* U���ļ���дģ��, ���ӷ�ʽ: ����+��ѯ */
/* MCS-51��Ƭ��C����ʾ������, ��������V3.0A�����ϰ汾��ģ�� */
/* ��Ϊʹ��U���ļ���дģ�������ʹ��U���ļ����ӳ����,����ռ�ý��ٵĵ�Ƭ����Դ,����ʹ��89C51��Ƭ������ */

#include <reg51.h>
#include <absacc.h>
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN			32		/* ���·������,������б�ָܷ�����С���������Լ�·��������00H,CH375ģ��֧�ֵ����ֵ��64,��Сֵ��13 */
#include "..\CH375HM.H"

/* ��·���ӷ�ʽ
   ��Ƭ��    ģ��
    P0    =  D0-D7
    RD    =  RD#
    WR    =  WR#
    ?     =  CS#   ���û���ⲿRAM,��ôCS#=P26,����г���16KB���ⲿRAM,��ôCS#=P27 & ! P26 & ...,����CS#��Ƭѡ��ַΪBXXXH
    P20   =  A0
    INT0  =  INT#  ��Ȼ���ӵ�INT0,���Ǳ�����ֻ�ǲ�ѯģ���INT#��״̬,���Կ�����P1�ڵ���ͨI/O���Ŵ���INT0
*/
#define CH375HM_INDEX	XBYTE[0xBCF0]	/* CH375ģ��������˿ڵ�I/O��ַ */
#define CH375HM_DATA	XBYTE[0xBDF1]	/* CH375ģ������ݶ˿ڵ�I/O��ַ */
#define CH375HM_INT_WIRE		INT0	/* �ٶ�CH375ģ���INT#�������ӵ���Ƭ����INT0���� */

/* �ٶ��ļ����ݻ�����: ExtRAM: 0000H-7FFFH */
unsigned char xdata DATA_BUF[ 512 * 64 ] _at_ 0x0000;	/* �ⲿRAM���ļ����ݻ�����,�Ӹõ�Ԫ��ʼ�Ļ��������Ȳ�С��һ�ζ�д�����ݳ���,����Ϊ512�ֽ� */

CMD_PARAM		mCmdParam;				/* Ĭ������¸ýṹ��ռ��64�ֽڵ�RAM,�����޸�MAX_PATH_LEN����,���޸�Ϊ32ʱ,ֻռ��32�ֽڵ�RAM */
unsigned char	mIntStatus;				/* CH375ģ����ж�״̬���߲������״̬ */

sbit	LED_OUT		=	P1^4;			/* P1.4 �͵�ƽ����LED��ʾ,���ڼ����ʾ����Ľ��� */

/* ����ģ��Ĳ��ڶ�дʱ������������ڶ�д��ʽ,���޸�����3���ӳ��� */
#define CH375HM_INDEX_WR( Index )	{ CH375HM_INDEX = (Index); }	/* д������ַ */
#define CH375HM_DATA_WR( Data )		{ CH375HM_DATA = (Data); }		/* д���� */
#define CH375HM_DATA_RD( )			( CH375HM_DATA )				/* ������ */


/* �Ժ���Ϊ��λ��ʱ,������24MHzʱ�� */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* ��24MHzʱ������ʱ500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* ��24MHzʱ������ʱ500uS */
	}
}

/* ִ������ */
unsigned char	ExecCommandBuf( unsigned char cmd, unsigned char len, unsigned char xdata *bufstart )
/* ����������������������,���ز���״̬��,��������ͷ��ز�������CMD_PARAM�ṹ�� */
/* �������bufstart������CMD_FileRead����CMD_FileWrite����,ָ���ⲿRAM����������ʼ��ַ,���Բο��жϷ�ʽC�������ȫ�ֱ���buffer�ķ�ʽ */
{
	unsigned char		i, status;
	unsigned char data	*buf;
	unsigned char xdata	*CurrentBuf;
	CH375HM_INDEX_WR( PARA_COMMAND_ADDR );
	CH375HM_DATA_WR( cmd );  /* ��������ַPARA_COMMAND_ADDRд�������� */
	if ( len ) {  /* �в��� */
		i = len;
		CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* ָ�򻺳��� */
		buf = (unsigned char *)&mCmdParam;  /* ָ�������������ʼ��ַ */
		do {
			CH375HM_DATA_WR( *buf );  /* ��������ַPARA_BUFFER_ADDR��ʼ,д����� */
			buf ++;
		} while ( -- i );
	}
	CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
	CH375HM_DATA_WR( len | PARA_CMD_BIT_ACT );  /* ��������ַPARA_CMD_LEN_ADDRд����������ĳ���,���λ֪ͨģ��,˵��������Ѿ�д��,����ʼִ������ */
	CurrentBuf = bufstart;  /* �ⲿRAM��������ʼ��ַ,������FileRead����FileWrite���� */
	while ( 1 ) {  /* �������ݴ���,ֱ��������ɲ��˳� */

#if 1
		while ( CH375HM_INT_WIRE );  /* �ȴ�ģ����ɲ��������͵�ƽ�ж�,��Ѽ�ⷽʽ�Ƕ�ģ���INT#�źŽ����½��ر��ؼ�� */
#else
		do {  /* �������Ҫ������ʽ��д,��ô���Բ�ѯģ��������뵥Ԫ�����ѯģ��INT#���� */
			CH375HM_INDEX_WR( PARA_COMMAND_ADDR );
		} while ( CH375HM_DATA_RD( ) );  /* ģ��������ʱ��ֵ����0,�������ڷ�������ʽ��д */
#endif

		CH375HM_INDEX_WR( PARA_STATUS_ADDR );  /* д��������ַ */
		status = CH375HM_DATA_RD( );  /* ��������ַPARA_STATUS_ADDR��ȡ�ж�״̬ */
		CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
		CH375HM_DATA_WR( PARA_CMD_BIT_INACT );  /* �ж�Ӧ��,ȡ������ģ����ж����� */
/* ��Ϊģ�����յ��ж�Ӧ���3uS֮�ڲų����ж�����,����,����ǲ�ѯINT#�źŵĵ͵�ƽ,��ô�ڷ����ж�Ӧ���3uS֮�ڲ�Ӧ���ٲ�ѯINT#�źŵ�״̬
   ��������51��Ƭ������,����Ĵ���ʱ���Ѿ�����3uS,���Բ���������ʱ�ȴ�ģ�鳷���ж����� */
		if ( status == ERR_SUCCESS ) {  /* �����ɹ� */
			CH375HM_INDEX_WR( PARA_STS_LEN_ADDR );
			i = CH375HM_DATA_RD( );  /* ��������ַPARA_STS_LEN_ADDR��ȡ���ؽ�����ݵĳ���,���� */
			if ( i ) {  /* �н������ */
				CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* ָ�򻺳��� */
				buf = (unsigned char *)&mCmdParam;  /* ָ�������������ʼ��ַ */
				do {
					*buf = CH375HM_DATA_RD( );  /* ��������ַPARA_BUFFER_ADDR��ʼ,��ȡ��� */
					buf ++;
				} while ( -- i );
			}
//			status = ERR_SUCCESS;
			break;  /* �����ɹ����� */
		}
		else if ( status == USB_INT_DISK_READ ) {  /* ���ڴ�U�̶����ݿ�,�������ݶ��� */
			CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* ָ�򻺳��� */
			i = 64;  /* ���� */
			do {  /* Ҫ����ļ����ݶ�д�ٶ�,��γ����û�����Ч�ʸ���,��C51��,do+while��for����while�ṹЧ�ʸ� */
				*CurrentBuf = CH375HM_DATA_RD( );  /* ��������ַ0��63���ζ���64�ֽڵ����� */
				CurrentBuf ++;  /* ��ȡ�����ݱ��浽�ⲿ������ */
			} while ( -- i );  /* ������һС��C�����û�����Ч��Ҫ�߽�һ�� */
			CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
			CH375HM_DATA_WR( PARA_CMD_BIT_ACT );  /* ֪ͨģ�����,˵��64�ֽ������Ѿ���ȡ��� */
		}
		else if ( status == USB_INT_DISK_WRITE ) {  /* ������U��д���ݿ�,��������д�� */
			CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* ָ�򻺳��� */
			i = 64;  /* ���� */
			do {
				CH375HM_DATA_WR( *CurrentBuf );  /* ��������ַ0��63����д��64�ֽڵ����� */
				CurrentBuf ++;  /* д������������ⲿ������ */
			} while ( -- i );
			CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
			CH375HM_DATA_WR( PARA_CMD_BIT_ACT );  /* ֪ͨģ�����,˵��64�ֽ������Ѿ�д����� */
		}
		else if ( status == USB_INT_DISK_RETRY ) {  /* ��д���ݿ�ʧ������,Ӧ������޸Ļ�����ָ�� */
			CH375HM_INDEX_WR( PARA_BUFFER_ADDR );  /* ָ�򻺳��� */
			i = CH375HM_DATA_RD( );  /* ���ģʽ��Ϊ�ظ�ָ���ֽ����ĸ�8λ,�����С��ģʽ��ô���յ����ǻظ�ָ���ֽ����ĵ�8λ */
			status = CH375HM_DATA_RD( );  /* ���ģʽ��Ϊ�ظ�ָ���ֽ����ĵ�8λ,�����С��ģʽ��ô���յ����ǻظ�ָ���ֽ����ĸ�8λ */
			CurrentBuf -= ( (unsigned short)i << 8 ) + status;  /* ���Ǵ��ģʽ�µĻظ�ָ��,����С��ģʽ,Ӧ����( (unsigned short)status << 8 ) + i */
			CH375HM_INDEX_WR( PARA_CMD_LEN_ADDR );
			CH375HM_DATA_WR( PARA_CMD_BIT_ACT );  /* ֪ͨģ�����,˵������״̬���Ѿ�������� */
		}
		else {  /* ����ʧ�� */
			if ( status == ERR_DISK_DISCON || status == ERR_USB_CONNECT ) {  /* U�̸ո����ӻ��߶Ͽ�,Ӧ����ʱ��ʮ�����ٲ��� */
				mDelaymS( 100 );
				if ( CH375HM_INT_WIRE ) break;  /* û���ж��򷵻�,�����Ȼ���ж�����˵��֮ǰ���ж���U�̲��֪ͨ�ж�,�����ٴ�����������ж϶��ݲ����� */
			}
			else break;  /* ����ʧ�ܷ��� */
		}
	}
/*	while( CH375HM_INT_WIRE == 0 );  �����Ƭ���ٶȺܿ�,�п��ܸó��򷵻�ǰģ����δ�����ж�����,��ôӦ�õȴ��ж�����������Ч */
	return( status );
}

/* ִ������ */
unsigned char	ExecCommand( unsigned char cmd, unsigned char len )
/* ����������������������,���ز���״̬��,��������ͷ��ز�������CMD_PARAM�ṹ�� */
{
	return( ExecCommandBuf( cmd, len, 0 ) );  /* ֻ��CMD_FileRead����CMD_FileWrite�����õ��������bufstart,��������û���õ� */
}

/* ������״̬,�����������ʾ������벢ͣ��,Ӧ���滻Ϊʵ�ʵĴ�����ʩ */
void	mStopIfError( unsigned char iError )
{
	unsigned char	led;
	if ( iError == ERR_SUCCESS ) return;  /* �����ɹ� */
	printf( "Error: %02X\n", (unsigned short)iError );  /* ��ʾ���� */
	led=0;
	while ( 1 ) {
		LED_OUT = led&1;  /* LED��˸ */
		mDelaymS( 100 );
		led^=1;
	}
}

/* Ϊprintf��getkey���������ʼ������ */
void	mInitSTDIO( )
{
	SCON = 0x50;
	PCON = 0x80;
	TMOD = 0x20;
	TH1 = 0xf3;  /* 24MHz����, 9600bps */
	TR1 = 1;
	TI = 1;
}

main( ) {
	unsigned char	i, c, SecCount;
	unsigned long	OldSize;
	unsigned short	NewSize, count;
	LED_OUT = 0;  /* ������LED��һ����ʾ���� */
	mDelaymS( 200 );  /* ��ʱ100����,CH375ģ���ϵ����Ҫ100�������ҵĸ�λʱ�� */
	mDelaymS( 250 );
	LED_OUT = 1;
	mInitSTDIO( );
	printf( "Start\n" );
/* ������·��ʼ�� */
	while ( 1 ) {  /* ��ѭ�� */
		printf( "Wait Udisk\n" );
		while ( 1 ) {  /* ʹ�ò�ѯ��ʽ��U���Ƿ����� */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* ��ѯ��ǰģ���״̬ */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus >= DISK_CONNECT ) break;  /* U���Ѿ����� */
			mDelaymS( 200 );  /* �����ڴ����дU��ʱ�ٲ�ѯ,û�б�Ҫһֱ������ͣ�ز�ѯ,�����õ�Ƭ����������,û�¿�������ʱ�ȴ�һ���ٲ�ѯ */
		}
		mDelaymS( 200 );  /* ��ʱ,��ѡ����,�е�USB�洢����Ҫ��ʮ�������ʱ */
		LED_OUT = 0;  /* LED�� */
/* ���U���Ƿ�׼����,�����U�̲���Ҫ��һ��,����ĳЩU�̱���Ҫִ����һ�����ܹ��� */
		for ( i = 0; i < 5; i ++ ) {
			mDelaymS( 100 );
			printf( "Ready ?\n" );
			if ( ExecCommand( CMD_DiskReady, 0 ) == ERR_SUCCESS ) break;  /* ��ѯ�����Ƿ�׼���� */
		}
/* ��ȡԭ�ļ� */
		printf( "Open\n" );
		memcpy( mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C", MAX_PATH_LEN );  /* �ļ���,���ļ���C51��Ŀ¼�� */
		i = ExecCommand( CMD_FileOpen, MAX_PATH_LEN );  /* ���ļ�,���������Ϊ���ֵ,ʡ���ټ���������� */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* ERR_MISS_DIR˵��û���ҵ�C51��Ŀ¼,ERR_MISS_FILE˵��û���ҵ��ļ� */
/* �г���Ŀ¼�µ��ļ� */
			printf( "List file \\*\n" );
			for ( c = 0; c < 255; c ++ ) {  /* �������ǰ255���ļ� */
/*				memcpy( mCmdParam.Enumer.mPathName, "\\C51\\CH375*", MAX_PATH_LEN );*/  /* ����C51��Ŀ¼����CH375��ͷ���ļ���,*Ϊͨ��� */
				memcpy( mCmdParam.Enumer.mPathName, "\\*", MAX_PATH_LEN );  /* �����ļ���,*Ϊͨ���,�����������ļ�������Ŀ¼ */
/*				i = strlen( mCmdParam.Enumer.mPathName );*/  /* �����ļ����ĳ��� */
				for ( i = 0; i < MAX_PATH_LEN - 1; i ++ ) if ( mCmdParam.Enumer.mPathName[ i ] == 0 ) break;  /* ָ�������ļ����Ľ����� */
				mCmdParam.Enumer.mPathName[ i ] = c;  /* ���������滻Ϊ���������,��0��255 */
				i = ExecCommand( CMD_FileEnumer, i+1 );  /* ö���ļ�,����ļ����к���ͨ���*,��Ϊ�����ļ�������,��������ĳ��Ⱥܺü��� */
				if ( i == ERR_MISS_FILE ) break;  /* ��Ҳ��������ƥ����ļ�,�Ѿ�û��ƥ����ļ��� */
				if ( i == ERR_SUCCESS ) {  /* ��������ͨ�����ƥ����ļ���,�ļ�����������·������������� */
					printf( "  match file %03d#: %s\n", (unsigned int)c, mCmdParam.Enumer.mPathName );  /* ��ʾ��ź���������ƥ���ļ���������Ŀ¼�� */
					continue;  /* ����������һ��ƥ����ļ���,�´�����ʱ��Ż��1 */
				}
				else {  /* ���� */
					mStopIfError( i );
					break;
				}
			}
			strcpy( DATA_BUF, "Note: \xd\nԭ���Ǵ��㽫/C51/CH375HFT.C�ļ��е�Сд��ĸת�ɴ�д��д���µ��ļ�,�����Ҳ�������ļ�\xd\n" );
			OldSize = 0;
			NewSize = strlen( DATA_BUF );  /* ���ļ��ĳ��� */
			SecCount = ( NewSize + 511 ) >> 9;  /* (NewSize+511)/512, �����ļ���������,��Ϊ��д��������Ϊ��λ�� */
		}
		else {  /* �ҵ��ļ�\C51\CH375HFT.C���߳��� */
			mStopIfError( i );
			printf( "Query\n" );
			i = ExecCommand( CMD_FileQuery, 0 );  /* ��ѯ��ǰ�ļ�����Ϣ,û��������� */
			mStopIfError( i );
			printf( "Read\n" );
			OldSize = mCmdParam.Modify.mFileSize;  /* ԭ�ļ��ĳ��� */
			if ( OldSize > (unsigned long)(64*512) ) {  /* ��ʾ���õ�62256ֻ��32K�ֽ� */
				SecCount = 64;  /* ������ʾ���õ�62256ֻ��32K�ֽ�,����ֻ��ȡ������64������,Ҳ���ǲ�����32768�ֽ� */
				NewSize = 64*512;  /* ����RAM�����������Ƴ��� */
			}
			else {  /* ���ԭ�ļ���С,��ôʹ��ԭ���� */
				SecCount = ( OldSize + 511 ) >> 9;  /* (OldSize+511)/512, �����ļ���������,��Ϊ��д��������Ϊ��λ�� */
				NewSize = (unsigned short)OldSize;  /* ԭ���� */
			}
			printf( "Size=%ld, Len=%d, Sec=%d\n", OldSize, NewSize, (unsigned short)SecCount );
			mCmdParam.Read.mSectorCount = SecCount;  /* ��ȡȫ������,�������60��������ֻ��ȡ60������ */
			i = ExecCommandBuf( CMD_FileRead, 1, &DATA_BUF );  /* ���ļ���ȡ���� */
			mStopIfError( i );
/*
			����ļ��Ƚϴ�,һ�ζ�����,������������CMD_FileRead������ȡ,�ļ�ָ���Զ�����ƶ�
			while ( ʣ��δ���� ) {
				mCmdParam.Read.mSectorCount = 32;
				ExecCommandBuf( CMD_FileRead, 1, &DATA_BUF + �Ѿ���ȡ�ĳ��� );   ������ļ�ָ���Զ�����
				TotalLength += 32*512;  �ۼ��ļ��ܳ���
			}

		    ���ϣ����ָ��λ�ÿ�ʼ��д,�����ƶ��ļ�ָ��
			mCmdParam.Locate.mSectorOffset = 3;  �����ļ���ǰ3��������ʼ��д
			i = ExecCommand( CMD_FileLocate, 4 );  ��������ĳ���4��sizeof( mCmdParam.Locate.mSectorOffset )
			mCmdParam.Read.mSectorCount = 10;
			ExecCommandBuf( CMD_FileRead, 1, &DATA_BUF );   ֱ�Ӷ�ȡ���ļ��ĵ�(512*3)���ֽڿ�ʼ������,ǰ3������������

			���ϣ�������������ӵ�ԭ�ļ���β��,�����ƶ��ļ�ָ��
			i = ExecCommand( CMD_FileOpen, (unsigned char)( strlen( mCmdParam.Open.mPathName ) + 1 ) );
			mCmdParam.Locate.mSectorOffset = 0xffffffff;  �Ƶ��ļ���β��,������Ϊ��λ,���ԭ�ļ���3�ֽ�,���512�ֽڿ�ʼ����
			i = ExecCommand( CMD_FileLocate, sizeof( mCmdParam.Locate.mSectorOffset ) );
			mCmdParam.Write.mSectorCount = 10;
			ExecCommandBuf( CMD_FileWrite, 1, &DATA_BUF );   ��ԭ�ļ��ĺ�����������
*/
			printf( "Close\n" );
			mCmdParam.Close.mUpdateLen = 0;
			i = ExecCommand( CMD_FileClose, 1 );  /* �ر��ļ� */
			mStopIfError( i );

			i = DATA_BUF[200];
			DATA_BUF[200] = 0;  /* ���ַ���������־,�����ʾ200���ַ� */
			printf( "Line 1: %s\n", DATA_BUF );
			DATA_BUF[200] = i;  /* �ָ�ԭ�ַ� */
			for ( count=0; count < NewSize; count ++ ) {  /* ���ļ��е�Сд�ַ�ת��Ϊ��д */
				c = DATA_BUF[ count ];
				if ( c >= 'a' && c <= 'z' ) DATA_BUF[ count ] = c - ( 'a' - 'A' );
			}
		}
/* �������ļ� */
		printf( "Create\n" );
/*		memcpy( mCmdParam.Create.mPathName, "\\NEWFILE.TXT", MAX_PATH_LEN );*/
		memcpy( mCmdParam.Create.mPathName, "\\˫���Ұ�.TXT", MAX_PATH_LEN );  /* ���ļ���,�ڸ�Ŀ¼�� */
		i = ExecCommand( CMD_FileCreate, MAX_PATH_LEN );  /* �½��ļ�����,����ļ��Ѿ���������ɾ�������½� */
		mStopIfError( i );
		printf( "Write\n" );
		mCmdParam.Write.mSectorCount = 0x1;  /* д��һ������512�ֽ� */
		i = ExecCommandBuf( CMD_FileWrite, 1, &DATA_BUF );  /* ���ļ�д������ */
		mStopIfError( i );
		if ( SecCount > 1 ) {  /* ��Ϊ���ݲ�����255������,��������ܹ�һ��д��,����Ϊ����ʾ,���������д�� */
			mCmdParam.Write.mSectorCount = SecCount - 1;
/*	buffer = & DATA_BUF + 512;  ���Ÿղŵ�д,�������û���������ʼ��ַ */
			i = ExecCommandBuf( CMD_FileWrite, 1, &DATA_BUF + 512 );  /* ���ļ�д������,����������ǰ��д���512�ֽ�֮�� */
			mStopIfError( i );
		}
		printf( "Modify\n" );
		mCmdParam.Modify.mFileAttr = 0xff;  /* �������: �µ��ļ�����,Ϊ0FFH���޸� */
		mCmdParam.Modify.mFileTime = 0xffff;  /* �������: �µ��ļ�ʱ��,Ϊ0FFFFH���޸�,ʹ���½��ļ�������Ĭ��ʱ�� */
		mCmdParam.Modify.mFileDate = ( (2004-1980)<<9 ) + ( 5<<5 ) + 18;  /* �������: �µ��ļ�����: 2004.05.18 */
		mCmdParam.Modify.mFileSize = NewSize;  /* �������: ���ԭ�ļ���С,��ô�µ��ļ�������ԭ�ļ�һ����,����RAM���� */
		i = ExecCommand( CMD_FileModify, 4+2+2+1 );  /* �޸ĵ�ǰ�ļ�����Ϣ,�޸����ںͳ���,��������Ϊsizeof(mCmdParam.Modify.mFileSize)+... */
		mStopIfError( i );
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 0;  /* ��Ҫ�Զ������ļ�����,����Զ�����,��ô�ó�������512�ı��� */
		i = ExecCommand( CMD_FileClose, 1 );
		mStopIfError( i );

/* ɾ��ĳ�ļ� */
		printf( "Erase\n" );
		memcpy( mCmdParam.Create.mPathName, "\\OLD", MAX_PATH_LEN );  /* ����ɾ�����ļ���,�ڸ�Ŀ¼�� */
		i = ExecCommand( CMD_FileErase, MAX_PATH_LEN );  /* ɾ���ļ����ر� */
/*		mStopIfError( i );*/

/* ��ѯ������Ϣ */
		printf( "Disk\n" );
		i = ExecCommand( CMD_DiskQuery, 0 );
		mStopIfError( i );
		i = mCmdParam.Query.mDiskFat;
		if ( i == 1 ) i = 12;
		else if ( i == 2 ) i = 16;
		else if ( i == 3 ) i = 32;
		printf( "FatCode=FAT%d, TotalSector=%ld, FreeSector=%ld\n", (unsigned short)i, mCmdParam.Query.mTotalSector, mCmdParam.Query.mFreeSector );
/* �ȴ�U�̶Ͽ� */
		printf( "Take_out\n" );
		while ( 1 ) {  /* ʹ�ò�ѯ��ʽ��U���Ƿ�Ͽ� */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* ��ѯ��ǰģ���״̬ */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus <= DISK_DISCONNECT ) break;  /* U���Ѿ��Ͽ� */
			mDelaymS( 200 );  /* û�б�Ҫһֱ������ͣ�ز�ѯ,�����õ�Ƭ����������,û�¿�������ʱ�ȴ�һ���ٲ�ѯ */
		}
		LED_OUT = 1;  /* LED�� */
	}
}