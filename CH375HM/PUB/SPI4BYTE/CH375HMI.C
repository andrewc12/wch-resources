/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Module      @CH375  **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* U���ļ���дģ��, ���ӷ�ʽ: ����ģ��SPIʱ��+��ѯ */
/* MCS-51��Ƭ��C����ʾ������ */
/* ��Ϊʹ��U���ļ���дģ�������ʹ��U���ļ����ӳ����,����ռ�ý��ٵĵ�Ƭ����Դ,����ʹ��89C51��Ƭ������ */
/* ���ֽ�Ϊ��λ����U���ļ���д,��Ƭ����RAMֻ��Ҫ��ʮ���ֽ�,����Ҫ�ⲿRAM */

#include <reg51.h>
#include <absacc.h>
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN			32		/* ���·������,������б�ָܷ�����С���������Լ�·��������00H,CH375ģ��֧�ֵ����ֵ��62,��Сֵ��13 */
#include "..\CH375HM.H"

/* ��·���ӷ�ʽ,4��SPI,��SCS����ɹ���SPI����
   ��Ƭ��    ģ��
    P1.0  =  SDI
    P1.1  =  SDO
    P1.2  =  SCK
    P1.3  =  SCS
    ����  =  INT#  ������ͨ��SPI������ѯģ���INT#��״̬
*/
sbit	P10					=	P1^0;
sbit	P11					=	P1^1;
sbit	P12					=	P1^2;
sbit	P13					=	P1^3;
#define	CH375HM_SPI_SDI			P10		/* �ٶ�CH375ģ���SDI�������ӵ���Ƭ����P10���� */
#define	CH375HM_SPI_SDO			P11		/* �ٶ�CH375ģ���SDO�������ӵ���Ƭ����P11���� */
#define	CH375HM_SPI_SCK			P12		/* �ٶ�CH375ģ���SCK�������ӵ���Ƭ����P12���� */
#define	CH375HM_SPI_SCS			P13		/* �ٶ�CH375ģ���SCS�������ӵ���Ƭ����P13���� */

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

/* ����һ���ֽ����ݸ�CH375ģ��,ͬʱ����һ���ֽ�,��SPIģʽ0��ʽ(SPI_SCKĬ��Ϊ0) */
unsigned char	mSpiExchange( unsigned char c )
{
	unsigned char	i, d;
	d = 0;
	CH375HM_SPI_SCK = 0;
	for ( i = 8; i != 0; i -- ) {  /* 8��λ */
		if ( c & 0x80 ) CH375HM_SPI_SDI = 1;  /* ��ģ���SDI��������������� */
		else CH375HM_SPI_SDI = 0;
		d <<= 1;
		if ( CH375HM_SPI_SDO ) d ++;  /* ���ģ���SDO�������Ϊ�ߵ�ƽ������λ1 */
		CH375HM_SPI_SCK = 1;  /* SPIʱ�������أ�ģ��������ݲ�������� */
		c <<= 1;
		CH375HM_SPI_SCK = 0;
	}
/* �����Ƭ���Ͽ�ʹ��mSpiExchange�ӳ���ִ��ʱ�����10uS�������Ǵ���1MHz��Ӳ��SPI�ӿڣ���ô�˴�Ӧ�ü���ʱȷ�����ӳ���������ڴ���10uS */
/* ��������������ģ�鵥Ƭ������Ϊ18.432MHz����������ģ�鵥Ƭ������Ƶ�ʼӱ�����ô����ʱ��Ҫ��Ӧ�ü���һ�� */
/* ��ͨMCS51��Ƭ��������ģ���SPI�ӿڽ����������κ���ʱ */
	return( d );
}

#if 0
/* ����һ���ֽ����ݸ�CH375ģ��,ͬʱ����һ���ֽ�,��SPIģʽ3��ʽ(SPI_SCKĬ��Ϊ1) */
unsigned char	mSpiExchange3( unsigned char c )
{
	unsigned char	i, d;
	d = 0;
	for ( i = 8; i != 0; i -- ) {   /* 8��λ */
		CH375HM_SPI_SCK = 0;
		if ( c & 0x80 ) CH375HM_SPI_SDI = 1;  /* ��ģ���SDI��������������� */
		else CH375HM_SPI_SDI = 0;
		d <<= 1;
		if ( CH375HM_SPI_SDO ) d ++;  /* ���ģ���SDO�������Ϊ�ߵ�ƽ������λ1 */
		CH375HM_SPI_SCK = 1;  /* SPIʱ�������أ�ģ��������ݲ�������� */
		c <<= 1;
	}
/* ʱ��Ҫ��ͬ����mSpiExchange�ӳ���SPIģʽ0 */
	return( d );
}
#endif

