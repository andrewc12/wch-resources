/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2004   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* CH375 主机文件系统接口 */
/* 支持: FAT12/FAT16/FAT32 */

/* MCS-51单片机C语言的U盘文件读写示例程序, 适用于89C52或者更大程序空间的单片机 */
/* 长文件名操作示例, 包括创建长文件名和从短文件名获得对应的长文件名
   注意: CH375HF?.H头文件需做少量修改, 在mCmdParam联合结构中增加ReadBlock和WriteBlock的参数结构
*/
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR复制", 所以速度较慢, 适用于所有MCS51单片机 */


/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF6.LIB    如果将CH375HF6换成CH375HF4就可以不支持FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>
#include <string.h>

/* 以下定义的详细说明请看CH375HF6.H文件 */
#define LIB_CFG_DISK_IO         1       /* 磁盘读写的数据的复制方式,1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_FILE_IO         1       /* 文件读写的数据的复制方式,0为"外部子程序",1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_INT_EN          0       /* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */

#define CH375_CMD_PORT_ADDR     0xBDF1  /* CH375命令端口的I/O地址 */
#define CH375_DAT_PORT_ADDR     0xBCF0  /* CH375数据端口的I/O地址 */
/* 62256提供的32KB的RAM分为两部分: 0000H-01FFH为磁盘读写缓冲区, 0200H-7FFFH为文件数据缓冲区 */
#define DISK_BASE_BUF_ADDR      0x0000  /* 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */
#define DISK_BASE_BUF_LEN       4096    /* 默认的磁盘数据缓冲区大小为512字节,建议选择为2048甚至4096以支持某些大扇区的U盘,为0则禁止在.H文件中定义缓冲区并由应用程序在pDISK_BASE_BUF中指定 */
#define FILE_DATA_BUF_ADDR      0x1000  /* 外部RAM的文件数据缓冲区的起始地址,缓冲区长度不小于一次读写的数据长度 */
/* 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以外部RAM剩余长度为32256字节 */
#define FILE_DATA_BUF_LEN       0x6800  /* 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度 */
/* 如果准备使用双缓冲区交替读写,那么不要定义FILE_DATA_BUF_LEN,而是在参数中指定缓冲区起址,用CH375FileReadX代替CH375FileRead,用CH375FileWriteX代替CH375FileWrite */

#define CH375_INT_WIRE          INT0    /* P3.2, INT0, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM     1       /* 未调用CH375FileEnumer程序故禁止以节约代码 */
#define NO_DEFAULT_CH375_F_QUERY    1       /* 未调用CH375FileQuery程序故禁止以节约代码 */

#include "CH375HF6.H"               /* 如果不需要支持FAT32,那么请选用CH375HF4.H */

/* 在P1.4连接一个LED用于监控演示程序的进度,低电平LED亮,当U盘插入后亮 */
sbit P1_4  = P1^4;
#define LED_OUT_INIT( )     { P1_4 = 1; }   /* P1.4 高电平 */
#define LED_OUT_ACT( )      { P1_4 = 0; }   /* P1.4 低电平驱动LED显示 */
#define LED_OUT_INACT( )    { P1_4 = 1; }   /* P1.4 低电平驱动LED显示 */
sbit P1_5  = P1^5;
/* 在P1.5连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘操作时亮 */
#define LED_RUN_ACT( )      { P1_5 = 0; }   /* P1.5 低电平驱动LED显示 */
#define LED_RUN_INACT( )    { P1_5 = 1; }   /* P1.5 低电平驱动LED显示 */
sbit P1_6  = P1^6;
/* 在P1.6连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘写操作时亮 */
#define LED_WR_ACT( )       { P1_6 = 0; }   /* P1.6 低电平驱动LED显示 */
#define LED_WR_INACT( )     { P1_6 = 1; }   /* P1.6 低电平驱动LED显示 */

/* 延时100毫秒,不精确 */
void    mDelay100mS( )
{
    UINT8   i, j, c;
    for ( i = 200; i != 0; i -- ) for ( j = 200; j != 0; j -- ) c+=3;
}

