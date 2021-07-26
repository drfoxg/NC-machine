// NC mashine.h : main header file for the NC MASHINE application
//

#if !defined(AFX_NCMASHINE_H__606C400A_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_)
#define AFX_NCMASHINE_H__606C400A_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CNCmashineApp:
// See NC mashine.cpp for the implementation of this class
//

class CNCmashineApp : public CWinApp
{
public:
		CNCmashineApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNCmashineApp)
	public:
		virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
		COleTemplateServer m_server;
		// Server object for document creation
	//{{AFX_MSG(CNCmashineApp)
		afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NCMASHINE_H__606C400A_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_)
