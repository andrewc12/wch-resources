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
/* ���ļ�������ʾ��, �����������ļ����ʹӶ��ļ�����ö�Ӧ�ĳ��ļ���
   ע��: CH375HF?.Hͷ�ļ����������޸�, ��mCmdParam���Ͻṹ������ReadBlock��WriteBlock�Ĳ����ṹ
*/
/* CH375��INT#���Ų��ò�ѯ��ʽ����, ���ݸ��Ʒ�ʽΪ"��DPTR����", �����ٶȽ���, ����������MCS51��Ƭ�� */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF6.LIB    �����CH375HF6����CH375HF4�Ϳ��Բ�֧��FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>
#include <string.h>

/* ���¶������ϸ˵���뿴CH375HF6.H�ļ� */
#define LIB_CFG_DISK_IO         1       /* ���̶�д�����ݵĸ��Ʒ�ʽ,1Ϊ"��DPTR����",2Ϊ"˫DPTR����",3Ϊ"��DPTR��P2+R0����" */
#define LIB_CFG_FILE_IO         1       /* �ļ���д�����ݵĸ��Ʒ�ʽ,0Ϊ"�ⲿ�ӳ���",1Ϊ"��DPTR����",2Ϊ"˫DPTR����",3Ϊ"��DPTR��P2+R0����" */
#define LIB_CFG_INT_EN          0       /* CH375��INT#�������ӷ�ʽ,0Ϊ"��ѯ��ʽ",1Ϊ"�жϷ�ʽ" */

#define CH375_CMD_PORT_ADDR     0xBDF1  /* CH375����˿ڵ�I/O��ַ */
#define CH375_DAT_PORT_ADDR     0xBCF0  /* CH375���ݶ˿ڵ�I/O��ַ */
/* 62256�ṩ��32KB��RAM��Ϊ������: 0000H-01FFHΪ���̶�д������, 0200H-7FFFHΪ�ļ����ݻ����� */
#define DISK_BASE_BUF_ADDR      0x0000  /* �ⲿRAM�Ĵ������ݻ���������ʼ��ַ,�Ӹõ�Ԫ��ʼ�Ļ���������ΪSECTOR_SIZE */
#define DISK_BASE_BUF_LEN       4096    /* Ĭ�ϵĴ������ݻ�������СΪ512�ֽ�,����ѡ��Ϊ2048����4096��֧��ĳЩ��������U��,Ϊ0���ֹ��.H�ļ��ж��建��������Ӧ�ó�����pDISK_BASE_BUF��ָ�� */
#define FILE_DATA_BUF_ADDR      0x1000  /* �ⲿRAM���ļ����ݻ���������ʼ��ַ,���������Ȳ�С��һ�ζ�д�����ݳ��� */
/* ������ʾ���õ�62256ֻ��32K�ֽ�,����CH375�ӳ�����512�ֽ�,�����ⲿRAMʣ�೤��Ϊ32256�ֽ� */
#define FILE_DATA_BUF_LEN       0x6800  /* �ⲿRAM���ļ����ݻ�����,���������Ȳ�С��һ�ζ�д�����ݳ��� */
/* ���׼��ʹ��˫�����������д,��ô��Ҫ����FILE_DATA_BUF_LEN,�����ڲ�����ָ����������ַ,��CH375FileReadX����CH375FileRead,��CH375FileWriteX����CH375FileWrite */

#define CH375_INT_WIRE          INT0    /* P3.2, INT0, CH375���ж���INT#����,����CH375��INT#����,���ڲ�ѯ�ж�״̬ */

#define NO_DEFAULT_CH375_F_ENUM     1       /* δ����CH375FileEnumer����ʽ�ֹ�Խ�Լ���� */
#define NO_DEFAULT_CH375_F_QUERY    1       /* δ����CH375FileQuery����ʽ�ֹ�Խ�Լ���� */

#include "CH375HF6.H"               /* �������Ҫ֧��FAT32,��ô��ѡ��CH375HF4.H */

