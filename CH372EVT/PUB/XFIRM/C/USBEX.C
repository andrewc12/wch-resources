/*;CH372/CH375 USB device mode & external firmware
; U2(AT89C51) Program
;
; Website:  http://winchiphead.com
; Email:    tech@winchiphead.com
; Author:   W.ch 2003.12, 2005.03
;
;****************************************************************************
CH375 �ⲿ�̼���ʽ����
�����������׼����Ͷ˵�2�ļ򵥶�д

*/

/* MCS-51��Ƭ��C���Ե�ʾ������ */
#pragma NOAREGS
#include <reg52.h>
#include "CH375INC.H"

typedef	union _REQUEST_PACK{
	unsigned char  buffer[8];
	struct{
		unsigned char	 bmReuestType;    	 //��׼������
		unsigned char	 bRequest;		   	//�������
		unsigned int     wValue;			//����ѡ���
		unsigned int     wIndx;				//����
		unsigned int     wLength;				//���ݳ���
	}r;
} mREQUEST_PACKET,	*mpREQUEST_PACKET;

//�豸������
unsigned char  code DevDes[]={
						0x12			//��������С			
					  , 0x01			//����DEVICE
					 , 0x10				//USB�淶�汾��Ϣ
				     , 	0x01
					,   0xFF			//����룬
					,  0x80				//�������	
					,   0x37			//Э����
					,  0x08				//�˵�0�������Ϣ����С
					,  0x48				//����ID
					,   0x43
					,   0x37			//��ƷID	
					,   0x55
					,   0x00			//�豸�汾��Ϣ
					,   0x01
					,   0x00			//����ֵ	
					,   0x00
					,   0x00
					,   0x01			//�������õ���Ŀ	
					,   00				//������
					,   00
					,   00
					,	00
					,	00
					,	00
					};
//����������
unsigned char   code ConDes[]={			//����������
					  0x09					//��������С
					,  0x02					//����CONFIG
					,  0x27					//�����ô����������ݴ�С
					,  0x00					//
					,  0x01					//�ӿ���
					,  0x01					//����ֵ
					,  0x00					//����
					,  0x80					//��Դ����
					,  0x40					//��Ҫ���ߵ�Դ
										//�ӿ�������
					,  0x09					//��������С								
					,  0x04					//����INTERFACE				
					,  0x00					//ʶ����
					,  0x00					//������ֵ
					,  0x03					//֧�ֵĶ˵���
					,  0xFF					//�����
					,  0x80					//�������
					,  0x37					//Э����
					,  0x00					//����
										//�˵�������
					,  0x07					//������С
					,  0x05					//����ENDPOINT
					,  0x82					//�˵���Ŀ������
					,  0x02					//֧�ֵĴ�������
					,  0x40					//֧�ֵ������Ϣ����С
					,  0x00
					,  0x00					//
				
					,  0x07
					,  0x05
					,  0x02
					,  0x02
					,  0x40
					,  0x00
					,  0x00
					
					,  0x07
					,  0x05
					,  0x81
					,  0x03
					,  0x08
					,  0x00
					,  0x01

					,  0x07
					,  0x05
					,  0x01
					,  0x02
					,  0x08
					,  0x00
					,  0x00
				};		//����������
unsigned char  code LangDes[]={0x04,0x03,0x09,0x04};		//����������
unsigned char  code SerDes[]={0x12,0x03,'C',0,'H',0,'3',0,'7',0,'5',0,'U',0,'S',0,'B',0};		//�ַ���������

unsigned char mVarSetupRequest;						//	;USB������
unsigned short mVarSetupLength;					//		;�������ݳ���
unsigned char  code * VarSetupDescr;						//	;������ƫ�Ƶ�ַ

unsigned char VarUsbAddress	;					//

bit CH375FLAGERR;						//������0
bit CH375TRANSBIT;
bit	CH375CONFLAG;
					                     //���ñ�־

unsigned char volatile xdata CH375_CMD_PORT _at_ 0xBDF1;		/* CH375����˿ڵ�I/O��ַ */
unsigned char volatile xdata CH375_DAT_PORT _at_ 0xBCF0;		/* CH375���ݶ˿ڵ�I/O��ַ */

mREQUEST_PACKET  request;
sbit  CH375ACT  = P1^4;


/* ��ʱ2΢��,����ȷ */
void Delay1us(){
	;
}

void	Delay2us( )
{
	unsigned char i;
#define DELAY_START_VALUE	1  								/* ���ݵ�Ƭ����ʱ��ѡ���ֵ,20MHz����Ϊ0,30MHz����Ϊ2 */
	for ( i=DELAY_START_VALUE; i!=0; i-- );
}

