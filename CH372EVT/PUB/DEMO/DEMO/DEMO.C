// 2003.09.08, 2003.12.28, 2004.12.10, 2004.12.28
//****************************************
//**  Copyright  (C)  W.ch  1999-2004   **
//**  Web:  http://www.winchiphead.com  **
//****************************************
//**  EXE for USB interface chip CH375  **
//**  C, VC5.0                          **
//****************************************
//
// USB总线接口芯片CH375的应用层演示程序 V1.2
// 南京沁恒电子有限公司  作者: W.ch 2004.12
// CH375-EXE  V1.2 , Support: Ctrl/Bulk/Int
// 运行环境: Windows 98/ME, Windows 2000/XP
// support USB chip: CH372/CH375
//


#define		mTHIS_VERSION		0x12		// 当前版本
#define		mTHIS_VER_STR		"1.2"		// 当前版本字符串

#include	<windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"CH375DLL.H"		// CH372/CH375的动态链接库
#pragma comment(lib,"CH375DLL")

#include	"DEMO.H"

#define		IDC_INT_PRESS		2222	// 按键按下事件信号
#define		IDC_INT_RELEASE		2223	// 按键释放事件信号
#define		IDC_INT_DEVARRIVAL	2224	// 设备插上事件信号
#define     IDC_INT_DEVREMOVAL  2225    // 设备拔出事件信号

typedef	struct	_COMMAND_PACKET	{	// 自定义的命令包结构
	UCHAR	mCommandCode;		// 命令请求码,见下面的定义
	UCHAR	mCommandCodeNot;	// 命令码的反码,用于校验命令包
	union	{
		UCHAR	mParameter[5];	// 参数
		struct {
			UCHAR	mBufferID;  // 缓冲区识别码,本程序针对MCS51单片机定义: 1-专用功能寄存器SFR, 2-内部RAM, 3-外部RAM, 不过本程序实际只演示内部RAM
			USHORT	mBufferAddr;	// 读写操作的起始地址,寻址范围是0000H-0FFFFH,低字节在前
			USHORT	mLength;	// 数据块总长度,低字节在前
		};
	};
}	mCOMMAND_PACKET,	*mpCOMMAND_PACKET;

#define		CONST_CMD_LEN		0x07	// 命令块的长度
// 由于命令与数据都是通过数据下传管道(USB端点2的OUT)下传, 为了防止两者混淆,
// 我们可以在计算机应用程序与单片机程序之间约定, 命令块的长度总是7, 而数据块的长度肯定不是7, 例如64,32等
// 另外, 可以约定, 命令块的首字节是命令码, 等等
// 本程序约定命令码: 80H-0FFH是通用命令,适用于各种应用
//                   00H-7FH是专用命令,针对各种应用特别定义
// 通用命令
#define		DEF_CMD_GET_INFORM		0x90	// 获取下位机的说明信息,长度不超过64个字符,字符串以00H结束
#define		DEF_CMD_TEST_DATA		0x91	// 测试命令,下位机将PC机发来的命令包的所有数据取反后返回
#define		DEF_CMD_CLEAR_UP		0xA0	// 在上传数据块之前进行同步,实际是让下位机清除上传缓冲区的已有内容
#define		DEF_CMD_UP_DATA			0xA1	// 从下位机的指定地址的缓冲区中读取数据块(上传数据块)
#define		DEF_CMD_DOWN_DATA		0xA2	// 向下位机的指定地址的缓冲区中写入数据块(下传数据块)
// 专用命令
#define		DEMO_CH451_CMD			0x56	// PC发送命令给CH451,用于演示CH451的功能
// 对于MCS51单片机在使用通用命令时,还需要指定缓冲区识别码
#define		ACCESS_MCS51_SFR		1		// 读写51单片机的SFR
#define		ACCESS_MCS51_IRAM		2		// 读写51单片机的内部RAM
#define		ACCESS_MCS51_XRAM		3		// 读写51单片机的外部RAM

HINSTANCE   mSaveInstance;		// 实例
HWND        mSaveDialogMain;	// 主对话框
HWND        mEnterDialog;	// 输入数据对话框

UCHAR	mEnterBuffer[8];		// 数据输入缓冲区
UINT	mBufferLength = 0;		// 缓冲区内的有效数据长度
UINT	mShowLED = 0;			// 不为0则LED亮
UINT	mKeyCode = 0xff;		// 按键编号
UINT	mIndex = 0;				// 如果有多个CH372/CH375则指示序号
CHAR	mCaptionInform[] = " 信息提示 ";
BOOL    openflag=FALSE;
BOOL    Arrivalflag=FALSE; //是设备插上

LRESULT CALLBACK mDialogMain(HWND,UINT,WPARAM,LPARAM);  //主对话框调用程序
LRESULT CALLBACK mDialogEnter(HWND,UINT,WPARAM,LPARAM); //输入数字对话框调用程序
VOID SendCH452Command();  //发送CH452命令.//mEnterBuffer存储的是三位十六进制的命令字符
UCHAR	mCharToBcd(UCHAR);								// 将ASCII字符转换为一位BCD码
VOID	mSyncKeyboard(HWND,UINT);						// 同步按键显示
VOID	CALLBACK mInterruptEvent(PUCHAR);						// 中断服务程序
UINT	mDownloadData( UCHAR *iBuffer, UINT iLength );	// 下传数据块,返回实际传输长度
UINT	mUploadData( UCHAR *iBuffer, UINT iLength );	// 上传数据块,返回实际传输长度
VOID CALLBACK NotifyRoutine (ULONG	iEventStatus );  // 设备事件通知回调程序
VOID Testcommunication();   //测试PC机与单片机之间的USB通讯
VOID devremoval();        //设备移除处理函数
VOID devarrival();        //设备插入处理函数


