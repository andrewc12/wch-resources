/* 2004.06.05
****************************************
**  Copyright  (C)  W.ch  1999-2005   **
**  Web:  http://www.winchiphead.com  **
****************************************
**  USB Host File Interface for CH375 **
**  TC2.0@PC, KC7.0@MCS51             **
****************************************
*/
/* CH375 主机文件系统接口 */
/* 支持: FAT12/FAT16/FAT32 */

/* MCS-51单片机C语言的U盘文件读写示例程序, 适用于89C52或者更大程序空间的单片机 */
/* 本程序用于演示创建/打开/删除长文件名文件 */
/* CH375的INT#引脚采用查询方式处理, 数据复制方式为"单DPTR复制", 所以速度较慢, 适用于所有MCS51单片机
   本例适用于V2.8及以上版本的CH375子程序库 */

/* 仅供有FAT文件系统和长文件名相关技术知识基础的用户参考,支持小写字母或者汉字等不超过256个字符的文件名,
   对该类知识不了解的用户建议不要涉及长文件名,建议只使用现成的短文件名,短文件名支持8+3格式,大写字母或者汉字 */

/* C51   CH375HFT.C */
/* LX51  CH375HFT.OBJ , CH375HF4.LIB    如果将CH375HF4换成CH375HF6就可以支持FAT32 */
/* OHX51 CH375HFT */

#include <reg52.h>
#include <stdio.h>
#include <string.h>

/* 以下定义的详细说明请看CH375HF6.H文件 */
#define LIB_CFG_DISK_IO			1		/* 磁盘读写的数据的复制方式,1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_FILE_IO			1		/* 文件读写的数据的复制方式,0为"外部子程序",1为"单DPTR复制",2为"双DPTR复制",3为"单DPTR和P2+R0复制" */
#define LIB_CFG_INT_EN			0		/* CH375的INT#引脚连接方式,0为"查询方式",1为"中断方式" */
/*#define LIB_CFG_FILE_IO_DEFAULT	1*/		/* 使用CH375HF6.H提供的默认"外部子程序" */
/*#define LIB_CFG_UPD_SIZE		1*/		/* 在添加数据后是否自动更新文件长度: 0为"不更新",1为"自动更新" */
/* 默认情况下,如果扇区数/字节数不为0那么CH375FileWrite/CH375ByteWrite只负责写入数据而不修改文件长度,
   如果需要每次写完数据后会自动修改/更新文件长度,那么可以使全局变量CH375LibConfig的位4为1,
   如果长时间不写入数据则应该更新文件长度,防止突然断电后前面写入的数据与文件长度不相符,
   如果确定不会突然断电或者后面很快有数据不断写入则不必更新文件长度,可以提高速度并减少U盘损耗(U盘内部的内存寿命有限,不宜频繁改写) */

#define CH375_CMD_PORT_ADDR		0xBDF1	/* CH375命令端口的I/O地址 */
#define CH375_DAT_PORT_ADDR		0xBCF0	/* CH375数据端口的I/O地址 */
/* 62256提供的32KB的RAM分为两部分: 0000H-01FFH为磁盘读写缓冲区, 0200H-7FFFH为文件数据缓冲区 */
#define	DISK_BASE_BUF_ADDR		0x0000	/* 外部RAM的磁盘数据缓冲区的起始地址,从该单元开始的缓冲区长度为SECTOR_SIZE */
#define FILE_DATA_BUF_ADDR		0x0200	/* 外部RAM的文件数据缓冲区的起始地址,缓冲区长度不小于一次读写的数据长度 */
/* 由于演示板用的62256只有32K字节,其中CH375子程序用512字节,所以外部RAM剩余长度为32256字节 */
#define FILE_DATA_BUF_LEN		0x200	/* 外部RAM的文件数据缓冲区,缓冲区长度不小于一次读写的数据长度,本例要求不小于0x400即可 */
#define FILE_DATA_BUF_ADDR1		0x400	
unsigned char xdata FileDataBuf1[FILE_DATA_BUF_LEN] _at_ FILE_DATA_BUF_ADDR1 ;
#define CH375_INT_WIRE			INT0	/* P3.2, INT0, CH375的中断线INT#引脚,连接CH375的INT#引脚,用于查询中断状态 */

#define NO_DEFAULT_CH375_F_ENUM		1		/* 未调用CH375FileEnumer程序故禁止以节约代码 */
#define NO_DEFAULT_CH375_F_QUERY	1		/* 未调用CH375FileQuery程序故禁止以节约代码 */

#include "..\CH375HF6.H"				/* 如果不需要支持FAT32,那么请选用CH375HF4.H */
#include    "FILELONG.H"
/* 在P1.4连接一个LED用于监控演示程序的进度,低电平LED亮,当U盘插入后亮 */
sbit P1_4  = P1^4;
#define LED_OUT_INIT( )		{ P1_4 = 1; }	/* P1.4 高电平 */
#define LED_OUT_ACT( )		{ P1_4 = 0; }	/* P1.4 低电平驱动LED显示 */
#define LED_OUT_INACT( )	{ P1_4 = 1; }	/* P1.4 低电平驱动LED显示 */
sbit P1_5  = P1^5;
/* 在P1.5连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘操作时亮 */
#define LED_RUN_ACT( )		{ P1_5 = 0; }	/* P1.5 低电平驱动LED显示 */
#define LED_RUN_INACT( )	{ P1_5 = 1; }	/* P1.5 低电平驱动LED显示 */
sbit P1_6  = P1^6;
/* 在P1.6连接一个LED用于监控演示程序的进度,低电平LED亮,当对U盘写操作时亮 */
#define LED_WR_ACT( )		{ P1_6 = 0; }	/* P1.6 低电平驱动LED显示 */
#define LED_WR_INACT( )		{ P1_6 = 1; }	/* P1.6 低电平驱动LED显示 */
/* 以毫秒为单位延时,不精确,适用于24MHz时钟 */
void	mDelaymS( unsigned char delay )
{
	unsigned char	i, j, c;
	for ( i = delay; i != 0; i -- ) {
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
		for ( j = 200; j != 0; j -- ) c += 3;  /* 在24MHz时钟下延时500uS */
	}
}

/* 将程序空间的字符串复制到内部RAM中,返回字符串长度 */
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

