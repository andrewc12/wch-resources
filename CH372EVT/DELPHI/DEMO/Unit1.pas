unit Unit1;
interface
Type
    COMMAND_PACKET=record     // 自定义的命令包结构
    mCommandCode : Byte;      // 命令请求码,见下面的定义
    mCommandCodeNot : Byte;   // 命令码的反码,用于校验命令包
    mParameter:array[0..4] of Byte;   // 参数
            //  mParamter(4)相当于 UCHAR   mBufferID;  // 缓冲区识别码,本程序针对MCS51单片机定义: 1-专用功能寄存器SFR, 2-内部RAM, 3-外部RAM, 不过本程序实际只演示内部RAM
            //  USHORT  mBufferAddr;    // 读写操作的起始地址,寻址范围是0000H-0FFFFH,低字节在前
            //  USHORT  mLength;    // 数据块总长度,低字节在前
End;
type
     Tintbuffpkt=record
     buff1 : integer ;
     buff2 : integer ;
     buff3 : integer ;
     buff4 : integer ;
     buff5 : integer ;
     buff6 : integer ;
     buff7 : integer ;
     buff8 : integer ;
end;
var
      mCOMMAND_PACKET : COMMAND_PACKET ;
      mEnterBuffer : String;
      mBufferLength : cardinal;       // 缓冲区内的有效数据长度
      mShowLED : Integer=0;
      mIndex : cardinal=0;
      mKeyCode : Integer=$FF;
      mCaptionInform :array[0..7]of char='信息提示';
      mDemoReq : COMMAND_PACKET;
      mOpen : Integer;
      index:integer;     //按下的按钮序号
      intbuff:Tintbuffpkt;
Const DEMO_GET_KEY = $34;
      DEMO_RET_OK = $0;
      CONST_CMD_LEN = $7;           // 命令块的长度
// 由于命令与数据都是通过数据下传管道(USB端点2的OUT)下传, 为了防止两者混淆,
// 我们可以在计算机应用程序与单片机程序之间约定, 命令块的长度总是7, 而数据块的长度肯定不是7, 例如64,32等
// 另外, 可以约定, 命令块的首字节是命令码, 等等
// 本程序约定命令码: 80H-0FFH是通用命令,适用于各种应用
//                   00H-7FH是专用命令,针对各种应用特别定义
// 通用命令
   DEF_CMD_GET_INFORM = $90;          // 获取下位机的说明信息,长度不超过64个字符,字符串以00H结束
   DEF_CMD_TEST_DATA = $91;           // 测试命令,下位机将PC机发来的命令包的所有数据取反后返回
   DEF_CMD_CLEAR_UP = $A0;            // 在上传数据块之前进行同步,实际是让下位机清除上传缓冲区的已有内容
   DEF_CMD_UP_DATA = $A1;             // 从下位机的指定地址的缓冲区中读取数据块(上传数据块)
   DEF_CMD_DOWN_DATA = $A2;           // 向下位机的指定地址的缓冲区中写入数据块(下传数据块)
// 专用命令
   DEMO_CH451_CMD = $56;              // PC发送命令给CH451,用于演示CH451的功能
// 对于MCS51单片机在使用通用命令时,还需要指定缓冲区识别码
   ACCESS_MCS51_SFR = 1;              // 读写51单片机的SFR
   ACCESS_MCS51_IRAM = 2;             // 读写51单片机的内部RAM
   ACCESS_MCS51_XRAM = 3;             // 读写51单片机的外部RAM

function strtochar(str:string):char;
function mCharToBcd(iChar:CHAR ):Byte;// 十六进制转BCD
function strLeft(str:shortstring ; count:integer):string;
function StrRight(str:string ; count:integer):string;
function GetDisBtHwnd(index:cardinal):cardinal;
function GetBtHwnd(KeyNo:cardinal):cardinal;      //获取相应键号按钮句柄
procedure mSyncKeyboard(iKeyCode : cardinal);

implementation
uses main,Ch375dll,Windows,sysutils,enterdate;

