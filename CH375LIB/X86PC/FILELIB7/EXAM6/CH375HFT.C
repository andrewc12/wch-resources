/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC 8086/X86, Small Model    **
****************************************
*/
/* CH375 �����ļ�ϵͳ�ӿ� */
/* ֧��: FAT12/FAT16/FAT32 */

/* 8086/X86��Ƭ��C���Ե�U���ļ���дʾ������ */
/* �ó���U���е�/C51/CH375HFT.C�ļ��е�ǰ600���ַ���ʾ����,
   ����Ҳ���ԭ�ļ�CH375HFT.C, ��ô�ó�����ʾC51��Ŀ¼��������CH375��ͷ���ļ���,
   ����Ҳ���C51��Ŀ¼, ��ô�ó�����ʾ��Ŀ¼�µ������ļ���,
   ��󽫳����е�һ���ַ���д��д���½����ļ�"NEWFILE.TXT"��
*/
/* CH375��INT#���Ų��ò�ѯ��ʽ����, ���ݸ��Ʒ�ʽΪ"�ڲ�����", ������������8086���ݵ�X86��Ƭ��, �ı���ʾ��������Ϣ */

/* �������ֽ�Ϊ��λ��дU���ļ�,��д�ٶȽ�����ģʽ��,���������ֽ�ģʽ��д�ļ�����Ҫ�ļ����ݻ�����FILE_DATA_BUF,
   �����ܹ�ֻ��Ҫ600�ֽڵ�RAM,�����ڵ�Ƭ��Ӳ����Դ���ޡ�������С���Ҷ�д�ٶ�Ҫ�󲻸ߵ�ϵͳ */


/* TCC -ms -IC:\TC\INCLUDE -O -r -Z -c CH375HFT.C */
/* TLINK CH375HFT.OBJ C:\TC\LIB\C0S.OBJ, CH375HFT.EXE, , ..\CH375HF7.LIB C:\TC\LIB\CS.LIB */

#include <dos.h>
#include <string.h>
#include <stdio.h>


/* ���¶������ϸ˵���뿴CH375HF7.H�ļ� */
#define LIB_CFG_FILE_IO			1		/* �ļ���д�����ݵĸ��Ʒ�ʽ,0Ϊ"�ⲿ�ӳ���",1Ϊ"�ڲ�����" */
#define LIB_CFG_INT_EN			0		/* CH375��INT#�������ӷ�ʽ,0Ϊ"��ѯ��ʽ",1Ϊ"�жϷ�ʽ" */

/* ��Ƭ����RAM����,����CH375�ӳ�����512�ֽ�,ʣ��RAM���ֿ��������ļ���д���� */
#define DISK_BASE_BUF_LEN		4096	/* Ĭ�ϵĴ������ݻ�������СΪ512�ֽ�,����ѡ��Ϊ2048����4096��֧��ĳЩ��������U��,Ϊ0���ֹ��.H�ļ��ж��建��������Ӧ�ó�����pDISK_BASE_BUF��ָ�� */
/* #define FILE_DATA_BUF_LEN		0x0200	/* �ⲿRAM���ļ����ݻ�����,���������Ȳ�С��һ�ζ�д�����ݳ��� */
/* ���׼��ʹ��˫�����������д,��ô��Ҫ����FILE_DATA_BUF_LEN,�����ڲ�����ָ����������ַ,��CH375FileReadX����CH375FileRead,��CH375FileWriteX����CH375FileWrite */

unsigned short	mPortBaseAddr;			/* CH375EDM������ģ���I/O��ַ */

#define CH375EDM_DATA_PORT		( mPortBaseAddr + 0 )	/* CH375EDM�����ݶ˿ڵ�I/O��ַ */
#define CH375EDM_COMMAND_PORT	( mPortBaseAddr + 1 )	/* CH375EDM������˿ڵ�I/O��ַ */
#define CH375EDM_STATUS_PORT	( mPortBaseAddr + 2 )	/* CH375EDM��״̬�˿ڵ�I/O��ַ */