/* ��ʱ50����,����ȷ */
void	Delay50ms( )
{
	unsigned char i, j;
	for ( i=200; i!=0; i-- ) for ( j=250; j!=0; j-- );
}

/* ��PC���ĵ��ֽ���ǰ��16λ������ת��ΪC51�ĸ��ֽ���ǰ������ */
//unsigned int	BIG_ENDIAN( unsigned int value )
//{
//	unsigned int  in, out;
//	in = value;
//	((unsigned char *)&out)[1] = ((unsigned char *)&in)[0];
//	((unsigned char *)&out)[0] = ((unsigned char *)&in)[1];
//	return( out );
//}

void CH375_WR_CMD_PORT( unsigned char cmd ) { 				 /* ��CH375������˿�д������,���ڲ�С��4uS,�����Ƭ���Ͽ�����ʱ */
	//delay2us();
	CH375_CMD_PORT=cmd;
	Delay2us( );
}

void CH375_WR_DAT_PORT( unsigned char dat ) { 				 /* ��CH375�����ݶ˿�д������,���ڲ�С��1.5uS,�����Ƭ���Ͽ�����ʱ */
	CH375_DAT_PORT=dat;
	Delay1us();  											/* ��ΪMCS51��Ƭ����������ʵ����������ʱ */
}

unsigned char CH375_RD_DAT_PORT() {  						/* ��CH375�����ݶ˿ڶ�������,���ڲ�С��1.5uS,�����Ƭ���Ͽ�����ʱ */
	Delay1us( );  										/* ��ΪMCS51��Ƭ����������ʵ����������ʱ */
	return( CH375_DAT_PORT );

}
/* CH375��ʼ���ӳ��� */
void	CH375_Init( )
{
/* ����USB����ģʽ, ��Ҫ���� */
	CH375_WR_CMD_PORT( CMD_SET_USB_MODE );
	CH375_WR_DAT_PORT( 1 );  									/* ����Ϊʹ�����ù̼���USB�豸��ʽ */
	for ( ;; ) {  											/* �ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS */
		if ( CH375_RD_DAT_PORT( )==CMD_RET_SUCCESS ) break;
	}

/* ���������ж�,�ٶ�CH375������INT0 */
	IT0 = 0;  /* ���ⲿ�ź�Ϊ�͵�ƽ���� */
	IE0 = 0;  /* ���жϱ�־ */
	EX0 = 1;  /* ����CH375�ж� */
}
//*********************************************************

//*********************************************************
//�˵�0�����ϴ�
void mCh375Ep0Up(){
	unsigned char i,len;
	if(mVarSetupLength){												//���Ȳ�Ϊ0������峤�ȵ�����
		if(mVarSetupLength<=8){
			len=mVarSetupLength;
			mVarSetupLength=0;
        }	//����С��8����Ҫ��ĳ���
		else{
			len=8;
			mVarSetupLength-=8;
		}							                        		//���ȴ���8����8�������ܳ��ȼ�8
	    CH375_WR_CMD_PORT(CMD_WR_USB_DATA3);						//����д�˵�0������
       	CH375_WR_DAT_PORT(len);										//д�볤��
    	for(i=0;i!=len;i++)
        CH375_WR_DAT_PORT(request.buffer[i]);	              		//ѭ��д������
    }
	else{
		CH375_WR_CMD_PORT(CMD_WR_USB_DATA3);						//����д�˵�0������
		CH375_WR_DAT_PORT(0);					                   //�ϴ�0�������ݣ�����һ��״̬�׶�
	}
}


//*********************************************************

//�����������Ա��ϴ�
void mCh375DesUp(){
	unsigned char k;        
	for (k=0; k!=8; k++ ) {
         request.buffer[k]=*VarSetupDescr;  								//���θ���8����������
         VarSetupDescr++;
    }
}