//程序入口
int	APIENTRY	WinMain( HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd )
{
	CHAR	*CmdLine;
	CmdLine = lpCmdLine;
	mSaveInstance = hInstance;
	if ( CmdLine != NULL && *CmdLine >= '0' && *CmdLine <= '9' ) {  // 在命令行中指定CH375设备序号
// 如果运行DEMO时在命令行指定参数0至9,则对应CH375设备序号0-9,如果没有命令行参数则默认为CH375设备序号0
// 例如,执行"DEMO 1"对应1#CH375设备,执行"DEMO 8"对应8号CH375设备
// 命令行带参数可以通过Windows操作系统的"开始"中的"运行"窗口执行,或者在DOS窗口中执行
		mIndex = *CmdLine - '0';
	}
	return( DialogBox( hInstance, "IDD_MAIN", NULL, mDialogMain ) );    //调用主对话框
}

//主对话框事件  
LRESULT CALLBACK mDialogMain( HWND hDialog, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	mCOMMAND_PACKET	mDemoReq;
	UINT		mLength;
	switch( uMessage )
	{
		case WM_INITDIALOG:        //初始化
			mSaveDialogMain = hDialog;
// 需要使用DLL则需要先加载
			CheckDlgButton(mSaveDialogMain,IDC_WordShift,BST_CHECKED);
			CheckDlgButton(mSaveDialogMain,IDC_SegUnLight,BST_CHECKED);

			if ( LoadLibrary( "CH375DLL.DLL" ) == NULL )  // 加载DLL失败,可能未安装到系统中
			{
				MessageBox( hDialog, "无法加载CH372/CH375的DLL", mCaptionInform, MB_ICONSTOP | MB_OK );
				EndDialog( hDialog, 0x81 );  // 关闭对话框
				return( TRUE );
			}

/* 为了从多个CH372/CH375设备中找出自己的设备,应用程序可以查询USB设备的ID号(厂商ID和产品ID),直到ID比较正确,可以在打开设备之前或者之后查询
#define		MY_VENDOR_ID	0x4348
#define		MY_PRODUCT_ID	0x5537
			for ( mIndex = 0; mIndex < mCH375_MAX_NUMBER; mIndex ++ )
			{
				if ( CH375GetUsbID( mIndex ) == ( ( MY_PRODUCT_ID << 16 ) | MY_VENDOR_ID ) ) break;  // 获取USB设备ID,返回数据中,低16位为厂商ID,高16位为产品ID,错误时返回全0(无效ID)
			}
			if ( mIndex >= mCH375_MAX_NUMBER )
			{
				MessageBox( hDialog, "找不到指定的CH372/CH375设备", mCaptionInform, MB_ICONSTOP | MB_OK );
				EndDialog( hDialog, 0x81 );  // 关闭对话框
				return( TRUE );
			}*/

            if(!CH375SetDeviceNotify(mIndex,NULL,NotifyRoutine)) //设置设备插拔监视
				MessageBox( hDialog, "设置监视CH372/CH375设备插拔失败", mCaptionInform, MB_ICONSTOP | MB_OK );

			if ( CH375OpenDevice( mIndex ) == INVALID_HANDLE_VALUE )  // 打开设备 
			{
				openflag=FALSE;
                NotifyRoutine(CH375_DEVICE_REMOVE);   //设备没打开,按钮禁用
				return ( TRUE );
				break;
			}
			else  //打开成功
				openflag=TRUE;			    

			NotifyRoutine(CH375_DEVICE_ARRIVAL);  //设备打开,按钮可用
			CH375SetTimeout( mIndex, 3000, 3000 );  // 设置USB数据读写的超时,超过3000mS未完成读写将强制返回,避免一直等待下去
            Testcommunication(); // 测试PC机与单片机之间的USB通讯,仅作演示,本身没有意义
/* 下面是下传/上传数据块的例子
			{
				UCHAR mBuffer[4096];
				mBuffer[0]=data;	// 准备下传的数据
				mLength = mDownloadData( mBuffer, 4096 ) ;	// 将数据块从计算机下传给单片机,返回实际传输长度
				mLength = mUploadData( mBuffer, 4096 ) ;	// 从单片机上传数据块到计算机,返回实际传输长度
			}*/
			CH375SetExclusive( mIndex, FALSE );  // 临时设置共享使用当前CH375设备,因为下面设置中断服务程序时需要使用当前这个CH375设备
// 下面加载中断服务程序,中断服务程序是在应用层执行的,其线程优先级是THREAD_PRIORITY_TIME_CRITICAL
// 当单片机有事需要通知计算机时,可以用CMD_WR_USB_DATA5命令写入中断特征数据,计算机的mInterruptEvent线程将会收到该中断特征数据
// 然后mInterruptEvent线程向主程序发出消息进行处理,mInterruptEvent线程相当于中断服务程序,代替主程序定时查询单片机
			CH375SetIntRoutine( mIndex, mInterruptEvent );  //设置中断
			Sleep( 50 );  // 主程序休息50mS,以等待上一个操作CH375SetIntRoutine创建线程并由该线程再次打开CH375,否则下一个操作CH375SetExclusive将导致创建的线程无法打开CH375
			CH375SetExclusive( mIndex, TRUE );  // 设置独占使用当前CH375设备,在此之后其它应用程序不能同时使用当前这个CH375设备,实际是无法再打开CH375设备
			return ( TRUE );
			break;
		case WM_COMMAND:
			switch ( LOWORD( wParam ) )
			{				
				char i=0,j=0,k=0;
				char ledval[4]="",oldledval[4]="";

				case IDC_N1:  // 发送数据到演示板上的第1个数码管,左边第1个
				case IDC_N2:
				case IDC_N3:
				case IDC_N4:
				case IDC_N5:
				case IDC_N6:
				case IDC_N7:
				case IDC_N8:  // 发送数据到演示板上的第8个数码管,右边第1个
                    if(Arrivalflag== 0){  //将软件上的显示信息发送到评估板,不要输入,保证上下显示信息相同,					
						if(DialogBox( mSaveInstance, "IDD_ENTER", NULL, mDialogEnter )==IDC_CANCEL)
							break;
					}
					Arrivalflag=FALSE;  					
					if ( mBufferLength )  // 已经输入数据
					{
						UCHAR	mBcdCode;
						UCHAR	mNumber;
						if ( mEnterBuffer[0] == ' ' ) mBcdCode = 0x10;  // 空格
						else if ( mEnterBuffer[0] == '=' ) mBcdCode = 0x13;
						else if ( mEnterBuffer[0] == '.' ) mBcdCode = 0x1a;
						else mBcdCode = mCharToBcd( mEnterBuffer[0] );
						// 如果输入一个字符和一个小数点,则在字符右下角追加小数点
						if ( mBufferLength >= 2 && mEnterBuffer[1] == '.' ) mBcdCode |= 0x80;
						if ( mBcdCode != 0xff )  // 输入的字符有效
						{   
							ZeroMemory(&mDemoReq,sizeof(mDemoReq));
							mNumber = LOWORD( wParam ) - IDC_N1;  // 如果IDC_N1至IDC_N8有序,则结果为0-7,分别对应于各个数码管按钮
							mNumber = 7 - mNumber;  // 以最高位为左,最低位为右,所以N1对应位7,N8对应0
							mDemoReq.mCommandCode = DEMO_CH451_CMD;
							mDemoReq.mCommandCodeNot = ~ DEMO_CH451_CMD;
							mDemoReq.mParameter[2] = 0x08 | mNumber;  // CH451命令:加载字数据到指定数码管
							mDemoReq.mParameter[1] = mBcdCode;  // 字数据,对应于BCD译码方式
							mLength = CONST_CMD_LEN;	// 命令包的长度
							if ( CH375WriteData( mIndex, &mDemoReq, &mLength ) )  // 通过CH375发送数据,成功
						// 如果输入时mLength大于64,则成功返回时,为了更加可靠,可以检查实际发送的长度是否符合
							{
								if(lstrlen(mEnterBuffer)==1)
									mEnterBuffer[1] = 0;  // 字符串结束符,只要一个字符
								else
									mEnterBuffer[2] = 0;
								SetDlgItemText( hDialog, LOWORD( wParam ), CharUpper(mEnterBuffer) );   //设置按钮上的字符
							}
							else
								MessageBox( hDialog, "CH375WriteData 失败", mCaptionInform, MB_OK | MB_ICONERROR );
						}
						else 
							MessageBox( hDialog, "请输入字符:\n0-9,A-F或者空格,小数点", mCaptionInform, MB_OK | MB_ICONERROR );
					}
					else 
						MessageBox( hDialog, "请至少输入一个字符:\n0-9,A-F或者空格,小数点", mCaptionInform, MB_OK | MB_ICONERROR );					
					break;				
				case IDC_INT_PRESS:  // 键被按下,以下先释放原按键再读取新按键值
					mSyncKeyboard( hDialog, 0xff );  // 释放上次的按键
					mSyncKeyboard( hDialog, mKeyCode );  // 根据键值作同步显示					
					break;
				case IDC_INT_RELEASE:  // 按键释放
					mSyncKeyboard( hDialog, 0xff );  // 释放按键					
					break;				
				case IDC_CMD1:  // 以十六进制输入3个字符共12位到演示板上的CH451芯片
					if(DialogBox( mSaveInstance, "IDD_ENTER", NULL, mDialogEnter )==IDC_CANCEL)
						break;				
					SendCH452Command(); //发送CH452命令					
					break;
				case IDC_INT_DEVARRIVAL:  //从插拔通知中断程序中发来的设备插入消息
					devarrival();					
					break;
                case IDC_INT_DEVREMOVAL:  //从插拔通知中断程序中发来的设备移除消息
					devremoval();					
					break;												
				case IDC_LeftShift:   //字数据左移/左循环
					if(IsDlgButtonChecked(mSaveDialogMain,IDC_WordShift)==BST_CHECKED){ //左移位
						strcpy(mEnterBuffer,"300\0");//左移位命令
						mBufferLength=lstrlen(mEnterBuffer);
						SendCH452Command(); //发送CH452命令
                        
						//窗体上LED数码管显示值左移
						for(i=0;i<7;i++){
							GetDlgItemText(mSaveDialogMain,IDC_N1+i+1,ledval,4);
							SetDlgItemText(mSaveDialogMain,IDC_N1+i,ledval);
						}
						SetDlgItemText(mSaveDialogMain,IDC_N1+7,"0");
					}
					else {  //字数据左循环
						strcpy(mEnterBuffer,"301\0");  //左循环命令
						mBufferLength=lstrlen(mEnterBuffer);
						SendCH452Command();  //发送CH452命令
                        GetDlgItemText(mSaveDialogMain,IDC_N1,oldledval,4);
                        //窗体上LED数码管显示值左循环
						for(i=0;i<7;i++){
							GetDlgItemText(mSaveDialogMain,IDC_N1+i+1,ledval,4);
							SetDlgItemText(mSaveDialogMain,IDC_N1+i,ledval);
						}
						SetDlgItemText(mSaveDialogMain,IDC_N1+i,oldledval);
					}
					break;
				case IDC_RightShift:  //数据字右移/右循环
					if(IsDlgButtonChecked(mSaveDialogMain,IDC_WordShift)==BST_CHECKED){ //右移位
						strcpy(mEnterBuffer,"302\0");//右移位命令
						mBufferLength=lstrlen(mEnterBuffer);
						SendCH452Command(); //发送CH452命令
						//窗体上LED数码管显示值右移
						for(i=0;i<7;i++){
							GetDlgItemText(mSaveDialogMain,IDC_N8-i-1,ledval,4);
							SetDlgItemText(mSaveDialogMain,IDC_N8-i,ledval);
						}
						SetDlgItemText(mSaveDialogMain,IDC_N1,"0");
					}
					else {  //右循环
						strcpy(mEnterBuffer,"303\0");//左移位命令字符
						mBufferLength=lstrlen(mEnterBuffer);
						SendCH452Command(); //发送CH452命令
						GetDlgItemText(mSaveDialogMain,IDC_N8,oldledval,4);
						for(i=0;i<7;i++){
							GetDlgItemText(mSaveDialogMain,IDC_N8-i-1,ledval,4);
							SetDlgItemText(mSaveDialogMain,IDC_N8-i,ledval);
						}
						SetDlgItemText(mSaveDialogMain,IDC_N1,oldledval);
					}
					break;
				case IDC_FlashLed1: //数码管闪烁
				case IDC_FlashLed2:
				case IDC_FlashLed3:
				case IDC_FlashLed4:
				case IDC_FlashLed5:
				case IDC_FlashLed6:
				case IDC_FlashLed7:
				case IDC_FlashLed8:  //BST_UNCHECKED=0x0000 ,BST_CHECKED=0x0001

					mEnterBuffer[0]='6';
					j=0;k=0;
					for (i=0;i<4;i++){  //取闪烁数码管地址共八位,对应于从左往右LED1..LED8.为1,闪烁;为0,停止闪烁
						j=(IsDlgButtonChecked(mSaveDialogMain,IDC_FlashLed1+i)<<(3-i)) +j;						
						k=(IsDlgButtonChecked(mSaveDialogMain,IDC_FlashLed5+i)<<(3-i)) +k;
					}
					if(j>9)
						mEnterBuffer[1]=j-10+'A'; //对应数值字符
					else
						mEnterBuffer[1]=j+'0';
					if(k>9)
						mEnterBuffer[2]=k-10+'A';
					else
						mEnterBuffer[2]=k+'0';
					mBufferLength=lstrlen(mEnterBuffer);
					SendCH452Command(); //发送CH452命令
					break;	
				case IDC_SetSegLight:  //设置段
					GetDlgItemText(mSaveDialogMain,IDC_SegAddr,&mEnterBuffer[1],3);					
					if((mCharToBcd(mEnterBuffer[1])<<4) +mCharToBcd(mEnterBuffer[2])>0x40){
						MessageBox( hDialog, "请输入在00H...40H范围内的两位数值", mCaptionInform, MB_OK | MB_ICONERROR );
						break;
					}
					if(IsDlgButtonChecked(mSaveDialogMain,IDC_SegUnLight)==BST_CHECKED){ //段位置0
						mEnterBuffer[0]='1';						
						mEnterBuffer[1]=mCharToBcd(mEnterBuffer[1])+8;
						if(mEnterBuffer[1]>9)
							mEnterBuffer[1]=mEnterBuffer[1]-10+'A';
						else
							mEnterBuffer[1]=mEnterBuffer[1]+'0';
					}
					else if(IsDlgButtonChecked(mSaveDialogMain,IDC_SegLight)==BST_CHECKED){ //段位置0
						mEnterBuffer[0]='1';						
						mEnterBuffer[1]=mCharToBcd(mEnterBuffer[1])+12-10+'A';											
					}
					else if(IsDlgButtonChecked(mSaveDialogMain,IDC_SegUnLight)==BST_CHECKED){ //段位置0
						mEnterBuffer[0]='1';						
					}
					mEnterBuffer[3]='\0';
					mBufferLength=lstrlen(mEnterBuffer);
					SendCH452Command(); //发送CH452命令
					break;
				case IDC_SetShowPara:
					GetDlgItemText(mSaveDialogMain,IDC_LimitVal,&mEnterBuffer[1],2);
					if(IsDlgButtonChecked(mSaveDialogMain,IDC_BCDCoding)==BST_CHECKED){
						mEnterBuffer[1]=mCharToBcd(mEnterBuffer[1])+8;
						if(mEnterBuffer[1]>9 && mEnterBuffer[1]<16)
							mEnterBuffer[1]=mEnterBuffer[1]-10+'A';
						else if(mEnterBuffer[1]<10)
							mEnterBuffer[1]=mEnterBuffer[1]+'0';
						else
							mEnterBuffer[1]='8';
					}
					else
						mEnterBuffer[1]=mCharToBcd(mEnterBuffer[1])+'0';
					mEnterBuffer[0]='5';
					mEnterBuffer[2]='0';
					mBufferLength=lstrlen(mEnterBuffer);
					SendCH452Command(); //发送CH452命令
					break;
               /*
				case IDC_SDP:      //指定数码管小数点亮灭
					j=0;k=0;                    
					for (i=0;i<8;i++){ //取置位地址一次只能熄灭指定地址的一个发光管
						if(IsDlgButtonChecked(mSaveDialogMain,IDC_SetLED1+i)==BST_CHECKED)
							j=7-i;  //Dig号
						if(IsDlgButtonChecked(mSaveDialogMain,IDC_SA+i)==BST_CHECKED)
							k=i*8;  //Seg号
					}
					i=j+k;  //段地址
					mEnterBuffer[0]='1';
                    if(IsDlgButtonChecked(mSaveDialogMain,IDC_SegLight)==BST_CHECKED)
						mEnterBuffer[1]=(i/16 +12-10)+'A';  //段亮
					else{    //段灭
						mEnterBuffer[1]=(i/16+8);
						if(mEnterBuffer[1]>9)
							mEnterBuffer[1]=(mEnterBuffer[1]-10) +'A';    
					    else
							mEnterBuffer[1]=mEnterBuffer[1] +'0'; 
					}
					mEnterBuffer[2]=i%16;
					if(mEnterBuffer[2]>9)
						mEnterBuffer[2]=mEnterBuffer[2]-10 +'A';
					else
						mEnterBuffer[2]=mEnterBuffer[2] +'0';	
					mBufferLength=lstrlen(mEnterBuffer);
					SendCH452Command(); //发送CH452命令
					break;*/
				case WM_DESTROY:  // 退出
					CH375CloseDevice( mIndex );  // 退出程序前必须关闭CH375设备
					EndDialog( hDialog, 1 );
					PostQuitMessage(0);
					return( TRUE );
					break;
			}
			break; 
	}
	return ( FALSE );
}

