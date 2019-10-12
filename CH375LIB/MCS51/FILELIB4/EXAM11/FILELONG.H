#ifndef  CH375_LONG_NAME
#define  CH375_LONG_NAME  1
#endif

typedef struct _LONG_NAME{
	UINT8  LDIR_Ord ;		/*长文件名的组号，如果为0X40则表示最后一个组*/
	UINT16  LDIR_Name1[5];		/*长文件名的前5个字节*/
	UINT8  LDIR_Attr;		/*属性必须为ATTR_LONG_NAME*/
	UINT8  LDIR_Type;		/* 为0表示长文件名的子项*/
	UINT8  LDIR_Chksum;		/*短文件名的校验和*/
	UINT16  LDIR_Name2[6];		/*长文件名的6-11个字符*/
	UINT8  LDIR_FstClusLO[2];	/*为0*/
	UINT16  LDIR_Name3[2];		/*长文件名的12-13各自。字符*/
		
} F_LONG_NAME;				/*定义长文件名*/

#ifdef  CH375_LONG_NAME
#define MAX_LONG_NAME  4
#define  CH375_FILE_LONG_NAME    MAX_LONG_NAME*13+1

unsigned int xdata LongFileName[CH375_FILE_LONG_NAME];	/*长文件名空间只存储文件名不存路径*/	

/*bit 0 无长文件名*/
/*bit 1  有长文件名*/

#endif

