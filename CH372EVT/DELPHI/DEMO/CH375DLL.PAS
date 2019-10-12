unit CH375DLL;

interface
// 2003.09.08, 2003.12.28, 2004.10.15, 2004.12.05, 2004.12.10, 2005.01.20, 2005.02.23, 2005.07.15
//****************************************
//**  Copyright  (C)  W.ch  1999-2005   **
//**  Web:  http://www.winchiphead.com  **
//****************************************
//**  DLL for USB interface chip CH375  **
//**  C, VC5.0                          **
//****************************************
//
// USB总线接口芯片CH375的应用层接口库  V2.1
// 南京沁恒电子有限公司  作者: W.ch 2005.07
// CH375-DLL  V2.1 , Support: Ctrl/Bulk/Int
// 运行环境: Windows 98/ME, Windows 2000/XP
// support USB chip: CH372/CH375
//
uses main,SysUtils,unit1;
Const
   mCH375_PACKET_LENGTH = 64;         // CH375支持的数据包的长度
   mCH375_MAX_NUMBER = 16;            // 最多同时连接的CH375数
   mMAX_BUFFER_LENGTH = $1000;        // 数据缓冲区最大长度
   mDEFAULT_BUFFER_LEN = $400;        // 数据缓冲区默认长度1024

// CH375端点地址
   mCH375_ENDP_INTER_UP = $81;        // CH375的中断数据上传端点的地址
   mCH375_ENDP_INTER_DOWN = $1;       // CH375的中断数据下传端点的地址
   mCH375_ENDP_DATA_UP = $82;         // CH375的数据块上传端点的地址
   mCH375_ENDP_DATA_DOWN = $2;        //CH375的数据块下传端点的地址

// 设备层接口提供的管道操作命令
   mPipeDeviceCtrl = $4;              // CH375的综合控制管道;
   mPipeInterUp = $5;                 //CH375的中断数据上传管道;
   mPipeDataUp = $6;                  //CH375的数据块上传管道
   mPipeDataDown = $7;                //CH375的数据块下传管道
   mPipeAuxDown =$8;	              // CH375的辅助数据下传管道

// 应用层接口的功能代码
   mFuncNoOperation = $H0;            // 无操作
   mFuncGetVersion = $1;              // 获取驱动程序版本号
   mFuncGetConfig = $2;               // 获取USB设备配置描述符
   mFuncSetExclusive = $b;	      // 设置独占使用
   mFuncResetDevice = $C;             // 复位USB设备
   mFuncResetPipe = $D;               // 复位USB管道
   mFuncAbortPipe = $E;               // 取消USB管道的数据请求
   mFuncSetTimeout = $0f;	      // 设置USB通讯超时
   mFuncBufferMode = $10;			// 设定缓冲上传模式及查询缓冲区中的数据长度

// USB设备标准请求代码
   mUSB_CLR_FEATURE = $1;
   mUSB_SET_FEATURE = $3;
   mUSB_GET_STATUS = $0 ;
   mUSB_SET_ADDRESS = $5;
   mUSB_GET_DESCR = $6;
   mUSB_SET_DESCR = $7;
   mUSB_GET_CONFIG = $8;
   mUSB_SET_CONFIG = $9;
   mUSB_GET_INTERF = $A;
   mUSB_SET_INTERF = $B;
   mUSB_SYNC_FRAME = $C;

// CH375控制传输的厂商专用请求类型
   mCH375_VENDOR_READ = $C0;          //通过控制传输实现的CH375供应商专用读操作
   mCH375_VENDOR_WRITE = $40;         //通过控制传输实现的CH375供应商专用写操作

// CH375控制传输的供应商专用请求代码
   mCH375_SET_CONTROL = $51;          // 输出控制信号
   mCH375_GET_STATUS = $52;           // 输入状态信号

// 寄存器的位定义
   mBitInputRxd = $2;                 // 只读,RXD#引脚输入状态,1:高电平,0:低电平
   mBitInputReq = $4;                 // 只读,REQ#引脚输入状态,1:高电平,0:低电平