// 输入数据对话框
LRESULT CALLBACK mDialogEnter( HWND hDialog, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	switch( uMessage )
	{
		case WM_INITDIALOG:
			mBufferLength = 0;
			mEnterDialog=hDialog;
			return ( TRUE );
			break;
		case WM_COMMAND:
			switch( LOWORD( wParam ) )
			{
				case IDC_OK:
					mBufferLength = GetDlgItemText( hDialog, IDC_EDIT1, mEnterBuffer, 4 );  // 取得输入的字符
					EndDialog( hDialog, IDC_OK );
					return ( TRUE );
					break;
				case IDC_CANCEL:					
				case WM_DESTROY:
					mBufferLength = 0;					
					EndDialog( hDialog, IDC_CANCEL );			
					return ( TRUE );
					break;
			}
		break;
	}
	return ( FALSE );
}

void SendCH452Command() //发送CH452命令.输入的命令码为三位十六进制的数值;
                        //mEnterBuffer存储的是三位十六进制的字符
						//mCharToBcd :将一位十六进制的字符转为数值
{
	char temB[50]="",temD[50]="";
	mCOMMAND_PACKET	mDemoReq;
	UINT		mLength;
	if ( mBufferLength >= 3 )  // 已经输入数据,并且长度超过3位
	{
		UCHAR	mBcdCode[3];
		mBcdCode[0] = mCharToBcd( mEnterBuffer[0] );
		mBcdCode[1] = mCharToBcd( mEnterBuffer[1] );
		mBcdCode[2] = mCharToBcd( mEnterBuffer[2] );
		if ( mBcdCode[0] != 0xff && mBcdCode[1] != 0xff && mBcdCode[2] != 0xff )  // 输入的三个字符都有效
		{
			mDemoReq.mCommandCode = DEMO_CH451_CMD;
			mDemoReq.mCommandCodeNot = ~ DEMO_CH451_CMD;
			mDemoReq.mParameter[2] = mBcdCode[0];  // CH451命令:高4位
			mDemoReq.mParameter[1] = mBcdCode[1] << 4 | mBcdCode[2];  // 低8位的字节数据
			mLength = CONST_CMD_LEN;	// 命令包的长度
            
			ltoa((mBcdCode[0]<<8)+(mBcdCode[1]<<4)+(mBcdCode[2]),temD,2);
			sprintf(temB,"已发送命令码: %012sB (%03XH)",temD,(mBcdCode[0]<<8)+(mBcdCode[1]<<4)+(mBcdCode[2]));
			SetDlgItemText(mSaveDialogMain,IDC_cmdcode,temB);

			if (! CH375WriteData( mIndex, &mDemoReq, &mLength ) )  // 通过CH375发送数据,成功
				MessageBox( mSaveDialogMain, "CH375WriteData 失败", mCaptionInform, MB_OK | MB_ICONERROR );
		}
		else
			MessageBox( mSaveDialogMain, "请输入3个字符0-9,A-F", mCaptionInform, MB_OK | MB_ICONERROR );
	}
	else 
		MessageBox( mSaveDialogMain, "请至少输入3个字符0-9,A-F", mCaptionInform, MB_OK | MB_ICONERROR );
}
// 将ASCII字符转换为一位BCD码,有效字符是0-9,A-F,a-f,无效字符返回0FFH
UCHAR	mCharToBcd(
	UCHAR	iChar )  // 输入的ASCII字符
{
	UCHAR	mBCD;
	if ( iChar >= '0' && iChar <= '9' ) mBCD = iChar -'0';
	else if ( iChar >= 'A' && iChar <= 'F' ) mBCD = iChar - 'A' + 0x0a;
	else if ( iChar >= 'a' && iChar <= 'f' ) mBCD = iChar - 'a' + 0x0a;
	else mBCD = 0xff;
	return( mBCD );
}