/* 将程序空间的字符串复制到内部RAM中,返回字符串长度 */
UINT8	mCopyCodeStringToIRAM1( UINT8 idata *iDestination, UINT8 xdata *iSource )
{
	UINT8	i = 0;
	while ( *iDestination = *iSource ) {
		iDestination ++;
		iSource ++;
		i ++;
	}
	return( i );
}
/* 将程序空间的字符串复制到内部RAM中,返回字符串长度 */
UINT8	mCopyCodeStringToXRAM( UINT8 xdata *iDestination, UINT8 code *iSource )
{
	UINT8	i = 0;
	while ( *iDestination = *iSource ) {
		iDestination ++;
		iSource ++;
		i ++;
	}
	return( i );
}
/* 检查操作状态,如果错误则显示错误代码并停机 */
void	mStopIfError( UINT8 iError )
{
	if ( iError == ERR_SUCCESS ) return;  /* 操作成功 */
	printf( "Error: %02X\n", (UINT16)iError );  /* 显示错误 */
	while ( 1 ) {
		LED_OUT_ACT( );  /* LED闪烁 */
		mDelaymS( 200 );
		LED_OUT_INACT( );
		mDelaymS( 200 );
	}
}

/* 为printf和getkey输入输出初始化串口 */
void	mInitSTDIO( )
{
	SCON = 0x50;
	PCON = 0x80;
	TMOD = 0x21;
	TH1 = 0xf3;  /* 24MHz晶振, 9600bps */
	TR1 = 1;
	TI = 1;
}

/*计算短文件名的校验和，*/
unsigned char ChkSum (P_FAT_DIR_INFO pDir1)
{
	unsigned char FcbNameLen;
	unsigned char Sum;
	Sum = 0;
	for (FcbNameLen=0; FcbNameLen!=11; FcbNameLen++) {
		//if(pDir1->DIR_Name[FcbNameLen]==0x20)continue;
		Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + pDir1->DIR_Name[FcbNameLen];
	}
	return (Sum);
}

/*分析缓冲区的目录项和长文件名是否相同,返回00-15为找到长文件名相同的文件00-15表示对应短文件名在目录
项的位置,返回0X80-8F表示分析到目录项的结尾,以后都是未用的目录项,返回0FF表示此扇区无匹配的目录项*/
UINT8  mLDirCheck(P_FAT_DIR_INFO pDir2,F_LONG_NAME xdata *pLdir1){
	UINT8 i,j,k,sum,nodir,nodir1;
	F_LONG_NAME xdata *pLdir2;
	unsigned int  xdata *pLName;	
	unsigned char  data1;		
	for(i=0;i!=16;i++){						
		if(pDir2->DIR_Name[0]==0xe5){pDir2+=1;continue;}		/*如果此项被删除继续分析下一目录*/			/*是被删除的文件名则跳过*/
		if(pDir2->DIR_Name[0]==0x00){return i|0x80;}			/*分析到以下空间没有文件名存在退出*/
		if((pDir2->DIR_Attr==0x0f)|(pDir2->DIR_Attr==0x8)){pDir2+=1;continue;}  /*如果找到的是卷标或者长文件名继续*/	
		/*找到一个短文件名*/
			k=i-1;					/*长文件名项应在短文件名上面*/
			if(i==0){				/*如果此短文件名在本扇区第一项*/
				pLdir2=pLdir1;			/*长文件名应在上一扇区的最后一项*/
				k=15;				/*记录长文件名位置*/
				pLdir2+=15;			/*偏移到结尾*/
			}	
			else pLdir2=(F_LONG_NAME xdata *)(pDir2-1);	/*取长文件名目录项*/
		    	sum=ChkSum(pDir2); 				/*计算累加和*/
			pLName=LongFileName;				/*指项指定的长文件名*/
		 	nodir=0;					/*初始化标志*/
		 	nodir1=1;
		 	while(1){
				if((pLdir2->LDIR_Ord!=0xe5)&(pLdir2->LDIR_Attr==ATTR_LONG_NAME)& (pLdir2->LDIR_Chksum==sum)){  /*找到一个长文件名*/
					for(j=0;j!=5;j++){		
						if((pLdir2->LDIR_Name1[j]==0x00)|(*pLName==0))continue;	/*分析到长文件名结尾*/
						if((pLdir2->LDIR_Name1[j]==0xff)|(*pLName==0))continue;    /*分析到长文件名结尾*/
						if(pLdir2->LDIR_Name1[j]!=*pLName){		/*不等则设置标志*/	
							nodir=1;
							break;
						}
						pLName++;
					}
					if(nodir==1)break;								/*文件名不同退出*/
					for(j=0;j!=6;j++){		
						if((pLdir2->LDIR_Name2[j]==0x00)|(*pLName==0))continue;
						if((pLdir2->LDIR_Name2[j]==0xff)|(*pLName==0))continue;
						if(*pLName!=pLdir2->LDIR_Name2[j]){nodir=1;break;}
						pLName++;
					}
					if(nodir==1)break;								/*文件名不同退出*/
					for(j=0;j!=2;j++){		
						if((pLdir2->LDIR_Name3[j]==0x00)|(*pLName==0))continue;
						if((pLdir2->LDIR_Name3[j]==0xff)|(*pLName==0))continue;
						if(*pLName!=pLdir2->LDIR_Name3[j]){nodir=1;break;}
						pLName++;
					}
					if(nodir==1)break;								/*文件名不同退出*/				
				 	if((data1=pLdir2->LDIR_Ord&0x40)==0x40){nodir1=0;break;} /*找到长文件名，并且比较结束*/	
			  	}
				else break;							/*不是连续对应的长文件名退出*/
				if(k==0){
					pLdir2=pLdir1;
					pLdir2+=15;
					k=15;
			    }
				else {
					k=k-1;
					pLdir2-=1;
				}
		    }	
	    if(nodir1==0)	return i;			/*表示找到长文件名，返回短文件名在的目录项*/
		pDir2+=1;
    }	
	return 0xff;				/*指搜索完这一个扇区没找到响应的长文件名*/
}

