// Ch375TestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ch375Test.h"
#include "Ch375TestDlg.h"
#include "ch375dll.h"
#include "stdio.h"
#include "conio.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define		TEST_START			0x20	// 测试过程开始
#define		TEST_DATA			0x21	// 测试数据正确性
#define		TEST_UPLOAD			0x22	// 测试上传数据块
#define		TEST_DOWNLOAD		0x23	// 测试下传数据块

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCh375TestDlg dialog

CCh375TestDlg::CCh375TestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCh375TestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCh375TestDlg)
	m_err = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCh375TestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCh375TestDlg)
	DDX_Control(pDX, IDC_CHECK_TEST, m_test);
	DDX_Control(pDX, IDC_LIST_DES, m_describe);
	DDX_Text(pDX, IDC_EDIT_ERR, m_err);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCh375TestDlg, CDialog)
	//{{AFX_MSG_MAP(CCh375TestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnButtonClear)
//	ON_BN_CLICKED(IDC_CHECK_OPEN, OnCheckOpen)
	ON_BN_CLICKED(IDC_CHECK_TEST, OnCheckTest)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCh375TestDlg message handlers

BOOL CCh375TestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	m_err = 0;
	m_trdFlag = FALSE;
	m_OpenFlag = FALSE;

	m_test.SetCheck(0);
	mOpenDevice();
	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCh375TestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCh375TestDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCh375TestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CCh375TestDlg::OnButtonClear() 
{
	m_describe.ResetContent();
}

void CCh375TestDlg::OnCheckOpen() 
{

}

//测试。。。
void CCh375TestDlg::OnCheckTest()   
{
	// TODO: Add your control notification handler code here

	CWinThread * mThread = NULL;
	m_trdFlag = TRUE;
	if(m_test.GetCheck())		//开始测试
	{
		if(!m_OpenFlag)
		{
			Sleep(100);
			CH375CloseDevice( 0 );
			if(!mOpenDevice())
			{
				m_describe.InsertString(-1,_T("设备未能成功打开！"));
				return;
			}
		}
		m_test.EnableWindow(FALSE);
		mThread = AfxBeginThread(mThreadTest,this,THREAD_PRIORITY_NORMAL,0,0,NULL); 
		if(mThread)
		{
			if(m_test.GetCheck())
			{
			}
			else
			{
				m_test.SetCheck(0);
			}
		}
		else
		{
			m_trdFlag = FALSE;
			m_test.SetCheck(0);
			MessageBox("Thread failed!","CH375TEST",MB_OK|MB_ICONSTOP);
		}
		UpdateData(FALSE);
		
	}
	else					//测试中断
	{
		int x=100;
		m_trdFlag = FALSE;
		m_test.SetCheck(0);
		m_describe.InsertString(-1,_T("the Test is terminated"));
		UpdateData(FALSE);
		while(x)
			x--;
	}

	UpdateData(FALSE);
}