// 同步按键显示,键被按下时显示◎,否则显示键号
void	mSyncKeyboard(
	HWND	iDialog,  // 主对话框
	UINT	iKeyCode )  // 输入的按键值,00H-3FH则键被按下,0FFH则按下的键被释放
{
// 以下方法只适用于IDC_K0至IDC_K63完全有序的情况
	static	UINT	mKeyNo=0;
	UCHAR	mKeyBuffer[8];

	if ( iKeyCode == 0xff )  // 释放刚按下的键
	{
		sprintf( mKeyBuffer, "%0d", mKeyNo );  // 键号字符串
		SetDlgItemText( iDialog, IDC_K0 + mKeyNo, mKeyBuffer );  // 恢复显示键号
	}
	else  // 键被按下
	{
		mKeyNo = iKeyCode & 0x3f ;  // 键号0-63
		SetDlgItemText( iDialog, IDC_K0 + mKeyNo, "◎" );  // 00H-3FH键被按下则显示◎
	}

// 下面是另一种直接方法,代码较多
/*
	switch( iKey )
	{
		case 0x00:
			SetDlgItemText( iDialog, IDC_K0, "◎" );
			break;
		case 0x01:
			SetDlgItemText( iDialog, IDC_K1, "◎" );
			break;
		......
		case 0x3f:
			SetDlgItemText( iDialog, IDC_K63, "◎" );
			break;
		case 0xff:
			SetDlgItemText( iDialog, ???, "??" );
			break;
	}
*/
}

