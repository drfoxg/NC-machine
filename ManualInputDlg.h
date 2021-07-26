#pragma once
#include "afxwin.h"
#include "FInterpreter.h"

// fManualInputDlg dialog

class fManualInputDlg : public CDialog
{
	DECLARE_DYNAMIC(fManualInputDlg)

public:
	fManualInputDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~fManualInputDlg();

// Dialog Data
	enum { IDD = IDD_MANUAL };
public:
	CString ManualProg;
	CEdit edManualProg;
	virtual BOOL OnInitDialog();
	
// Implementation
protected:
    HACCEL m_hAccel;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedStart();
	DECLARE_MESSAGE_MAP()
private:
	FInterpreter *Interpreter;
};