/*检查上级子目录并打开*/
UINT8 mChkName(unsigned char data *pJ){
		UINT8 i,j;
	j = 0xFF;
	for ( i = 0; i != sizeof( mCmdParam.Create.mPathName ); i ++ ) {  /* 检查目录路径 */
		if ( mCmdParam.Create.mPathName[ i ] == 0 ) break;
		if ( mCmdParam.Create.mPathName[ i ] == PATH_SEPAR_CHAR1 || mCmdParam.Create.mPathName[ i ] == PATH_SEPAR_CHAR2 ) j = i;  /* 记录上级目录 */
	}
	i = ERR_SUCCESS;
	if ( j == 0 || j == 2 && mCmdParam.Create.mPathName[1] == ':' ) {  /* 在根目录下创建 */
		mCmdParam.Open.mPathName[ 0]='/';
		mCmdParam.Open.mPathName[ 1]=0;
		i=CH375FileOpen();			/*打开根目录*/
		if ( i == ERR_OPEN_DIR ) i = ERR_SUCCESS;  /* 成功打开上级目录 */	
	}
	else {
		if ( j != 0xFF ) {  /* 对于绝对路径应该获取上级目录的起始簇号 */
			mCmdParam.Create.mPathName[ j ] = 0;
			i = CH375FileOpen( );  /* 打开上级目录 */
			if ( i == ERR_SUCCESS ) i = ERR_MISS_DIR;  /* 是文件而非目录 */
			else if ( i == ERR_OPEN_DIR ) i = ERR_SUCCESS;  /* 成功打开上级目录 */
			mCmdParam.Create.mPathName[ j ] = PATH_SEPAR_CHAR1;  /* 恢复目录分隔符 */
		}		
	}
	*pJ=j;										/*指针中返回一组数据*/
	return i;
}

/*根据指定的长文件名搜索对应的短文件名长文件名空间放入长文件名,短文件名空间放入路径00结尾*/
UINT8  mLoopkUpSName(){
	UINT8  BlockSer1;				/*定义两个扇区块内记数*/
	unsigned char xdata ParData[MAX_PATH_LEN];			/**/
	UINT16	tempSec;						/*扇区偏移*/
	UINT8 i,j,k;
	F_LONG_NAME   xdata *pLDirName; 
	P_FAT_DIR_INFO  pDirName;
	bit  FBuf;
	unsigned char data *pBuf1;
	CH375DirtyBuffer();
	for(k=0;k!=MAX_PATH_LEN;k++)ParData[k]=mCmdParam.Other.mBuffer[k];	/*保存当前路径*/
	i=mChkName(&j);										
	if ( i == ERR_SUCCESS ) {  /* 成功获取上级目录的起始簇号 */
		BlockSer1=0;
		FBuf=0;					/*初始化*/	
		tempSec=0;
		FileDataBuf1[0]=0xe5;	/*第一次用，无效缓冲区*/
		k=0xff;
		while(1){							/*下面是读取并分析目录项*/			
			pDirName=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;		/*短文件名指针指向缓冲区*/		
			pLDirName=FBuf?FILE_DATA_BUF_ADDR:FILE_DATA_BUF_ADDR1;
															/*当前处理的文件缓冲区*/																/*这里使用双向缓冲区，去处理文件名*/
			mCmdParam.ReadX.mSectorCount=1;				/*读取一扇区数据*/
			mCmdParam.ReadX.mDataBuffer=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;
			FBuf=!FBuf;												/*缓冲区标志翻转*/
			i=CH375FileReadX( );
			if(mCmdParam.ReadX.mSectorCount==0){k=0xff;break;}
				k=mLDirCheck(pDirName,pLDirName);
				if(k!=0x0ff){break;}			/*找到文件或者找到文件结尾退出*/
			}
			if(k<16){
						pDirName+=k;		/*所找的文件短文件名在此目录项*/
			    		if(j!=0xff){	
					  		 for(k=0;k!=j+1;k++)mCmdParam.Other.mBuffer[k]=ParData[k];	
						}
						pBuf1=&mCmdParam.Other.mBuffer[j+1];	/*取文件名的地址*/
						for(i=0;i!=8;i++){
							if(pDirName->DIR_Name[i]==0x20)continue;
					    	else{
						 		*pBuf1=pDirName->DIR_Name[i];
						 		pBuf1++;
			 				}
						}
						if(pDirName->DIR_Name[i]!=0x20){
							*pBuf1='.';
						 	pBuf1++;					/*不是目录则有扩展名*/
						}
						for(;i!=11;i++){
							if(pDirName->DIR_Name[i]==0x20)continue;
							else {
								*pBuf1=pDirName->DIR_Name[i];
						 		pBuf1++;
			 				}
						}
						*pBuf1=00;
						 for(k=0;k!=j+1;k++)ParData[k]=mCmdParam.Other.mBuffer[k];
								/*复制短文件名*/
					i=CH375FileClose( );
					for(k=0;k!=j+1;k++)mCmdParam.Other.mBuffer[k]=ParData[k];			
		   	}
			else 	k=0xff;				/*返回没找到指定的长文件名文件？？？？？？？*/	 
			i=CH375FileClose( );
	  }
	  else {k=i;};
	  return k;	
}