/* ִ������ */
unsigned char	ExecCommand( unsigned char cmd, unsigned char len )
/* ����������������������,���ز���״̬��,��������ͷ��ز�������CMD_PARAM�ṹ�� */
{
	unsigned char		i, j, status;
	CH375HM_SPI_SCS = 0;  /* ����SPIƬѡ */
	mSpiExchange( cmd );  /* д�������� */
	mSpiExchange( len );  /* д����������ĳ��� */
	if ( len ) {  /* �в��� */
		for ( i = 0; i != len; i ++ ) mSpiExchange( mCmdParam.Other.mBuffer[ i ] );  /* ����д����� */
	}
	CH375HM_SPI_SCS = 1;  /* ����SPIƬѡ */
	mDelaymS( 1 );  /* ��ʱ��С��5uS���� */
	while ( 1 ) {  /* �������ݴ���,ֱ��������ɲ��˳� */
//		while ( CH375HM_INT_WIRE );  /* �ȴ�ģ����ɲ��������͵�ƽ�ж�,��Ѽ�ⷽʽ�Ƕ�ģ���INT#�źŽ����½��ر��ؼ�� */
		CH375HM_SPI_SCS = 0;  /* ����SPIƬѡ */
		status = mSpiExchange( 0xFF );  /* д��0xFF��Ϊ��Ч������(��Ӧ��д����ֵ),����ģ�����״̬ */
		if ( status == 0xFF ) {  /* ģ�������δ���,Ҳ����INT#û���жϲ��� */
			CH375HM_SPI_SCS = 1;  /* ����SPIƬѡ */
			mDelaymS( 1 );
			continue;  /* �����ȴ�ģ����ɲ��� */
		}
		if ( status == ERR_SUCCESS ) {  /* �����ɹ� */
			i = mSpiExchange( 0 );  /* ���ؽ�����ݵĳ���,д��0û������,�������κ�ֵ */
			if ( i ) {  /* �н������ */
				j = 0;
				do {  /* ʹ��do+while�ṹ����Ϊ��Ч�ʸ���for */
					mCmdParam.Other.mBuffer[ j ] = mSpiExchange( 0 );  /* ���ս�����ݲ����浽�����ṹ��,д��0û������ */
					j ++;
				} while ( -- i );
			}
			CH375HM_SPI_SCS = 1;  /* ����SPIƬѡ */
			break;  /* �����ɹ����� */
		}
#if 0
/* �����Ҫ������Ϊ��λ��д�ļ����ͱ��봦��USB_INT_DISK_READ/USB_INT_DISK_WRITE���ɲο�SPI5������д������ExecCommand�ӳ��� */
		else if ( status == USB_INT_DISK_READ ) {  /* ���ڴ�U�̶����ݿ�,�������ݶ��� */
			i = 64;
			do {
				*buffer = mSpiExchange( 0 );  /* ���ν���64�ֽڵ����� */
				buffer ++;  /* ���յ����ݱ��浽�ⲿ������ */
			} while ( -- i );
		}
		else if ( status == USB_INT_DISK_WRITE ) {  /* ������U��д���ݿ�,��������д�� */
			i = 64;
			do {
				mSpiExchange( *buffer );  /* ���η���64�ֽڵ����� */
				buffer ++;  /* ���͵����������ⲿ������ */
			} while ( -- i );
		}
		else if ( status == USB_INT_DISK_RETRY ) {  /* ��д���ݿ�ʧ������,Ӧ������޸Ļ�����ָ�� */
			i = mSpiExchange( 0 );  /* ���ģʽ��Ϊ�ظ�ָ���ֽ����ĸ�8λ,�����С��ģʽ��ô���յ����ǻظ�ָ���ֽ����ĵ�8λ */
			status = mSpiExchange( 0 );  /* ���ģʽ��Ϊ�ظ�ָ���ֽ����ĵ�8λ,�����С��ģʽ��ô���յ����ǻظ�ָ���ֽ����ĸ�8λ */
			buffer -= ( (unsigned short)i << 8 ) + status;  /* ���Ǵ��ģʽ�µĻظ�ָ��,����С��ģʽ,Ӧ����( (unsigned short)status << 8 ) + i */
		}
#endif
		else {  /* ����ʧ�� */
			CH375HM_SPI_SCS = 1;  /* ����SPIƬѡ */
			if ( status == ERR_DISK_DISCON || status == ERR_USB_CONNECT ) mDelaymS( 100 );  /* U�̸ո����ӻ��߶Ͽ�,Ӧ����ʱ��ʮ�����ٲ��� */
			break;  /* ����ʧ�ܷ��� */
		}
		CH375HM_SPI_SCS = 1;  /* ����SPIƬѡ */
	}
	return( status );
}