/* ��P1.4����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,��U�̲������ */
sbit P1_4  = P1^4;
#define LED_OUT_INIT( )     { P1_4 = 1; }   /* P1.4 �ߵ�ƽ */
#define LED_OUT_ACT( )      { P1_4 = 0; }   /* P1.4 �͵�ƽ����LED��ʾ */
#define LED_OUT_INACT( )    { P1_4 = 1; }   /* P1.4 �͵�ƽ����LED��ʾ */
sbit P1_5  = P1^5;
/* ��P1.5����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,����U�̲���ʱ�� */
#define LED_RUN_ACT( )      { P1_5 = 0; }   /* P1.5 �͵�ƽ����LED��ʾ */
#define LED_RUN_INACT( )    { P1_5 = 1; }   /* P1.5 �͵�ƽ����LED��ʾ */
sbit P1_6  = P1^6;
/* ��P1.6����һ��LED���ڼ����ʾ����Ľ���,�͵�ƽLED��,����U��д����ʱ�� */
#define LED_WR_ACT( )       { P1_6 = 0; }   /* P1.6 �͵�ƽ����LED��ʾ */
#define LED_WR_INACT( )     { P1_6 = 1; }   /* P1.6 �͵�ƽ����LED��ʾ */

/* ��ʱ100����,����ȷ */
void    mDelay100mS( )
{
    UINT8   i, j, c;
    for ( i = 200; i != 0; i -- ) for ( j = 200; j != 0; j -- ) c+=3;
}

/* ������ռ���ַ������Ƶ��ڲ�RAM��,�����ַ������� */
UINT8   mCopyCodeStringToIRAM( UINT8 idata *iDestination, UINT8 code *iSource )
{
    UINT8   i = 0;
    while ( *iDestination = *iSource ) {
        iDestination ++;
        iSource ++;
        i ++;
    }
    return( i );
}

/* ������״̬,�����������ʾ������벢ͣ�� */
void    mStopIfError( UINT8 iError )
{
    if ( iError == ERR_SUCCESS ) return;  /* �����ɹ� */
    printf( "Error: %02X\n", (UINT16)iError );  /* ��ʾ���� */
    while ( 1 ) {
        LED_OUT_ACT( );  /* LED��˸ */
        mDelay100mS( );
        LED_OUT_INACT( );
        mDelay100mS( );
    }
}

/* Ϊprintf��getkey���������ʼ������ */
void    mInitSTDIO( )
{
    SCON = 0x50;
    PCON = 0x80;
    TMOD = 0x20;
    TH1 = 0xf3;  /* 24MHz����, 9600bps */
    TR1 = 1;
    TI = 1;
}

/*====================���ļ��������ĺ궨�弫��ȫ�ֱ��� =======================*/
// ���ļ�����������(0��20)*26
#define     LONG_NAME_BUF_LEN       (20*26)
#define     UNICODE_ENDIAN          1           // 1ΪUNICDOE��˱��� 0ΪС��
// ���ļ�����Ż�����(Unicode����)
UINT8   xdata LongNameBuf[ LONG_NAME_BUF_LEN ];

#define     TRUE        1
#define     FALSE       0

// ��������
#define     ERR_NO_NAME             0X44        // �˶��ļ���û�г��ļ��������ĳ��ļ�
#define     ERR_BUF_OVER            0X45        // ���ļ����������
#define     ERR_LONG_NAME           0X46        // ����ĳ��ļ���
#define     ERR_NAME_EXIST          0X47        // �˶��ļ�������
/*============================================================================*/

/*==============================================================================
������: CheckNameSum

��������: ��鳤�ļ����Ķ��ļ��������

==============================================================================*/
UINT8 CheckNameSum( UINT8 *p )
{
UINT8 FcbNameLen;
UINT8 Sum;

    Sum = 0;
    for (FcbNameLen=0; FcbNameLen!=11; FcbNameLen++)
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *p++;
    return Sum;
}

