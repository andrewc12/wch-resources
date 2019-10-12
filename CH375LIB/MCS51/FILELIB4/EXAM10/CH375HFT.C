/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* CH375 �����ļ�ϵͳ�ӿ� */
/* ֧��: FAT12/FAT16/FAT32 */

/* MCS-51��Ƭ��C���Ե�U���ļ���дʾ������, ������89C52���߸������ռ�ĵ�Ƭ�� */
/* ������������ʾ�����ļ�Ŀ¼��,����:�޸��ļ���,�����ļ��Ĵ������ں�ʱ��� */
/* CH375��INT#���Ų��ò�ѯ��ʽ����, ���ݸ��Ʒ�ʽΪ"��DPTR����", �����ٶȽ���, ����������MCS51��Ƭ��
   ����������V2.4�����ϰ汾��CH375�ӳ���� */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF6.LIB */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>
#include <string.h>

/* ���¶������ϸ˵���뿴CH375HF6.H�ļ� */
#define LIB_CFG_DISK_IO			1		/* ���̶�д�����ݵĸ��Ʒ�ʽ,1Ϊ"��DPTR����",2Ϊ"˫DPTR����",3Ϊ"��DPTR��P2+R0����" */
#define LIB_CFG_FILE_IO			1		/* �ļ���д�����ݵĸ��Ʒ�ʽ,0Ϊ"�ⲿ�ӳ���",1Ϊ"��DPTR����",2Ϊ"˫DPTR����",3Ϊ"��DPTR��P2+R0����" */
#define LIB_CFG_INT_EN			0		/* CH375��INT#�������ӷ�ʽ,0Ϊ"��ѯ��ʽ",1Ϊ"�жϷ�ʽ" */
/*#define LIB_CFG_FILE_IO_DEFAULT	1*/		/* ʹ��CH375HF6.H�ṩ��Ĭ��"�ⲿ�ӳ���" */
/*#define LIB_CFG_UPD_SIZE		1*/		/* ���������ݺ��Ƿ��Զ������ļ�����: 0Ϊ"������",1Ϊ"�Զ�����" */
/* Ĭ�������,���������/�ֽ�����Ϊ0��ôCH375FileWrite/CH375ByteWriteֻ����д�����ݶ����޸��ļ�����,
   �����Ҫÿ��д�����ݺ���Զ��޸�/�����ļ�����,��ô����ʹȫ�ֱ���CH375LibConfig��λ4Ϊ1,
   �����ʱ�䲻д��������Ӧ�ø����ļ�����,��ֹͻȻ�ϵ��ǰ��д����������ļ����Ȳ����,
   ���ȷ������ͻȻ�ϵ���ߺ���ܿ������ݲ���д���򲻱ظ����ļ�����,��������ٶȲ�����U�����(U���ڲ����ڴ���������,����Ƶ����д) */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375����˿ڵ�I/O��ַ */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375���ݶ˿ڵ�I/O��ַ */
/* 62256�ṩ��32KB��RAM��Ϊ������: 0000H-01FFHΪ���̶�д������, 0200H-7FFFHΪ�ļ����ݻ����� */
#define	DISK_BASE_BUF_ADDR		0x0000	/* �ⲿRAM�Ĵ������ݻ���������ʼ��ַ,�Ӹõ�Ԫ��ʼ�Ļ���������ΪSECTOR_SIZE */
#define DISK_BASE_BUF_LEN		4096	/* Ĭ�ϵĴ������ݻ�������СΪ512�ֽ�,����ѡ��Ϊ2048����4096��֧��ĳЩ��������U��,Ϊ0���ֹ��.H�ļ��ж��建��������Ӧ�ó�����pDISK_BASE_BUF��ָ�� */

#define CH375_INT_WIRE			INT0	/* P3.2, INT0, CH375���ж���INT#����,����CH375��INT#����,���ڲ�ѯ�ж�״̬ */

#define NO_DEFAULT_CH375_F_ENUM		1		/* δ����CH375FileEnumer����ʽ�ֹ�Խ�Լ���� */

