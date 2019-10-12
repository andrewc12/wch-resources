/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, IAR_C/EC++EW23_226C@AVR **
****************************************
*/
/* CH375 �����ļ�ϵͳ�ӿ� */
/* ֧��: FAT12/FAT16/FAT32 */

/* AVR��Ƭ��C���Ե�U���ļ���дʾ������, �����ھ��в�����1KB����RAM�ĵ�Ƭ��, ���RAM��������1100�ֽ�, ��ô��Ҫ���ô��̻��������ļ������� */
/* �ó���U���е�/C51/CH375HFT.C�ļ��е�Сд��ĸת�ɴ�д��ĸ��, д���½����ļ�NEWFILE.TXT��,
   ����Ҳ���ԭ�ļ�CH375HFT.C, ��ô�ó�����ʾC51��Ŀ¼��������CH375��ͷ���ļ���, ���½�NEWFILE.TXT�ļ���д����ʾ��Ϣ,
   ����Ҳ���C51��Ŀ¼, ��ô�ó�����ʾ��Ŀ¼�µ������ļ���, ���½�NEWFILE.TXT�ļ���д����ʾ��Ϣ
*/
/* CH375��INT#���Ų��ò�ѯ��ʽ����, ���ݸ��Ʒ�ʽΪ"�ڲ�����", ������������ATmega128��Ƭ��, ����0��������Ϣ,9600bps */

/* ICCAVR CH375HFT.C -l CH375HFT.LST -o CH375HFT.R90 -v3 -ms -y --enhanced_core --initializers_in_flash */
/* XLINK CH375HFT.R90 -o CH375HFT.HEX -Fintel-extended ..\AVR\lib\cl3s-ec.r90 CH375HFJ.R90 -f ..\AVR\config\lnkm128s.xcl */

#define	ENABLE_BIT_DEFINITIONS	1
#include <iom128.h>
#include <string.h>
#include <stdio.h>

/* ���¶������ϸ˵���뿴CH375HFJ.H�ļ� */
#define LIB_CFG_FILE_IO			1		/* �ļ���д�����ݵĸ��Ʒ�ʽ,0Ϊ"�ⲿ�ӳ���",1Ϊ"�ڲ�����" */
#define LIB_CFG_INT_EN			0		/* CH375��INT#�������ӷ�ʽ,0Ϊ"��ѯ��ʽ",1Ϊ"�жϷ�ʽ" */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375����˿ڵ�I/O��ַ */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375���ݶ˿ڵ�I/O��ַ */
/* ��Ƭ����1KB��RAM��Ϊ������: 0200H-03FFHΪ���̶�д������, 0400H-05FFHΪ�ļ����ݻ����� */
#define	DISK_BASE_BUF_ADDR		0x0200	/* �ⲿRAM�Ĵ������ݻ���������ʼ��ַ,�Ӹõ�Ԫ��ʼ�Ļ���������ΪSECTOR_SIZE */
#define DISK_BASE_BUF_LEN		2048	/* Ĭ�ϵĴ������ݻ�������СΪ512�ֽ�,����ѡ��Ϊ2048����4096��֧��ĳЩ��������U��,Ϊ0���ֹ��.H�ļ��ж��建��������Ӧ�ó�����pDISK_BASE_BUF��ָ�� */
#define FILE_DATA_BUF_ADDR		0x0A00	/* �ⲿRAM���ļ����ݻ���������ʼ��ַ,���������Ȳ�С��һ�ζ�д�����ݳ��� */
/* ��Ƭ����RAM����,����CH375�ӳ�����512�ֽ�,�����ⲿRAMʣ�೤��Ϊ512�ֽ�,��ʹ�Ǿ���2K����RAM�ĵ�Ƭ��,��ȥ��ջ�ͱ�����ռ��,���������Ϊ1K�ֽ� */
#define FILE_DATA_BUF_LEN		0x0200	/* �ⲿRAM���ļ����ݻ�����,���������Ȳ�С��һ�ζ�д�����ݳ��� */
/* ���׼��ʹ��˫�����������д,��ô��Ҫ����FILE_DATA_BUF_LEN,�����ڲ�����ָ����������ַ,��CH375FileReadX����CH375FileRead,��CH375FileWriteX����CH375FileWrite */

