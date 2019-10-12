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
/* ������������ʾ���U���Ƿ�д����,��ʾģ�������˵İ�ȫ�Ƴ�,Ҳ���Բο��������д����������� */
/* CH375��INT#���Ų��ò�ѯ��ʽ����, ����������MCS51��Ƭ��, ����������V2.4�����ϰ汾��CH375�ӳ����, ������CH375AоƬ */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF4.LIB    �����CH375HF4����CH375HF6�Ϳ���֧��FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>
#include <string.h>
#include <intrins.h>

/* ���¶������ϸ˵���뿴CH375HF6.H�ļ� */
#define	MAX_BYTE_IO				48		/* ���ֽ�Ϊ��λ���ζ�д�ļ�ʱ����󳤶�,Ĭ��ֵ��29,ֵ����ռ���ڴ��,ֵС�򳬹��ó��ȱ���ֶ�ζ�д */

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
#define NO_DEFAULT_CH375_F_QUERY	1		/* δ����CH375FileQuery����ʽ�ֹ�Խ�Լ���� */

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

/* ����ΪCH375Ӳ���ӿ��ӳ��� */
#ifdef __C51__  // MCS51����

#define xWriteCH375Cmd( c )		{ CH375_CMD_PORT = ( c ); _nop_( ); _nop_( ); _nop_( ); _nop_( ); }	/* д�����ʱ2uS */
#define xWriteCH375Data( c )	{ CH375_DAT_PORT = ( c ); _nop_( ); }	/* д���ݲ���ʱ1uS */
#define xReadCH375Data( )		( CH375_DAT_PORT )	/* ����������,��С����Ϊ1uS,������ʱ */

#else  // MCS51����������Ƭ����ģ�Ⲣ��

void xWriteCH375Cmd( UINT8 mCmd )		/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375д���� */
{
	mDelay1uS( ); mDelay1uS( );  /* ������ʱ1uS */
	P0 = mCmd;  /* ��CH375�Ĳ���������� */
	CH375_A0 = 1;
	CH375_CS = 0;
	CH375_WR = 0;  /* �����Чд�����ź�, дCH375оƬ������˿� */
	CH375_CS = 0;  /* �ò���������,������ʱ,CH375Ҫ���д������ȴ���50nS */
	CH375_WR = 1;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ */
	CH375_CS = 1;
	CH375_A0 = 0;
	P0 = 0xFF;  /* ��ֹ������� */
	mDelay1uS( ); mDelay1uS( );  /* ������ʱ2uS */
}

void xWriteCH375Data( UINT8 mData )		/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375д���� */
{
	P0 = mData;  /* ��CH375�Ĳ���������� */
	CH375_A0 = 0;
	CH375_CS = 0;
	CH375_WR = 0;  /* �����Чд�����ź�, дCH375оƬ�����ݶ˿� */
	CH375_CS = 0;  /* �ò���������,������ʱ,CH375Ҫ���д������ȴ���50nS */
	CH375_WR = 1;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ */
	CH375_CS = 1;
	P0 = 0xFF;  /* ��ֹ������� */
	mDelay1uS( );  /* ������ʱ1uS */
}

UINT8 xReadCH375Data( void )			/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375������ */
{
	UINT8	mData;
	mDelay1uS( );  /* ������ʱ1uS */
	P0 = 0xFF;  /* ���� */
	CH375_A0 = 0;
	CH375_CS = 0;
	CH375_RD = 0;  /* �����Чд�����ź�, ��CH375оƬ�����ݶ˿� */
	CH375_CS = 0;  /* �ò���������,������ʱ,CH375Ҫ���д������ȴ���50nS */
	mData = P0;  /* ��CH375�Ĳ����������� */
	CH375_RD = 1;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ */
	CH375_CS = 1;
	return( mData );
}

#endif

/* ��CH375�����˵�Ľ��ջ�������ȡ���ݿ�,���ض�ȡ�������ܳ��� */
UINT8	ReadUsbData( UINT8 *iBuffer )
{
	UINT8	mCount, mLength;
	xWriteCH375Cmd( CMD_RD_USB_DATA0 );  /* �ӵ�ǰUSB�жϵĶ˵㻺������ȡ���ݿ� */
	mLength = xReadCH375Data( );  /* �������ݵĳ��� */
	for ( mCount = mLength; mCount != 0; mCount -- ) {  /* ���ݳ��ȶ�ȡ���� */
		*iBuffer = xReadCH375Data( );  /* �������ݲ����� */
		iBuffer ++;
	}
	return( mLength );
}