/* 将程序空间的字符串复制到内部RAM中,返回字符串长度 */
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

/* 检查操作状态,如果错误则显示错误代码并停机 */
void    mStopIfError( UINT8 iError )
{
    if ( iError == ERR_SUCCESS ) return;  /* 操作成功 */
    printf( "Error: %02X\n", (UINT16)iError );  /* 显示错误 */
    while ( 1 ) {
        LED_OUT_ACT( );  /* LED闪烁 */
        mDelay100mS( );
        LED_OUT_INACT( );
        mDelay100mS( );
    }
}

/* 为printf和getkey输入输出初始化串口 */
void    mInitSTDIO( )
{
    SCON = 0x50;
    PCON = 0x80;
    TMOD = 0x20;
    TH1 = 0xf3;  /* 24MHz晶振, 9600bps */
    TR1 = 1;
    TI = 1;
}

/*====================长文件名新添的宏定义极其全局变量 =======================*/
// 长文件名缓冲区从(0到20)*26
#define     LONG_NAME_BUF_LEN       (20*26)
#define     UNICODE_ENDIAN          1           // 1为UNICDOE大端编码 0为小端
// 长文件名存放缓冲区(Unicode编码)
UINT8   xdata LongNameBuf[ LONG_NAME_BUF_LEN ];

#define     TRUE        1
#define     FALSE       0

// 函数返回
#define     ERR_NO_NAME             0X44        // 此短文件名没有长文件名或错误的长文件
#define     ERR_BUF_OVER            0X45        // 长文件缓冲区溢出
#define     ERR_LONG_NAME           0X46        // 错误的长文件名
#define     ERR_NAME_EXIST          0X47        // 此短文件名存在
/*============================================================================*/