#include "..\CH375HF6.H"				/* �������Ҫ֧��FAT32,��ô��ѡ��CH375HF4.H */

/* ��P1.4����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,��U�̲������ */
sbit P1_4  = P1^4;
#define LED_OUT_INIT( )		{ P1_4 = 1; }	/* P1.4 �ߵ�ƽ */
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 �͵�ƽ����LED��ʾ */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 �͵�ƽ����LED��ʾ */
sbit P1_5  = P1^5;
/* ��P1.5����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,����U�̲���ʱ�� */
#define LED_RUN_ACT( )		{ P1_5 = 0; }	/* P1.5 �͵�ƽ����LED��ʾ */
#define LED_RUN_INACT( )	{ P1_5 = 1; }	/* P1.5 �͵�ƽ����LED��ʾ */
sbit P1_6  = P1^6;
/* ��P1.6����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,����U��д����ʱ�� */
#define LED_WR_ACT( )		{ P1_6 = 0; }	/* P1.6 �͵�ƽ����LED��ʾ */
#define LED_WR_INACT( )		{ P1_6 = 1; }	/* P1.6 �͵�ƽ����LED��ʾ */

/* �Ժ���Ϊ��λ��ʱ,����ȷ,������24MHzʱ�� */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* ��24MHzʱ������ʱ500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* ��24MHzʱ������ʱ500uS */
	}
}

/* ������ռ���ַ������Ƶ��ڲ�RAM��,�����ַ������� */
UINT8	mCopyCodeStringToIRAM( UINT8 idata *iDestination, UINT8 code *iSource )
{
	UINT8	i = 0;
	while ( *iDestination = *iSource ) {
		iDestination ++;
		iSource ++;
		i ++;
	}
	return( i );
}

/* ������״̬,�����������ʾ������벢ͣ��,Ӧ���滻Ϊʵ�ʵĴ�����ʩ */
void	mStopIfError( UINT8 iError )
{
	if ( iError == ERR_SUCCESS ) return;  /* �����ɹ� */
	printf( "Error: %02X\n", (UINT16)iError );  /* ��ʾ���� */
	while ( 1 ) {
		LED_OUT_ACT( );  /* LED��˸ */
		mDelaymS( 200 );
		LED_OUT_INACT( );
		mDelaymS( 200 );
	}
}

/* Ϊprintf��getkey���������ʼ������ */
void	mInitSTDIO( )
{
	SCON = 0x50;
	PCON = 0x80;
	TMOD = 0x21;
	TH1 = 0xf3;  /* 24MHz����, 9600bps */
	TR1 = 1;
	TI = 1;
}

/* �޸�ָ���ļ����ļ���,�����C�ļ����޸�ΪTXT�ļ� */
/* �������:   ԭʼ�ļ�����mCmdParam.Open.mPathName�� */
/* ����״̬��: ERR_SUCCESS = �޸��ļ����ɹ�,
               ����״̬��ο�CH375HF?.H */
UINT8	RenameFileName( void )
{
	UINT8			i;
	P_FAT_DIR_INFO	mFileDir;
	i = CH375FileOpen( );  /* ���ļ� */
	if ( i == ERR_SUCCESS ) {
		/* �ļ���д������... */
		i = CH375FileQuery( );  /* ��ѯ�ļ�����,�Ա㽫������ݵ����ڴ������޸� */
		if ( i == ERR_SUCCESS ) {
			mFileDir = (P_FAT_DIR_INFO)( (PUINT8X)(&DISK_BASE_BUF[0]) + CH375vFdtOffset );  /* ���ڴ���,��ǰFDT����ʼ��ַ */
			if ( mFileDir -> DIR_Name[8] == 'C' && mFileDir -> DIR_Name[9] == ' ' ) {  /* �ļ���չ����C */
				mFileDir -> DIR_Name[8] = 'T';  /* �޸��ļ���չ��ΪTXT */
				mFileDir -> DIR_Name[9] = 'X';  /* ͬ�����������޸��ļ����� */
				mFileDir -> DIR_Name[10] = 'T';
			}
/* ���½��޸Ĺ������ݴ��ڴ�������ˢ�µ�U���� */
			mCmdParam.Modify.mFileAttr = mFileDir -> DIR_Attr;  /* ׼�����޸��ļ�����,ʵ�ʱ���ԭֵ */
			mCmdParam.Modify.mFileDate = mCmdParam.Modify.mFileTime = 0xFFFF;  /* ���޸��ļ����ں�ʱ�� */
			mCmdParam.Modify.mFileSize = 0xFFFFFFFF;  /* ���޸��ļ����� */
			i = CH375FileModify( );  /* ʵ���ǽ��ڴ��иո��޸Ĺ����ļ������� */
			if ( i == ERR_SUCCESS ) {
				/* �ļ���д������... */
				mCmdParam.Close.mUpdateLen = 0;
				i = CH375FileClose( );  /* �ر��ļ� */
			}
		}
	}
	return( i );
}