/*==============================================================================
������: AnalyzeLongName

��������: �������ļ��� �����м�����26����

==============================================================================*/
UINT8 AnalyzeLongName( void )
{
UINT8   i, j;
UINT16  index;

    i = FALSE;
    for( index=0; index!=LONG_NAME_BUF_LEN; index = index + 2 )
    {
        if( ( LongNameBuf[index] == 0 ) && ( LongNameBuf[index+1] == 0 ) )
        {
            i = TRUE;
            break;
        }
    }
    if( ( i == FALSE ) || ( index == 0) )
        return 0;                   // ����0��ʾ����ĳ��ļ���

    i = index % 26;
    if( i != 0 )
    {
        index += 2;
        if( index % 26 != 0 )       // ��0�պý���
        {
            for( j=i+2; j!=26; j++ )// ��ʣ��������Ϊ0XFF
                LongNameBuf[index++] = 0xff;
        }
    }
    return  (index / 26);
}

/*==============================================================================
������: CH375CreateLongName

��������: �������ļ���,��Ҫ������ļ���������·��

==============================================================================*/
UINT8 CH375CreateLongName( void )
{
// ���� �����ļ�·�� ����һ�����ļ� �õ�FDTƫ�ƺ����������� ɾ���ļ�
// ���ƫ������ ����ʧ�� ��FAT12/16���ڸ�Ŀ¼�� �����Ϻ��ٴδ����ļ�
UINT8   i;
UINT8   len;                                // ���·���ĳ���
UINT16  index;                              // ���ļ�ƫ������
UINT16  indexBak;                           // ���ļ�ƫ����������
UINT32  Secoffset;                          // ����ƫ��

UINT8   Fbit;                               // ��һ�ν���д����
UINT8   Mult;                               // ���ļ�������26�ı���
UINT8   MultBak;                            // ���ļ�������26�ı�������

UINT16  Backoffset;                         // �����ļ�ƫ�Ʊ���
UINT16  BackoffsetBak;                      // ����ƫ�Ʊ��ݵı���
UINT32  BackFdtSector;                      // ����ƫ����һ������
UINT8   sum;                                // ���泤�ļ�����У���

UINT8   xdata BackPathBuf[MAX_PATH_LEN];    // �����ļ�·��

    Mult = AnalyzeLongName( );              // ���泤�ļ�����26�ı���
    if( Mult == 0 )
        return ERR_LONG_NAME;
    MultBak = Mult;

    i = CH375FileOpen();                    // ���ļ��������򷵻ش���
    if( i == ERR_SUCCESS )
        return ERR_NAME_EXIST;

    i = CH375FileCreate( );
    if( i == ERR_SUCCESS )
    {
        Backoffset = CH375vFdtOffset;
        BackoffsetBak = Backoffset;
        BackFdtSector = CH375vFdtLba;
        sum = CheckNameSum( &DISK_BASE_BUF[Backoffset ] );
        for( i=0; i!=MAX_PATH_LEN; i++ )    // ���ļ�·�����б���
            BackPathBuf[i] = mCmdParam.Open.mPathName[i];
        CH375FileErase( );                  // ɾ�����ļ�

        Secoffset   = 0;                    // ��0��ʼƫ��
        index       = Mult*26;              // �õ����ļ����ĳ���
        indexBak    = index;
        Fbit        = FALSE;                // Ĭ��û�н���
        // ���ϼ� ���������������
        P_RETRY:
        for(len=0; len!=MAX_PATH_LEN; len++)
        {
            if(mCmdParam.Open.mPathName[len] == 0)
                break;                      // �õ��ַ�������
        }

        for(i=len-1; i!=0xff; i--)          // �õ��ϼ�Ŀ¼λ��
        {
            if((mCmdParam.Open.mPathName[i] == '\\') || (mCmdParam.Open.mPathName[i] == '/'))
                break;
        }
        mCmdParam.Open.mPathName[i] = 0x00;

        if( i==0 )                          // ��һ��Ŀ¼ע��:���ڸ�Ŀ¼��ʼ���������
        {
            mCmdParam.Open.mPathName[0] = '/';
            mCmdParam.Open.mPathName[1] = 0;
        }

        i = CH375FileOpen();                // ���ϼ�Ŀ¼
        if( i == ERR_OPEN_DIR )
        {
            while( 1 )                      // ѭ����д ֱ�����
            {
                mCmdParam.Locate.mSectorOffset = Secoffset;
                i = CH375FileLocate( );
                if( i == ERR_SUCCESS )
                {
                    if( Fbit )             // �ڶ��ν����д����
                    {
                        if( mCmdParam.Locate.mSectorOffset != 0x0FFFFFFFF )
                        {
                            BackFdtSector = mCmdParam.Locate.mSectorOffset;
                            Backoffset = 0;
                        }
                        else
                        {
                            for( i=0; i!=MAX_PATH_LEN; i++ )// ��ԭ�ļ�·��
                                mCmdParam.Open.mPathName[i] = BackPathBuf[i];
                            i = CH375FileCreate( );         // ���пռ���չ
                            if( i != ERR_SUCCESS )
                                return i;
                            CH375FileErase( );
                            goto P_RETRY;                   // ���´��ϼ�Ŀ¼
                        }
                    }

                    if( BackFdtSector == mCmdParam.Locate.mSectorOffset )
                    {
                        mCmdParam.ReadX.mSectorCount = 1;   // ��һ�����������̻�����
                        mCmdParam.ReadX.mDataBuffer = &DISK_BASE_BUF[0];
                        i = CH375FileReadX( );
                        CH375DirtyBuffer( );                // ������̻�����
                        if( i!= ERR_SUCCESS )
                            return i;

                        i = ( CH375vSectorSize - Backoffset ) / 32;
                        if( Mult > i )
                            Mult = Mult - i;                // ʣ��ı���
                        else
                        {
                            i = Mult;
                            Mult = 0;
                        }

                        for( len=i; len!=0; len-- )
                        {
                            indexBak -= 26;
                            index = indexBak;
                            for( i=0; i!=5; i++)            // ���ļ�����1-5���ַ�
                            {                               // �ڴ�����UNICODE��С�˷�ʽ���
                                #if UNICODE_ENDIAN == 1
                                DISK_BASE_BUF[Backoffset + i*2 + 2 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                #else
                                DISK_BASE_BUF[Backoffset + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + i*2 + 2 ] =
                                    LongNameBuf[index++];
                                #endif
                            }

                            for( i =0; i!=6; i++)           // ���ļ�����6-11���ַ�
                            {
                                #if UNICODE_ENDIAN == 1
                                DISK_BASE_BUF[Backoffset + 14 + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + 14 + i*2 ] =
                                    LongNameBuf[index++];
                                #else
                                DISK_BASE_BUF[Backoffset + 14 + i*2 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + 14 + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                #endif
                            }

                            for( i=0; i!=2; i++)            // ���ļ�����12-13���ַ�
                            {
                                #if UNICODE_ENDIAN == 1
                                DISK_BASE_BUF[Backoffset + 28 + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + 28 + i*2 ] =
                                    LongNameBuf[index++];
                                #else
                                DISK_BASE_BUF[Backoffset + 28 + i*2 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + 28 + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                #endif
                            }

                            DISK_BASE_BUF[Backoffset + 0x0b] = 0x0f;
                            DISK_BASE_BUF[Backoffset + 0x0c] = 0x00;
                            DISK_BASE_BUF[Backoffset + 0x0d] = sum;
                            DISK_BASE_BUF[Backoffset + 0x1a] = 0x00;
                            DISK_BASE_BUF[Backoffset + 0x1b] = 0x00;
                            DISK_BASE_BUF[Backoffset] = MultBak--;
                            Backoffset += 32;
                        }

                        if( !Fbit )
                        {
                            Fbit = TRUE;
                            DISK_BASE_BUF[ BackoffsetBak ] |= 0x40;
                        }

                        mCmdParam.WriteB.mLbaCount = 1;
                        mCmdParam.WriteB.mLbaStart = BackFdtSector;
                        mCmdParam.WriteB.mDataBuffer = DISK_BASE_BUF;
                        i = CH375WriteBlock();
                        if( i!= ERR_SUCCESS )
                            return i;

                        if( Mult==0 )
                        {                               // ��ԭ�ļ�·��
                            for( i=0; i!=MAX_PATH_LEN; i++ )
                                mCmdParam.Open.mPathName[i] = BackPathBuf[i];
                            i = CH375FileCreate( );
                            return i;
                        }
                    }
                }
                else
                    return i;
                Secoffset++;
            }
        }
    }
    return i;
}

/*==============================================================================

������: GetUpSectorData

��������: �ɵ�ǰ�����õ���һ�����������ݣ����ڴ��̻�����

==============================================================================*/
UINT8 GetUpSectorData( UINT32 *NowSector )
{
UINT8  i;
UINT8  len;             // ���·���ĳ���
UINT32 index;           // Ŀ¼����ƫ��������

    index = 0;
    for(len=0; len!=MAX_PATH_LEN; len++)
    {
        if(mCmdParam.Open.mPathName[len] == 0)          // �õ��ַ�������
            break;
    }

    for(i=len-1; i!=0xff; i--)                          // �õ��ϼ�Ŀ¼λ��
    {
        if((mCmdParam.Open.mPathName[i] == '\\') || (mCmdParam.Open.mPathName[i] == '/'))
            break;
    }
    mCmdParam.Open.mPathName[i] = 0x00;

    if( i==0 )  // ��һ��Ŀ¼ע��:���ڸ�Ŀ¼��ʼ���������
    {
        mCmdParam.Open.mPathName[0] = '/';
        mCmdParam.Open.mPathName[1] = 0;
        i = CH375FileOpen();
        if ( i == ERR_OPEN_DIR )
            goto P_NEXT0;
    }
    else
    {
        i = CH375FileOpen();
        if ( i == ERR_OPEN_DIR )
        {
            while( 1 )
            {
                P_NEXT0:
                mCmdParam.Locate.mSectorOffset = index;
                i = CH375FileLocate( );
                if( i == ERR_SUCCESS )
                {
                    if( *NowSector == mCmdParam.Locate.mSectorOffset )
                    {
                        if( index==0 )                          // ���ڸ�Ŀ¼�����Ŀ�ʼ
                            return ERR_NO_NAME;
                        mCmdParam.Locate.mSectorOffset = --index;
                        i = CH375FileLocate( );                 // ����һ������������
                        if( i == ERR_SUCCESS )
                        {                                       // ���±��浱ǰ����������
                            *NowSector = mCmdParam.Locate.mSectorOffset;
                            mCmdParam.ReadX.mSectorCount = 1;   // ��һ�����������̻�����
                            mCmdParam.ReadX.mDataBuffer = &DISK_BASE_BUF[0];
                            i = CH375FileReadX( );
                            CH375DirtyBuffer( );                // ������̻�����
                            return i;
                        }
                        else
                            return i;
                    }
                }
                else
                    return i;
                index++;
            }
        }
    }
    return i;
}

/*==============================================================================

������: CH375GetLongName

��������: ���������ļ���·��(�������ļ����ļ���)�õ���Ӧ�ĳ��ļ���

==============================================================================*/
UINT8 CH375GetLongName( void )
{
// ��Ҫ����������С
// ��һ�������ļ��Ƿ��ҵ��ļ�,�����ļ��Ƿ����,���õ�FDT�ڴ�������ƫ�ƺ���������
// �ڶ����������������Ϣ���Ƿ��г��ļ������ڣ��Ƿ���Ŀ¼�ĵ�һ�������Ŀ�ʼ
// ��������ʵ�����ƫ��һ������?��ȡ���ļ���(����512�ֽڵ�U��)
UINT8   i;
UINT16  index;          // �ڳ��ļ����������ڵ�����
UINT32  BackFdtSector;  // ����ƫ����һ������
UINT8   sum;            // ���泤�ļ�����У���
UINT16  Backoffset;     // �����ļ�ƫ�Ʊ���
UINT16  offset;         // �������ļ�ƫ��32����
UINT8   FirstBit;       // ���ļ�����Խ����������־λ
UINT8   xdata BackPathBuf[MAX_PATH_LEN]; // �����ļ�·��

    i = CH375FileOpen( );
    if( ( i == ERR_SUCCESS ) || ( i == ERR_OPEN_DIR ) )
    {
        for( i=0; i!=MAX_PATH_LEN; i++ )
            BackPathBuf[i] = mCmdParam.Open.mPathName[i];
        // ������ɶ�·���ı���

        sum = CheckNameSum( &DISK_BASE_BUF[CH375vFdtOffset] );
        index = 0;
        FirstBit = FALSE;
        Backoffset = CH375vFdtOffset;
        BackFdtSector = CH375vFdtLba;
        if( CH375vFdtOffset == 0 )
        {
            // ���ж��Ƿ���һ��������ʼ �Ƿ��ڸ�Ŀ¼��ʼ ���������ƫ��
            if( FirstBit == FALSE )
                FirstBit = TRUE;
            i = GetUpSectorData( &BackFdtSector );
            if( i == ERR_SUCCESS )
            {
                CH375vFdtOffset = CH375vSectorSize;
                goto P_NEXT1;
            }
        }
        else
        {
            // ��ȡƫ�ƺ�����ݣ�ֱ��������������������ƫ��
            P_NEXT1:
            offset = CH375vFdtOffset;
            while( 1 )
            {
                if( offset != 0 )
                {
                    offset = offset - 32;
                    if( ( DISK_BASE_BUF[offset + 11] == ATTR_LONG_NAME )
                        && (  DISK_BASE_BUF[offset + 13] == sum ) )
                    {
                        if( (index + 26) > LONG_NAME_BUF_LEN )
                            return ERR_BUF_OVER;

                        for( i=0; i!=5; i++)            // ���ļ�����1-5���ַ�
                        {                               // �ڴ�����UNICODE��С�˷�ʽ���
                            #if UNICODE_ENDIAN == 1
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + i*2 + 2];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + i*2 + 1];
                            #else
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + i*2 + 1];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + i*2 + 2];
                            #endif
                        }

                        for( i =0; i!=6; i++)           // ���ļ�����6-11���ַ�
                        {
                            #if UNICODE_ENDIAN == 1
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 14 + i*2 + 1];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + + 14 + i*2 ];
                            #else
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + + 14 + i*2 ];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 14 + i*2 + 1];
                            #endif

                        }

                        for( i=0; i!=2; i++)            // ���ļ�����12-13���ַ�
                        {
                            #if UNICODE_ENDIAN == 1
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 28 + i*2 + 1];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 28 + i*2 ];
                            #else
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 28 + i*2 ];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 28 + i*2 + 1];
                            #endif
                        }

                        if( DISK_BASE_BUF[offset] & 0X40 )
                        {
                            if( ! (((LongNameBuf[index -1] ==0x00)
                                && (LongNameBuf[index -2] ==0x00))
                                || ((LongNameBuf[index -1] ==0xFF)
                                && (LongNameBuf[index -2 ] ==0xFF))))
                            {                           // �����պ�Ϊ26�ֽڳ��������ļ���
                                if(index + 52 >LONG_NAME_BUF_LEN )
                                    return ERR_BUF_OVER;
                                LongNameBuf[ index ] = 0x00;
                                LongNameBuf[ index + 1] = 0x00;
                            }
                            return ERR_SUCCESS;         // �ɹ���ɳ��ļ����ռ����
                        }
                    }
                    else
                        return ERR_NO_NAME;             // ����ĳ��ļ���,���򷵻�
                }
                else
                {
                    if( FirstBit == FALSE )
                        FirstBit = TRUE;
                    else                                // ����ڶ��ν���
                    {
                        for( i=0; i!=MAX_PATH_LEN; i++ )// ��ԭ·��
                            mCmdParam.Open.mPathName[i] = BackPathBuf[i];
                    }
                    i = GetUpSectorData( &BackFdtSector );
                    if( i == ERR_SUCCESS )
                    {
                        CH375vFdtOffset = CH375vSectorSize;
                        goto P_NEXT1;
                    }
                    else
                        return i;
                    // ���ƫ������
                }
            }
        }
    }
    return i;                // ���ش���
}

