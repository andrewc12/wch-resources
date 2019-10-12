// Ch375Test.h : main header file for the CH375TEST application
//

#if !defined(AFX_CH375TEST_H__4368B037_A173_41CF_92D5_D3D1698670E8__INCLUDED_)
#define AFX_CH375TEST_H__4368B037_A173_41CF_92D5_D3D1698670E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCh375TestApp:
// See Ch375Test.cpp for the implementation of this class
//

class CCh375TestApp : public CWinApp
{
public:
	CCh375TestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCh375TestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCh375TestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CH375TEST_H__4368B037_A173_41CF_92D5_D3D1698670E8__INCLUDED_)
