/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, IAR_C/EC++_2.10A@MSP430 **
****************************************
*/
/* CH375 �����ļ�ϵͳ�ӿ� */
/* ֧��: FAT12/FAT16/FAT32 */

/* MSP430��Ƭ��C���Ե�U���ļ���дʾ������, �����ھ��в�����600�ֽ�RAM�ĵ�Ƭ�� */
/* �ó���U���е�/C51/CH375HFT.C�ļ��е�ǰ600���ַ���ʾ����,
   ����Ҳ���ԭ�ļ�CH375HFT.C, ��ô�ó�����ʾC51��Ŀ¼��������CH375��ͷ���ļ���,
   ����Ҳ���C51��Ŀ¼, ��ô�ó�����ʾ��Ŀ¼�µ������ļ���,
   ��󽫳���ROM�е�һ���ַ���д��д���½����ļ�"NEWFILE.TXT"��
*/
/* CH375��INT#���Ų��ò�ѯ��ʽ����, ���ݸ��Ʒ�ʽΪ"�ڲ�����", ������������MSP430F449��Ƭ��, ����0��������Ϣ,9600bps */

/* �������ֽ�Ϊ��λ��дU���ļ�,��д�ٶȽ�����ģʽ��,���������ֽ�ģʽ��д�ļ�����Ҫ�ļ����ݻ�����FILE_DATA_BUF,
   �����ܹ�ֻ��Ҫ600�ֽڵ�RAM,�����ڵ�Ƭ��Ӳ����Դ���ޡ�������С���Ҷ�д�ٶ�Ҫ�󲻸ߵ�ϵͳ */


/* ICC430 CH375HFT.C -l CH375HFT.LST -o CH375HFT.R43 */
/* XLINK CH375HFT.R43 -o CH375HFT.TXT -Fmsp430_txt ..\430\lib\cl430f.r43 CH375HF8.R43 -f ..\430\config\lnk430F449.xcl */

#include <msp430x44x.h>
#include <string.h>
#include <stdio.h>

/* ���¶������ϸ˵���뿴CH375HF8.H�ļ� */
#define LIB_CFG_FILE_IO			1		/* �ļ���д�����ݵĸ��Ʒ�ʽ,0Ϊ"�ⲿ�ӳ���",1Ϊ"�ڲ�����" */
#define LIB_CFG_INT_EN			0		/* CH375��INT#�������ӷ�ʽ,0Ϊ"��ѯ��ʽ",1Ϊ"�жϷ�ʽ" */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375����˿ڵ�I/O��ַ */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375���ݶ˿ڵ�I/O��ַ */

/* ��Ƭ����RAM������: 0200H-03FFHΪ���̶�д������, ���ֽ�Ϊ��λ��д�ļ�����Ҫ�ļ����ݶ�д������FILE_DATA_BUF */
#define	DISK_BASE_BUF_ADDR		0x0200	/* �ⲿRAM�Ĵ������ݻ���������ʼ��ַ,�Ӹõ�Ԫ��ʼ�Ļ���������ΪSECTOR_SIZE */
#define FILE_DATA_BUF_ADDR		0xF000	/* �ⲿRAM���ļ����ݻ���������ʼ��ַ,���������Ȳ�С��һ�ζ�д�����ݳ���,�ֽ�ģʽ���øû����� */
/* ��Ƭ����RAM����,����CH375�ӳ�����512�ֽ�,��ʹ�Ǿ���2K����RAM�ĵ�Ƭ��,��ȥ��ջ�ͱ�����ռ��,���������Ϊ1K�ֽ� */
#define FILE_DATA_BUF_LEN		0x0200	/* �ⲿRAM���ļ����ݻ�����,���������Ȳ�С��һ�ζ�д�����ݳ���,�ֽ�ģʽ���øû����� */

#define CH375_INT_WIRE			( P1IN & 0x10 )	/* P1.4, CH375���ж���INT#����,����CH375��INT#����,���ڲ�ѯ�ж�״̬ */