/*
���ļ���ʾ��(UNICODE����Ĵ�С�� ������UNICODE_ENDIAN������ͬ)
������LongName���������:
�������ļ������������������� 1.����(unicode ���)���ַ���ĩβ������0��ʾ����;2.ANSI������ļ���.TXT
*/
UINT8 code LongName[] =
#if UNICODE_ENDIAN ==1
{
    0x5E, 0xFA, 0x7A, 0xCB, 0x95, 0x7F, 0x65, 0x87, 0x4E, 0xF6, 0x54, 0x0D, 0xFF, 0x0C, 0x8F, 0x93,
    0x51, 0x65, 0x4E, 0x24, 0x4E, 0x2A, 0x53, 0xC2, 0x65, 0x70, 0xFF, 0x1A, 0x00, 0x20, 0x00, 0x31,
    0x00, 0x2E, 0x91, 0xC7, 0x75, 0x28, 0x00, 0x28, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x69, 0x00, 0x63,
    0x00, 0x6F, 0x00, 0x64, 0x00, 0x65, 0x00, 0x20, 0x59, 0x27, 0x7A, 0xEF, 0x00, 0x29, 0xFF, 0x0C,
    0x5B, 0x57, 0x7B, 0x26, 0x4E, 0x32, 0x67, 0x2B, 0x5C, 0x3E, 0x75, 0x28, 0x4E, 0x24, 0x4E, 0x2A,
    0x00, 0x30, 0x88, 0x68, 0x79, 0x3A, 0x7E, 0xD3, 0x67, 0x5F, 0x00, 0x3B, 0x00, 0x32, 0x00, 0x2E,
    0x00, 0x41, 0x00, 0x4E, 0x00, 0x53, 0x00, 0x49, 0x7F, 0x16, 0x78, 0x01, 0x77, 0xED, 0x65, 0x87,
    0x4E, 0xF6, 0x54, 0x0D, 0x00, 0x2E, 0x00, 0x54, 0x00, 0x58, 0x00, 0x54
};
#else
{
    0xFA, 0x5E, 0xCB, 0x7A, 0x7F, 0x95, 0x87, 0x65, 0xF6, 0x4E, 0x0D, 0x54, 0x0C, 0xFF, 0x93, 0x8F,
    0x65, 0x51, 0x24, 0x4E, 0x2A, 0x4E, 0xC2, 0x53, 0x70, 0x65, 0x1A, 0xFF, 0x20, 0x00, 0x31, 0x00,
    0x2E, 0x00, 0xC7, 0x91, 0x28, 0x75, 0x28, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x69, 0x00, 0x63, 0x00,
    0x6F, 0x00, 0x64, 0x00, 0x65, 0x00, 0x20, 0x00, 0x27, 0x59, 0xEF, 0x7A, 0x29, 0x00, 0x0C, 0xFF,
    0x57, 0x5B, 0x26, 0x7B, 0x32, 0x4E, 0x2B, 0x67, 0x3E, 0x5C, 0x28, 0x75, 0x24, 0x4E, 0x2A, 0x4E,
    0x30, 0x00, 0x68, 0x88, 0x3A, 0x79, 0xD3, 0x7E, 0x5F, 0x67, 0x3B, 0x00, 0x32, 0x00, 0x2E, 0x00,
    0x41, 0x00, 0x4E, 0x00, 0x53, 0x00, 0x49, 0x00, 0x16, 0x7F, 0x01, 0x78, 0xED, 0x77, 0x87, 0x65,
    0xF6, 0x4E, 0x0D, 0x54, 0x2E, 0x00, 0x54, 0x00, 0x58, 0x00, 0x54, 0x00
};
#endif

