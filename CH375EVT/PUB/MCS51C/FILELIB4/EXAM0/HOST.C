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

/* MCS-51��Ƭ��C���Ե�U���ļ���дʾ������, ������89C52���߸������ռ�ĵ�Ƭ��,Ҳ������ATMEL/PHILIPS/SST�Ⱦ���1KB�ڲ�RAM�ĵ�Ƭ�� */
/* �ó���U���е�/C51/CH375HFT.C�ļ��е�ǰ200���ַ���ʾ����,
   ����Ҳ���ԭ�ļ�CH375HFT.C, ��ô�ó�����ʾC51��Ŀ¼��������CH375��ͷ���ļ���,
   ����Ҳ���C51��Ŀ¼, ��ô�ó�����ʾ��Ŀ¼�µ������ļ���,
   ��󽫳���ROM�е�һ���ַ���д��д���½����ļ�"NEWFILE.TXT"��
*/
/* CH375��INT#���Ų��ò�ѯ��ʽ����, ���ݸ��Ʒ�ʽΪ"��DPTR����",��������õ����ٶ�����,
   ����ֻʹ��512�ֽڵ��ⲿRAM, ͬʱ��Ϊ�������ݻ��������ļ����ݻ�����, ��ʾû���ⲿRAM���ǵ�Ƭ��������RAM����768�ֽڵ�Ӧ�� */

/* �������ֽ�Ϊ��λ��дU���ļ�,��д�ٶȽ�����ģʽ��,���������ֽ�ģʽ��д�ļ�����Ҫ�ļ����ݻ�����FILE_DATA_BUF,
   �����ܹ�ֻ��Ҫ600�ֽڵ�RAM,�����ڵ�Ƭ��Ӳ����Դ���ޡ�������С���Ҷ�д�ٶ�Ҫ�󲻸ߵ�ϵͳ */

/*#define 	NO_DEFAULT_CH375_INT		1*/	/* ��Ӧ�ó����ж���NO_DEFAULT_CH375_INT���Խ�ֹĬ�ϵ��жϴ�������,Ȼ�������б�д�ĳ�������� */
/*#define		CH375HF_NO_CODE		1*/
#include "CH375.H"

#ifdef	NO_DEFAULT_CH375_INT			/* ���б�д�жϴ�������,�����˳�ʱ����,�����ڵȴ��жϵĹ����п����������� */
void xQueryInterrupt( void )			/* ��ѯCH375�жϲ������ж�״̬,�ó��������ܿ��Բο�CH375HF?.H�ļ� */
{
	UINT16	i;
	for ( i = 65535; i != 0; i -- ) {  /* ��������¸ù���Ϊ�����뵽��ʮ����,ż��Ҳ��ﵽ���ٺ��� */
		if ( CH375_INT_WIRE == 0 ) break;  /* ���CH375���ж���������͵�ƽ��˵��CH375������� */
/*		if ( ( CH375_CMD_PORT & 0x80 ) == 0 ) break;  ����CH375BоƬ,Ҳ��ѯCH375B������˿ڵ�λ7Ϊ0˵���ж���������͵�ƽ */
		�ڵȴ�CH375�жϵĹ�����,������Щ��Ҫ��ʱ����������
	}
	if ( i == 0 ) CH375��ʱ,ͨ����Ӳ������;
	CH375_CMD_PORT = CMD_GET_STATUS;  /* ��ȡ��ǰ�ж�״̬ */
	mDelay2uS( );  /* ����������,����������ʱ2uS,�����ö��NOP�ղ���ָ��ʵ�� */
	CH375IntStatus = CH375_DAT_PORT;  /* ��ȡ�ж�״̬ */
	if ( CH375IntStatus == USB_INT_DISCONNECT ) CH375DiskStatus = DISK_DISCONNECT;  /* ��⵽USB�豸�Ͽ��¼� */
	else if ( CH375IntStatus == USB_INT_CONNECT ) CH375DiskStatus = DISK_CONNECT;  /* ��⵽USB�豸�����¼� */
}
#endif

/* �Ժ���Ϊ��λ��ʱ,����ȷ,������24MHzʱ�� */
void	mDelaymS( UINT8 delay )
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

/* ������״̬,�����������ʾ������벢ͣ�� */
void	mStopIfError( UINT8 iError )
{
	if ( iError == ERR_SUCCESS ) return;  /* �����ɹ� */
	printf( "Error: %02X\n", (UINT16)iError );  /* ��ʾ���� */
	while ( 1 ) {
		LED_UDISK_IN( );  /* LED��˸ */
		mDelaymS( 100 );
		LED_UDISK_OUT( );
		mDelaymS( 100 );
	}
}