// 直接输入的状态信号的位定义
   mStateRXD = $200;                  // RXD#引脚输入状态,1:高电平,0:低电平
   mStateREQ = $400;                  // REQ#引脚输入状态,1:高电平,0:低电平

type
  PVOID =Pointer;
  plong=pcardinal;
  TiIntRoutine=procedure(mbuffer:pbytearray);stdcall;

Type
        mUspValue=record
        mUspValueLow : Byte;
        mUspValueHigh : Byte;
End;
Type
        mUspIndex=record
        mUspIndexLow : Byte;
        mUspIndexHigh  : Byte;
End ;
Type
    USB_SETUP_PKT=record
    mUspReqType : Byte;
    mUspRequest : Byte;
    mUspValue : mUspValue;
    mUspIndex : mUspIndex;
    mLength : Integer;
End ;
Type
   WIN32_COMMAND=record               //定义WIN32命令接口结构
   mFunction : cardinal;              //输入时指定功能代码或者管道号
                                      //输出时返回操作状态
   mLength : cardinal;                //存取长度,返回后续数据的长度
   mBuffer:array[0..(mCH375_PACKET_LENGTH-1)] of Byte;         //数据缓冲区,长度为0至255B                                           '数据缓冲区,长度为0至255B
End ;

var
   mUSB_SETUP_PKT :USB_SETUP_PKT;
   mWIN32_COMMAND : WIN32_COMMAND;
   mm:procedure(mbuffer:pbytearray);stdcall;

   Function CH375OpenDevice(iIndex :cardinal):cardinal ;Stdcall; external 'CH375DLL.DLL' ;
//打开CH375设备,返回句柄,出错则无效. 指定CH375设备序号,0对应第一个设备

   procedure CH375CloseDevice(iIndex :cardinal) ;Stdcall; external 'CH375DLL.DLL';
//关闭CH375设备,指定CH375设备序号

   Function CH375GetVersion():cardinal  ;Stdcall; external 'CH375DLL.DLL';
//获得DLL版本号,返回版本号

   Function CH375DriverCommand (           // 直接传递命令给驱动程序,出错则返回0,否则返回数据长度
                 iIndex:cardinal;          // 指定CH375设备序号,V1.6以上DLL也可以是设备打开后的句柄
                 ioCommand:WIN32_COMMAND   // 命令结构的指针
                 ):cardinal;Stdcall;external 'CH375DLL.DLL';

