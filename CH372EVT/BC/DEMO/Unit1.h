//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
        TLabel *Label1;
        TLabel *Label2;
        TBitBtn *btnCmd;
        TBitBtn *btnSend;
        TBitBtn *btnClose;
        TEdit *edtN1;
        TEdit *edtCmd;
    TButton *btnInt;
    TButton *btnRe;
    TLabel *Label3;
    TEdit *edtInt;
        void __fastcall btnSendClick(TObject *Sender);
    void __fastcall btnCloseClick(TObject *Sender);
    void __fastcall btnCmdClick(TObject *Sender);
    void __fastcall edtN1KeyPress(TObject *Sender, char &Key);
    void __fastcall edtCmdKeyPress(TObject *Sender, char &Key);
    void __fastcall btnIntClick(TObject *Sender);
    void __fastcall btnReClick(TObject *Sender);

private:	// User declarations
public:
    char * mCaptionInform;
    ULONG mIndex;		// User declarations
    __fastcall TForm1(TComponent* Owner);
    UCHAR	mCharToBcd(UCHAR);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
#define		CONST_CMD_LEN		0x07	// 命令块的长度
#define		DEMO_CH451_CMD			0x56	// PC发送命令给CH451,用于演示CH451的功能
#define		IDC_INT_PRESS		2222	// 按键按下事件信号
#define		IDC_INT_RELEASE		2223	// 按键释放事件信号

VOID	CALLBACK mInterruptEvent(PUCHAR);
//---------------------------------------------------------------------------
#endif