/* ��CH375�����˵�ķ��ͻ�����д�����ݿ� */
void	WriteUsbData( UINT8 *iBuffer, UINT8 iCount )
{
	xWriteCH375Cmd( CMD_WR_USB_DATA7 );  /* ��USB�����˵�ķ��ͻ�����д�����ݿ� */
	xWriteCH375Data( iCount );  /* �������ݵĳ��� */
	for ( ; iCount != 0; iCount -- ) {  /* ���ݳ���д������ */
		xWriteCH375Data( *iBuffer );  /* ������д�� */
		iBuffer ++;
	}
}

/* ���U���Ƿ�д����, ����USB_INT_SUCCESS˵������д,����0xFF˵��ֻ��/д����,��������ֵ˵���Ǵ������ */
/* ����BulkOnly����Э���������Բο�������Ӵ���,�޸�ǰ�����˽�USB MassStorage��SCSI�淶 */
UINT8	IsDiskWriteProtect( void )
{
	UINT8	mIfSubClass, mLength, mDevSpecParam;
	if ( CH375Version2 == 0 ) return( ERR_USB_DISK_ERR );  /* CH375S��֧�� */
	CH375IntStatus = 0;  /* ���жϱ�־ */
	xWriteCH375Cmd( CMD_GET_DESCR );  /* ��ȡ������������ */
	xWriteCH375Data( 2 );  /* ���������� */
	xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬ */
	if ( CH375IntStatus == USB_INT_SUCCESS ) {  /* �����ɹ� */
		ReadUsbData( (UINT8 *)&mCmdParam );  /* ��ȡ������,��ȷ��mCmdParam����MAX_BYTE_IO����32�ֽ�,��������ѡ������ */
		mIfSubClass = mCmdParam.Other.mBuffer[9+6];  /* ���� USB_CFG_DESCR_LONG.itf_descr.bInterfaceSubClass */
		mCmdParam.BOC.mCBW.mCBW_DataLen = 0x10;  /* ���ݴ��䳤�� */
		mCmdParam.BOC.mCBW.mCBW_Flag = 0x80;  /* ���䷽��Ϊ���� */
		if ( mIfSubClass == 6 ) {  /* ��������ѡ�������� */
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[0] = 0x1A;  /* ��������ֽ�, MODE SENSE(6) */
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[1] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[2] = 0x3F;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[3] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[4] = 0x10;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[5] = 0x00;
		}
		else {
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[0] = 0x5A;  /* ��������ֽ�, MODE SENSE(10) */
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[1] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[2] = 0x3F;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[3] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[4] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[5] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[6] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[7] = 0x00;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[8] = 0x10;
			mCmdParam.BOC.mCBW.mCBW_CB_Buf[9] = 0x00;
		}
		WriteUsbData( (UINT8 *)&mCmdParam, 31 );  /* ��CH375�����˵�ķ��ͻ�����д��CBW���ݿ�,ʣ�ಿ��CH375�Զ�� */
		CH375IntStatus = 0;
		xWriteCH375Cmd( CMD_DISK_BOC_CMD );  /* ��USB�洢��ִ��BulkOnly����Э�� */
		xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬ */
		if ( CH375IntStatus == USB_INT_SUCCESS ) {  /* �����ɹ� */
			mLength = ReadUsbData( (UINT8 *)&mCmdParam );  /* ��CH375�����˵�Ľ��ջ�������ȡ���ݿ� */
			if ( mLength > 3 ) {  /* MODE SENSE��������ݵĳ�����Ч */
				if ( mIfSubClass == 6 ) mDevSpecParam = mCmdParam.Other.mBuffer[2];  /* MODE SENSE(6), device specific parameter */
				else mDevSpecParam = mCmdParam.Other.mBuffer[3];  /* MODE SENSE(10), device specific parameter */
				if ( mDevSpecParam & 0x80 ) return( 0xFF );  /* U��д���� */
				else return( USB_INT_SUCCESS );  /* U��û��д���� */
			}
			return( ERR_USB_DISK_ERR );
		}
		mIfSubClass = CH375IntStatus;  /* �ݴ� */
		xWriteCH375Cmd( CMD_DISK_R_SENSE );  /* ���USB�洢������ */
		xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬ */
		return( mIfSubClass );
	}
	return( CH375IntStatus );
}