main( )
{
    UINT8   i;
    UINT16  j;
    LED_OUT_INIT( );
    LED_OUT_ACT( );  /* ������LED��һ����ʾ���� */
    mDelay100mS( );  /* ��ʱ100���� */
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
//      while ( CH375DiskStatus != DISK_CONNECT ) xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̲��� */
        while ( CH375DiskStatus < DISK_CONNECT ) {  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̲��� */
            if ( CH375DiskConnect( ) == ERR_SUCCESS ) break;  /* ���豸�����򷵻سɹ�,CH375DiskConnectͬʱ�����ȫ�ֱ���CH375DiskStatus */
            mDelay100mS( );
        }
        LED_OUT_ACT( );  /* LED�� */
        mDelay100mS( );  /* ��ʱ,��ѡ����,�е�USB�洢����Ҫ��ʮ�������ʱ */
        mDelay100mS( );

/* ���U���Ƿ�׼����,����ĳЩU�̱���Ҫִ����һ�����ܹ��� */
        for ( i = 0; i < 5; i ++ ) {  /* �е�U�����Ƿ���δ׼����,�������Ա����� */
            mDelay100mS( );
            printf( "Ready ?\n" );
            if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* ��ѯ�����Ƿ�׼���� */
        }
#if DISK_BASE_BUF_LEN
        if ( DISK_BASE_BUF_LEN < CH375vSectorSize ) {  /* ���������ݻ������Ƿ��㹻��,CH375vSectorSize��U�̵�ʵ��������С */
            printf( "Too large sector size\n" );
            while ( CH375DiskConnect( ) == ERR_SUCCESS ) mDelay100mS( );
            continue;
        }
#endif
/* ��ѯ������������ */
/*      printf( "DiskSize\n" );
        i = CH375DiskSize( );
        mStopIfError( i );
        printf( "TotalSize = %u MB \n", (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * (CH375vSectorSize/512) / 2048 ) );  // ��ʾΪ��MBΪ��λ������
        // ԭ���㷽�� (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * CH375vSectorSize / 1000000 ) �п���ǰ����������˺������, �����޸ĳ���ʽ
*/
        LED_RUN_ACT( );  /* ��ʼ����U�� */

/*==================== ������ʾ��������ȡ���ļ��� ============================*/
        // ���Ƴ��ļ���(UNICODE ���)��LongNameBuf��
        memcpy( LongNameBuf, LongName, sizeof(LongName) );
        // ĩβ������0��ʾ����
        LongNameBuf[sizeof(LongName)] = 0x00;
        LongNameBuf[sizeof(LongName) + 1] = 0x00;
        // �ó��ļ�����ANSI������ļ���(8+3��ʽ)
        strcpy( mCmdParam.Create.mPathName, "\\C51\\AA\\���ļ���.TXT" );
        i = CH375CreateLongName( );
        if( i == ERR_SUCCESS )
            printf( "Created Long Name OK!!\n" );
        else
            printf( "Error Code: %02X\n", (UINT16)i );

        printf( "Get long Name#\n" );
        strcpy( mCmdParam.Open.mPathName, "\\C51\\AA\\���ļ���.TXT" );
        // ������Ҫ�����ļ���������·��
        i = CH375GetLongName( );
        if( i == ERR_SUCCESS )
        {
            // ���ļ����ռ����,��UNICODE���뷽ʽ(��UNICODE_ENDIAN����)
            // �����LongNameBuf������,���ļ������������0����.
            // ������ʾ����������������
            printf( "LongNameBuf: " );
            for( j=0; j!=LONG_NAME_BUF_LEN; j++ )
                printf( "%02X ", (UINT16)LongNameBuf[j] );
            printf( "\n" );
        }
        else
            printf( "Error Code: %02X\n", (UINT16)i );
/*==============================================================================*/

        LED_RUN_INACT( );
        printf( "Take out\n" );
//      while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̰γ� */
        while ( CH375DiskStatus >= DISK_CONNECT ) {  /* ��ѯCH375�жϲ������ж�״̬,�ȴ�U�̰γ� */
            if ( CH375DiskConnect( ) != ERR_SUCCESS ) break;
            mDelay100mS( );
        }
        LED_OUT_INACT( );  /* LED�� */
        mDelay100mS( );
        mDelay100mS( );
    }
}