/* �����С�˸�ʽ�����ݴ���,�ļ�ϵͳ������С��,��MCS51�����Ǵ��,���Բ���Ҫת�� */
UINT16	SwapUINT16( UINT16 d )
{
	return( ( d << 8 ) & 0xFF00 | ( d >> 8 ) & 0xFF );
}

/* Ϊָ���ļ����ô������ں�ʱ�� */
/* �������:   ԭʼ�ļ�����mCmdParam.Open.mPathName��, �µĴ������ں�ʱ��: iCreateDate, iCreateTime */
/* ����״̬��: ERR_SUCCESS = ���óɹ�,
               ����״̬��ο�CH375HF?.H */
UINT8	SetFileCreateTime( UINT16 iCreateDate, UINT16 iCreateTime )
{
	UINT8			i;
	P_FAT_DIR_INFO	mFileDir;
	i = CH375FileOpen( );  /* ���ļ� */
	if ( i == ERR_SUCCESS ) {
		/* �ļ���д������... */
		i = CH375FileQuery( );  /* ��ѯ�ļ�����,�Ա㽫������ݵ����ڴ������޸� */
		if ( i == ERR_SUCCESS ) {
			mFileDir = (P_FAT_DIR_INFO)( (PUINT8X)(&DISK_BASE_BUF[0]) + CH375vFdtOffset );  /* ���ڴ���,��ǰFDT����ʼ��ַ */
//			mFileDir -> DIR_CrtTime = iCreateTime;  /* �ļ�������ʱ��,������С�˸�ʽ */
			mFileDir -> DIR_CrtTime = SwapUINT16( iCreateTime );  /* MCS51��Ƭ���Ǵ�˸�ʽ */
//			mFileDir -> DIR_CrtDate = iCreateDate;  /* �ļ�����������,������С�˸�ʽ */
			mFileDir -> DIR_CrtDate = SwapUINT16( iCreateDate );  /* MCS51��Ƭ���Ǵ�˸�ʽ */

//			mFileDir -> DIR_WrtTime = MAKE_FILE_TIME( ʱ, ��, �� );  /* �ļ��޸�ʱ�� */
//			mFileDir -> DIR_LstAccDate = MAKE_FILE_DATE( ��, ��, �� );  /* ���һ�δ�ȡ���������� */

/* ���½��޸Ĺ������ݴ��ڴ�������ˢ�µ�U���� */
			mCmdParam.Modify.mFileAttr = mFileDir -> DIR_Attr;  /* ׼�����޸��ļ�����,ʵ�ʱ���ԭֵ */
			mCmdParam.Modify.mFileDate = mCmdParam.Modify.mFileTime = 0xFFFF;  /* ���޸��ļ����ں�ʱ�� */
			mCmdParam.Modify.mFileSize = 0xFFFFFFFF;  /* ���޸��ļ����� */
			i = CH375FileModify( );  /* ʵ���ǽ��ڴ��иո��޸Ĺ����ļ������� */
			if ( i == ERR_SUCCESS ) {
				/* �ļ���д������... */
				mCmdParam.Close.mUpdateLen = 0;
				i = CH375FileClose( );  /* �ر��ļ� */
			}
		}
	}
	return( i );
}