/* ��ȫ�Ƴ�U��, ����USB_INT_SUCCESS˵�����԰�ȫ�Ƴ�,����˵�����ܰ�ȫ�Ƴ�,ֻ��ǿ���Ƴ� */
/* �ڲ�����U��׼�����û��γ�U��ǰ����, ��ֹ�û�����γ�U�̶�ʧ���� */
UINT8	SafeRemoveDisk( void )
{
	UINT8	i;
	for ( i = 0; i < 10; i ++ ) {  /* �е�U�����Ƿ���δ׼����,�������Ա����� */
		mDelaymS( 100 );
		if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* ��ѯ�����Ƿ�׼���� */
	}
	mDelaymS( 10 );
	xWriteCH375Cmd( CMD_SET_CONFIG );  /* ����USB���� */
	xWriteCH375Data( 0 );  /* ȡ������,�ᵼ�ºܶ�U�̵�LED���� */
	xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬ */
	mDelaymS( 10 );
	if ( i < 5 && CH375IntStatus == USB_INT_SUCCESS ) return( USB_INT_SUCCESS );  /* �����ɹ� */
	else return( 0xFF );
/* ����ȡ��SOF���ᵼ�¾������U�̵�LED���� */
/* ����˴�ȡ��SOF, ��ô��˶�Ӧ, �ڼ�⵽U�̲����Ӧ���ٻָ�Ϊģʽ6���ָ�SOF�� */
//	xWriteCH375Cmd( CMD_SET_USB_MODE );  /* ����USB����ģʽ */
//	xWriteCH375Data( 5 );  /* ֹͣ����SOF��,��⵽U�̲���������ģʽ6�ָ�SOF������ */
//	mDelaymS( 1 );
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

		printf( "Check Disk Write Protect ? ...\n" );
		i = IsDiskWriteProtect( );  /* ���U���Ƿ�д����, ����USB_INT_SUCCESS˵������д,����0xFF˵��ֻ��/д����,��������ֵ˵���Ǵ������ */
		if ( i != USB_INT_SUCCESS && i != 0xFF ) {  /* ����ʧ�� */
			printf( "Again ...\n" );
			mDelaymS( 100 );
			i = IsDiskWriteProtect( );  /* ����һ�� */
		}
		if ( i == USB_INT_SUCCESS ) {  /* ����д */
			printf( "... No !\n" );
			LED_WR_ACT( );  /* д���� */
			printf( "Create a File\n" );
			mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "\\NEWFILE.TXT" );  /* ���ļ���,�ڸ�Ŀ¼�� */
			i = CH375FileCreate( );  /* �½��ļ�����,����ļ��Ѿ���������ɾ�������½� */
			mStopIfError( i );
			mCmdParam.ByteWrite.mByteBuffer[0] = 'O';
			mCmdParam.ByteWrite.mByteBuffer[1] = 'K';
			mCmdParam.ByteWrite.mByteCount = 2;  /* д�����ݵ��ַ���,���ζ�д�ĳ��Ȳ��ܳ���MAX_BYTE_IO,�ڶ��ε���ʱ���Ÿղŵ����д */
			i = CH375ByteWrite( );  /* ���ļ�д������ */
			mStopIfError( i );
			printf( "Close\n" );
			mCmdParam.Close.mUpdateLen = 0;  /* ����Ҫ�Զ������ļ����� */
			i = CH375FileClose( );
			mStopIfError( i );
			LED_WR_INACT( );
			if ( SafeRemoveDisk( ) == USB_INT_SUCCESS ) {  /* ��ȫ�Ƴ�U�� */
				printf( "Safe Remove !\n" );
			}
			else {
				printf( "Unsafe Remove !\n" );
			}
		}
		else if ( i == 0xFF ) {  /* д���� */
			printf( "... Yes !\n" );
		}
		else {
			mStopIfError( i );  /* ��ʾ������� */
		}
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