/* CH375�жϷ������INT0,ʹ�üĴ�����1 */
void	mCH375Interrupt( ) interrupt 0 using 1
{
	unsigned char InterruptStatus;
	unsigned char length, c1, len;
	unsigned char   *pBuf;
	unsigned char   mBuf[64];
	CH375_WR_CMD_PORT(CMD_GET_STATUS);  									/* ��ȡ�ж�״̬��ȡ���ж����� */
	InterruptStatus =CH375_RD_DAT_PORT();  									/* ��ȡ�ж�״̬ */
	IE0 = 0;  																/* ���жϱ�־,��Ӧ��INT0�ж� */
	switch(InterruptStatus){  // �����ж�״̬
		case  USB_INT_EP2_OUT:  											// �����˵��´��ɹ� 
			pBuf=mBuf;																	//����δ����
			CH375_WR_CMD_PORT(CMD_RD_USB_DATA);									//��������������
			length=CH375_RD_DAT_PORT();											//���ȶ������ǳ���														
			for(len=0;len!=length;len++,pBuf++)*pBuf=CH375_RD_DAT_PORT();	//�����ݶ��뵽������			 
			pBuf=mBuf;
// ��ʾ�ش�
			CH375_WR_CMD_PORT(CMD_WR_USB_DATA7);								//����д�ϴ��˵�����
			CH375_WR_DAT_PORT(length);	
			for(len=0;len!=length;len++,pBuf++)CH375_WR_DAT_PORT(*pBuf);	//������д���ϴ��˵�
			break;
		case   USB_INT_EP2_IN:												 //�����˵��ϴ��ɹ�,δ����
			CH375_WR_CMD_PORT (CMD_UNLOCK_USB);								//�ͷŻ�����
			break;
		case   USB_INT_EP1_IN:	 											//�ж϶˵��ϴ��ɹ���δ����
			CH375_WR_CMD_PORT (CMD_UNLOCK_USB);								//�ͷŻ�����
			break;
		case   USB_INT_EP1_OUT:	  											//�ж϶˵��´��ɹ���δ����
			CH375_WR_CMD_PORT(CMD_RD_USB_DATA);									//��������������
			if(length=CH375_RD_DAT_PORT()){										//����Ϊ0����
				for(len=0;len!=length;len++)c1=CH375_RD_DAT_PORT();					//ȡ���´�����
			}
//			CH375_WR_CMD_PORT (CMD_UNLOCK_USB);								//�ͷŻ�����,���ǰ��û��CMD_RD_USB_DATA������ʹ�ñ�����
			break;
		case   USB_INT_EP0_SETUP: 											//���ƶ˵㽨���ɹ�
	    	CH375_WR_CMD_PORT(CMD_RD_USB_DATA);
			length=CH375_RD_DAT_PORT();
			for(len=0;len!=length;len++)request.buffer[len]=CH375_RD_DAT_PORT();  // ȡ������
			mVarSetupRequest = 0xff;										//������״̬
			CH375TRANSBIT = 0;												//���־
			if(length==0x08){
			    mVarSetupLength=(request.buffer[6] | (unsigned short)request.buffer[7]<<8);							//���ƴ������ݳ����������Ϊ128
				if( !(request.r.bmReuestType&0x80) ){									//��ʾ��Ҫ�ϴ�����
						CH375FLAGERR = 2;																	//�ϴ������ñ�־
				}
				else CH375FLAGERR = 0;																//�´��������־
				if((c1=request.r.bmReuestType)&0x40){         					 //��������δ����
				}
				if((c1=request.r.bmReuestType)&0x20){          					//������δ����
				}
				if(!((c1=request.r.bmReuestType)&0x60)){          				//��׼����
					mVarSetupRequest=request.r.bRequest;							//�ݴ��׼������
					switch(request.r.bRequest){  // ������׼����
						case DEF_USB_CLR_FEATURE:									//�������
							if((c1=request.r.bmReuestType&0x1F)==0X02){					//���Ƕ˵㲻֧��
								switch(request.buffer[4]){
									case 0x82:
										CH375_WR_CMD_PORT(CMD_SET_ENDP7);					//����˵�2�ϴ�
										CH375_WR_DAT_PORT(0x8E);                			//����������˵�
										break;
									case 0x02:
										CH375_WR_CMD_PORT(CMD_SET_ENDP6);
										CH375_WR_DAT_PORT(0x80);							//����˵�2�´�
										break;
									case 0x81:
										CH375_WR_CMD_PORT(CMD_SET_ENDP5);					//����˵�1�ϴ�
										CH375_WR_DAT_PORT(0x8E);
										break;
									case 0x01:
										CH375_WR_CMD_PORT(CMD_SET_ENDP4);					//����˵�1�´�
										CH375_WR_DAT_PORT(0x80);
										break;
									default:
										break;
								}
							}
							else{
								CH375FLAGERR=1;								//��֧�ֵ�������ԣ��ô����־
							}
							break;
						case DEF_USB_GET_STATUS:								//���״̬
							request.buffer[0]=0;
							request.buffer[1]=0;								//�ϴ�״̬
							break;
						case DEF_USB_SET_ADDRESS:								//���õ�ַ
							VarUsbAddress=request.buffer[2];					//�ݴ�USB���������ĵ�ַ
							break;
						case DEF_USB_GET_DESCR: 								//���������
							if(request.buffer[3]==1)							//�豸�������ϴ�
								VarSetupDescr=DevDes;
							else if(request.buffer[3]==2){		 					//�����������ϴ�
								if(mVarSetupLength > (ConDes[2] | (unsigned short)ConDes[3]<<8))
									mVarSetupLength = ConDes[2] | (unsigned short)ConDes[3]<<8;
								VarSetupDescr=ConDes;
							}
							else if(request.buffer[3]==3) {
								if ( request.buffer[2]== 0 ) VarSetupDescr=LangDes;
								else VarSetupDescr=SerDes; 						//���ַ�������
							}
							mCh375DesUp();											//������������֧��					          							
							break;
						case DEF_USB_GET_CONFIG:									//�������
							request.buffer[0]=0;									//û��������0
							if(CH375CONFLAG) request.buffer[0]=1;									//�Ѿ�������1����������������涨��
							break;
						case DEF_USB_SET_CONFIG:                 					//��������
							CH375CONFLAG=0;
							CH375ACT=1;
							if ( request.buffer[2] != 0 ) {
								CH375CONFLAG=1;											//�������ñ�־
								CH375ACT=0;												//�����������ź�
							}
							break;
						case DEF_USB_GET_INTERF:										//�õ��ӿ�
							request.buffer[0]=1;									//�ϴ��ӿ�����������ֻ֧��һ���ӿ�
							break;
						default :
							CH375FLAGERR=1;											//��֧�ֵı�׼����
							break;
					}
				}
			}
			else {  //��֧�ֵĿ��ƴ��䣬����8�ֽڵĿ��ƴ���
				CH375FLAGERR=1;
			}
			if(CH375FLAGERR==0) mCh375Ep0Up();										//û�д���/���������ϴ���������Ϊ0�ϴ�Ϊ״̬
			else if( CH375FLAGERR == 2 ) {
				if( mVarSetupLength == 0 ){											//�ϴ�0������
					CH375FLAGERR = 0;
					mCh375Ep0Up();	
				}
			}
			else{
				CH375_WR_CMD_PORT(CMD_SET_ENDP3);								//���ö˵�1ΪSTALL��ָʾһ������
				CH375_WR_DAT_PORT(0x0F);
			}
			break;
		case   USB_INT_EP0_IN:													//���ƶ˵��ϴ��ɹ�
			if(mVarSetupRequest==DEF_USB_GET_DESCR){								//�������ϴ�
				mCh375DesUp();
				mCh375Ep0Up();															
			}
			else if(mVarSetupRequest==DEF_USB_SET_ADDRESS){							//���õ�ַ
				CH375_WR_CMD_PORT(CMD_SET_USB_ADDR);
				CH375_WR_DAT_PORT(VarUsbAddress);								//����USB��ַ,�����´������USB��ַ
			}
			CH375_WR_CMD_PORT (CMD_UNLOCK_USB);								//�ͷŻ�����
			break;
		case   USB_INT_EP0_OUT:													//���ƶ˵��´��ɹ�
			CH375_WR_CMD_PORT(CMD_RD_USB_DATA);									//��������������
			if(length=CH375_RD_DAT_PORT()){										//����Ϊ0����
				mVarSetupLength -= length;
				for(len=0;len!=length;len++)c1=CH375_RD_DAT_PORT();					//ȡ���´�����
				CH375TRANSBIT = 1;
			}
			if( CH375TRANSBIT ){											
				if( (length<8) || (mVarSetupLength == 0) ){
					CH375TRANSBIT = 0;
					CH375_WR_CMD_PORT(CMD_WR_USB_DATA3);						//����д�˵�0������
					CH375_WR_DAT_PORT(0);					                   //�ϴ�0�������ݣ�����һ��״̬�׶�
				}
			}
			break;
		default:
			if((InterruptStatus&0x03)==0x03){									//���߸�λ
				CH375FLAGERR=0;													//������0
				CH375CONFLAG=0;													//������0
				mVarSetupLength=0;
				CH375TRANSBIT = 0;
				CH375ACT=1;														//������������

			}
			else{																//���֧��
				;
			}
			CH375_WR_CMD_PORT (CMD_UNLOCK_USB);									//�ͷŻ�����
			break;
	}
}

main( ) {
	Delay50ms( );	/* ��ʱ�ȴ�CH375��ʼ�����,�����Ƭ����CH375�ṩ��λ�ź��򲻱���ʱ */
	CH375_Init( );  /* ��ʼ��CH375 */
    EA=1;
    while(1);		/* ������ */
}