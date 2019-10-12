unit EnterDate;
interface 
uses Windows, SysUtils, Variants, Classes, Forms, StdCtrls, Controls;
type
  TEnterDlg = class(TForm)
    OKBtn: TButton;
    CancelBtn: TButton;
    Text1: TEdit;
    Label1: TLabel;   //按下的按钮序号
    procedure OKBtnClick(Sender: TObject);
    procedure CancelBtnClick(Sender: TObject);
  private
    { Private declarations }
  public

    { Public declarations }
  end;

var
  EnterDlg: TEnterDlg;
  index : Integer=0;   //按下的按钮序号
Const  DEMO_CH451_CMD= $56;

implementation
uses Ch375dll,unit1,Main ;
{$R *.dfm}
var
    mNumber : Byte;
    mBcdCode1:Byte;
    mLength:cardinal;
    mBcdCode:array[0..2] of Byte;

procedure TEnterDlg.OKBtnClick(Sender: TObject);
var
   ilen:Integer;
   mDemoReq:COMMAND_PACKET;
   BtDisChar:pchar;
begin
    mEnterBuffer := Text1.Text;
    If index <> -1 Then              //数码管对应的按键
      begin
        If Text1.Text <> '' Then
          begin
            case mEnterBuffer[1] of
                 ' ':mBcdCode1:=$10; // 空格
                 '=':mBcdCode1:=$13;
                 '.':mBcdCode1:=$1A
            Else
                mBcdCode1 := mCharToBcd(strtochar(strLeft(mEnterBuffer, 1)));  //取输入字符串中最左边一位
            END;
            ilen := Length(mEnterBuffer);
            If (ilen >= 2) And (strRight(strLeft(mEnterBuffer, 2), 1) = '.') Then  mBcdCode1 := (mBcdCode1 Or $80);
            If mBcdCode1 <> $FF Then            //输入的字符有效
               begin
                mNumber := index;               // 如果Bt1至Bt8有序,则结果为0-7,分别对应于各个数码管按钮
                mNumber := 7 - mNumber;
                mDemoReq.mCommandCode := DEMO_CH451_CMD;
                mDemoReq.mCommandCodeNot := $FF - DEMO_CH451_CMD;
                mDemoReq.mParameter[2] := ($8 Or mNumber);      // CH451命令:加载字数据到指定数码管
                mDemoReq.mParameter[1] := mBcdCode1;            //字数据,对应于BCD译码方式
                mLength := CONST_CMD_LEN;    // 命令包的长度
                ////On Error Resume Next
                If CH375WriteData(mIndex, @mDemoReq, @mLength) Then    // 通过CH371发送数据,成功
                // 如果输入时mLength大于8,则成功返回时,为了更加可靠,可以检查实际发送的长度是否符合
                    begin
                    //mEnterBuffer[2] := '0'; // 字符串结束符,只要一个字符
                    //frmMain.Command1(index).Caption = Left(mEnterBuffer, 1)  //设置按钮上的字符
                    Btdischar:=AnsiStrUpper(pchar(copy(mEnterBuffer,1,1)));
                    SetDlgItemText( frmmain.Handle , GetDisBtHwnd(index),Btdischar);
                    end
                Else
                   application.MessageBox('CH375WriteData 失败！','信息提示',Mb_ok+MB_ICONEXCLAMATION);
               END
            Else
                application.MessageBox('请输入字符0-9,A-F或者空格','信息提示',Mb_ok+MB_ICONEXCLAMATION);
          END
        Else
            application.MessageBox('请输入字符0-9,A-F或者空格','信息提示',Mb_ok+MB_ICONEXCLAMATION);
        //index := 0;
        EXIT;  //退出当前窗体
      end
    Else
    begin
      if index=-1 then             //命令按钮触发
      begin
        If (Length(mEnterBuffer) >= 3) Then    // 已经输入数据,并且长度超过3位
          begin
            mBcdCode[0] := mCharToBcd(strtochar(strLeft(mEnterBuffer, 1)));
            mBcdCode[1] := mCharToBcd(strtochar(strRight(strLeft(mEnterBuffer, 2), 1)));
            mBcdCode[2] := mCharToBcd(strtochar(strRight(strLeft(mEnterBuffer, 3), 1)));
            If (mBcdCode[0] <> $FF) And (mBcdCode[1] <> $FF) And (mBcdCode[2] <> $FF) Then    // 输入的三个字符都有效
              begin
                mDemoReq.mCommandCode := DEMO_CH451_CMD;
                mDemoReq.mCommandCodeNot := $FF - DEMO_CH451_CMD;
                mDemoReq.mParameter[2] := mBcdCode[0];  //CH451命令:高4位
                mDemoReq.mParameter[1] := mBcdCode[1] * $10 Or mBcdCode[2]; // 低8位的字节数据
                mLength := CONST_CMD_LEN;    // 命令包的长度
                If CH375WriteData(mIndex, @mDemoReq, @mLength) Then    // 通过CH375发送数据,成功
                    application.MessageBox('命成功发送到CH451', '信息提示',Mb_ok+MB_ICONEXCLAMATION)
                Else
                    application.MessageBox('CH375WriteData 失败','信息提示',Mb_ok+MB_ICONEXCLAMATION);
              end
            Else
                application.MessageBox('请输入3个字符0-9,A-F','信息提示',Mb_ok+MB_ICONEXCLAMATION);
          end
        Else
           begin
            application.MessageBox('请至少输入3个字符0-9,A-F','信息提示',Mb_ok+MB_ICONEXCLAMATION);
            exit;
           end;
      end;
  end;
end;

procedure TEnterDlg.CancelBtnClick(Sender: TObject);
begin
    mEnterBuffer := '' ;         //放弃按钮
    exit;
end;

end.