/*这里可以创建文件的长文件名，在短文件名空间输入路径以及参考短文件名，在长文件名空间输入该文件长文件名的UNICODE代码，
返回状态,00表示成功，并且在短文件名空间返回真实的短文件名，其他为不成功状态*/
/*创建并打开*/
UINT8  mCreatLName(){
	UINT8  BlockSer1;				/*定义两个扇区块内记数*/
	unsigned char xdata ParData[MAX_PATH_LEN];			/**/
	UINT16	tempSec;						/*扇区偏移*/
	UINT8 i,j,k,x,sum,y,z;
	F_LONG_NAME   xdata *pLDirName; 
	P_FAT_DIR_INFO  pDirName,pDirName1;
	bit  FBuf;		
	unsigned char data *pBuf1;
	unsigned int xdata *pBuf;
	CH375DirtyBuffer(  );
	for(k=0;k!=MAX_PATH_LEN;k++)ParData[k]=mCmdParam.Other.mBuffer[k];			/**/
	i=mChkName(&j);
	if ( i == ERR_SUCCESS ) {  /* 成功获取上级目录的起始簇号 */
		BlockSer1=0;
		FBuf=0;					/*初始化*/	
		tempSec=0;
		FileDataBuf1[0]=0xe5;	/*无效上次缓冲区*/
		k=0xff;
		while(1){							/*下面是读取并分析目录项*/			
			pDirName=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;		/*短文件名指针指向缓冲区*/		
			pLDirName=FBuf?FILE_DATA_BUF_ADDR:FILE_DATA_BUF_ADDR1;
			mCmdParam.ReadX.mSectorCount=1;				/*读取一扇区数据*/
			mCmdParam.ReadX.mDataBuffer=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;  /*当前处理的文件缓冲区,这里使用双向缓冲区，去处理文件名*/
			FBuf=!FBuf;												/*缓冲区标志翻转*/
			i=CH375FileReadX( );
			if(mCmdParam.ReadX.mSectorCount==0){k=0xff;break;}
			tempSec+=1;
			k=mLDirCheck(pDirName,pLDirName);
			z=k;
			z&=0x0f;
			if(k!=0x0ff){break;}			/*找到文件或者找到文件结尾退出*/
		 }
		   if(k<16){
						pDirName+=k;		/*所找的文件短文件名在此目录项*/
			    		if(j!=0xff){	
					  	 	for(k=0;k!=j+1;k++)mCmdParam.Other.mBuffer[k]=ParData[k];	
						}
						pBuf1=&mCmdParam.Other.mBuffer[j+1];	/*取文件名的地址*/
						//else pBuf1=&mCmdParam.Other.mBuffer;		
						for(i=0;i!=8;i++){
							if(pDirName->DIR_Name[i]==0x20)continue;
							else {
						 		*pBuf1=pDirName->DIR_Name[i];
						 		pBuf1++;
			 				}
						}
						if(pDirName->DIR_Name[i]!=0x20){
							*pBuf1='.';
							 pBuf1++;
						}
						for(;i!=11;i++){
							if(pDirName->DIR_Name[i]==0x20)continue;
							else {
								*pBuf1=pDirName->DIR_Name[i];
						 		pBuf1++;
			 				}

						}					/*复制短文件名*/
				    i=CH375FileClose();
				    	i=CH375FileCreate();					/*疑惑这里要不要恢复到刚进入此函数时的簇号*/
						return i;		/*创建文件,返回状态*/
		   	}
			else {					/*表示目录项枚举到结束位置，要创建文件*/
				if(k==0xff){z=00;tempSec+=1;}
				i=CH375FileClose();
					for(k=0;k!=MAX_PATH_LEN;k++)mCmdParam.Other.mBuffer[k]=ParData[k];		/*试创建文件短文件名*/					
					for(x=0x31;x!=0x3a;x++){					/*生成短文件名*/
						for(y=0x31;y!=0x3a;y++){
							for(i=0x31;i!=0x3a;i++){
								mCmdParam.Other.mBuffer[j+7]=i;
								mCmdParam.Other.mBuffer[j+6]='~';	
								mCmdParam.Other.mBuffer[j+5]=y;
								mCmdParam.Other.mBuffer[j+4]=x;
								if(CH375FileOpen()!=ERR_SUCCESS)goto XAA1;  /**/	
							}
						}
					
					}
					 i=0xff;
					 goto   XBB;				/*命名无法正确进行*/  
XAA1:
					i=CH375FileCreate();
					if(i!=ERR_SUCCESS);//{goto XCC;}			/*出错则不能继续进行*/
					for(k=0;k!=MAX_PATH_LEN;k++)ParData[k]=mCmdParam.Other.mBuffer[k];		/*试创建文件短文件名*/	
					i=mChkName(&j);
					mCmdParam.Locate.mSectorOffset=tempSec-1;
					i=CH375FileLocate();
					if(i!=ERR_SUCCESS);//{goto XCC;}			/*出错则不能继续进行*/
					mCmdParam.ReadX.mSectorCount=1;
					mCmdParam.ReadX.mDataBuffer=FILE_DATA_BUF_ADDR;	
					pDirName=FILE_DATA_BUF_ADDR;
					pDirName+=z;					/*指向创建文件名的偏移*/					
					i=CH375FileReadX();				/*读取下一个扇区的数据，取第一个目录项就是刚才创建的短文件名*/					
					if(i!=ERR_SUCCESS);//{goto XCC;}				/*这里要做出错误处理*/
					for(i=0;i!=CH375_FILE_LONG_NAME;i++){ 
						if(LongFileName[i]==00)break;			/*计算长文件名的长度*/
					}
					for(k=i+1;k!=CH375_FILE_LONG_NAME;k++){ 	/*将无效长目录处填充*/
						LongFileName[k]=0xffff;
					}
					pBuf=FILE_DATA_BUF_ADDR1;	/**/
					*pBuf=0;						/*清缓冲区一个字节*/														
					*pBuf=0;						/*清缓冲区一个字节*/																	
					k=i/13;							/*取长文件名组数*/
					i=i%13;
					if(i!=0)k=k+1;				/*有余数则算一组*/
					i=k;
					//pLDirName=(F_LONG_NAME   xdata *)pDirName;	
					k=i+z;					/*z为短文件偏移,z-1为长文件偏移*/
					if(k<16){
						pDirName1=FILE_DATA_BUF_ADDR;
						pDirName1+=k;
						pLDirName=FILE_DATA_BUF_ADDR;	
						pLDirName+=k-1;
					}
					else if(k==16){
					    pDirName1=FILE_DATA_BUF_ADDR1;
						pDirName1+=k-16;
						pLDirName=FILE_DATA_BUF_ADDR;
						pLDirName+=k-1;
						}
					else if(k>16){
					    pDirName1=FILE_DATA_BUF_ADDR1;
						pDirName1+=k-16;
						pLDirName=FILE_DATA_BUF_ADDR1;
						pLDirName+=k-1-16;
						}
					/*复制短文件名,将短文件名复制到指定区域*/
					pDirName1->DIR_NTRes=pDirName->DIR_NTRes;
					pDirName1->DIR_CrtTimeTenth=pDirName->DIR_CrtTimeTenth;
					pDirName1->DIR_CrtTime=pDirName->DIR_CrtTime;
					pDirName1->DIR_CrtDate=pDirName->DIR_CrtDate;
					pDirName1->DIR_LstAccDate=pDirName->DIR_LstAccDate;
					pDirName1->DIR_FstClusHI=pDirName->DIR_FstClusHI;
					pDirName1->DIR_WrtTime=pDirName->DIR_WrtTime;
					pDirName1->DIR_WrtDate=pDirName->DIR_WrtDate;
					pDirName1->DIR_FstClusLO=pDirName->DIR_FstClusLO;
					pDirName1->DIR_FileSize=pDirName->DIR_FileSize;
					pDirName1->DIR_Attr=pDirName->DIR_Attr;
					
					pDirName1->DIR_Name[0]=pDirName->DIR_Name[0];
					pDirName1->DIR_Name[1]=pDirName->DIR_Name[1];
					pDirName1->DIR_Name[2]=pDirName->DIR_Name[2];
					pDirName1->DIR_Name[3]=pDirName->DIR_Name[3];
					pDirName1->DIR_Name[4]=pDirName->DIR_Name[4];
					pDirName1->DIR_Name[5]=pDirName->DIR_Name[5];
					pDirName1->DIR_Name[6]=pDirName->DIR_Name[6];
					pDirName1->DIR_Name[7]=pDirName->DIR_Name[7];
					pDirName1->DIR_Name[8]=pDirName->DIR_Name[8];
					pDirName1->DIR_Name[9]=pDirName->DIR_Name[9];
					pDirName1->DIR_Name[10]=pDirName->DIR_Name[10];
					pDirName1->DIR_Name[10]=pDirName->DIR_Name[10];
					sum=ChkSum(pDirName1);				/*计算累加和*/
					pBuf=LongFileName;					/*指向长文件名空间*/
					y=1;
					if(k>16){
						for(i=1;i!=k-16+1;i++){					/*>?????*/
							pLDirName->LDIR_Ord=y;
							pLDirName->LDIR_Name1[0]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name1[1]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name1[2]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name1[3]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name1[4]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Attr=0x0f;
							pLDirName->LDIR_Type=0;		
							pLDirName->LDIR_Chksum=sum;
							pLDirName->LDIR_Name2[0]=*pBuf;
							pBuf++;		
							pLDirName->LDIR_Name2[1]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name2[2]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name2[3]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name2[4]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name2[5]=*pBuf;
							pBuf++;
							pLDirName->LDIR_FstClusLO[0]=0;
							pLDirName->LDIR_FstClusLO[1]=0;
							pLDirName->LDIR_Name3[0]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name3[1]=*pBuf;
							pBuf++;		
							pLDirName--;
							y+=1;						
						}
						k=16;
						k=16;
						k=16;
						i=0;
						pLDirName=FILE_DATA_BUF_ADDR;
						pLDirName+=k-1;
					  }	
					  if(k>16)k=16;
							for(i=1;i!=k-z;i++){					/*>?????*/
							pLDirName->LDIR_Ord=y;
							pLDirName->LDIR_Name1[0]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name1[1]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name1[2]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name1[3]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name1[4]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Attr=0x0f;
							pLDirName->LDIR_Type=0;		
							pLDirName->LDIR_Chksum=sum;
							pLDirName->LDIR_Name2[0]=*pBuf;
							pBuf++;		
							pLDirName->LDIR_Name2[1]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name2[2]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name2[3]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name2[4]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name2[5]=*pBuf;
							pBuf++;
							pLDirName->LDIR_FstClusLO[0]=0;
							pLDirName->LDIR_FstClusLO[1]=0;
							pLDirName->LDIR_Name3[0]=*pBuf;
							pBuf++;
							pLDirName->LDIR_Name3[1]=*pBuf;
							pBuf++;		
							pLDirName--;
							y+=1;
						}
						pLDirName->LDIR_Ord=y|0x40;
						pLDirName->LDIR_Name1[0]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name1[1]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name1[2]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name1[3]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name1[4]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Attr=0x0f;
						pLDirName->LDIR_Type=0;		
						pLDirName->LDIR_Chksum=sum;
						pLDirName->LDIR_Name2[0]=*pBuf;
						pBuf++;		
						pLDirName->LDIR_Name2[1]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name2[2]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name2[3]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name2[4]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name2[5]=*pBuf;
						pBuf++;
						pLDirName->LDIR_FstClusLO[0]=0;
						pLDirName->LDIR_FstClusLO[1]=0;
						pLDirName->LDIR_Name3[0]=*pBuf;
						pBuf++;
						pLDirName->LDIR_Name3[1]=*pBuf;
						pBuf++;		
						pBuf=(unsigned int xdata *)pDirName1;
						pBuf+=16;			
						if(pBuf<(FILE_DATA_BUF_ADDR+0x200)){
							i=2;
							while(1){
								*pBuf=0;
								pBuf++;
								if(pBuf==FILE_DATA_BUF_ADDR+0x200)break;			
							}
							i++;
						}
						else if(pBuf<(FILE_DATA_BUF_ADDR1+0x200)){
							i=1;
							while(1){
								*pBuf=0;
								pBuf++;
								if(pBuf==(FILE_DATA_BUF_ADDR1+0x200))break;			
							}
							i++;
						}
					mCmdParam.Locate.mSectorOffset=tempSec-1;
					CH375DirtyBuffer();
					i=CH375FileLocate();
					if(i!=ERR_SUCCESS);			/*出错则不能继续进行*/
					mCmdParam.ReadX.mSectorCount=1;						/*下面重新*/
					mCmdParam.ReadX.mDataBuffer=FILE_DATA_BUF_ADDR;	
						CH375DirtyBuffer();						
					i=CH375FileWriteX();				/*读取下一个扇区的数据，取第一个目录项就是刚才创建的短文件名*/					
					CH375DirtyBuffer(  );
					if(i!=ERR_SUCCESS);				/*这里要做出错误处理*/
					pBuf=FILE_DATA_BUF_ADDR1;	/**/
					if(*pBuf!=0){
					 	mCmdParam.ReadX.mSectorCount=1;
						mCmdParam.ReadX.mDataBuffer=FILE_DATA_BUF_ADDR1;	
						i=CH375FileWriteX();
					CH375DirtyBuffer(  );
					}
				/*如果是在根目录下操作应关闭根目录*/
				/*下面还要打开文件*/
			   i=CH375FileClose();	
			for(k=0;k!=MAX_PATH_LEN;k++)mCmdParam.Other.mBuffer[k]=ParData[k];		/*试创建文件短文件名*/	
			  i=CH375FileOpen();					/*打开创建的文件*/
			 return i;
			}
	}
XBB: {
  		return i=0xfe;
	}
}