#define NO_DEFAULT_CH375_F_ENUM		1		/* δ����CH375FileEnumer����ʽ�ֹ�Խ�Լ���� */
#define NO_DEFAULT_CH375_F_QUERY	1		/* δ����CH375FileQuery����ʽ�ֹ�Խ�Լ���� */

#pragma language=extended
#include "..\CH375HF8.H"
#pragma language=default

/* ����MSP430������ϵͳ����,������I/O����ģ�����CH375�Ĳ��ڶ�дʱ�� */
/* �����е�Ӳ�����ӷ�ʽ����(ʵ��Ӧ�õ�·���Բ����޸�����3�����ڶ�д�ӳ���) */
/* MSP430��Ƭ��������    CH375оƬ������
         P1.4                 INT#
         P1.3                 A0
         P1.2                 CS#
         P1.1                 WR#
         P1.0                 RD#
         P4(8λ�˿�)         D7-D0       */

void CH375_PORT_INIT( )  /* ����ʹ��ͨ��I/Oģ�鲢�ڶ�дʱ��,���Խ��г�ʼ�� */
{
	P1OUT = ( P1OUT | 0x07 ) & 0xF7;  /* ����A0Ϊ�͵�ƽ,CS,WR,RDĬ��Ϊ�ߵ�ƽ */
	P1DIR = ( P1DIR | 0x0F ) & 0xEF;  /* ����INT#Ϊ����,����CS,WR,RD,A0Ϊ��� */
	P4DIR = 0;  /* ����8λ�������� */
}

void xWriteCH375Cmd( UINT8 mCmd )		/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375д���� */
{
	_NOP( ); _NOP( ); _NOP( );  /* ������ʱ2uS,ʵ������ģ��I/O������ֻ��������ʱ */
	P1DIR |= 0x0F;  /* ����P1��A0,CS,WR,RDΪ��������ź� */
	P4OUT = mCmd;  /* ��CH375�Ĳ���������� */
	P4DIR = 0xFF;  /* д��������������� */
	P1OUT |= 0x0F;  /* ָ��CH375оƬ������˿�, A0(P1.3)=1; */
	P1OUT &= 0xF9;  /* �����Чд�����ź�, дCH375оƬ������˿�, A0(P1.3)=1; CS(P1.2)=0; WR=(P1.1)=0; RD(P1.0)=1; */
	_NOP( );  /* �ò���������,������ʱ,CH375Ҫ���д�������Ϊ100nS */
	P1OUT |= 0x07;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ, A0(P1.3)=1; CS(P1.2)=1; WR=(P1.1)=1; RD(P1.0)=1; */
	P1OUT &= 0xF7;  /* ���A0(P1.3)=0; ��ѡ���� */
	P4DIR = 0;  /* ��ֹ������� */
	_NOP( ); _NOP( ); _NOP( ); _NOP( ); _NOP( );  /* ������ʱ2uS,ʵ������ģ��I/O������ֻ��������ʱ */
}

void xWriteCH375Data( UINT8 mData )		/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375д���� */
{
	P4OUT = mData;  /* ��CH375�Ĳ���������� */
	P4DIR = 0xFF;  /* д��������������� */
	P1OUT &= 0xF1;  /* �����Чд�����ź�, дCH375оƬ�����ݶ˿�, A0(P1.3)=0; CS(P1.2)=0; WR=(P1.1)=0; RD(P1.0)=1; */
	_NOP( );  /* �ò���������,������ʱ,CH375Ҫ���д�������Ϊ100nS */
	P1OUT |= 0x07;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ, A0(P1.3)=0; CS(P1.2)=1; WR=(P1.1)=1; RD(P1.0)=1; */
	P4DIR = 0;  /* ��ֹ������� */
	_NOP( );  /* ������ʱ1uS,ʵ������ģ��I/O��������һ���б�Ҫ */
}

