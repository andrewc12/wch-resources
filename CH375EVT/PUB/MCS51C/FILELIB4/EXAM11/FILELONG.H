#ifndef  CH375_LONG_NAME
#define  CH375_LONG_NAME  1
#endif

typedef struct _LONG_NAME{
	UINT8  LDIR_Ord ;		/*���ļ�������ţ����Ϊ0X40���ʾ���һ����*/
	UINT16  LDIR_Name1[5];		/*���ļ�����ǰ5���ֽ�*/
	UINT8  LDIR_Attr;		/*���Ա���ΪATTR_LONG_NAME*/
	UINT8  LDIR_Type;		/* Ϊ0��ʾ���ļ���������*/
	UINT8  LDIR_Chksum;		/*���ļ�����У���*/
	UINT16  LDIR_Name2[6];		/*���ļ�����6-11���ַ�*/
	UINT8  LDIR_FstClusLO[2];	/*Ϊ0*/
	UINT16  LDIR_Name3[2];		/*���ļ�����12-13���ԡ��ַ�*/
		
} F_LONG_NAME;				/*���峤�ļ���*/

#ifdef  CH375_LONG_NAME
#define MAX_LONG_NAME  4
#define  CH375_FILE_LONG_NAME    MAX_LONG_NAME*13+1

unsigned int xdata LongFileName[CH375_FILE_LONG_NAME];	/*���ļ����ռ�ֻ�洢�ļ�������·��*/	

/*bit 0 �޳��ļ���*/
/*bit 1  �г��ļ���*/

#endif
