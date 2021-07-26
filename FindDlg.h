#pragma once
#include "afxwin.h"
#include "FInterpreter.h"

// fFindDlg dialog

class fFindDlg : public CDialog
{
	DECLARE_DYNAMIC(fFindDlg)

public:
	fFindDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~fFindDlg();

// Dialog Data
	enum { IDD = IDD_FIND };
	FInterpreter *Interpreter;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString TextToFind;
	afx_msg void OnBnClickedFind();
	// // Результат поиска: найден (отсутствует)
	CString Result;
	virtual BOOL OnInitDialog();
	CEdit edFind;
};