// 中断服务程序是由CH375驱动程序中断后通过DLL在应用层模拟调用的
VOID	CALLBACK	mInterruptEvent(  // 中断服务程序
	PUCHAR			iBuffer )  // 指向一个缓冲区,提供当前的中断特征数据
{
// iBuffer指向一个8个字节的缓冲区,该缓冲区中的内容是由单片机用CMD_WR_USB_DATA5命令写入CH375的数据
// CH375产生中断后,产生一个消息通知主程序
	if ( iBuffer[0] == 1 )  // 检查中断特征数据
	{
		mKeyCode = iBuffer[1] & 0x3f;  // 返回的键值,则单片机存放在中断特征数据缓冲区的第2字节
		PostMessage( mSaveDialogMain, WM_COMMAND, IDC_INT_PRESS, 0 );  // 中断特征数据1则键被按下
	}
	else if ( iBuffer[0] == 2 ) PostMessage( mSaveDialogMain, WM_COMMAND, IDC_INT_RELEASE, 0 );  // 中断特征数据2则键被释放

// 如果使用中断特征数据,则根据该数据作相应的处理
/*
	switch( iBuffer[0] )
	{
		case 1: 中断特征数据为1
		case 2: 中断特征数据为2
		.....
		case 6: 中断特征数据为6
	}
*/
}