void host( ) {
	UINT8	i, c, TotalCount;
	UINT8	code *pCodeStr;
	UINT16	EnumCount;

#if DISK_BASE_BUF_LEN == 0
	pDISK_BASE_BUF = &my_buffer[0];  /* ����.H�ļ��ж���CH375��ר�û�����,�����û�����ָ��ָ������Ӧ�ó���Ļ��������ں����Խ�ԼRAM */
#endif

	i = CH375LibInit( );  /* ��ʼ��CH375������CH375оƬ,�����ɹ�����0 */
	mStopIfError( i );
	while ( 1 ) {
		printf( "Insert USB disk\n" );
		while ( CH375DiskStatus < DISK_CONNECT ) {  /* �ȴ�U�̲��� */
			if ( IsKeyPress( ) ) {  /* �м����� */
				printf( "Exit USB host mode\n" );
				return;
			}
/*			if ( CH375_INT_WIRE == 0 ) xQueryInterrupt( );*/  /* ���CH375�ж�,��ô��ѯCH375�жϲ������ж�״̬,���Ըĳ��жϷ�ʽ */
			mDelaymS( 100 );  /* û��ҪƵ����ѯ */
			if ( CH375DiskConnect( ) == ERR_SUCCESS ) break;	/* ��ѯ��ʽ: �������Ƿ�����,���سɹ�˵������ */
		}
		LED_UDISK_IN( );  /* LED�� */
		mDelaymS( 250 );  /* ��ʱ,��ѡ����,�е�USB�洢����Ҫ��ʮ�������ʱ */

/* ���U���Ƿ�׼����,����ĳЩU�̱���Ҫִ����һ�����ܹ��� */
		for ( i = 0; i < 5; i ++ ) {  /* �е�U�����Ƿ���δ׼����,�������Ա����� */
			mDelaymS( 100 );
			printf( "Ready ?\n" );
//			if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* ��ѯ�����Ƿ�׼����,��֧��CH375S,��Լ����ռ� */
			if ( CH375sDiskReady( ) == ERR_SUCCESS ) break;  /* ��ѯ�����Ƿ�׼����,֧��CH375S��CH375A,��ռ�ø���Ĵ���ռ� */
		}

#if DISK_BASE_BUF_LEN
		if ( DISK_BASE_BUF_LEN < CH375vSectorSize ) {  /* ���������ݻ������Ƿ��㹻��,CH375vSectorSize��U�̵�ʵ��������С */
			printf( "Too large sector size\n" );
			while ( CH375DiskConnect( ) == ERR_SUCCESS ) mDelaymS( 100 );
			continue;
		}
#endif
/* ��ȡԭ�ļ� */
		printf( "Open\n" );
		mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, "/C51/CH375HFT.C" );  /* �ļ���,���ļ���C51��Ŀ¼�� */
		i = CH375FileOpen( );  /* ���ļ� */
		if ( i == ERR_MISS_DIR || i == ERR_MISS_FILE ) {  /* û���ҵ�C51��Ŀ¼,û���ҵ�CH375HFT.C�ļ� */
/* �г��ļ� */
			if ( i == ERR_MISS_DIR ) pCodeStr = "/*";  /* C51��Ŀ¼���������г���Ŀ¼�µ������ļ� */
			else pCodeStr = "/C51/CH375*";  /* CH375HFT.C�ļ����������г�\C51��Ŀ¼�µ���CH375��ͷ���ļ� */
			printf( "List file %s\n", pCodeStr );
			for ( EnumCount = 0; EnumCount < 10000; EnumCount ++ ) {  /* �������ǰ10000���ļ�,ʵ����û������ */
				i = mCopyCodeStringToIRAM( mCmdParam.Open.mPathName, pCodeStr );  /* �����ļ���,*Ϊͨ���,�����������ļ�������Ŀ¼ */
				mCmdParam.Open.mPathName[ i ] = 0xFF;  /* �����ַ������Ƚ��������滻Ϊ���������,��0��254,�����0xFF��255��˵�����������CH375vFileSize������ */
				CH375vFileSize = EnumCount;  /* ָ������/ö�ٵ���� */
				i = CH375FileOpen( );  /* ���ļ�,����ļ����к���ͨ���*,��Ϊ�����ļ������� */
/* CH375FileEnum �� CH375FileOpen ��Ψһ�����ǵ����߷���ERR_FOUND_NAMEʱ��ô��Ӧ��ǰ�߷���ERR_SUCCESS */
				if ( i == ERR_MISS_FILE ) break;  /* ��Ҳ��������ƥ����ļ�,�Ѿ�û��ƥ����ļ��� */
				if ( i == ERR_FOUND_NAME ) {  /* ��������ͨ�����ƥ����ļ���,�ļ�����������·������������� */
					printf( "  match file %04d#: %s\n", (unsigned int)EnumCount, mCmdParam.Open.mPathName );  /* ��ʾ��ź���������ƥ���ļ���������Ŀ¼�� */
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
			TotalCount = 200;  /* ׼����ȡ�ܳ��� */
			printf( "���ļ��ж�����ǰ%d���ַ���:\n",(UINT16)TotalCount );
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
/* �������ļ�(����ԭ�ļ�����),������ԭ���ļ����������ݵ�������ο�EXAM7��EXAM8 */
		LED_WR_NOW( );  /* д���� */
		printf( "Create\n" );
		mCopyCodeStringToIRAM( mCmdParam.Create.mPathName, "/NEWFILE.TXT" );  /* ���ļ���,�ڸ�Ŀ¼��,�����ļ��� */
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
		printf( "Close\n" );
		mCmdParam.Close.mUpdateLen = 1;  /* �Զ������ļ�����,���ֽ�Ϊ��λд�ļ�,�����ó����ر��ļ��Ա��Զ������ļ����� */
		i = CH375FileClose( );
		mStopIfError( i );
		LED_NOT_WR( );
#endif

		printf( "Take out USB disk\n" );
//		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̰γ� */
		while ( CH375DiskStatus >= DISK_CONNECT ) {  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̰γ� */
			if ( CH375DiskConnect( ) != ERR_SUCCESS ) break;
			mDelaymS( 100 );
		}
		LED_UDISK_OUT( );  /* LED�� */
		mDelaymS( 100 );
	}
}