// 该程序在调用后返回数据长度,并且仍然返回命令结构,如果是读操作,则数据返回在命令结构中,
// 返回的数据长度在操作失败时为0,操作成功时为整个命令结构的长度,例如读一个字节,则返回mWIN32_COMMAND_HEAD+1,
// 命令结构在调用前,分别提供:管道号或者命令功能代码,存取数据的长度(可选),数据(可选)
// 命令结构在调用后,分别返回:操作状态代码,后续数据的长度(可选),
// 操作状态代码是由WINDOWS定义的代码,可以参考NTSTATUS.H,
// 后续数据的长度是指读操作返回的数据长度,数据存放在随后的缓冲区中,对于写操作一般为0

   Function CH375GetDrvVersion:cardinal;Stdcall; external'CH375DLL.DLL';
   //获得驱动程序版本号,返回版本号,出错则返回0

   Function CH375ResetDevice(iIndex:cardinal) : Boolean ;Stdcall; external 'CH375DLL.DLL';
   //复位USB设备, iIndex 指定CH375设备序号

   Function CH375GetDeviceDescr (iIndex :cardinal;oBuffer:pvoid;ioLength:plong) : Boolean ;Stdcall; external'CH375DLL.DLL';
   //读取设备描述符
   //iIndex指定CH375设备序号
   //oBuffer指向一个足够大的缓冲区,用于保存描述符
   //ioLength指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度

   Function CH375GetConfigDescr(iIndex :cardinal; oBuffer :pvoid; ioLength:plong) : Boolean ;Stdcall; external 'CH375DLL.DLL' ;
   //读取配置描述符
   //iIndex指定CH375设备序号
   //oBuffer指向一个足够大的缓冲区,用于保存描述符
   //ioLength  指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度

   Function CH375SetIntRoutine (iIndex:cardinal;iIntRoutine :TiIntRoutine) :boolean ;Stdcall; external 'CH375DLL.DLL';
   //设定中断服务程序
   //iIndex指定CH375设备序号
   //iIntRoutine指定中断服务程序,为NULL则取消中断服务,否则在中断时调用该程序
   //"TiIntRoutine"过程传递参数要用非默认的'Register'方式传递.

   Function CH375ReadInter(iIndex:cardinal;oBuffer:pvoid;ioLength :plong) :Boolean ; Stdcall;external 'CH375DLL.DLL';
   //读取中断数据
   //iIndex指定CH375设备序号
   //oBuffer指向一个足够大的缓冲区,用于保存描述符
   //ioLength指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度

   Function CH375AbortInter (iIndex :cardinal) :Boolean ;Stdcall; external 'CH375DLL.DLL';
   //放弃中断数据读操作
   //iIndex  指定CH375设备序号

   Function CH375ReadData (iIndex :cardinal;oBuffer :pvoid;ioLength :plong):Boolean ;Stdcall; external 'CH375DLL.DLL'; //读取数据块
   //iIndex指定CH375设备序号,oBuffer指向一个足够大的缓冲区,用于保存描述符,ioLength指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度

   function CH375AbortRead(iIndex:cardinal):boolean ;Stdcall; external 'CH375DLL.DLL' ;
   //放弃数据块读操作
   //iIndex  指定CH375设备序号

   Function CH375WriteData(iIndex :cardinal;iBuffer :pvoid;ioLength :plong):longbool ;Stdcall; external 'CH375DLL.DLL';
   //先写出标准的数据块(命令,长度不超过8字节),再读取标准的数据块(应答,长度不超过8字节)
   //iIndex指定CH375设备序号,oBuffer指向一个足够大的缓冲区,用于保存描述符,ioLength指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度

   Function CH375AbortWrite (iIndex:cardinal):boolean ;Stdcall; external 'CH375DLL.DLL';
   //放弃数据块写操作
   //iIndex  指定CH375设备序号

   Function CH375WriteRead (iIndex:cardinal;iBuffer:byte;oBuffer :pvoid;ioLength:plong):Boolean ;Stdcall; external 'CH375DLL.DLL'; //写出数据块
   //iIndex指定CH375设备序号,iBuffer指向一个缓冲区,放置准备写出的数据,长度不大于mCH375_PACKET_LENGTH,oBuffer指向一个足够大的缓冲区,用于保存描述符,ioLength指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度

   Function CH375GetStatus(iIndex:cardinal;var iStatus:plong):boolean ;Stdcall; external 'CH375DLL.DLL';
    //通过CH375直接输入数据和状态
    //iIndex   指定CH375设备序号
    //iStatus  指向一个双字单元,用于保存状态数据
    //位7-位0对应CH375的D7-D0引脚,位9对应CH375的RXD#引脚,位10对应CH375的REQ#引脚

   Function CH375SetTimeout(         // 设置USB数据读写的超时
	    iIndex:cardinal;         // 指定CH375设备序号
	    iWriteTimeout:cardinal;  // 指定USB写出数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
	    iReadTimeout:cardinal    // 指定USB读取数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
                            ):boolean;Stdcall; external 'CH375DLL.DLL';

   Function CH375WriteAuxData(     // 写出辅助数据
	iIndex:cardinal;           // 指定CH375设备序号
	iBuffer:pvoid;             // 指向一个缓冲区,放置准备写出的数据
	ioLength:plong             // 指向长度单元,输入时为准备写出的长度,返回后为实际写出的长度
                              ):boolean;Stdcall; external 'CH375DLL.DLL';

   Function CH375SetExclusive(  // 设置独占使用当前CH375设备
	iIndex:cardinal;        // 指定CH375设备序号
	iExclusive:cardinal     // 为0则设备可以共享使用,非0则独占使用
         ):boolean;Stdcall; external 'CH375DLL.DLL';

   Function CH375GetUsbID(  // 获取USB设备ID,返回数据中,低16位为厂商ID,高16位为产品ID,错误时返回全0(无效ID)
	    iIndex:cardinal // 指定CH375设备序号
                         ):cardinal;Stdcall; external 'CH375DLL.DLL';

   Function CH375GetDeviceName(  // 返回指向CH375设备名称的缓冲区,出错则返回NULL
	    iIndex:cardinal      // 指定CH375设备序号,0对应第一个设备
                               ):pvoid;Stdcall; external 'CH375DLL.DLL';

  Function CH375SetBufUpload(       // 设定内部缓冲上传模式
       iIndex:cardinal;             // 指定CH375设备序号,0对应第一个设备
        iEnableOrClear:cardinal ):longbool;Stdcall; external 'CH375DLL.DLL';  // 为0则禁止内部缓冲上传模式,使用直接上传,非0则启用内部缓冲上传模式并清除缓冲区中的已有数据