VOID CALLBACK NotifyRoutine (  // 设备事件通知回调程序
	  ULONG	iEventStatus )  // 设备事件和当前状态: 0=设备拔出事件, 3=设备插入事件
{
	//在中断程序中对系统资源不能操作太多,所以将插拔事件处理通过PostMessage()放到IDC_INT_DEVARRIVAL中处理
	if(iEventStatus==CH375_DEVICE_ARRIVAL)  
		SendMessage( mSaveDialogMain, WM_COMMAND, IDC_INT_DEVARRIVAL, 0 ); 	//将设备插入消息发送到窗体进行处理			
	else if(iEventStatus==CH375_DEVICE_REMOVE)	
		SendMessage( mSaveDialogMain, WM_COMMAND, IDC_INT_DEVREMOVAL, 0 ); 	//将设备拔出消息发送到窗体进行处理		
	return;
}
VOID devarrival() //设备插入处理程序
{
	char i;	
	if(!openflag)
	{
		if ( CH375OpenDevice( mIndex ) == INVALID_HANDLE_VALUE )  /* 设备插入打开设备 */
		{
			MessageBox( mSaveDialogMain, "无法打开CH372/CH375设备", mCaptionInform, MB_ICONSTOP | MB_OK );
			return;
		}
		else
			openflag=TRUE;

		CH375SetTimeout( mIndex, 3000, 3000 );  // 设置USB数据读写的超时,超过3000mS未完成读写将强制返回,避免一直等待下去
		Testcommunication();
		CH375SetExclusive( mIndex, FALSE );  // 临时设置共享使用当前CH375设备,因为下面设置中断服务程序时需要使用当前这个CH375设备
		CH375SetIntRoutine( mIndex, mInterruptEvent );  //设置中断
		Sleep( 50 );  // 主程序休息50mS,以等待上一个操作CH375SetIntRoutine创建线程并由该线程再次打开CH375,否则下一个操作CH375SetExclusive将导致创建的线程无法打开CH375
		CH375SetExclusive( mIndex, TRUE );  // 设置独占使用当前CH375设备,在此之后其它应用程序不能同时使用当前这个CH375设备,实际是无法再打开CH375设备
	}
    //设备打开,输入按钮可用,设备没打开,输入按钮禁用
	for(i=0;i<8;i++){
		Arrivalflag=TRUE;  //忽略数据输入对话框
		mBufferLength = GetDlgItemText( mSaveDialogMain, IDC_N1+i, mEnterBuffer, 4 );  // 取LED按钮上显示的字符
		//软件与EVT数码管显示同步
		SendMessage(mSaveDialogMain,WM_COMMAND,IDC_N1+i,0);	//发送当前窗体数码管按钮IDC_N1..IDC_N8的显示值
		//输入按钮可用
		EnableWindow(GetDlgItem(mSaveDialogMain,IDC_N1+i),TRUE);
	    EnableWindow(GetDlgItem(mSaveDialogMain,IDC_FlashLed1+i),TRUE); //闪烁按钮		
	}
	SendMessage(mSaveDialogMain,WM_COMMAND,IDC_FlashLed1,0); //发送当前窗体数码管闪烁控制状态

	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_SetSegLight),TRUE); //设置段位显示按钮
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_SetShowPara),TRUE); //设置显示参数按钮	
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_LeftShift),TRUE);   //字左移按钮
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_RightShift),TRUE);  //字右移按钮
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_CMD1),TRUE);        //输入命令码按钮		
	SetDlgItemText(mSaveDialogMain,IDC_devstatue,"**CH372/CH375设备已插上");  //设备插拔状态显示
}
VOID devremoval() //设备拔出处理程序
{
	char i;	
	if(openflag)
	{
		CH375CloseDevice( mIndex );  // 退出程序前必须关闭CH375设备
		openflag=FALSE;
	}
	for(i=0;i<8;i++){
		EnableWindow(GetDlgItem(mSaveDialogMain,IDC_N1+i),FALSE); //LED 
	    EnableWindow(GetDlgItem(mSaveDialogMain,IDC_FlashLed1+i),FALSE); //闪烁按钮
 	}
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_SetSegLight),FALSE);  //设置段位显示按钮
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_SetShowPara),FALSE);  //设置显示参数按钮
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_LeftShift),FALSE);    //字左移按钮
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_RightShift),FALSE);   //字右移按钮
	EnableWindow(GetDlgItem(mSaveDialogMain,IDC_CMD1),FALSE);         //输入命令码按钮
	SetDlgItemText(mSaveDialogMain,IDC_devstatue,"**CH372/CH375设备已拔出");//设备插拔状态显示	
}