/* ������״̬,�����������ʾ������벢ͣ�� */
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
	unsigned char	i;
	unsigned short	count;
	unsigned char	*pStr;
	LED_OUT = 0;  /* ������LED��һ����ʾ���� */
	mDelaymS( 250 );  /* ��ʱ500����,CH375ģ���ϵ����Ҫ500�������ҵĸ�λʱ�� */
	mDelaymS( 250 );
	LED_OUT = 1;
	mInitSTDIO( );
	printf( "Start\n" );
	while ( 1 ) {  /* ��ѭ�� */
		printf( "Wait\n" );
		while ( 1 ) {  /* ʹ�ò�ѯ��ʽ��U���Ƿ����� */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* ��ѯ��ǰģ���״̬ */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus >= DISK_CONNECT ) break;  /* U���Ѿ����� */
			mDelaymS( 100 );  /* �����ڴ����дU��ʱ�ٲ�ѯ,û�б�Ҫһֱ������ͣ�ز�ѯ,�����õ�Ƭ����������,û�¿�������ʱ�ȴ�һ���ٲ�ѯ */
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
		strcpy( mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C" );  /* �ļ���,���ļ���C51��Ŀ¼�� */
		i = ExecCommand( CMD_FileOpen, MAX_PATH_LEN );  /* ���ļ�,���������Ϊ���ֵ,ʡ���ټ���������� */
		TempLength = 0;
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* ERR_MISS_DIR˵��û���ҵ�C51��Ŀ¼,ERR_MISS_FILE˵��û���ҵ��ļ� */
			printf( "�Ҳ���ԭ�ļ�/C51/CH375HFT.C\n" );
		}
		else {  /* �ҵ��ļ�\C51\CH375HFT.C���߳��� */
			mStopIfError( i );
			mCmdParam.ByteRead.mByteCount = 6;  /* �������6�ֽ�����, ���ζ�д�ĳ��Ȳ��ܳ��� sizeof( mCmdParam.ByteWrite.mByteBuffer ) */
			i = ExecCommand( CMD_ByteRead, 1 );  /* ���ֽ�Ϊ��λ��ȡ���� */
			mStopIfError( i );
			printf( "���ļ��ж�����ǰ6���ַ���[" );
			for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );
			printf( "]\n" );
			if ( mCmdParam.ByteRead.mByteCount<6 ) printf( "�Ѿ����ļ���ĩβ\n" );
			if ( mCmdParam.ByteRead.mByteCount==6 ) {  /* δ���ļ�ĩβ */
				mCmdParam.ByteRead.mByteCount = 20;  /* �����ٶ���20�ֽ�����, ���ζ�д�ĳ��Ȳ��ܳ��� sizeof( mCmdParam.ByteWrite.mByteBuffer ) */
				i = ExecCommand( CMD_ByteRead, 1 );  /* ���ֽ�Ϊ��λ��ȡ����,���Ÿղŵ����� */
				mStopIfError( i );
				TempLength = mCmdParam.ByteRead.mByteCount;  /* �ڶ��ζ����ֽ��� */
				memcpy( TempBuffer, mCmdParam.ByteRead.mByteBuffer, TempLength );  /* �ݴ�ڶ��ζ����������Ա�д�����ļ��� */
				printf( "���ļ��ж����ĵ�6���ַ���ʼ������[" );
				for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );
				printf( "]\n" );
				if ( mCmdParam.ByteRead.mByteCount<20 ) printf( "�Ѿ����ļ���ĩβ\n" );
			}
			printf( "Close\n" );
			mCmdParam.Close.mUpdateLen = 0;
			i = ExecCommand( CMD_FileClose, 1 );  /* �ر��ļ� */
			mStopIfError( i );
		}
/* �������ļ� */
		printf( "Create\n" );
/*		strcpy( mCmdParam.Create.mPathName, "\\NEWFILE.TXT" );*/
		strcpy( mCmdParam.Create.mPathName, "\\˫���Ұ�.TXT" );  /* ���ļ���,�ڸ�Ŀ¼�� */
		i = ExecCommand( CMD_FileCreate, MAX_PATH_LEN );  /* �½��ļ�����,����ļ��Ѿ���������ɾ�������½� */
		mStopIfError( i );
		printf( "ByteLocate\n" );
