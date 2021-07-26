// NC mashineDoc.h : interface of the CNCmashineDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_NCMASHINEDOC_H__606C4011_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_)
#define AFX_NCMASHINEDOC_H__606C4011_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ModeDlg.h"
#include "ManualInputDlg.h"
#include "FindDlg.h"
#include "FInterpreter.h"

class FInterpreter;

class CNCmashineDoc : public CDocument
{
protected: // create from serialization only
		CNCmashineDoc();
	DECLARE_DYNCREATE(CNCmashineDoc)

// Attributes
public:
	TCHAR FileName[_MAX_PATH];
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNCmashineDoc)
	public:
		virtual BOOL OnNewDocument();
		virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	FInterpreter *Interpreter;
	fModeDlg ModeDlg;
	fManualInputDlg ManualInputDlg;
	fFindDlg FindDlg;
	virtual ~CNCmashineDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CNCmashineDoc)
		afx_msg void OnFileOpen();
		afx_msg void OnRunStart();
		afx_msg void OnRunMode();
		afx_msg void OnUpdateRunStart(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CNCmashineDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
public:
	virtual void OnCloseDocument();
private:
	FInterpreter::_Mode docMode;
	bool IsM01;
	bool IsFrameSkip;
public:
	afx_msg void OnRunFind();
	afx_msg void OnUpdateRunFind(CCmdUI *pCmdUI);
	afx_msg void OnRunStop();
	afx_msg void OnUpdateRunStop(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRunMode(CCmdUI *pCmdUI);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NCMASHINEDOC_H__606C4011_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_)