#define CH375_INT_WIRE			( PINB & 0x10 )	/* PINB.4, CH375���ж���INT#����,����CH375��INT#����,���ڲ�ѯ�ж�״̬ */

#define NO_DEFAULT_CH375_F_ENUM		1		/* δ����CH375FileEnumer����ʽ�ֹ�Խ�Լ���� */
#define NO_DEFAULT_CH375_F_QUERY	1		/* δ����CH375FileQuery����ʽ�ֹ�Խ�Լ���� */

#pragma language=extended
#include "..\CH375HFJ.H"
#pragma language=default

/* ��ЩAVR��Ƭ���ṩ����ϵͳ����,��ôֱ�ӽ�CH375������ϵͳ������,��8λI/O��ʽ���ж�д */
/* ��ȻAtmega128�ṩϵͳ����,���������ٶ�������ϵͳ����,������I/O����ģ�����CH375�Ĳ��ڶ�дʱ�� */
/* �����е�Ӳ�����ӷ�ʽ����(ʵ��Ӧ�õ�·���Բ����޸�����3�����ڶ�д�ӳ���) */
/*    ��Ƭ��������     CH375оƬ������
       PINB.4                INT#
       PORTB.3               A0
       PORTB.2               CS#
       PORTB.1               WR#
       PORTB.0               RD#
      PORTA(8λ�˿�)        D7-D0       */

void mDelay1uS( )  /* ������ʱ1uS,���ݵ�Ƭ����Ƶ���� */
{
	UINT8	i;
	for ( i = 5; i != 0; i -- );
}

void CH375_PORT_INIT( )  /* ����ʹ��ͨ��I/Oģ�鲢�ڶ�дʱ��,���Խ��г�ʼ�� */
{
	DDRA = 0x00;  /* ����8λ����Ϊ���� */
	PORTB = 0x07;  /* ����CS,WR,RDĬ��Ϊ�ߵ�ƽ */
	DDRB = 0x0F;  /* ����CS,WR,RD,A0Ϊ���,����INT#Ϊ���� */
}

void xWriteCH375Cmd( UINT8 mCmd )		/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375д���� */
{
	mDelay1uS( ); mDelay1uS( );  /* ������ʱ1uS */
/*	*(volatile unsigned char *)CH375_CMD_PORT_ADDR = mCmd;  ͨ������ֱ�Ӷ�дCH375������ͨI/Oģ�� */
	PORTB |= 0x08;  /* ���A0=1 */
	PORTA = mCmd;  /* ��CH375�Ĳ���������� */
	DDRA = 0xFF;  /* ����D0-D7��� */
	PORTB &= 0xF9;  /* �����Чд�����ź�, дCH375оƬ������˿�, A0=1; CS=0; WR=0; RD=1; */
	DDRA = 0xFF;  /* �ò���������,������ʱ,CH375Ҫ���д������ȴ���100nS */
	PORTB |= 0x07;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ, A0=1; CS=1; WR=1; RD=1; */
	DDRA = 0x00;  /* ��ֹ������� */
	PORTB &= 0xF7;  /* ���A0=0; ��ѡ���� */
	mDelay1uS( ); mDelay1uS( );  /* ������ʱ2uS */
}

void xWriteCH375Data( UINT8 mData )		/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375д���� */
{
/*	*(volatile unsigned char *)CH375_DAT_PORT_ADDR = mData;  ͨ������ֱ�Ӷ�дCH375������ͨI/Oģ�� */
	PORTA = mData;  /* ��CH375�Ĳ���������� */
	DDRA = 0xFF;  /* ����D0-D7��� */
	PORTB &= 0xF1;  /* �����Чд�����ź�, дCH375оƬ�����ݶ˿�, A0=0; CS=0; WR=0; RD=1; */
	DDRA = 0xFF;  /* �ò���������,������ʱ,CH375Ҫ���д������ȴ���100nS */
	PORTB |= 0x07;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ, A0=0; CS=1; WR=1; RD=1; */
	DDRA = 0x00;  /* ��ֹ������� */
	mDelay1uS( );  /* ������ʱ1.2uS */
}