/*删除指定短文件名的文件，同时删除对应的长文件名，返回状态和非长文件操作相同*/
UINT8  mdeleteFile(){
	UINT8  BlockSer1;				/*定义两个扇区块内记数*/
	unsigned char xdata ParData[MAX_PATH_LEN];	/*定义一个存放文件名的缓冲区*/
	UINT16	tempSec;						/*扇区偏移*/
	UINT8 a,i,j,k,x,sum;
	F_LONG_NAME   xdata *pLDirName;		/*长文件名指针*/ 
	P_FAT_DIR_INFO  pDirName;			/*短文件名指针*/	
	bit  FBuf;							/*定义一个文件缓冲区的翻转位*/
	unsigned char xdata *pBuf;			/*指向缓冲区的指针*/
	for(k=0;k!=MAX_PATH_LEN;k++)ParData[k]=mCmdParam.Other.mBuffer[k];			/**/
	i=mChkName(&j);
	if ( i == ERR_SUCCESS ) {  /* 成功获取上级目录的起始簇号 */
		BlockSer1=0;
		FBuf=0;					/*初始化*/	
		tempSec=0;				
	    while(1){							/*下面是读取并分析目录项*/
				pDirName=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;		/*短文件名指针指向缓冲区*/		
				mCmdParam.ReadX.mSectorCount=1;				/*读取一扇区数据*/
				mCmdParam.ReadX.mDataBuffer=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;  /*当前处理的文件缓冲区,这里使用双向缓冲区，去处理文件名*/
				FBuf=!FBuf;												/*缓冲区标志翻转*/
				i=CH375FileReadX( );
				if(i!=ERR_SUCCESS)goto XLL;				
				if(mCmdParam.ReadX.mSectorCount==0){k=16;break;}			/*表示没有数据读出*/
				tempSec+=1;												/*扇区记数加一*/
				for(k=0;k!=16;k++){																	
				pBuf=&ParData[j+1];						
				if(pDirName->DIR_Name[0]==0){k=15;a=1;continue;}		/*第一个字节为0，表示以后没有有效的目录项了*/
				if(pDirName->DIR_Name[0]==0xe5){pDirName++;continue;}			/*第一个字节为0XE5表示此项被删除*/
				if(pDirName->DIR_Attr==ATTR_VOLUME_ID){pDirName++;continue;}		/*为卷标，不分析*/
				if(pDirName->DIR_Attr==ATTR_LONG_NAME){pDirName++;continue;}		/*为长文件名，不分析*/
				for(i=0;i!=8;i++){									/*分析文件名是否相同*/
					if(pDirName->DIR_Name[i]==0x20)continue;		/*为20不分析*/        
					if(pDirName->DIR_Name[i]!=*pBuf)break;
					else pBuf++;
				}
				if(i!=8){pDirName++;continue;}						/*没有找到匹配的短文件名*/
				if(*pBuf=='.')pBuf++;
				for(;i!=11;i++){
					if(pDirName->DIR_Name[i]==0x20)continue;				
					if(pDirName->DIR_Name[i]!=*pBuf)break;		
					else pBuf++;
				}
				if(i==11){break;}								/*表示找到文件名*/
				pDirName++;
			}
			if(i==11)break;								/*找到*/
			if(a==1){k=16;break;}
		}
		if(k!=16){
				 x=0;      /*下面找短文件名然后删除*/
				 sum=ChkSum(pDirName); 					       /*计算和*/				
				 pLDirName=FBuf?FILE_DATA_BUF_ADDR:FILE_DATA_BUF_ADDR1;	/*长文件名指针指向缓冲区*/
				 pLDirName+=k-1;
				 if(k==0){pLDirName=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;  /*如果短文件名是丛第一组开始的，长文件名就要往起移动*/
						pLDirName+=15;						
						k=15;					
					}
				while(1){	
					if((pLDirName->LDIR_Attr==0x0f)&(pLDirName->LDIR_Chksum==sum)&(pLDirName->LDIR_Ord!=0xe5)){
						pLDirName->LDIR_Ord=0xe5;				/*删除长文件名*/
				   		x++;
						k=k-1;
					}
					else break;							/*没有长文件名则条出*/
					if(x==15)break;						/*最大限制长文件名为16*13字节*/
					if(k==0){								/*首先移动文件指针*/					
						pLDirName=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;
						pLDirName+=15;														
					}
					else {pLDirName-=1; k-=1;}
				}
					if(tempSec!=0){
						tempSec-=1;
						mCmdParam.Locate.mSectorOffset=(unsigned long)tempSec;		/*将分析过的目录项重新写入*/
						i=CH375FileLocate();
						if(i!=ERR_SUCCESS)goto XLL;	
						mCmdParam.ReadX.mSectorCount=1;
						mCmdParam.ReadX.mDataBuffer=FBuf?FILE_DATA_BUF_ADDR:FILE_DATA_BUF_ADDR1;
						i=CH375FileWriteX( );
						if(i!=ERR_SUCCESS)goto XLL;
					}
				if(tempSec!=0){
					tempSec-=1;
					mCmdParam.Locate.mSectorOffset=(unsigned long)tempSec;		/*将分析过的目录项重新写入*/
					i=CH375FileLocate();
					if(i!=ERR_SUCCESS)goto XLL;	
					mCmdParam.ReadX.mSectorCount=1;
					mCmdParam.ReadX.mDataBuffer=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;
					i=CH375FileWriteX( );
					if(i!=ERR_SUCCESS)goto XLL;
				}
			}
	}		
	CH375DirtyBuffer(  );		
	i=CH375FileClose( );		
	CH375DirtyBuffer(  );						/*清磁盘缓冲区*/
	for(k=0;k!=MAX_PATH_LEN;k++)mCmdParam.Other.mBuffer[k]=ParData[k];			/**/	
	i=CH375FileErase( );
	return i;
XLL:  return i;								/*出错时，返回错误信息*/						
}