// 如果启用内部缓冲上传模式,那么CH375驱动程序创建线程自动接收USB上传数据到内部缓冲区,同时清除缓冲区中的已有数据,当应用程序调用CH375ReadData后将立即返回缓冲区中的已有数据

  Function CH375QueryBufUpload(  // 查询内部上传缓冲区中的已有数据包个数,成功返回数据包个数,出错返回-1
        	iIndex:cardinal ):integer;Stdcall; external 'CH375DLL.DLL';  // 指定CH375设备序号,0对应第一个设备

  Function CH375SetBufDownload(  // 设定内部缓冲下传模式
	iIndex:cardinal;  // 指定CH375设备序号,0对应第一个设备
	iEnableOrClear:cardinal ):longbool;Stdcall; external 'CH375DLL.DLL';  // 为0则禁止内部缓冲下传模式,使用直接下传,非0则启用内部缓冲下传模式并清除缓冲区中的已有数据
// 如果启用内部缓冲下传模式,那么当应用程序调用CH375WriteData后将仅仅是将USB下传数据放到内部缓冲区并立即返回,而由CH375驱动程序创建的线程自动发送直到完毕


  Function CH375QueryBufDownload(  // 查询内部下传缓冲区中的剩余数据包个数(尚未发送),成功返回数据包个数,出错返回-1
        iIndex:cardinal ):integer;Stdcall; external 'CH375DLL.DLL';  // 指定CH375设备序号,0对应第一个设备


  Function CH375ResetInter(  // 复位中断数据读操作
	 iIndex:cardinal ):longbool;Stdcall; external 'CH375DLL.DLL';  // 指定CH375设备序号


  Function CH375ResetAux(  // 复位辅助数据写操作
	iIndex:cardinal ):longbool;Stdcall; external 'CH375DLL.DLL';  // 指定CH375设备序号


  Function CH375ResetRead(  // 复位数据块读操作
        iIndex:cardinal ):longbool;Stdcall; external 'CH375DLL.DLL';  // 指定CH375设备序号


  Function CH375ResetWrite(  // 复位数据块写操作
	iIndex:cardinal ):longbool;Stdcall; external 'CH375DLL.DLL';  // 指定CH375设备序号

  type
	mPCH375_NOTIFY_ROUTINE=Procedure (iEventStatus:cardinal );stdcall;  // 设备事件通知回调程序
       // 设备事件和当前状态(在下行定义): 0=设备拔出事件, 3=设备插入事件

  const	CH375_DEVICE_ARRIVAL=	3;		// 设备插入事件,已经插入
	CH375_DEVICE_REMOVE_PEND=1;		// 设备将要拔出
        CH375_DEVICE_REMOVE=0;		// 设备拔出事件,已经拔出


  Function CH375SetDeviceNotify(  // 设定设备事件通知程序
	iIndex:cardinal;  // 指定CH375设备序号,0对应第一个设备
	iDeviceID:PCHAR;  // 可选参数,指向字符串,指定被监控的设备的ID,字符串以\0终止
	iNotifyRoutine:mPCH375_NOTIFY_ROUTINE ):longbool;Stdcall; external 'CH375DLL.DLL';  // 指定设备事件回调程序,为NULL则取消事件通知,否则在检测到事件时调用该程序

implementation

end.