//		mCmdParam.ByteLocate.mByteOffset = 0;  /* �ƶ����ļ�ͷ,�������»ص��ļ�ͷ,�Ա�д�����ݸ���ԭ���� */
//		ExecCommand( CMD_ByteLocate, 4 );  /* ���ֽ�Ϊ��λ�ƶ��ļ�ָ�� */
//		mCmdParam.ByteLocate.mByteOffset = 0xFFFFFFFF;  /* �ƶ����ļ�β,������CMD_FileOpen���ļ���,����׷�����ݵ��Ѵ��ļ���ĩβ */
//		ExecCommand( CMD_ByteLocate, 4 );  /* ���ֽ�Ϊ��λ�ƶ��ļ�ָ�� */
		printf( "Write\n" );
		pStr = "Note: \xd\xa������������ֽ�Ϊ��λ����U���ļ���д,��Ƭ��ֻ��Ҫ�м�ʮ�ֽڵ�RAM,����Ҫ�ⲿRAM,\xd\xa���ȴ�/C51/CH375HFT.C�ļ��ж���ǰ20���ַ�,Ȼ��д����˵������һ��\xd\xa";
		count = strlen( pStr );  /* ׼��д������ݵ��ܳ��� */
		while ( count ) {  /* ����ϴ�,�ֶ��д�� */
			if ( count < sizeof( mCmdParam.ByteWrite.mByteBuffer ) ) i = count;  /* ֻʣ���һЩ����Ҫд�� */
			else i = sizeof( mCmdParam.ByteWrite.mByteBuffer );  /* ���ݽ϶�,�ֶ��д�� */
			count -= i;  /* ���� */
			memcpy( mCmdParam.ByteWrite.mByteBuffer, pStr, i );  /* ����׼��д������ݵ������ṹ��,Դ���ݿ�������ADC��,���������Գ���ռ��˵����Ϣ */
			pStr += i;
			mCmdParam.ByteWrite.mByteCount = i;  /* ָ������д����ֽ��� */
			i = ExecCommand( CMD_ByteWrite, 1+i );  /* ���ֽ�Ϊ��λ���ļ�д������ */
			mStopIfError( i );
		}
//		mCmdParam.ByteWrite.mByteCount = 0;  /* ָ��д��0�ֽ�,����ˢ���ļ��ĳ���,ע������ֽ�����Ϊ0��ôCMD_ByteWriteֻ����д�����ݶ����޸��ļ����� */
//		ExecCommand( CMD_ByteWrite, 1 );  /* ���ֽ�Ϊ��λ���ļ�д������,��Ϊ��0�ֽ�д��,����ֻ���ڸ����ļ��ĳ���,���׶���д�����ݺ�,���������ְ취�����ļ����� */
		memcpy( mCmdParam.ByteWrite.mByteBuffer, TempBuffer, TempLength );
		mCmdParam.ByteWrite.mByteCount = TempLength;  /* ��ԭ�ļ��е�20���ֽڵ��������ӵ����ļ���ĩβ */
		i = ExecCommand( CMD_ByteWrite, 1+TempLength );  /* ���ֽ�Ϊ��λ���ļ�д������ */
		mStopIfError( i );
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 1;  /* �Զ������ļ�����,�����ֽ�Ϊ��λ���ļ�д�����ݺ�,���û����0���ȵ�CMD_ByteWrite�����ļ�����,��ô�����ڹر��ļ�ʱ��ģ���Զ������ļ����� */
		i = ExecCommand( CMD_FileClose, 1 );  /* �ر��ļ�,�����ֽ�Ϊ��λ���ļ�д��(׷��)���ݺ�,�����������ļ���ر��ļ� */
		mStopIfError( i );

/* �ȴ�U�̶Ͽ� */
		printf( "Take_out\n" );
		while ( 1 ) {  /* ʹ�ò�ѯ��ʽ��U���Ƿ�Ͽ� */
			i = ExecCommand( CMD_QueryStatus, 0 );  /* ��ѯ��ǰģ���״̬ */
			mStopIfError( i );
			if ( mCmdParam.Status.mDiskStatus <= DISK_DISCONNECT ) break;  /* U���Ѿ��Ͽ� */
			mDelaymS( 100 );  /* û�б�Ҫһֱ������ͣ�ز�ѯ,�����õ�Ƭ����������,û�¿�������ʱ�ȴ�һ���ٲ�ѯ */
		}
		LED_OUT = 1;  /* LED�� */
	}
}