UINT8 xReadCH375Data( void )			/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375������ */
{
	UINT8	mData;
/*	mData = *(volatile unsigned char *)CH375_DAT_PORT_ADDR;  ͨ������ֱ�Ӷ�дCH375������ͨI/Oģ�� */
	mDelay1uS( );  /* ������ʱ1.2uS */
	DDRA = 0x00;  /* �������� */
	PORTB &= 0xF2;  /* �����Ч�������ź�, ��CH375оƬ�����ݶ˿�, A0=0; CS=0; WR=1; RD=0; */
	DDRA = 0x00;  /* �ò���������,������ʱ,CH375Ҫ���д������ȴ���100nS */
	mData = PINA;  /* ��CH375�Ĳ���PA�������� */
	PORTB |= 0x07;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ, A0=0; CS=1; WR=1; RD=1; */
	return( mData );
}

/* ��P0.2����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED�� */
#define LED_OUT_INIT( )		{ PORTB |= 0x80; DDRB |= 0x80; }	/* PORTB.7 �ߵ�ƽΪ������� */
#define LED_OUT_ACT( )		{ PORTB &= 0x7F; }	/* PORTB.7 �͵�ƽ����LED��ʾ */
#define LED_OUT_INACT( )	{ PORTB |= 0x80; }	/* PORTB.7 �͵�ƽ����LED��ʾ */

