// Ch375TestDlg.h : header file
//

#if !defined(AFX_CH375TESTDLG_H__82DE3C2B_C050_42E1_922E_F997A6B363DD__INCLUDED_)
#define AFX_CH375TESTDLG_H__82DE3C2B_C050_42E1_922E_F997A6B363DD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CCh375TestDlg dialog
UINT mThreadTest(LPVOID pParam);

class CCh375TestDlg : public CDialog
{
// Construction
public:
	
	BOOL mOpenDevice();
	void StopTest();
	BOOL m_trdFlag;
	BOOL m_OpenFlag;
	unsigned char mReadBuf[4100];
	unsigned char mBuffer[4100];
	CCh375TestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCh375TestDlg)
	enum { IDD = IDD_CH375TEST_DIALOG };
	CButton	m_test;
	CListBox	m_describe;
	long	m_err;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCh375TestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CCh375TestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonClear();
	afx_msg void OnCheckOpen();
	afx_msg void OnCheckTest();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CH375TESTDLG_H__82DE3C2B_C050_42E1_922E_F997A6B363DD__INCLUDED_)