#define CH375_INT_WIRE			( inportb( CH375EDM_STATUS_PORT ) & 0x01 )	/* CH375EDM���ж���INT#����,���ڲ�ѯ�ж�״̬ */

#define NO_DEFAULT_CH375_F_ENUM		1		/* δ����CH375FileEnumer����ʽ�ֹ�Խ�Լ���� */
#define NO_DEFAULT_CH375_F_QUERY	1		/* δ����CH375FileQuery����ʽ�ֹ�Խ�Լ���� */

#include "..\CH375HF7.H"

/* ������CH375EDM������ģ�����ӵ�PC����ISA����PCI���� */
/* ��ISA������, I/O��ַ�ٶ�Ϊ250H */
/* ��PCI������, I/O��ַ��BIOS�Զ�����, Ҳ������CH365���ض�ַ���ܱ���ΪԭISA��ַ */

#ifndef CH365_PCI_CARD
#define CH375EDM_ISA_IO_ADDR	0x0250
#endif

void mDelay1_2uS( )  /* ������ʱ1.2uS,���ݵ�Ƭ����Ƶ���� */
{
	UINT16	i;
	for ( i = 3; i != 0; i -- ) inportb( CH375EDM_STATUS_PORT );
}

void CH375_PORT_INIT( )  /* ����ʹ��ͨ��I/Oģ�鲢�ڶ�дʱ��,���Խ��г�ʼ�� */
{
#ifdef CH365_PCI_CARD
	union	REGS	mReg;
	mReg.x.ax = 0xb102;
	mReg.x.cx = 0x5049;  /* �豸ID */
	mReg.x.dx = 0x4348;  /* ����ID */
	mReg.x.si = 0;  /* ������һ�� */
	int86 ( 0x1a, &mReg, &mReg );  /* ����PCI��BIOS */
	if ( mReg.h.ah == 0 ) {  /* ���óɹ� */
		mReg.x.ax = 0xb109;
		mReg.x.di = 0x0010;
		int86 ( 0x1a, &mReg, &mReg );  /* ����PCI��BIOS */
		mPortBaseAddr = mReg.x.cx & 0xfffe;  /* PCI�豸��I/O��ַ */
	}
	else {  /* û�м�⵽CH365 */
		printf( "CH365 PCI card not found\n" );
		exit( 1 );  /* ǿ�Ƴ�����ֹ */
		while( 1 );
	}
#else
	mPortBaseAddr = CH375EDM_ISA_IO_ADDR;  /* CH375������ģ���ISA��ַ */
#endif
}

void xWriteCH375Cmd( UINT8 mCmd )		/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375д���� */
{
	mDelay1_2uS( ); mDelay1_2uS( );  /* ������ʱ2uS */
	outportb( CH375EDM_COMMAND_PORT, mCmd );  /* ��CH375EDM������� */
	mDelay1_2uS( ); mDelay1_2uS( );  /* ������ʱ2uS */
}

void xWriteCH375Data( UINT8 mData )		/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375д���� */
{
	outportb( CH375EDM_DATA_PORT, mData );  /* ��CH375EDM������� */
	mDelay1_2uS( );  /* ������ʱ1.2uS */
}

UINT8 xReadCH375Data( void )			/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375������ */
{
	mDelay1_2uS( );  /* ������ʱ1.2uS */
	return( inportb( CH375EDM_DATA_PORT ) );  /* ��CH375EDM�������� */
}

/* ���ı���ʾ����LED���ڼ����ʾ����Ľ��� */
#define LED_OUT_INIT( )		{ putchar(7); }	/* ��ʼ�� */
#define LED_OUT_ACT( )		{ printf("LED action\n"); }	/* �͵�ƽ����LED��ʾ */
#define LED_OUT_INACT( )	{ printf("LED inaction\n"); }	/* �͵�ƽ����LED��ʾ */