UINT mThreadTest(LPVOID pParam)
{
	unsigned long	i, mLength, mTestCount, mErrCnt, mStep, mTotal;
	char strtemp[2000];
	double speed;

	CCh375TestDlg *pDlg = (CCh375TestDlg *)pParam;

	if(pDlg->m_trdFlag)
	{
		mErrCnt=0;
		pDlg->m_err = 0;
		pDlg->m_describe.InsertString(-1,_T("*** Step-0: notice MCU start test"));

		pDlg->mBuffer[0]=TEST_START;
		mTotal=1;
		if ( CH375WriteData( 0, &pDlg->mBuffer, &mTotal ) )  // 发送成功
		{
			pDlg->m_describe.InsertString(-1,_T("Start test now..."));
		}
		else         // 写操作失败
		{ 
			mErrCnt++;
			pDlg->m_err++;
			pDlg->m_describe.InsertString(-1,_T( "S0-T0 CH375WriteData return error, length=1" ));
		}

		Sleep(200);

		pDlg->mBuffer[0]=TEST_START;
		mTotal=1;
		if ( CH375WriteData( 0, &pDlg->mBuffer, &mTotal ) )  // 发送成功
		{
		}
		else       // 写操作失败
		{  
			mErrCnt++;
			pDlg->m_err++;
			pDlg->m_describe.InsertString(-1,_T( "S0-T1 CH375WriteData return error, length=1" ));
		}

		Sleep(200);
		
		//******************************
		//correctness test
		pDlg->m_describe.InsertString(-1,_T( "*** Step-1: test data correctness: 2000 times, random length and data" ));

		for ( mTestCount=0; mTestCount<2000; ++mTestCount )  // 循环测试
		{
			mStep=mTestCount&0x03;

			switch( mStep )
			{
				case 0: memset( pDlg->mBuffer, 0x00, 64 );  // 发送全00H/返回全FFH
						break;
				case 1: memset( pDlg->mBuffer, 0xff, 64 );  // 发送全FFH/返回全00H
						break;
				default: for ( i=0; i<64; i+=2 ) *(unsigned int *)(&(pDlg->mBuffer[i]))=rand();  // 发送随机数/返回位反码
						break;
			}

			pDlg->mBuffer[0]=TEST_DATA;
			mTotal=rand();  // 随机数长度
			mTotal=mTotal%100;

			if ( mTotal == 0 || mTotal > 64 ) 
				mTotal=(mTotal & 0x01)?64:63;

			sprintf(strtemp,"Cng=%4ld,Len=%2d,Data:%02x,%02x,%02x,%02x...\xd",mTestCount,mTotal,pDlg->mBuffer[0],pDlg->mBuffer[1],pDlg->mBuffer[2],pDlg->mBuffer[3]);
			pDlg->m_describe.InsertString(-1,_T(strtemp));
	
			if ( CH375WriteData( 0, &pDlg->mBuffer, &mTotal ) )  // 发送成功
			{
				mLength = 64;
				if ( CH375ReadData( 0, &pDlg->mReadBuf, &mLength ) )  // 接收成功
				{
					if ( mLength != mTotal || mLength==0 ) 
					{
						mErrCnt++;
						pDlg->m_err++;

						sprintf(strtemp,"S1-T%0ld-C%ld return length error: %ld (%ld)\x0",mStep,mTestCount,mLength,mTotal);
						pDlg->m_describe.InsertString(-1,_T(strtemp));
					}
					else 
					{
						for ( i=0; i<mLength; ++i ) 
						{
							if ( (pDlg->mReadBuf[i]^0xff)!=pDlg->mBuffer[i] ) 
							{
								mErrCnt++;
								pDlg->m_err++;
								sprintf(strtemp,"S1-T%0ld-C%ld return error at %ld: %2XH (%02XH)\x0",mStep,mTestCount,i,pDlg->mReadBuf[i],pDlg->mBuffer[i]);
								pDlg->m_describe.InsertString(-1,_T(strtemp));
							}
						}
					}
				}
				else         // 读操作失败
				{
					mErrCnt++;
					pDlg->m_err++;
				
					sprintf(strtemp,"S1-T%0ld-C%ld CH375ReadData return error,length=%ld",mStep,mTestCount,mTotal);
					pDlg->m_describe.InsertString(-1,_T(strtemp));
				}
			}
			else 
			{  // 写操作失败
				mErrCnt++;
				pDlg->m_err++;
				
				sprintf(strtemp,"S1-T%ld-C%ld CH375WriteData return error,length=%ld",mStep,mTestCount,mTotal);
				pDlg->m_describe.InsertString(-1,_T(strtemp));
			}
		}
		
		if ( mErrCnt==0 ) 
			pDlg->m_describe.InsertString(-1,"*** passed    " );
		Sleep(100);
		
		//**************************
		//下传速度测试
		pDlg->m_describe.InsertString(-1,_T( "*** Step-2: test speed of download data: 2048KB data " ));
		pDlg->m_describe.SetCurSel(pDlg->m_describe.GetCount() - 1);
		for ( i=0; i<4096; i+=2 ) 
			*(unsigned long *)(&pDlg->mBuffer[i])=rand();  // 产生随机数以发送
		
		for ( i=0; i<4096; i+=64 ) 
			pDlg->mBuffer[i]=TEST_DOWNLOAD;  // 每64字节为一个数据包,首字节为命令码
		
		mTotal=0;
		mStep=GetTickCount();
		
		for ( mTestCount=0; mTestCount<500; ++mTestCount )  // 循环测试
		{
			*(unsigned long *)(&pDlg->mBuffer[4])=mTestCount;
			mLength = 4096;
			
			if ( CH375WriteData( 0, &pDlg->mBuffer, &mLength ) )  // 发送成功
			{
				if ( mLength != 4096 ) 
				{
					sprintf(strtemp,"S2-C%ld CH375WriteData actual length short %ld (4096)",mLength);
					pDlg->m_describe.InsertString(-1,strtemp);
				}
				mTotal +=mLength;  // 累计长度
			}
			else 
			{  // 写操作失败
				mErrCnt++;
				pDlg->m_err++;
				sprintf(strtemp,"S2-C%ld CH375WriteData return error",mTestCount);
				pDlg->m_describe.InsertString(-1,_T(strtemp));
			}
		}

		mLength=GetTickCount();
		mLength=mLength-mStep;
		speed=1000;
		
		if ( mLength !=0 ) 
			speed=speed*mTotal/mLength;
		else speed=9999999;
		
		sprintf(strtemp,"*** download speed = %7.1f Bytes/Sec,total=%ld bytes,time=%ld mS",speed,mTotal,mLength);
		pDlg->m_describe.InsertString(-1,_T(strtemp));
		pDlg->m_describe.SetCurSel(pDlg->m_describe.GetCount() - 1);
		
		Sleep(100);
		
		//***********************************
		//上传速度测试
		pDlg->m_describe.InsertString(-1,_T( "*** Step-3: test speed of upload data: 2048KB data\n" ));
		pDlg->m_describe.SetCurSel(pDlg->m_describe.GetCount() - 1);
		pDlg->mBuffer[0]=TEST_UPLOAD;
		mLength = 1;
		
		if ( CH375WriteData( 0, &pDlg->mBuffer, &mLength ) )  // 发送成功
		{
			mTotal=0;
			mStep=GetTickCount();
			for ( mTestCount=0; mTestCount<500; ++mTestCount )  // 循环测试
			{
				mLength = 4096;
				if ( CH375ReadData( 0, &pDlg->mBuffer, &mLength ) )  // 接收成功
				{
					if ( mLength != 4096 ) 
					{
						sprintf(strtemp,"S3-C%ld CH375ReadData actual length short %ld (4096)\x0",mLength);
						pDlg->m_describe.InsertString(-1,_T(strtemp));
					}
					mTotal +=mLength;  // 累计长度
				}
				else
				{  // 读操作失败
					mErrCnt++;
					pDlg->m_err++;
					memset(strtemp,0x00,sizeof(strtemp));
					pDlg->m_describe.InsertString(-1,_T(strtemp));	
					sprintf(strtemp,"S3-C%ld CH375ReadData return error",mTestCount);
				}
			}

			mLength=GetTickCount();
			mLength=mLength-mStep;
			speed=1000;

			if ( mLength !=0 ) 
				speed=speed*mTotal/mLength;
			else 
				speed=9999999;
			
			memset(strtemp,0x00,sizeof(strtemp));
			sprintf(strtemp,"*** upload speed = %7.1f bytes/Sec,total=%ld bytes,time=%ld mS",speed,mTotal,mLength);
			pDlg->m_describe.InsertString(-1,_T(strtemp));
			pDlg->m_describe.SetCurSel(pDlg->m_describe.GetCount() - 1);
		}
		else 
		{  // 写操作失败
			mErrCnt++;
			pDlg->m_err++;
			pDlg->m_describe.InsertString(-1,_T("S3 CH375WriteData return error"));
		}
		Sleep(100);
		
		//****************************************
		// 关闭CH375设备,如果不继续使用则必须关闭设备,就象读写完硬盘中的数据文件后要关闭一样
		memset(strtemp,0x00,sizeof(strtemp));
		sprintf(strtemp,"*** Total error = %ld",mErrCnt);
		
		pDlg->m_describe.InsertString(-1,_T(strtemp));
		pDlg->m_describe.InsertString(-1,"*** CH375CloseDevice: 0");
		
		CH375CloseDevice( 0 );
		pDlg->m_OpenFlag = FALSE;
		pDlg->m_describe.InsertString(-1,_T("Exit."));
		pDlg->m_describe.InsertString(-1,_T("设备已关闭"));
		pDlg->m_describe.SetCurSel(pDlg->m_describe.GetCount() - 1);
		pDlg->m_test.SetCheck(0);
		pDlg->m_test.EnableWindow(TRUE);
		
	}
	
	ExitThread(0);
	pDlg->UpdateData(FALSE);
	return 0;
}

BOOL CCh375TestDlg::mOpenDevice()
{
	if ( CH375OpenDevice( 0 ) == INVALID_HANDLE_VALUE )   /* 使用之前必须打开设备 */
	{
		m_describe.InsertString(-1,_T("设备打开失败，可能未安装动态链接库。"));
		m_OpenFlag = FALSE;
		return FALSE;
	}
	else
	{
		m_OpenFlag = TRUE;
		m_describe.InsertString(-1,_T("设备已打开。"));
		m_test.EnableWindow(TRUE);
		UpdateData(FALSE);
		return TRUE;
	}
	return FALSE;
}