/*读取指定短文件名的长文件名,返回长文件名在长文件名空间*/
UINT8  mLookUpLName(){
	UINT8  BlockSer1;				/*定义两个扇区块内记数*/
	unsigned char xdata ParData[MAX_PATH_LEN];			/**/
	UINT16	tempSec;						/*扇区偏移*/
	UINT8 a,i,j,x,k,sum;
	F_LONG_NAME   xdata *pLDirName; 
	P_FAT_DIR_INFO  pDirName;
	bit  FBuf;
	unsigned char xdata *pBuf;
	unsigned int xdata *pBuf1;
	for(k=0;k!=MAX_PATH_LEN;k++)ParData[k]=mCmdParam.Other.mBuffer[k];			/**/
		i=mChkName(&j);	
	if ( i == ERR_SUCCESS ) {  /* 成功获取上级目录的起始簇号 */
		BlockSer1=0;
		FBuf=0;					/*初始化*/	
		tempSec=0;				
		while(1){							/*下面是读取并分析目录项*/
			pDirName=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;		/*短文件名指针指向缓冲区*/		
			mCmdParam.ReadX.mSectorCount=1;				/*读取一扇区数据*/
			mCmdParam.ReadX.mDataBuffer=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;  /*当前处理的文件缓冲区,这里使用双向缓冲区，去处理文件名*/
			FBuf=!FBuf;												/*缓冲区标志翻转*/
			i=CH375FileReadX( );
			if(i!=ERR_SUCCESS)goto XFF;
			if(mCmdParam.ReadX.mSectorCount==0){k=16;break;}			/*表示没有数据读出*/
			tempSec+=1;												/*扇区记数加一*/
			for(k=0;k!=16;k++){																	
				pBuf=&ParData[j+1];						
				if(pDirName->DIR_Name[i]==0){k=15;a=1;continue;}		/*第一个字节为0，表示以后没有有效的目录项了*/
				if(pDirName->DIR_Name[i]==0xe5){pDirName++;continue;}			/*第一个字节为0XE5表示此项被删除*/
				if(pDirName->DIR_Attr==ATTR_VOLUME_ID){pDirName++;continue;}		/*为卷标，不分析*/
				if(pDirName->DIR_Attr==ATTR_LONG_NAME){pDirName++;continue;}		/*为长文件名，不分析*/
				for(i=0;i!=8;i++){									/*分析文件名是否相同*/
				if(pDirName->DIR_Name[i]==0x20)continue;		/*为20不分析*/        
				if(pDirName->DIR_Name[i]!=*pBuf)break;
				else pBuf++;
			}
			if(i!=8){pDirName++;continue;}						/*没有找到匹配的短文件名*/
			if(*pBuf=='.')pBuf++;
			for(;i!=11;i++){
				if(pDirName->DIR_Name[i]==0x20)continue;				
				if(pDirName->DIR_Name[i]!=*pBuf)break;		
				else pBuf++;
			}
			if(i==11){break;}								/*表示找到文件名*/
			pDirName++;
			}
			if(i==11)break;								/*找到*/
			if(a==1){k=16;break;}
		}
		if(k!=16){
					pBuf1=LongFileName;
						 x=0;
				 sum=ChkSum(pDirName); 					       /*计算和*/				
				 pLDirName=FBuf?FILE_DATA_BUF_ADDR:FILE_DATA_BUF_ADDR1;	/*长文件名指针指向缓冲区*/
				 pLDirName+=k-1;
				 if(k==0){pLDirName=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;  /*如果短文件名是丛第一组开始的，长文件名就要往起移动*/
						pLDirName+=15;						
						k=15;					
					}
				while(1){	
					if(pLDirName->LDIR_Attr==0x0f&pLDirName->LDIR_Chksum==sum&pLDirName->LDIR_Ord!=0xe5){
						for(j=0;j!=5;j++){	
						*pBuf1=pLDirName->LDIR_Name1[j];
						pBuf1++;
						}
						for(;j!=11;j++){	
						*pBuf1=pLDirName->LDIR_Name2[j-5];
						pBuf1++;
						}
							for(;j!=13;j++){	
						*pBuf1=pLDirName->LDIR_Name3[j-11];
						pBuf1++;
						}
						/*这里将长文件名复制出去，最大16*13个长文件名*/					
				   		x++;	 
					}
					else break;							/*没有长文件名则条出*/
					if(x==15)break;						/*最大限制长文件名为16*13字节*/
					if(k==0){						/*首先移动文件指针*/					
						pLDirName=FBuf?FILE_DATA_BUF_ADDR1:FILE_DATA_BUF_ADDR;
						pLDirName+=15;														
					}
					else {pLDirName-=1; k-=1;}
				}
			}					
	}
	*pBuf1=0;
	pBuf1++;
	i=CH375FileClose( );	
	return 0;					
XFF:  return i;
}

