#pragma once
#include "afxwin.h"

#include "FInterpreter.h"

// fModeDlg dialog

class fModeDlg : public CDialog
{
	DECLARE_DYNAMIC(fModeDlg)

public:
	fModeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~fModeDlg();

// Dialog Data
	enum { IDD = IDD_MODE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadioAuto();
	afx_msg void OnBnClickedRadioSemiauto();
	afx_msg void OnBnClickedRadioManual();
public:
	FInterpreter::_Mode dlgMode;
	bool IsM01;
	bool IsFrameSkip;
protected:
	CButton rbtnAuto;
	CButton rbtnSemiauto;
	CButton rbtnManual;
	virtual void OnCancel();
public:
	CButton chkFrameSkip;
	afx_msg void OnBnClickedCheckFrameSkip();
};