main( ) {
	UINT8	i;
	LED_OUT_INIT( );
	LED_OUT_ACT( );  /* ������LED��һ����ʾ���� */
	mDelaymS( 100 );  /* ��ʱ100���� */
	LED_OUT_INACT( );
	mInitSTDIO( );  /* Ϊ���ü����ͨ�����ڼ����ʾ���� */
	printf( "Start\n" );

#if DISK_BASE_BUF_LEN == 0
	pDISK_BASE_BUF = &my_buffer[0];  /* ����.H�ļ��ж���CH375��ר�û�����,�����û�����ָ��ָ������Ӧ�ó���Ļ��������ں����Խ�ԼRAM */
#endif

	i = CH375LibInit( );  /* ��ʼ��CH375������CH375оƬ,�����ɹ�����0 */
	mStopIfError( i );
/* ������·��ʼ�� */

	while ( 1 ) {
		printf( "Wait Udisk\n" );
		while ( CH375DiskStatus < DISK_CONNECT ) {  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̲��� */
			CH375DiskConnect( );
			mDelaymS( 100 );
		}
		LED_OUT_ACT( );  /* LED�� */
		mDelaymS( 200 );  /* ��ʱ,��ѡ����,�е�USB�洢����Ҫ��ʮ�������ʱ */

/* ���U���Ƿ�׼����,��ЩU�̲���Ҫ��һ��,����ĳЩU�̱���Ҫִ����һ�����ܹ��� */
		for ( i = 0; i < 3; i ++ ) {  /* �е�U�����Ƿ���δ׼����,�������Ա����� */
			mDelaymS( 100 );
			printf( "Ready ?\n" );
			if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* ��ѯ�����Ƿ�׼���� */
		}
#if DISK_BASE_BUF_LEN
		if ( DISK_BASE_BUF_LEN < CH375vSectorSize ) {  /* ���������ݻ������Ƿ��㹻��,CH375vSectorSize��U�̵�ʵ��������С */
			printf( "Too large sector size\n" );
			while ( CH375DiskConnect( ) == ERR_SUCCESS ) mDelaymS( 100 );
			continue;
		}
#endif
/* ��ѯ������������ */
/*		printf( "DiskSize\n" );
		i = CH375DiskSize( );  
		mStopIfError( i );
		printf( "TotalSize = %u MB \n", (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * (CH375vSectorSize/512) / 2048 ) );  // ��ʾΪ��MBΪ��λ������
		// ԭ���㷽�� (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * CH375vSectorSize / 1000000 ) �п���ǰ����������˺������, �����޸ĳ���ʽ
*/
		LED_RUN_ACT( );  /* ��ʼ����U�� */

		printf( "Open and rename CH375HFT.C to CH375HFT.TXT \n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/CH375HFT.C" );  /* ԭʼ�ļ���,���ļ��ڸ�Ŀ¼�� */
		LED_WR_ACT( );  /* д���� */
		i = RenameFileName( );  /* �޸��ļ���, C�ļ� => TXT�ļ� */
		mStopIfError( i );
		printf( "Set file create date & time to 2004.06.08 15:39:20 \n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/CH375HFT.TXT" );  /* ԭʼ�ļ��� */
		i = SetFileCreateTime( MAKE_FILE_DATE( 2004, 6, 8 ), MAKE_FILE_TIME( 15, 39, 20 ) );  /* Ϊָ���ļ����ô������ں�ʱ�� */
		mStopIfError( i );
		LED_WR_INACT( );
		LED_RUN_INACT( );
		printf( "Take out\n" );
		while ( CH375DiskStatus >= DISK_CONNECT ) {  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̰γ� */
			CH375DiskConnect( );
			mDelaymS( 100 );
		}
		LED_OUT_INACT( );  /* LED�� */
		mDelaymS( 200 );
	}
}