main( ) {
	UINT8	i,k;
	UINT16 X;
	LED_OUT_INIT( );
	LED_OUT_ACT( );  /* 开机后LED亮一下以示工作 */
	mDelaymS( 100 );  /* 延时100毫秒 */
	LED_OUT_INACT( );
	mInitSTDIO( );  /* 为了让计算机通过串口监控演示过程 */
	printf( "Start\n" );
	i = CH375LibInit( );  /* 初始化CH375程序库和CH375芯片,操作成功返回0 */
	mStopIfError( i );
/* 其它电路初始化 */

	while ( 1 ) {
	//	printf( "Wait Udisk\n" );
		while ( CH375DiskStatus != DISK_CONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘插入 */
		LED_OUT_ACT( );  /* LED亮 */
		mDelaymS( 200 );  /* 延时,可选操作,有的USB存储器需要几十毫秒的延时 */

/* 检查U盘是否准备好,有些U盘不需要这一步,但是某些U盘必须要执行这一步才能工作 */
		for ( i = 0; i < 5; i ++ ) {  /* 有的U盘总是返回未准备好,不过可以被忽略 */
			mDelaymS( 100 );
	//		printf( "Ready ?\n" );
			if ( CH375DiskReady( ) == ERR_SUCCESS ) break;  /* 查询磁盘是否准备好 */
		}
/* 查询磁盘物理容量 */
//		printf( "DiskSize\n" );
//		i = CH375DiskSize( );
//		mStopIfError( i );
//		printf( "TotalSize = %u MB \n", (unsigned int)( mCmdParam.DiskSize.mDiskSizeSec >> 11 ) );  /* 显示为以MB为单位的容量 */
		LED_RUN_ACT( );  /* 开始操作U盘 */

		X=0X4100;
		for(k=0;k!=0x0F;k++){		/*演示创建长文件名*/
			mCmdParam.Erase.mPathName[0]='/';   /*给出参考短文件名*/
			mCmdParam.Erase.mPathName[1]='C';
			mCmdParam.Erase.mPathName[2]='/';
			mCmdParam.Erase.mPathName[3]='A';
			mCmdParam.Erase.mPathName[4]='B';
			mCmdParam.Erase.mPathName[5]='C';
			mCmdParam.Erase.mPathName[6]='D';		
			mCmdParam.Erase.mPathName[7]='A';
			mCmdParam.Erase.mPathName[8]='~';
			mCmdParam.Erase.mPathName[9]='1';
			mCmdParam.Erase.mPathName[10]='2';
			mCmdParam.Erase.mPathName[11]='.';
			mCmdParam.Erase.mPathName[12]='C';
			mCmdParam.Erase.mPathName[13]=00;
			LongFileName[0]=0X4100;					/*给出UNICODE的长文件名*/						
			LongFileName[1]=0X4200;    /* 对于小端数据格式的单片机,例如AVR/MSP430/ARM等是0x0042,下同 */
			LongFileName[2]=0X4300;
			LongFileName[3]=0X4400;
			LongFileName[4]=0X4500;
			LongFileName[5]=0X4600;
			LongFileName[6]=0X4700;
			LongFileName[7]=0X4800;
			LongFileName[8]=0X4100;
			LongFileName[9]=0X6300;
			LongFileName[10]=0X6200;
			LongFileName[11]=0X6100;
			LongFileName[12]=X;
			LongFileName[13]=X;
			LongFileName[14]=0X2e00;
			LongFileName[15]=0X4300;
			LongFileName[16]=0X0000;
	
			i=mCreatLName();				/*创建长文件名*/			
		X+=0X100;
	}

		X=0X4100;
		for(k=0;k!=0x0F;k++){		/*演示创建长文件名*/
		mCmdParam.Erase.mPathName[0]='/';   /*给出参考短文件名*/
		mCmdParam.Erase.mPathName[1]='A';
		mCmdParam.Erase.mPathName[2]='B';
		mCmdParam.Erase.mPathName[3]='C';
		mCmdParam.Erase.mPathName[4]='D';		
		mCmdParam.Erase.mPathName[5]='A';
		mCmdParam.Erase.mPathName[6]='~';
		mCmdParam.Erase.mPathName[7]='1';
		mCmdParam.Erase.mPathName[8]='1';
		mCmdParam.Erase.mPathName[9]='.';
		mCmdParam.Erase.mPathName[10]='C';
		mCmdParam.Erase.mPathName[11]=00;

		LongFileName[0]=0X6100;					/*给出UNICODE的长文件名*/						
		LongFileName[1]=0X4200;
		LongFileName[2]=0X6300;
		LongFileName[3]=0X4400;
		LongFileName[4]=0X4500;
		LongFileName[5]=0X6500;
		LongFileName[6]=0X4700;
		LongFileName[7]=0X4800;
		LongFileName[8]=0X4100;
		LongFileName[9]=0X6300;
		LongFileName[10]=0X6200;
		LongFileName[11]=0X6100;
		LongFileName[12]=X;
		LongFileName[13]=X;
		LongFileName[14]=0X2e00;
		LongFileName[15]=0X4300;
		LongFileName[16]=0X0000;
	
			i=mCreatLName();				/*创建长文件名*/			
		X+=0X100;
	}
	X=0X4100;

for(k=0;k!=0x4;k++){	    /*查找并删除*/
		mCmdParam.Erase.mPathName[0]='/';		/*给出路径*/
		mCmdParam.Erase.mPathName[1]='C';
		mCmdParam.Erase.mPathName[2]='/';
		mCmdParam.Erase.mPathName[3]=00;
		LongFileName[0]=0X4100;				/*给出长文件名*/					
		LongFileName[1]=0X4200;
		LongFileName[2]=0X4300;
		LongFileName[3]=0X4400;
		LongFileName[4]=0X4500;
		LongFileName[5]=0X4600;
		LongFileName[6]=0X4700;
		LongFileName[7]=0X4800;
		LongFileName[8]=0X4100;
		LongFileName[9]=0X6300;
		LongFileName[10]=0X6200;
		LongFileName[11]=0X6100;
		LongFileName[12]=X;
		LongFileName[13]=X;
		LongFileName[14]=0X2e00;
		LongFileName[15]=0X4300;
		LongFileName[16]=0X0000;

		i=mLoopkUpSName();			/*查找短文件名*/
		i=mdeleteFile();			/*删除文件*/	
		X+=0X100;
	}

		X=0;
	while(1)
	{	
		mCmdParam.Erase.mPathName[0]='/';		/*给出路径*/
		mCmdParam.Erase.mPathName[1]='C';
		mCmdParam.Erase.mPathName[2]='/';
		mCmdParam.Erase.mPathName[3]='*';
		mCmdParam.Erase.mPathName[4]=X;
		mCmdParam.Erase.mPathName[5]=00;
		mCmdParam.Erase.mPathName[6]=00;
		i=CH375FileOpen();
		if ( i == ERR_MISS_FILE ) break;  /* 再也搜索不到匹配的文件,已经没有匹配的文件名 */
		if ( i == ERR_FOUND_NAME ) {  /* 搜索到与通配符相匹配的文件名,文件名及其完整路径在命令缓冲区中 */
		   i=mLookUpLName();		/*搜索对应的长文件名*/
		 }
		X++;
		if(X==0xfE)break;
	}		
		LED_WR_INACT( );
		LED_RUN_INACT( );
		printf( "Take out\n" );
		while ( CH375DiskStatus != DISK_DISCONNECT ) xQueryInterrupt( );  /* 查询CH375中断并更新中断状态,等待U盘拔出 */
		LED_OUT_INACT( );  /* LED灭 */
		mDelaymS( 200 );
	}
}