/* ��ʱָ������ʱ��,���ݵ�Ƭ����Ƶ����,����ȷ */
void	mDelaymS( UINT8 ms )
{
	UINT16	i;
	while ( ms -- ) for ( i = 2600; i != 0; i -- );
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

/* Ϊprintf��getkey���������ʼ������ */
//extern int _textmode;
void	mInitSTDIO( )
{
	UBRR0H = 0;
	UBRR0L = 103;  /* 9600bps@16MHz */
	UCSR0B = 0x18; /* BIT(RXEN) | BIT(TXEN); */
	UCSR0C = 0x06; /* BIT(UCSZ1) | BIT(UCSZ0); */
//	_textmode = 1;
}

/* ͨ��������������Ϣ */
int		putchar( int c )
{
	while ( !( UCSR0A & (1<<UDRE0)) );  /* Wait for empty transmit buffer */
	UDR0 = c;  /* Put data into buffer, sends the data */
	return( c );
}

main( ) {
	UINT8	i, c, SecCount;
	UINT16	NewSize, count;  /* ��ΪRAM��������,����NewSize����Ϊ16λ,ʵ��������ļ��ϴ�,Ӧ�÷ּ��ζ�д���ҽ�NewSize��ΪUINT32�Ա��ۼ� */
	UINT8	*pCodeStr;
	CH375_PORT_INIT( );
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
		strcpy( (char *)mCmdParam.Open.mPathName, "\\C51\\CH375HFT.C" );  /* �ļ���,���ļ���C51��Ŀ¼�� */
		i = CH375FileOpen( );  /* ���ļ� */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* û���ҵ��ļ� */
/* �г��ļ� */
			if ( i == ERR_MISS_DIR ) pCodeStr = "\\*";  /* C51��Ŀ¼���������г���Ŀ¼�µ��ļ� */
			else pCodeStr = "\\C51\\CH375*";  /* CH375HFT.C�ļ����������г�\C51��Ŀ¼�µ���CH375��ͷ���ļ� */
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
			pCodeStr = "�Ҳ���/C51/CH375HFT.C�ļ�\xd\n";
			for ( i = 0; i != 255; i ++ ) {
				if ( ( FILE_DATA_BUF[i] = *pCodeStr ) == 0 ) break;
				pCodeStr++;
			}
			NewSize = i;  /* ���ļ��ĳ��� */
			SecCount = 1;  /* (NewSize+CH375vSectorSize-1)/CH375vSectorSize, �����ļ���������,��Ϊ��д��������Ϊ��λ�� */
		}
		else {  /* �ҵ��ļ����߳��� */
			mStopIfError( i );
/*			printf( "Query\n" );
			i = CH375FileQuery( );  ��ѯ��ǰ�ļ�����Ϣ
			mStopIfError( i );*/
			printf( "Read\n" );
			if ( CH375vFileSize > FILE_DATA_BUF_LEN ) {  /* ������ʾ���õ�62256ֻ��32K�ֽ�,����CH375�ӳ�����512�ֽ�,����ֻ��ȡ������63������,Ҳ���ǲ�����32256�ֽ� */
				SecCount = FILE_DATA_BUF_LEN / CH375vSectorSize;  /* ������ʾ���õ�62256ֻ��32K�ֽ�,����CH375�ӳ�����512�ֽ�,����ֻ��ȡ������63������,Ҳ���ǲ�����32256�ֽ� */
				NewSize = FILE_DATA_BUF_LEN;  /* ����RAM�����������Ƴ��� */
			}
			else {  /* ���ԭ�ļ���С,��ôʹ��ԭ���� */
				SecCount = ( CH375vFileSize + CH375vSectorSize-1 ) / CH375vSectorSize;  /* �����ļ���������,��Ϊ��д��������Ϊ��λ��,�ȼ�CH375vSectorSize-1��Ϊ�˶����ļ�β������1�������Ĳ��� */
				NewSize = (UINT16)CH375vFileSize;  /* ԭ�ļ��ĳ��� */
			}
			printf( "Size=%ld, Len=%d, Sec=%d\n", CH375vFileSize, NewSize, (UINT16)SecCount );
			mCmdParam.Read.mSectorCount = SecCount;  /* ��ȡȫ������,�������60��������ֻ��ȡ60������ */
/*			current_buffer = & FILE_DATA_BUF[0];  ����ļ���д�����ݵĸ��Ʒ�ʽΪ"�ⲿ�ӳ���",��ô��Ҫ���ô�����ݵĻ���������ʼ��ַ */
			CH375vFileSize += CH375vSectorSize-1;  /* Ĭ�������,��������ʽ��ȡ����ʱ,�޷������ļ�β������1�������Ĳ���,���Ա�����ʱ�Ӵ��ļ������Զ�ȡβ����ͷ */
			i = CH375FileRead( );  /* ���ļ���ȡ���� */
			CH375vFileSize -= CH375vSectorSize-1;  /* �ָ�ԭ�ļ����� */
			mStopIfError( i );
/*
		����ļ��Ƚϴ�,һ�ζ�����,�����ٵ���CH375FileRead������ȡ,�ļ�ָ���Զ�����ƶ�
		while ( 1 ) {
			c = 32;   ÿ�ζ�ȡ32������
			mCmdParam.Read.mSectorCount = c;   ָ����ȡ��������
			CH375FileRead();   ������ļ�ָ���Զ�����
			��������
			if ( mCmdParam.Read.mSectorCount < c ) break;   ʵ�ʶ�������������С��˵���ļ��Ѿ�����
		}

	    ���ϣ����ָ��λ�ÿ�ʼ��д,�����ƶ��ļ�ָ��
		mCmdParam.Locate.mSectorOffset = 3;  �����ļ���ǰ3��������ʼ��д
		i = CH375FileLocate( );
		mCmdParam.Read.mSectorCount = 10;
		CH375FileRead();   ֱ�Ӷ�ȡ���ļ��ĵ�(CH375vSectorSize*3)���ֽڿ�ʼ������,ǰ3������������

	    ���ϣ�������������ӵ�ԭ�ļ���β��,�����ƶ��ļ�ָ��
		i = CH375FileOpen( );
		mCmdParam.Locate.mSectorOffset = 0xffffffff;  �Ƶ��ļ���β��,������Ϊ��λ,���ԭ�ļ���3�ֽ�,���CH375vSectorSize�ֽڿ�ʼ����
		i = CH375FileLocate( );
		mCmdParam.Write.mSectorCount = 10;
		CH375FileWrite();   ��ԭ�ļ��ĺ�����������

ʹ��CH375FileReadX�������ж������ݻ���������ʼ��ַ
		mCmdParam.ReadX.mSectorCount = 2;
		mCmdParam.ReadX.mDataBuffer = 0x2000;  �����������ݷŵ�2000H��ʼ�Ļ�������
		CH375FileReadX();   ���ļ��ж�ȡ2��������ָ��������

ʹ��CH375FileWriteX�������ж������ݻ���������ʼ��ַ
		mCmdParam.WiiteX.mSectorCount = 2;
		mCmdParam.WriteX.mDataBuffer = 0x4600;  ��4600H��ʼ�Ļ������е�����д��
		CH375FileWriteX();   ��ָ���������е�����д��2���������ļ���
*/
			printf( "Close\n" );
			i = CH375FileClose( );  /* �ر��ļ� */
			mStopIfError( i );

			i = FILE_DATA_BUF[100];
			FILE_DATA_BUF[100] = 0;  /* ���ַ���������־,�����ʾ100���ַ� */
			printf( "Line 1: %s\n", FILE_DATA_BUF );
			FILE_DATA_BUF[100] = i;  /* �ָ�ԭ�ַ� */
			for ( count=0; count < NewSize; count ++ ) {  /* ���ļ��е�Сд�ַ�ת��Ϊ��д */
				c = FILE_DATA_BUF[ count ];
				if ( c >= 'a' && c <= 'z' ) FILE_DATA_BUF[ count ] = c - ( 'a' - 'A' );
			}
		}

#ifdef EN_DISK_WRITE  /* �ӳ����֧��д���� */
/* �������ļ� */
		printf( "Create\n" );
		strcpy( (char *)mCmdParam.Create.mPathName, "\\NEWFILE.TXT" );  /* ���ļ���,�ڸ�Ŀ¼�� */
		i = CH375FileCreate( );  /* �½��ļ�����,����ļ��Ѿ���������ɾ�������½� */
		mStopIfError( i );
		printf( "Write\n" );
		mCmdParam.Write.mSectorCount = SecCount;  /* д���������������� */
/*		current_buffer = & FILE_DATA_BUF[0];  ����ļ���д�����ݵĸ��Ʒ�ʽΪ"�ⲿ�ӳ���",��ô��Ҫ���ô�����ݵĻ���������ʼ��ַ */
		i = CH375FileWrite( );  /* ���ļ�д������ */
		mStopIfError( i );
		printf( "Modify\n" );
		mCmdParam.Modify.mFileAttr = 0xff;  /* �������: �µ��ļ�����,Ϊ0FFH���޸� */
		mCmdParam.Modify.mFileTime = 0xffff;  /* �������: �µ��ļ�ʱ��,Ϊ0FFFFH���޸�,ʹ���½��ļ�������Ĭ��ʱ�� */
		mCmdParam.Modify.mFileDate = MAKE_FILE_DATE( 2004, 5, 18 );  /* �������: �µ��ļ�����: 2004.05.18 */
		mCmdParam.Modify.mFileSize = NewSize;  /* �������: ���ԭ�ļ���С,��ô�µ��ļ�������ԭ�ļ�һ����,����RAM����,����ļ����ȴ���64KB,��ôNewSize����ΪUINT32 */
		i = CH375FileModify( );  /* �޸ĵ�ǰ�ļ�����Ϣ,�޸����ںͳ��� */
		mStopIfError( i );
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 0;  /* ��Ҫ�Զ������ļ�����,����Զ�����,��ô�ó�������CH375vSectorSize�ı��� */
		i = CH375FileClose( );
		mStopIfError( i );

/* ɾ��ĳ�ļ� */
/*		printf( "Erase\n" );
		strcpy( (char *)mCmdParam.Create.mPathName, "\\OLD" );  ����ɾ�����ļ���,�ڸ�Ŀ¼��
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
	}
}