VOID Testcommunication()
{
	mCOMMAND_PACKET	mDemoReq;
	UINT		mLength;

	mDemoReq.mCommandCode = DEF_CMD_TEST_DATA;  // 测试命令,将PC机发来的所有数据取反后返回
	mDemoReq.mCommandCodeNot = ~ DEF_CMD_TEST_DATA;
	mDemoReq.mParameter[0] = 0x5a;  // 任意的测试数据,返回后将按位取反
	mDemoReq.mParameter[1] = 0x96;  // 任意的测试数据,返回后将按位取反
	mDemoReq.mParameter[2] = 0xf3;  // 任意的测试数据,返回后将按位取反
	mDemoReq.mParameter[3] = 0x4c;  // 任意的测试数据,返回后将按位取反
	mDemoReq.mParameter[4] = 0x39;  // 任意的测试数据,返回后将按位取反

	mLength = CONST_CMD_LEN;	// 命令包的长度
	if ( CH375WriteData( mIndex, &mDemoReq, &mLength ) )  // 通过CH375发送命令数据,成功
	{
		mLength = mCH375_PACKET_LENGTH;
		if ( CH375ReadData( mIndex, &mDemoReq, &mLength ) )  // 通过CH375接收应答数据,成功
		{
			if ( mLength == CONST_CMD_LEN ) 
			{
				if ( mDemoReq.mCommandCode != (UCHAR)~ DEF_CMD_TEST_DATA || mDemoReq.mParameter[0] != (UCHAR)~ 0x5a || mDemoReq.mParameter[1] != (UCHAR)~ 0x96 )
					MessageBox( mSaveDialogMain, "通过USB传输的数据有错误", mCaptionInform, MB_OK | MB_ICONERROR );
			}
			else MessageBox( mSaveDialogMain, "CH375数据测试返回的长度错误", mCaptionInform, MB_OK | MB_ICONERROR );
		}
		else 
			MessageBox( mSaveDialogMain, "CH375ReadData 失败", mCaptionInform, MB_OK | MB_ICONERROR );
	}
	else
		MessageBox( mSaveDialogMain, "CH375WriteData 失败", mCaptionInform, MB_OK | MB_ICONERROR );
}