/*==============================================================================
函数名: CheckNameSum

函数作用: 检查长文件名的短文件名检验和

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
函数名: AnalyzeLongName

函数作用: 整理长文件名 返回有几个的26长度

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
        return 0;                   // 返回0表示错误的长文件名

    i = index % 26;
    if( i != 0 )
    {
        index += 2;
        if( index % 26 != 0 )       // 加0刚好结束
        {
            for( j=i+2; j!=26; j++ )// 把剩余数据填为0XFF
                LongNameBuf[index++] = 0xff;
        }
    }
    return  (index / 26);
}

/*==============================================================================
函数名: CH375CreateLongName

函数作用: 创建长文件名,需要输入短文件名的完整路径

==============================================================================*/
UINT8 CH375CreateLongName( void )
{
// 分析 保留文件路径 创建一个空文件 得到FDT偏移和其所在扇区 删除文件
// 向后偏移扇区 可能失败 如FAT12/16处在根目录处 填充完毕后再次创建文件
UINT8   i;
UINT8   len;                                // 存放路径的长度
UINT16  index;                              // 长文件偏移索引
UINT16  indexBak;                           // 长文件偏移索引备份
UINT32  Secoffset;                          // 扇区偏移

UINT8   Fbit;                               // 第一次进入写扇区
UINT8   Mult;                               // 长文件名长度26的倍数
UINT8   MultBak;                            // 长文件名长度26的倍数备份

UINT16  Backoffset;                         // 保存文件偏移备份
UINT16  BackoffsetBak;                      // 保存偏移备份的备份
UINT32  BackFdtSector;                      // 保寸偏移上一个扇区
UINT8   sum;                                // 保存长文件名的校验和

UINT8   xdata BackPathBuf[MAX_PATH_LEN];    // 保存文件路径

    Mult = AnalyzeLongName( );              // 保存长文件名是26的倍数
    if( Mult == 0 )
        return ERR_LONG_NAME;
    MultBak = Mult;

    i = CH375FileOpen();                    // 短文件名存在则返回错误
    if( i == ERR_SUCCESS )
        return ERR_NAME_EXIST;

    i = CH375FileCreate( );
    if( i == ERR_SUCCESS )
    {
        Backoffset = CH375vFdtOffset;
        BackoffsetBak = Backoffset;
        BackFdtSector = CH375vFdtLba;
        sum = CheckNameSum( &DISK_BASE_BUF[Backoffset ] );
        for( i=0; i!=MAX_PATH_LEN; i++ )    // 对文件路径进行备份
            BackPathBuf[i] = mCmdParam.Open.mPathName[i];
        CH375FileErase( );                  // 删除此文件

        Secoffset   = 0;                    // 从0开始偏移
        index       = Mult*26;              // 得到长文件名的长度
        indexBak    = index;
        Fbit        = FALSE;                // 默认没有进入
        // 打开上级 进行数据填充数据
        P_RETRY:
        for(len=0; len!=MAX_PATH_LEN; len++)
        {
            if(mCmdParam.Open.mPathName[len] == 0)
                break;                      // 得到字符串长度
        }

        for(i=len-1; i!=0xff; i--)          // 得到上级目录位置
        {
            if((mCmdParam.Open.mPathName[i] == '\\') || (mCmdParam.Open.mPathName[i] == '/'))
                break;
        }
        mCmdParam.Open.mPathName[i] = 0x00;

        if( i==0 )                          // 打开一级目录注意:处在根目录开始的特殊情况
        {
            mCmdParam.Open.mPathName[0] = '/';
            mCmdParam.Open.mPathName[1] = 0;
        }

        i = CH375FileOpen();                // 打开上级目录
        if( i == ERR_OPEN_DIR )
        {
            while( 1 )                      // 循环填写 直到完成
            {
                mCmdParam.Locate.mSectorOffset = Secoffset;
                i = CH375FileLocate( );
                if( i == ERR_SUCCESS )
                {
                    if( Fbit )             // 第二次进入次写扇区
                    {
                        if( mCmdParam.Locate.mSectorOffset != 0x0FFFFFFFF )
                        {
                            BackFdtSector = mCmdParam.Locate.mSectorOffset;
                            Backoffset = 0;
                        }
                        else
                        {
                            for( i=0; i!=MAX_PATH_LEN; i++ )// 还原文件路径
                                mCmdParam.Open.mPathName[i] = BackPathBuf[i];
                            i = CH375FileCreate( );         // 进行空间扩展
                            if( i != ERR_SUCCESS )
                                return i;
                            CH375FileErase( );
                            goto P_RETRY;                   // 重新打开上级目录
                        }
                    }

                    if( BackFdtSector == mCmdParam.Locate.mSectorOffset )
                    {
                        mCmdParam.ReadX.mSectorCount = 1;   // 读一个扇区到磁盘缓冲区
                        mCmdParam.ReadX.mDataBuffer = &DISK_BASE_BUF[0];
                        i = CH375FileReadX( );
                        CH375DirtyBuffer( );                // 清除磁盘缓冲区
                        if( i!= ERR_SUCCESS )
                            return i;

                        i = ( CH375vSectorSize - Backoffset ) / 32;
                        if( Mult > i )
                            Mult = Mult - i;                // 剩余的倍数
                        else
                        {
                            i = Mult;
                            Mult = 0;
                        }

                        for( len=i; len!=0; len-- )
                        {
                            indexBak -= 26;
                            index = indexBak;
                            for( i=0; i!=5; i++)            // 长文件名的1-5个字符
                            {                               // 在磁盘上UNICODE用小端方式存放
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

                            for( i =0; i!=6; i++)           // 长文件名的6-11个字符
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

                            for( i=0; i!=2; i++)            // 长文件名的12-13个字符
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
                        {                               // 还原文件路径
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

函数名: GetUpSectorData

函数作用: 由当前扇区得到上一个扇区的数据，放在磁盘缓冲区

==============================================================================*/
UINT8 GetUpSectorData( UINT32 *NowSector )
{
UINT8  i;
UINT8  len;             // 存放路径的长度
UINT32 index;           // 目录扇区偏移扇区数

    index = 0;
    for(len=0; len!=MAX_PATH_LEN; len++)
    {
        if(mCmdParam.Open.mPathName[len] == 0)          // 得到字符串长度
            break;
    }

    for(i=len-1; i!=0xff; i--)                          // 得到上级目录位置
    {
        if((mCmdParam.Open.mPathName[i] == '\\') || (mCmdParam.Open.mPathName[i] == '/'))
            break;
    }
    mCmdParam.Open.mPathName[i] = 0x00;

    if( i==0 )  // 打开一级目录注意:处在根目录开始的特殊情况
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
                        if( index==0 )                          // 处于根目录扇区的开始
                            return ERR_NO_NAME;
                        mCmdParam.Locate.mSectorOffset = --index;
                        i = CH375FileLocate( );                 // 读上一个扇区的数据
                        if( i == ERR_SUCCESS )
                        {                                       // 以下保存当前所在扇区数
                            *NowSector = mCmdParam.Locate.mSectorOffset;
                            mCmdParam.ReadX.mSectorCount = 1;   // 读一个扇区到磁盘缓冲区
                            mCmdParam.ReadX.mDataBuffer = &DISK_BASE_BUF[0];
                            i = CH375FileReadX( );
                            CH375DirtyBuffer( );                // 清除磁盘缓冲区
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

函数名: CH375GetLongName

函数作用: 由完整短文件名路径(可以是文件或文件夹)得到相应的长文件名

==============================================================================*/
UINT8 CH375GetLongName( void )
{
// 需要变量扇区大小
// 第一步：打开文件是否找到文件,分析文件是否存在,并得到FDT在此扇区的偏移和所在扇区
// 第二步：分析上面的信息看是否有长文件名存在，是否处于目录的第一个扇区的开始
// 第三步：实现向后偏移一个扇区?读取长文件名(扇区512字节的U盘)
UINT8   i;
UINT16  index;          // 在长文件名缓冲区内的索引
UINT32  BackFdtSector;  // 保寸偏移上一个扇区
UINT8   sum;            // 保存长文件名的校验和
UINT16  Backoffset;     // 保存文件偏移备份
UINT16  offset;         // 扇区内文件偏移32倍数
UINT8   FirstBit;       // 长文件名跨越两个扇区标志位
UINT8   xdata BackPathBuf[MAX_PATH_LEN]; // 保存文件路径

    i = CH375FileOpen( );
    if( ( i == ERR_SUCCESS ) || ( i == ERR_OPEN_DIR ) )
    {
        for( i=0; i!=MAX_PATH_LEN; i++ )
            BackPathBuf[i] = mCmdParam.Open.mPathName[i];
        // 以上完成对路径的备份

        sum = CheckNameSum( &DISK_BASE_BUF[CH375vFdtOffset] );
        index = 0;
        FirstBit = FALSE;
        Backoffset = CH375vFdtOffset;
        BackFdtSector = CH375vFdtLba;
        if( CH375vFdtOffset == 0 )
        {
            // 先判断是否处于一个扇区开始 是否处于根目录开始 ，否则向后偏移
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
            // 读取偏移后的数据，直到结束。如果不够则向后偏移
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

                        for( i=0; i!=5; i++)            // 长文件名的1-5个字符
                        {                               // 在磁盘上UNICODE用小端方式存放
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

                        for( i =0; i!=6; i++)           // 长文件名的6-11个字符
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

                        for( i=0; i!=2; i++)            // 长文件名的12-13个字符
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
                            {                           // 处理刚好为26字节长倍数的文件名
                                if(index + 52 >LONG_NAME_BUF_LEN )
                                    return ERR_BUF_OVER;
                                LongNameBuf[ index ] = 0x00;
                                LongNameBuf[ index + 1] = 0x00;
                            }
                            return ERR_SUCCESS;         // 成功完成长文件名收集完成
                        }
                    }
                    else
                        return ERR_NO_NAME;             // 错误的长文件名,程序返回
                }
                else
                {
                    if( FirstBit == FALSE )
                        FirstBit = TRUE;
                    else                                // 否则第二次进入
                    {
                        for( i=0; i!=MAX_PATH_LEN; i++ )// 还原路径
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
                    // 向后偏移扇区
                }
            }
        }
    }
    return i;                // 返回错误
}

/*
长文件名示例(UNICODE编码的大小端 必须与UNICODE_ENDIAN定义相同)
以下是LongName里编码内容:
建立长文件名，输入两个参数： 1.采用(unicode 大端)，字符串末尾用两个0表示结束;2.ANSI编码短文件名.TXT
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
    LED_OUT_ACT( );  /* 开机后LED亮一下以示工作 */
    mDelay100mS( );  /* 延时100毫秒 */
    LED_OUT_INACT( );
    mInitSTDIO( );  /* 为了让计算机通过串口监控演示过程 */
    printf( "Start\n" );

#if DISK_BASE_BUF_LEN == 0
    pDISK_BASE_BUF = &my_buffer[0];  /* 不在.H文件中定义CH375的专用缓冲区,而是用缓冲区指针指向其它应用程序的缓冲区便于合用以节约RAM */
#endif

    i = CH375LibInit( );  /* 初始化CH375程序库和CH375芯片,操作成功返回0 */
    mStopIfError( i );
/* 其它电路初始化 */

    while ( 1 ) {
        printf( "Wait Udisk\n" );
//      while ( CH375DiskStatus != DISK_CONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘插入 */
        while ( CH375DiskStatus < DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘插入 */
            if ( CH375DiskConnect( ) == ERR_SUCCESS ) break;  /* 有设备连接则返回成功,CH375DiskConnect同时会更新全局变量CH375DiskStatus */
            mDelay100mS( );
        }
        LED_OUT_ACT( );  /* LED亮 */
        mDelay100mS( );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */
        mDelay100mS( );

/* 检查U盘是否准备好,但是某些U盘必须要执行这一步才能工作 */
        for ( i = 0; i < 5; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
            mDelay100mS( );
            printf( "Ready ?\n" );
            if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
        }
#if DISK_BASE_BUF_LEN
        if ( DISK_BASE_BUF_LEN < CH375vSectorSize ) {  /* 检查磁盘数据缓冲区是否足够大,CH375vSectorSize是U盘的实际扇区大小 */
            printf( "Too large sector size\n" );
            while ( CH375DiskConnect( ) == ERR_SUCCESS ) mDelay100mS( );
            continue;
        }
#endif
/* 查询磁盘物理容量 */
/*      printf( "DiskSize\n" );
        i = CH375DiskSize( );
        mStopIfError( i );
        printf( "TotalSize = %u MB \n", (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * (CH375vSectorSize/512) / 2048 ) );  // 显示为以MB为单位的容量
        // 原计算方法 (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec * CH375vSectorSize / 1000000 ) 有可能前两个数据相乘后导致溢出, 所以修改成上式
*/
        LED_RUN_ACT( );  /* 开始操作U盘 */

/*==================== 以下演示创建及读取长文件名 ============================*/
        // 复制长文件名(UNICODE 大端)到LongNameBuf里
        memcpy( LongNameBuf, LongName, sizeof(LongName) );
        // 末尾用两个0表示结束
        LongNameBuf[sizeof(LongName)] = 0x00;
        LongNameBuf[sizeof(LongName) + 1] = 0x00;
        // 该长文件名的ANSI编码短文件名(8+3格式)
        strcpy( mCmdParam.Create.mPathName, "\\C51\\AA\\长文件名.TXT" );
        i = CH375CreateLongName( );
        if( i == ERR_SUCCESS )
            printf( "Created Long Name OK!!\n" );
        else
            printf( "Error Code: %02X\n", (UINT16)i );

        printf( "Get long Name#\n" );
        strcpy( mCmdParam.Open.mPathName, "\\C51\\AA\\长文件名.TXT" );
        // 以上需要输入文件名的完整路径
        i = CH375GetLongName( );
        if( i == ERR_SUCCESS )
        {
            // 长文件名收集完成,以UNICODE编码方式(按UNICODE_ENDIAN定义)
            // 存放在LongNameBuf缓冲里,长文件名最后用两个0结束.
            // 以下显示缓冲区里所有数据
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
//      while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
        while ( CH375DiskStatus >= DISK_CONNECT ) {  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
            if ( CH375DiskConnect( ) != ERR_SUCCESS ) break;
            mDelay100mS( );
        }
        LED_OUT_INACT( );  /* LED灭 */
        mDelay100mS( );
        mDelay100mS( );
    }
}