UINT8 xReadCH375Data( void )			/* �ⲿ����ı�CH375�������õ��ӳ���,��CH375������ */
{
	UINT8	mData;
	_NOP( );  /* ������ʱ1uS,ʵ������ģ��I/O��������һ���б�Ҫ */
	P4DIR = 0;  /* ������������������ */
	P1OUT &= 0xF2;  /* �����Ч�������ź�, ��CH375оƬ�����ݶ˿�, A0(P1.3)=0; CS(P1.2)=0; WR=(P1.1)=1; RD(P1.0)=0; */
	_NOP( );  /* �ò���������,������ʱ,CH375Ҫ���д�������Ϊ100nS */
	mData = P4IN;  /* ��CH375�Ĳ����������� */
	P1OUT |= 0x07;  /* �����Ч�Ŀ����ź�, ��ɲ���CH375оƬ, A0(P1.3)=0; CS(P1.2)=1; WR=(P1.1)=1; RD(P1.0)=1; */
	return( mData );
}

/* ��P1.7����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED�� */
#define LED_OUT_INIT( )		{ P1DIR |= 0x80; }	/* P1.7 �ߵ�ƽΪ������� */
#define LED_OUT_ACT( )		{ P1OUT &= 0x7F; }	/* P1.7 �͵�ƽ����LED��ʾ */
#define LED_OUT_INACT( )	{ P1OUT |= 0x80; }	/* P1.7 �͵�ƽ����LED��ʾ */

/* ��ʱָ������ʱ��,������8MHzʱ��,����ȷ */
#pragma optimize=none
void	mDelaymS( UINT16 ms )
{
	UINT16	i;
	while ( ms -- ) for ( i = 1000; i != 0; i -- );
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
void	mInitSTDIO( )
{
	UTCTL0 = SSEL1;                       // UCLK = SMCLK
	UBR00 = 0x41;                         // 7.99MHz 9600bps
	UBR10 = 0x03;                         // 7.99MHz 9600bps
	UMCTL0 = 0x00;                        // no modulation
	UCTL0 = CHAR;                         // 8-bit character *SWRST*
	ME1 |= UTXE0 + URXE0;                 // Enable USART0 TXD/RXD
	P2SEL |= 0x30;                        // P2.4,5 = USART0 TXD/RXD
	P2DIR |= 0x10;                        // P2.4 output direction
}

/* ͨ��������������Ϣ */
int		putchar( int c )
{
	while ( ( IFG1 & UTXIFG0 ) == 0 );    // USART0 TX buffer ready?
	TXBUF0 = c;                           // char to TXBUF0
	return( c );
}

/* ѡ��8MHzʱ�� */
void	init_clk( )
{
	WDTCTL = WDTPW + WDTHOLD;       // stop watchdog timer
	SCFI0 |= FN_4;                  // x2 DCO frequency, 8MHz nominal DCO  
	SCFQCTL = 121;                  // (121+1) x 32768 x 2 = 7.99 Mhz
	FLL_CTL0 = DCOPLUS + XCAP18PF;  // DCO+ set so freq = xtal x D x N+1
}

main( ) {
	UINT8	i, c;
	UINT16	TotalCount;
	UINT8	*pCodeStr;
	init_clk( );
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
		for ( i = 0; i < 5; i ++ ) {  /* �е�U�����Ƿ���δ׼����,�������Ա����� */
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
			if ( i == ERR_MISS_DIR ) pCodeStr = "/*";  /* C51��Ŀ¼���������г���Ŀ¼�µ��ļ� */
			else pCodeStr = "/C51/CH375*";  /* CH375HFT.C�ļ����������г�\C51��Ŀ¼�µ���CH375��ͷ���ļ� */
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
				for ( i=0; i!=mCmdParam.ByteRead.mByteCount; i++ ) printf( "%C", mCmdParam.ByteRead.mByteBuffer[i] );  /* ��ʾ�������ַ� */
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
		pCodeStr = "Note: \xd\xa������������ֽ�Ϊ��λ����U���ļ���д,��Ƭ��ֻ��Ҫ��600�ֽڵ�RAM\xd\xa";
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
		i = FILE_DATA_BUF[0];  /* ��Ϊ�����ֽ�Ϊ��λ��д�ļ�,δ�õ��ļ����ݻ�����,Ϊ�˷�ֹ�������Ż����û���������һ�»����� */
	}
}