UINT	mDownloadData( UCHAR *iBuffer, UINT iLength )
{
	mCOMMAND_PACKET	mDemoReq;
	UINT		mLength;
	if ( iLength > 4096 )
	{
		MessageBox( mSaveDialogMain, "单次下传数据长度超过4096字节", mCaptionInform, MB_OK | MB_ICONERROR );
		return( FALSE );
	}
	mDemoReq.mCommandCode = DEF_CMD_DOWN_DATA;	// 连续下传数据块
	mDemoReq.mCommandCodeNot = ~ DEF_CMD_DOWN_DATA;
	mDemoReq.mBufferID = ACCESS_MCS51_XRAM; // 读写51单片机的外部RAM(本演示板的单片机没有外部RAM,所以无法演示)
	mDemoReq.mBufferAddr = 0x8200;	// 指定外部RAM缓冲区起始地址,该例是将数据下传到起始地址为0X8200的外部RAM
	mDemoReq.mLength = iLength;  // 传输数据总长度
	mLength = CONST_CMD_LEN;	// 命令包的长度
	if ( CH375WriteData( mIndex, &mDemoReq, &mLength ) )  // 通过CH375发送命令包,成功
	{
		mLength = iLength;	// 数据块的长度,一次下传不超过4096字节
		if ( ( mLength % 64 ) == CONST_CMD_LEN ) mLength ++;  // 防止数据包的长度与命令包的长度相同,如果相同,则多发送一个无效数据
		if ( CH375WriteData( mIndex, iBuffer, &mLength ) )  // 通过CH375发送数据,成功
			return( mLength );
		else MessageBox( mSaveDialogMain, "CH375WriteData 下传数据失败", mCaptionInform, MB_OK | MB_ICONERROR );
	}
	else MessageBox( mSaveDialogMain, "CH375WriteData 发送命令失败,DEF_CMD_DOWN_DATA", mCaptionInform, MB_OK | MB_ICONERROR );
	return( 0 );
}

UINT	mUploadData( UCHAR *iBuffer, UINT iLength )
{
	mCOMMAND_PACKET	mDemoReq;
	UINT		mLength;
	if ( iLength > 4096 )
	{
		MessageBox( mSaveDialogMain, "单次上传数据长度超过4096字节", mCaptionInform, MB_OK | MB_ICONERROR );
		return( FALSE );
	}
	mDemoReq.mCommandCode = DEF_CMD_CLEAR_UP;	// 连续上传数据块之前进行同步,实际是让单片机清除上传缓冲区的已有内容
	mDemoReq.mCommandCodeNot = ~ DEF_CMD_CLEAR_UP;
	mLength = CONST_CMD_LEN;	// 命令包的长度
	if ( CH375WriteData( mIndex, &mDemoReq, &mLength ) )  // 通过CH375发送命令包,成功
	{
		mDemoReq.mCommandCode = DEF_CMD_UP_DATA;	// 连续上传数据块
		mDemoReq.mCommandCodeNot = ~ DEF_CMD_UP_DATA;
		mDemoReq.mBufferID = ACCESS_MCS51_XRAM; // 读写51单片机的外部RAM(本演示板的单片机没有外部RAM,所以无法演示)
		mDemoReq.mBufferAddr = 0x8200;	// 指定外部RAM缓冲区起始地址,该例是将从起始地址为0X8200的外部RAM上传数据
		mDemoReq.mLength = iLength;  // 传输数据总长度
		mLength = CONST_CMD_LEN;	// 命令包的长度
		if ( CH375WriteData( mIndex, &mDemoReq, &mLength ) )  // 通过CH375发送命令包,成功
		{
			mLength = iLength;	// 数据块的长度,一次上传不超过4096字节
			if ( CH375ReadData( mIndex, iBuffer, &mLength ) )  // 通过CH375接收数据,成功
				return( mLength );
			else MessageBox( mSaveDialogMain, "CH375ReadData 上传数据失败", mCaptionInform, MB_OK | MB_ICONERROR );
		}
		else MessageBox( mSaveDialogMain, "CH375WriteData 发送命令失败,DEF_CMD_UP_DATA", mCaptionInform, MB_OK | MB_ICONERROR );
	}
	else MessageBox( mSaveDialogMain, "CH375WriteData 发送命令失败,DEF_CMD_CLEAR_UP", mCaptionInform, MB_OK | MB_ICONERROR );
	return( 0 );
}