procedure mSyncKeyboard(iKeyCode : cardinal);           // 输入的按键值,00H-3FH则键被按下,0FFH则按下的键被释放
// 以下方法只适用于IDC_K0至IDC_K63完全有序的情况
begin
    If iKeyCode = $FF Then                              // 释放刚按下的键
         //Command2(mKeyNo).Caption = CStr(mKeyNo);     //恢复显示键号
        SetDlgItemText( frmmain.Handle  , GetBtHwnd(mKeyNo), Pchar(inttostr(mKeyNo)))
    Else                                                // 键被按下
        begin
        mKeyNo := iKeyCode And $3F;                     // 键号0-63
        SetDlgItemText( frmmain.Handle  , GetBtHwnd(mKeyNo), pchar('◎') );  // 00H-3FH键被按下则显示◎
        End;
end;


 function GetDisBtHwnd(index:cardinal):cardinal;  //获取相应键号按钮句柄
  begin
     case index of

                  0:GetDisBtHwnd:=FrmMain.Button1.Handle;
                  1:GetDisBtHwnd:=FrmMain.Button2.Handle;
                  2:GetDisBtHwnd:=FrmMain.Button3.Handle;
                  3:GetDisBtHwnd:=FrmMain.Button4.Handle;
                  4:GetDisBtHwnd:=FrmMain.Button5.Handle;
                  5:GetDisBtHwnd:=FrmMain.Button6.Handle;
                  6:GetDisBtHwnd:=FrmMain.Button7.Handle;
                  7:GetDisBtHwnd:=FrmMain.Button8.Handle
     else
          GetDisBtHwnd:=0;
     end;
end;

function GetBtHwnd(KeyNo:cardinal):cardinal;  //获取键盘区相应键号按钮句柄
  begin
     case keyno of
                  0:GetBtHwnd:=FrmMain.Button10.Handle;
                  1:GetBtHwnd:=FrmMain.Button11.Handle;
                  2:GetBtHwnd:=FrmMain.Button12.Handle;
                  3:GetBtHwnd:=FrmMain.Button13.Handle;
                  4:GetBtHwnd:=FrmMain.Button14.Handle;
                  5:GetBtHwnd:=FrmMain.Button15.Handle;
                  6:GetBtHwnd:=FrmMain.Button16.Handle;
                  7:GetBtHwnd:=FrmMain.Button17.Handle;
                  8:GetBtHwnd:=FrmMain.Button18.Handle;
                  9:GetBtHwnd:=FrmMain.Button19.Handle;
                  10:GetBtHwnd:=FrmMain.Button20.Handle;
                  11:GetBtHwnd:=FrmMain.Button21.Handle;
                  12:GetBtHwnd:=FrmMain.Button22.Handle;
                  13:GetBtHwnd:=FrmMain.Button23.Handle;
                  14:GetBtHwnd:=FrmMain.Button24.Handle;
                  15:GetBtHwnd:=FrmMain.Button25.Handle;
                  16:GetBtHwnd:=FrmMain.Button26.Handle;
                  17:GetBtHwnd:=FrmMain.Button27.Handle;
                  18:GetBtHwnd:=FrmMain.Button28.Handle;
                  19:GetBtHwnd:=FrmMain.Button29.Handle;
                  20:GetBtHwnd:=FrmMain.Button30.Handle;
                  21:GetBtHwnd:=FrmMain.Button31.Handle;
                  22:GetBtHwnd:=FrmMain.Button32.Handle;
                  23:GetBtHwnd:=FrmMain.Button33.Handle;
                  24:GetBtHwnd:=FrmMain.Button34.Handle;
                  25:GetBtHwnd:=FrmMain.Button35.Handle;
                  26:GetBtHwnd:=FrmMain.Button36.Handle;
                  27:GetBtHwnd:=FrmMain.Button37.Handle;
                  28:GetBtHwnd:=FrmMain.Button38.Handle;
                  29:GetBtHwnd:=FrmMain.Button39.Handle;
                  30:GetBtHwnd:=FrmMain.Button40.Handle;
                  31:GetBtHwnd:=FrmMain.Button41.Handle;
                  32:GetBtHwnd:=FrmMain.Button42.Handle;
                  33:GetBtHwnd:=FrmMain.Button43.Handle;
                  34:GetBtHwnd:=FrmMain.Button44.Handle;
                  35:GetBtHwnd:=FrmMain.Button45.Handle;
                  36:GetBtHwnd:=FrmMain.Button46.Handle;
                  37:GetBtHwnd:=FrmMain.Button47.Handle;
                  38:GetBtHwnd:=FrmMain.Button48.Handle;
                  39:GetBtHwnd:=FrmMain.Button49.Handle;
                  40:GetBtHwnd:=FrmMain.Button50.Handle;
                  41:GetBtHwnd:=FrmMain.Button51.Handle;
                  42:GetBtHwnd:=FrmMain.Button52.Handle;
                  43:GetBtHwnd:=FrmMain.Button53.Handle;
                  44:GetBtHwnd:=FrmMain.Button54.Handle;
                  45:GetBtHwnd:=FrmMain.Button55.Handle;
                  46:GetBtHwnd:=FrmMain.Button56.Handle;
                  47:GetBtHwnd:=FrmMain.Button57.Handle;
                  48:GetBtHwnd:=FrmMain.Button58.Handle;
                  49:GetBtHwnd:=FrmMain.Button59.Handle;
                  50:GetBtHwnd:=FrmMain.Button60.Handle;
                  51:GetBtHwnd:=FrmMain.Button61.Handle;
                  52:GetBtHwnd:=FrmMain.Button62.Handle;
                  53:GetBtHwnd:=FrmMain.Button63.Handle;
                  54:GetBtHwnd:=FrmMain.Button64.Handle;
                  55:GetBtHwnd:=FrmMain.Button65.Handle;
                  56:GetBtHwnd:=FrmMain.Button66.Handle;
                  57:GetBtHwnd:=FrmMain.Button67.Handle;
                  58:GetBtHwnd:=FrmMain.Button68.Handle;
                  59:GetBtHwnd:=FrmMain.Button69.Handle;
                  60:GetBtHwnd:=FrmMain.Button70.Handle;
                  61:GetBtHwnd:=FrmMain.Button71.Handle;
                  62:GetBtHwnd:=FrmMain.Button72.Handle;
                  63:GetBtHwnd:=FrmMain.Button73.Handle;
     else
                     GetBtHwnd:=0;
  end;
