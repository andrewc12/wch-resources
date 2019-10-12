unit Main;

interface

uses
  Windows, SysUtils, Variants, Classes, Forms, StdCtrls, Controls;

type
  TFrmMain = class(TForm)


    Label1: TLabel;
    Label2: TLabel;
    Button1: TButton;
    Button2: TButton;
    Button3: TButton;
    Button4: TButton;
    Button5: TButton;
    Button6: TButton;
    Button7: TButton;
    Button8: TButton;
    Button9: TButton;
    Button10: TButton;
    Button11: TButton;
    Button12: TButton;
    Button13: TButton;
    Button14: TButton;
    Button15: TButton;
    Button16: TButton;
    Button17: TButton;
    Button18: TButton;
    Button19: TButton;
    Button20: TButton;
    Button21: TButton;
    Button22: TButton;
    Button23: TButton;
    Button24: TButton;
    Button25: TButton;
    Button26: TButton;
    Button27: TButton;
    Button28: TButton;
    Button29: TButton;
    Button30: TButton;
    Button31: TButton;
    Button32: TButton;
    Button33: TButton;
    Button34: TButton;
    Button35: TButton;
    Button36: TButton;
    Button37: TButton;
    Button38: TButton;
    Button39: TButton;
    Button40: TButton;
    Button41: TButton;
    Button42: TButton;
    Button43: TButton;
    Button44: TButton;
    Button45: TButton;
    Button46: TButton;
    Button47: TButton;
    Button48: TButton;
    Button49: TButton;
    Button50: TButton;
    Button51: TButton;
    Button52: TButton;
    Button53: TButton;
    Button54: TButton;
    Button55: TButton;
    Button56: TButton;
    Button57: TButton;
    Button58: TButton;
    Button59: TButton;
    Button60: TButton;
    Button61: TButton;
    Button62: TButton;
    Button63: TButton;
    Button64: TButton;
    Button65: TButton;
    Button66: TButton;
    Button67: TButton;
    Button68: TButton;
    Button69: TButton;
    Button70: TButton;
    Button71: TButton;
    Button72: TButton;
    Button73: TButton;
    devstatue: TLabel;
    pnpproce: TButton;
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button4Click(Sender: TObject);
    procedure Button5Click(Sender: TObject);
    procedure Button6Click(Sender: TObject);
    procedure Button7Click(Sender: TObject);
    procedure Button8Click(Sender: TObject);
    procedure Button9Click(Sender: TObject);
    procedure FormKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure FormKeyUp(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure pnpproceKeyUp(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    //function mInterruptEvent(mBuffer:pbytearray):cardinal;                //中断服务程序
    //procedure mCallInt();
  private
    { Private declarations }
  public

    { Public declarations }
  end;

var
  FrmMain: TFrmMain;
  mKeyNo :cardinal=0;  //模拟按键号
  const WM_KEYDOWN=$100;
        WM_KEYUP=$101;

implementation
uses CH375dll,unit1,EnterDate;
{$R *.dfm}

//iBuffer 指向一个缓冲区,提供当前的中断特征数据
//******建议此程序不要设置断点******
 // 中断服务程序
procedure mInterruptEvent(mBuffer:pbytearray );stdcall;  // 指向一个缓冲区,提供当前的中断特征数据
begin
    // CH371产生中断后,产生一个消息通知主程序
    If mBuffer[0] = 1 Then        // 检查中断特征数据
        begin
        mKeyCode := mBuffer[1] And $3F; //取键值
        PostMessage(frmmain.Handle, $100, 1, 0); // 中断特征数据为1则相应键号按钮被按下
        end
    Else
        If mBuffer[0] = 2 Then
         PostMessage(frmmain.Handle, $101, 1, 0); // 中断特征数据为2则相应键号按键被释放
end;

procedure mCallInt() ;
var  x:boolean;
begin
    x:= CH375SetIntRoutine(mIndex,mInterruptEvent);    //调用中断子程序
    If x =false Then
    begin
        MessageBox(frmmain.Handle,'无法调用中断', mCaptionInform, MB_OK + MB_ICONERROR );
        Exit;
    End;
end;

// 以下测试PC机与单片机之间的USB通讯,仅作演示,本身没有意义
procedure TestCommunication();
var     mLength : cardinal;
begin

    mDemoReq.mCommandCode := DEF_CMD_TEST_DATA;    // 测试命令,将PC机发来的所有数据取反后返回
    mDemoReq.mCommandCodeNot := $FF - DEF_CMD_TEST_DATA;
    mDemoReq.mParameter[0] := $5A;  // 任意的测试数据,返回后将按位取反
    mDemoReq.mParameter[1] := $96;  // 任意的测试数据,返回后将按位取反
    mDemoReq.mParameter[2] := $F3;  // 任意的测试数据,返回后将按位取反
    mDemoReq.mParameter[3] := $4C;  // 任意的测试数据,返回后将按位取反
    mDemoReq.mParameter[4] := $39;  // 任意的测试数据,返回后将按位取反
    mLength := CONST_CMD_LEN;       // 命令包的长度
    If CH375WriteData(mIndex,@mDemoReq,@mLength) Then        // 通过CH375发送命令数据,成功
    begin
      mLength := mCH375_PACKET_LENGTH;
      If (CH375ReadData(mIndex,@mDemoReq, @mLength)) Then        // 通过CH375接收应答数据,成功
        begin
          If (mLength = CONST_CMD_LEN) Then
            begin
              If (mDemoReq.mCommandCode <> ($FF - DEF_CMD_TEST_DATA))
                 Or (mDemoReq.mParameter[0] <> ($FF - $5A))
                 Or (mDemoReq.mParameter[1] <> ($FF - $96)) Then
                  application.MessageBox ('通过USB传输的数据有错误', mCaptionInform, Mb_ok+ MB_ICONEXCLAMATION);
            end
          Else  application.MessageBox ( 'CH375数据测试返回的长度错误',  mCaptionInform,mb_ok+ MB_ICONEXCLAMATION);
        end
      Else  application.MessageBox ( 'CH375ReadData 失败',  mCaptionInform,mb_ok+ MB_ICONEXCLAMATION);
    end
    Else  application.MessageBox ( 'CH375WriteData 失败',  mCaptionInform,mb_ok+ MB_ICONEXCLAMATION);

end;

//设备事件通知回调程序,将事件消息发送到指定控件的消息处理过程中处理.
Procedure NotifyRoutine(iEventStatus:cardinal );stdcall;
begin
    PostMessage(frmmain.pnpproce.Handle ,WM_KEYUP,iEventStatus,0);
end;

//插拔事件处理过程
procedure TFrmMain.pnpproceKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
var
    iEventStatus:cardinal;
    statue:longbool;
begin
    iEventStatus := Key;   //插拔事件代码
    statue:=FALSE;
    If (iEventStatus = CH375_DEVICE_ARRIVAL) Then
    begin
        if(mOpen=-1) then //设备还没打开
        begin
            mOpen := CH375OpenDevice(mIndex);                  //窗体加载时打开设备
            If (mOpen = -1) Then
            begin
                Application.MessageBox('无法打开CH375设备', '信息提示',0);
                Exit;
            End;
            CH375SetTimeout( mIndex, 3000, 3000 );  // 设置USB数据读写的超时,超过3000mS未完成读写将强制返回,避免一直等待下去
            TestCommunication();
            mCallInt ;  //调用中断程序
        end;
        frmmain.devstatue.Caption := '**CH372/CH375设备已插上';
        statue:=TRUE;
    end
    Else
    begin
        If (iEventStatus = CH375_DEVICE_REMOVE) Then
        begin
            if(mOpen<>-1) then
            begin
                CH375CloseDevice (mIndex);     //设备拔出,关闭设备句柄
                mOpen :=-1;
            end;
            devstatue.Caption := '**CH372/CH375设备已拔出';
            statue:=FALSE;
        End;
    end;
    //设备打开,按钮可用;设备没打开,按钮禁用
    //LED1...LED8
    frmmain.Button1.Enabled := statue;
    frmmain.Button2.Enabled := statue;
    frmmain.Button3.Enabled := statue;
    frmmain.Button4.Enabled := statue;
    frmmain.Button5.Enabled := statue;
    frmmain.Button6.Enabled := statue;
    frmmain.Button7.Enabled := statue;
    frmmain.Button8.Enabled := statue;
    //命令按钮
    frmmain.Button9.Enabled := statue;
end;

procedure TFrmMain.FormCreate(Sender: TObject);

begin
    mCaptionInform:= '信息提示';

    //设置设备插拔监视
    if(CH375SetDeviceNotify(mIndex,'',NotifyRoutine)=FALSE) then
        Application.MessageBox(  '设置监视CH372/CH375设备插拔失败', mCaptionInform, 0);

    mOpen := CH375OpenDevice(mIndex);                  //窗体加载时打开设备
    If mOpen = -1 Then
    begin
      //application.MessageBox( '无法打开CH375设备',mCaptionInform,MB_OK + MB_ICONEXCLAMATION);
      //halt(0);    //关闭程序
      NotifyRoutine( CH375_DEVICE_REMOVE);  //作设备拔出处理
      exit;
    End;



    CH375SetTimeout( mIndex, 3000, 3000 );  // 设置USB数据读写的超时,超过3000mS未完成读写将强制返回,避免一直等待下去

    //测试PC机与单片机之间的USB通讯,仅作演示,本身没有意义
    TestCommunication();

//下面加载中断服务程序 , 中断服务程序是在应用层执行的, 其线程优先级是THREAD_PRIORITY_TIME_CRITICAL
//当单片机有事需要通知计算机时 , 可以用CMD_WR_USB_DATA5命令写入中断特征数据, 计算机的mInterruptEvent线程将会收到该中断特征数据
//然后mInterruptEvent线程向主程序发出消息进行处理,mInterruptEvent线程相当于中断服务程序,代替主程序定时查询单片机

    mCallInt ;  //调用中断程序
    NotifyRoutine(CH375_DEVICE_ARRIVAL);    //作设备插入处理
end;

procedure TFrmMain.FormClose(Sender: TObject; var Action: TCloseAction);
begin
   CH375SetDeviceNotify(mIndex,'',mPCH375_NOTIFY_ROUTINE(0));     //取消息设备挺拔通知
   CH375SetIntRoutine(mIndex,TiIntRoutine(0));                    //取消中断上传
   CH375CloseDevice (mIndex);       //程序退出前关闭设备
   halt(0);
end;

procedure TFrmMain.FormDestroy(Sender: TObject);
begin
    CH375CloseDevice (mIndex);  //程序中断关闭设备
end;

procedure TFrmMain.Button1Click(Sender: TObject);  //数码管按钮1
var
  f: TEnterDlg;
begin
  index:=0;
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;
end;

procedure TFrmMain.Button2Click(Sender: TObject);  //数码管按钮2
var
  f: TEnterDlg;
begin
  index:=1;
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;
end;

procedure TFrmMain.Button3Click(Sender: TObject);   //数码管按钮3
var
  f: TEnterDlg;
begin
  index:=2;
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;

end;

procedure TFrmMain.Button4Click(Sender: TObject); //数码管按钮4
var
  f: TEnterDlg;
begin
  index:=3;
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;
end;

procedure TFrmMain.Button5Click(Sender: TObject); //数码管按钮5
var
  f: TEnterDlg;
begin
  index:=4;
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;

end;

procedure TFrmMain.Button6Click(Sender: TObject); //数码管按钮6
var
  f: TEnterDlg;
begin
  index:=5;
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;
end;

procedure TFrmMain.Button7Click(Sender: TObject);  //数码管按钮7
var
  f: TEnterDlg;
begin
  index:=6;
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;
end;

procedure TFrmMain.Button8Click(Sender: TObject); //数码管按钮8
var
  f: TEnterDlg;
begin
  index:=7;
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;

end;

procedure TFrmMain.Button9Click(Sender: TObject);
var
  f: TEnterDlg;
begin
  index:=-1;                    //识别命令按钮序号
  f := TEnterDlg.Create(Self);  //数据输入框显示
  f.ShowModal;
end;

//模拟键盘区中按键按下事件
//同步按键显示,键被按下时显示◎,否则显示键号
procedure TFrmMain.FormKeyDown(Sender: TObject; var Key: Word;
                                           Shift: TShiftState);
begin
    If Key = 1 Then                // 键被按下,下面是先释放原按键再读取新按键值
        begin
        mSyncKeyboard($FF);        // 释放上次的按键
        mSyncKeyboard(mKeyCode);   //根据键值作同步显示
        end;

end;

//模拟键盘区中按键按下事件
//同步按键显示,键被按下时显示◎,否则显示键号
procedure TFrmMain.FormKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
   If Key = 1 Then
      mSyncKeyboard($FF);   // 键盘按键松开
end;


end.