/* ��ʱָ������ʱ��,���ݵ�Ƭ����Ƶ����,����ȷ */
void	mDelaymS( UINT16 ms )
{
	UINT32	i;
	while ( ms -- ) for ( i = 800; i != 0; i -- ) mDelay1_2uS( );
}

/* ������״̬,�����������ʾ������벢ͣ�� */
void	mStopIfError( UINT8 iError )
{
	if ( iError == ERR_SUCCESS ) return;  /* �����ɹ� */
	printf( "Error: %02X\n", (UINT16)iError );  /* ��ʾ���� */
	while ( 1 ) {
		LED_OUT_ACT( );  /* LED��˸ */
		mDelaymS( 100 );
		LED_OUT_INACT( );
		mDelaymS( 100 );
	}
}

int		main( ) {
	UINT8	i, c;
	UINT16	TotalCount;
	UINT8	*pCodeStr;
	CH375_PORT_INIT( );
	LED_OUT_INIT( );
	LED_OUT_ACT( );  /* ������LED��һ����ʾ���� */
	mDelaymS( 100 );  /* ��ʱ100���� */
	LED_OUT_INACT( );
	printf( "Start\n" );

#if DISK_BASE_BUF_LEN == 0
	pDISK_BASE_BUF = &my_buffer[0];  /* ����.H�ļ��ж���CH375��ר�û�����,�����û�����ָ��ָ������Ӧ�ó���Ļ��������ں����Խ�ԼRAM */
#endif

	i = CH375LibInit( );  /* ��ʼ��CH375������CH375оƬ,�����ɹ�����0 */
	mStopIfError( i );
/* ������·��ʼ�� */

	while ( 1 ) {
		printf( "Wait Udisk\n" );
		while ( CH375DiskStatus != DISK_CONNECT ) xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̲��� */
		LED_OUT_ACT( );  /* LED�� */
		mDelaymS( 200 );  /* ��ʱ,��ѡ����,�е�USB�洢����Ҫ��ʮ�������ʱ */

/* ���U���Ƿ�׼����,��ЩU�̲���Ҫ��һ��,����ĳЩU�̱���Ҫִ����һ�����ܹ��� */
		for ( i = 0; i < 10; i ++ ) {  /* �е�U�����Ƿ���δ׼����,�������Ա����� */
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
/* ��ȡԭ�ļ� */
		printf( "Open\n" );
		strcpy( (char *)mCmdParam.Open.mPathName, "/C51/CH375HFT.C" );  /* �ļ���,���ļ���C51��Ŀ¼�� */
		i = CH375FileOpen( );  /* ���ļ� */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* û���ҵ��ļ� */
/* �г��ļ� */
			if ( i == ERR_MISS_DIR ) pCodeStr = (UINT8 *)"/*";  /* C51��Ŀ¼���������г���Ŀ¼�µ��ļ� */
			else pCodeStr = (UINT8 *)"/C51/CH375*";  /* CH375HFT.C�ļ����������г�\C51��Ŀ¼�µ���CH375��ͷ���ļ� */
			printf( "List file %s\n", pCodeStr );
			for ( c = 0; c < 255; c ++ ) {  /* �������ǰ255���ļ� */
				strcpy( (char *)mCmdParam.Open.mPathName, (char *)pCodeStr );  /* �����ļ���,*Ϊͨ���,�����������ļ�������Ŀ¼ */
				i = strlen( (char const *)mCmdParam.Open.mPathName );  /* �����ļ�������,�Դ����ļ��������� */
				mCmdParam.Open.mPathName[ i ] = c;  /* �����ַ������Ƚ��������滻Ϊ���������,��0��255 */
				i = CH375FileOpen( );  /* ���ļ�,����ļ����к���ͨ���*,��Ϊ�����ļ������� */
				if ( i == ERR_MISS_FILE ) break;  /* ��Ҳ��������ƥ����ļ�,�Ѿ�û��ƥ����ļ��� */
				if ( i == ERR_FOUND_NAME ) {  /* ��������ͨ�����ƥ����ļ���,�ļ�����������·������������� */
					printf( "  match file %03d#: %s\n", (unsigned int)c, mCmdParam.Open.mPathName );  /* ��ʾ��ź���������ƥ���ļ���������Ŀ¼�� */
					continue;  /* ����������һ��ƥ����ļ���,�´�����ʱ��Ż��1 */
				}
				else {  /* ���� */
					mStopIfError( i );
					break;
				}
			}
		}
		else {  /* �ҵ��ļ����߳��� */
			mStopIfError( i );
			TotalCount = 600;  /* ׼����ȡ�ܳ��� */
			printf( "���ļ��ж�����ǰ%d���ַ���:\n",TotalCount );
			while ( TotalCount ) {  /* ����ļ��Ƚϴ�,һ�ζ�����,�����ٵ���CH375ByteRead������ȡ,�ļ�ָ���Զ�����ƶ� */
				if ( TotalCount > MAX_BYTE_IO ) c = MAX_BYTE_IO;  /* ʣ�����ݽ϶�,���Ƶ��ζ�д�ĳ��Ȳ��ܳ��� sizeof( mCmdParam.ByteRead.mByteBuffer ) */
				else c = TotalCount;  /* ���ʣ����ֽ��� */
				mCmdParam.ByteRead.mByteCount = c;  /* ���������ʮ�ֽ����� */
				i = CH375ByteRead( );  /* ���ֽ�Ϊ��λ��ȡ���ݿ�,���ζ�д�ĳ��Ȳ��ܳ���MAX_BYTE_IO,�ڶ��ε���ʱ���Ÿղŵ����� */
				mStopIfError( i );
				TotalCount -= mCmdParam.ByteRead.mByteCount;  /* ����,��ȥ��ǰʵ���Ѿ��������ַ��� */
				for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%c", mCmdParam.ByteRead.mByteBuffer[i] );  /* ��ʾ�������ַ� */
				if ( mCmdParam.ByteRead.mByteCount < c ) {  /* ʵ�ʶ������ַ�������Ҫ��������ַ���,˵���Ѿ����ļ��Ľ�β */
					printf( "\n" );
					printf( "�ļ��Ѿ�����\n" );
					break;
				}
			}
/*	    ���ϣ����ָ��λ�ÿ�ʼ��д,�����ƶ��ļ�ָ��
		mCmdParam.ByteLocate.mByteOffset = 608;  �����ļ���ǰ608���ֽڿ�ʼ��д
		CH375ByteLocate( );
		mCmdParam.ByteRead.mByteCount = 5;  ��ȡ5���ֽ�
		CH375ByteRead( );   ֱ�Ӷ�ȡ�ļ��ĵ�608���ֽڵ�612���ֽ�����,ǰ608���ֽڱ�����

	    ���ϣ�������������ӵ�ԭ�ļ���β��,�����ƶ��ļ�ָ��
		CH375FileOpen( );
		mCmdParam.ByteLocate.mByteOffset = 0xffffffff;  �Ƶ��ļ���β��
		CH375ByteLocate( );
		mCmdParam.ByteWrite.mByteCount = 13;  д��13���ֽڵ�����
		CH375ByteWrite( );   ��ԭ�ļ��ĺ�����������,�¼ӵ�13���ֽڽ���ԭ�ļ���β������
		mCmdParam.ByteWrite.mByteCount = 2;  д��2���ֽڵ�����
		CH375ByteWrite( );   ������ԭ�ļ��ĺ�����������
		mCmdParam.ByteWrite.mByteCount = 0;  д��0���ֽڵ�����,ʵ���ϸò�������֪ͨ���������ļ�����
		CH375ByteWrite( );   д��0�ֽڵ�����,�����Զ������ļ��ĳ���,�����ļ���������15,�����������,��ôִ��CH375FileCloseʱҲ���Զ������ļ�����
*/
			printf( "Close\n" );
			i = CH375FileClose( );  /* �ر��ļ� */
			mStopIfError( i );
		}

#ifdef EN_DISK_WRITE  /* �ӳ����֧��д���� */
/* �������ļ� */
		printf( "Create\n" );
		strcpy( (char *)mCmdParam.Create.mPathName, "/NEWFILE.TXT" );  /* ���ļ���,�ڸ�Ŀ¼��,�����ļ��� */
		i = CH375FileCreate( );  /* �½��ļ�����,����ļ��Ѿ���������ɾ�������½� */
		mStopIfError( i );
		printf( "Write\n" );
		pCodeStr = (UINT8 *)"Note: \xd\xa������������ֽ�Ϊ��λ����U���ļ���д,��Ƭ��ֻ��Ҫ��600�ֽڵ�RAM\xd\xa";
		while( 1 ) {  /* �ֶ��д���ļ����� */
			for ( i=0; i<MAX_BYTE_IO; i++ ) {
				c = *pCodeStr;
				mCmdParam.ByteWrite.mByteBuffer[i] = c;
				if ( c == 0 ) break;  /* Դ�ַ������� */
				pCodeStr++;
			}
			if ( i == 0 ) break;  /* Դ�ַ�������,���д�ļ� */
			mCmdParam.ByteWrite.mByteCount = i;  /* д�����ݵ��ַ���,���ζ�д�ĳ��Ȳ��ܳ���MAX_BYTE_IO,�ڶ��ε���ʱ���Ÿղŵ����д */
			i = CH375ByteWrite( );  /* ���ļ�д������ */
			mStopIfError( i );
		}
/*		printf( "Modify\n" );
		mCmdParam.Modify.mFileAttr = 0xff;   �������: �µ��ļ�����,Ϊ0FFH���޸�
		mCmdParam.Modify.mFileTime = 0xffff;   �������: �µ��ļ�ʱ��,Ϊ0FFFFH���޸�,ʹ���½��ļ�������Ĭ��ʱ��
		mCmdParam.Modify.mFileDate = MAKE_FILE_DATE( 2004, 5, 18 );  �������: �µ��ļ�����: 2004.05.18
		mCmdParam.Modify.mFileSize = 0xffffffff;   �������: �µ��ļ�����,���ֽ�Ϊ��λд�ļ�Ӧ���ɳ����ر��ļ�ʱ�Զ����³���,���Դ˴����޸�
		i = CH375FileModify( );   �޸ĵ�ǰ�ļ�����Ϣ,�޸�����
		mStopIfError( i );
*/
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 1;  /* �Զ������ļ�����,���ֽ�Ϊ��λд�ļ�,�����ó����ر��ļ��Ա��Զ������ļ����� */
		i = CH375FileClose( );
		mStopIfError( i );

/* ɾ��ĳ�ļ� */
/*		printf( "Erase\n" );
		strcpy( (char *)mCmdParam.Create.mPathName, "/OLD" );  ����ɾ�����ļ���,�ڸ�Ŀ¼��
		i = CH375FileErase( );  ɾ���ļ����ر�
		if ( i != ERR_SUCCESS ) printf( "Error: %02X\n", (UINT16)i );  ��ʾ����
*/

/* ��ѯ������Ϣ */
/*		printf( "Disk\n" );
		i = CH375DiskQuery( );
		mStopIfError( i );
		printf( "Fat=%d, Total=%ld, Free=%ld\n", (UINT16)mCmdParam.Query.mDiskFat, mCmdParam.Query.mTotalSector, mCmdParam.Query.mFreeSector );
*/
#endif
		printf( "Take out\n" );
		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̰γ� */
		LED_OUT_INACT( );  /* LED�� */
		mDelaymS( 200 );
/*		i = FILE_DATA_BUF[0];*/  /* ��Ϊ�����ֽ�Ϊ��λ��д�ļ�,δ�õ��ļ����ݻ�����,Ϊ�˷�ֹ�������Ż����û���������һ�»����� */
	}
}