end;

function strtochar(str:string):char;
begin
    strtochar:=str[1];
end;

Function mCharToBcd(iChar:CHAR ):Byte; // 十六进制码转BCD码
var mBCD:Byte;
begin
    case iChar of
          '0'..'9': mBCD:=(ord(iChar) - ord('0'));
          'A'..'F': mBCD:=(ord(iChar) - ord('A') + $A);
          'a'..'f': mBCD:=(ord(iChar) - ord('a') + $A)
    Else
          mBCD:= $FF;
    end;
    mCharToBcd:= mBCD;
end;

function strLeft(str:shortstring ; count:integer):string; //左取字符
var res:string;
begin
         res:=copy( str,1,count);
         strleft:=res;
end;
Function StrRight(str:string ; count:integer):string;     //右取字符
var res:string;
begin
         res:=copy( str,length(str)-count+1,count);
         strright:=res;
end;

//下传数据例程
{
function mDownloadData(iBuffer:array of byte; iLength:cardinal ):cardinal;
var
    mDemoReq:COMMAND_PACKET;
    mLength:cardinal;
begin
    if ( iLength > 4096 ) then
    begin
      Messagebox( FrmMain.Handle, '单次下传数据长度超过4096字节', mCaptionInform, MB_OK+MB_ICONERROR );
      mDownloadData:=0;
      exit;
    end;
    mDemoReq.mCommandCode:= DEF_CMD_DOWN_DATA;	// 连续下传数据块
    mDemoReq.mCommandCodeNot := $FF- DEF_CMD_DOWN_DATA;
    mDemoReq.mParameter[0] := ACCESS_MCS51_XRAM; // 读写51单片机的外部RAM(本演示板的单片机没有外部RAM,所以无法演示)
    mDemoReq.mParameter[1] := 00;	// 指定外部RAM缓冲区起始地址,该例是将数据下传到起始地址为0X8200的外部RAM
    mDemoReq.mParameter[2] :=$82;
    mDemoReq.mParameter[3] := iLength mod 256;  // 传输数据总长度低二位
    mDemoReq.mParameter[4] := iLength div 256;
    mLength := CONST_CMD_LEN;	// 命令包的长度
    if ( CH375WriteData( mIndex, @mDemoReq, @mLength ) ) then // 通过CH375发送命令包,成功
    begin
      mLength := iLength;	// 数据块的长度,一次下传不超过4096字节
      if ( ( mLength mod 64 ) = CONST_CMD_LEN ) then mLength:=mLength+1;  // 防止数据包的长度与命令包的长度相同,如果相同,则多发送一个无效数据
      if ( CH375WriteData( mIndex, @mDemoReq, @mLength ) ) then // 通过CH375发送数据,成功
          mDownloadData:=mLength
      else MessageBox( frmmain.Handle, 'CH375WriteData 下传数据失败', mCaptionInform, MB_OK + MB_ICONERROR );
    end
    else MessageBox( frmmain.Handle, 'CH375WriteData 发送命令失败,DEF_CMD_DOWN_DATA', mCaptionInform, MB_OK + MB_ICONERROR );
         mDownloadData:=0;
end;

//上传数据例程
Function mUploadData( iBuffer:array of byte;iLength:cardinal ):cardinal;
var
	mDemoReq:COMMAND_PACKET ;
	mLength:cardinal;
begin
     if ( iLength > 4096 ) then
        MessageBox( frmmain.Handle, '单次上传数据长度超过4096字节', mCaptionInform, MB_OK + MB_ICONERROR );
	mDemoReq.mCommandCode := DEF_CMD_CLEAR_UP;	// 连续上传数据块之前进行同步,实际是让单片机清除上传缓冲区的已有内容
	mDemoReq.mCommandCodeNot :=$FF-DEF_CMD_CLEAR_UP;
	mLength := CONST_CMD_LEN;	// 命令包的长度
	if ( CH375WriteData( mIndex, @mDemoReq, @mLength ) ) then // 通过CH375发送命令包,成功
	  begin
	    mDemoReq.mCommandCode := DEF_CMD_UP_DATA;	// 连续上传数据块
	    mDemoReq.mCommandCodeNot := $FF- DEF_CMD_UP_DATA;
	    mDemoReq.mParameter[0]  := ACCESS_MCS51_XRAM; // 读写51单片机的外部RAM(本演示板的单片机没有外部RAM,所以无法演示)
            mDemoReq.mParameter[1] := 00;	// 指定外部RAM缓冲区起始地址,该例是将数据下传到起始地址为0X8200的外部RAM
            mDemoReq.mParameter[2] :=$82;
            mDemoReq.mParameter[3] := iLength mod 256;  // 传输数据总长度低二位
            mDemoReq.mParameter[4] := iLength div 256;
	    mLength := CONST_CMD_LEN;	// 命令包的长度
		if ( CH375WriteData( mIndex, @mDemoReq, @mLength ) ) then // 通过CH375发送命令包,成功
		  begin
			mLength := iLength;	// 数据块的长度,一次上传不超过4096字节
			if ( CH375ReadData( mIndex, @iBuffer, @mLength ) )  then// 通过CH375接收数据,成功
			    mUploadData := mLength
			else MessageBox( frmmain.Handle, 'CH375ReadData 上传数据失败', mCaptionInform, MB_OK + MB_ICONERROR );
		  end
		else MessageBox( frmmain.Handle, 'CH375WriteData 发送命令失败,DEF_CMD_UP_DATA', mCaptionInform, MB_OK + MB_ICONERROR );
	  end
	else MessageBox( frmmain.Handle, 'CH375WriteData 发送命令失败,DEF_CMD_CLEAR_UP', mCaptionInform, MB_OK + MB_ICONERROR );
	  mUploadData:= 0;
End;
}

end.




