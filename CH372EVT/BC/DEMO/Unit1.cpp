//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include "Unit1.h"
#include "CH375DLL.H"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
UINT	mKeyCode = 0xff;		// 按键编号
HWND        mSaveDialogMain;	// 主对话框

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
    mCaptionInform = " 信息提示 ";
    mIndex = 0;
    edtN1->Text = "";
    edtCmd->Text = "";
    edtInt->Text = "";
    mSaveDialogMain = this->Handle;

    if ( LoadLibrary( "CH375DLL.DLL" ) == NULL )  // 加载DLL失败,可能未安装到系统中
    {
        Application->MessageBox("无法加载CH375的DLL", "information", MB_ICONSTOP | MB_OK );
    }
    else if ( CH375OpenDevice(0) == INVALID_HANDLE_VALUE ) // 使用之前必须打开设备
    {
        Application->MessageBox("无法打开CH375设备", "information", MB_ICONSTOP | MB_OK );
    }
    CH375SetTimeout( mIndex, 3000, 3000 );  // 设置USB数据读写的超时,超过3000mS未完成读写将强制返回,避免一直等待下去
    CH375SetExclusive( mIndex, FALSE );  // 临时设置共享使用当前CH375设备,因为下面设置中断服务程序时需要使用当前这个CH375设备

    // 下面加载中断服务程序,中断服务程序是在应用层执行的,其线程优先级是THREAD_PRIORITY_TIME_CRITICAL
    // 当单片机有事需要通知计算机时,可以用CMD_WR_USB_DATA5命令写入中断特征数据,计算机的mInterruptEvent线程将会收到该中断特征数据
    // 然后mInterruptEvent线程向主程序发出消息进行处理,mInterruptEvent线程相当于中断服务程序,代替主程序定时查询单片机
    CH375SetIntRoutine( mIndex, mInterruptEvent );

    CH375SetExclusive( mIndex, TRUE );  // 设置独占使用当前CH375设备,在此之后其它应用程序不能同时使用当前这个CH375设备

}

//---------------------------------------------------------------------------
void __fastcall TForm1::btnSendClick(TObject *Sender)
{
    mCOMMAND_PACKET mDemoReq;
    UINT 		mLength;
    UCHAR       mEnterBuffer[8];
    if(edtN1->GetTextLen()>0)
    {
        UCHAR	mBcdCode;
        UCHAR	mNumber;
        strcpy(mEnterBuffer,edtN1->Text.c_str());
        if ( mEnterBuffer[0] == ' ' ) mBcdCode = 0x10;  // 空格
        else if ( mEnterBuffer[0] == '=' ) mBcdCode = 0x13;
        else if ( mEnterBuffer[0] == '.' ) mBcdCode = 0x1a;
        else mBcdCode = mCharToBcd( mEnterBuffer[0] );
        // 如果输入一个字符和一个小数点,则在字符右下角追加小数点
        if ( edtN1->GetTextLen() >= 2 && mEnterBuffer[1] == '.' ) mBcdCode |= 0x80;
        if ( mBcdCode != 0xff )  // 输入的字符有效
        {
            mNumber = 7;  // 以最高位为左,最低位为右,所以N1对应位7,N8对应0
            mDemoReq.mCommandCode = DEMO_CH451_CMD;
            mDemoReq.mCommandCodeNot = ~ DEMO_CH451_CMD;
            mDemoReq.mParameter[2] = 0x08 | mNumber;  // CH451命令:加载字数据到指定数码管
            mDemoReq.mParameter[1] = mBcdCode;  // 字数据,对应于BCD译码方式
            mLength = CONST_CMD_LEN;	// 命令包的长度
            if ( CH375WriteData( mIndex, (void *)&mDemoReq, (unsigned long *)&mLength ) )  // 通过CH375发送数据,成功
// 如果输入时mLength大于64,则成功返回时,为了更加可靠,可以检查实际发送的长度是否符合
            {
                mEnterBuffer[1] = 0;  // 字符串结束符,只要一个字符
             //   SetDlgItemText( hDialog, LOWORD( wParam ), mEnterBuffer );   //设置按钮上的字符
            }
            else Application->MessageBox( "CH375WriteData 失败", mCaptionInform, MB_OK | MB_ICONERROR );
        }
        else Application->MessageBox( "请输入字符0-9,A-F或者空格", mCaptionInform, MB_OK | MB_ICONERROR );
    }
    else
        Application->MessageBox("请输入字符0-9,A-F或者空格",mCaptionInform,MB_OK|MB_ICONSTOP);
    
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnCloseClick(TObject *Sender)
{
    CH375CloseDevice( mIndex );
    this->Close();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnCmdClick(TObject *Sender)
{
    mCOMMAND_PACKET mDemoReq;
    UINT 		mLength;
    UCHAR       mEnterBuffer[8];
  //  UINT	mBufferLength = 0;
    if ( edtCmd->GetTextLen() >= 3 )  // 已经输入数据,并且长度超过3位
    {
        UCHAR	mBcdCode[3];
        strcpy(mEnterBuffer,edtCmd->Text.c_str());
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
            if ( CH375WriteData( mIndex, (void *)&mDemoReq, (unsigned long *)&mLength ) )  // 通过CH375发送数据,成功
                Application->MessageBox( "命令成功发送到CH451", mCaptionInform, MB_OK );
            else Application->MessageBox( "CH375WriteData 失败", mCaptionInform, MB_OK | MB_ICONERROR );
        }
        else Application->MessageBox( "请输入3个字符0-9,A-F", mCaptionInform, MB_OK | MB_ICONERROR );
    }
    else Application->MessageBox( "请至少输入3个字符0-9,A-F", mCaptionInform, MB_OK | MB_ICONERROR );
}
//---------------------------------------------------------------------------

UCHAR	TForm1::mCharToBcd(	UCHAR	iChar )  // 输入的ASCII字符
{
	UCHAR	mBCD;
	if ( iChar >= '0' && iChar <= '9' ) mBCD = iChar -'0';
	else if ( iChar >= 'A' && iChar <= 'F' ) mBCD = iChar - 'A' + 0x0a;
	else if ( iChar >= 'a' && iChar <= 'f' ) mBCD = iChar - 'a' + 0x0a;
	else mBCD = 0xff;
	return( mBCD );
}
void __fastcall TForm1::edtN1KeyPress(TObject *Sender, char &Key)
{
    if(Key == 13)
        this->btnSend->Click();

}
//---------------------------------------------------------------------------

void __fastcall TForm1::edtCmdKeyPress(TObject *Sender, char &Key)
{
    if(Key == 13)
        this->btnCmd->Click();
}
//---------------------------------------------------------------------------

VOID	CALLBACK	mInterruptEvent(  // 中断服务程序
	PUCHAR			iBuffer )  // 指向一个缓冲区,提供当前的中断特征数据
{

// CH375产生中断后,产生一个消息通知主程序
	if ( iBuffer[0] == 1 )  // 检查中断特征数据
	{
		mKeyCode = iBuffer[1] & 0x3f;  // 返回的键值,则单片机存放在中断特征数据缓冲区的第2字节
		Form1->btnInt->Click();
	}
	else if ( iBuffer[0] == 2 )
        Form1->btnRe->Click();
   
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
void __fastcall TForm1::btnIntClick(TObject *Sender)
{
    UCHAR buf[8];
   // sprintf(buf,"02x%",mKeyCode);
 //   SetDlgItemText(this,edtInt,buf);
    edtInt->Text=AnsiString(mKeyCode+1);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnReClick(TObject *Sender)
{
    edtInt->Text = "0";
}
//---------------------------------------------------------------------------

