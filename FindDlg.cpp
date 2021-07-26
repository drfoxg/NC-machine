// FindDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NC mashine.h"
#include ".\finddlg.h"


// fFindDlg dialog

IMPLEMENT_DYNAMIC(fFindDlg, CDialog)
fFindDlg::fFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(fFindDlg::IDD, pParent)
	, TextToFind(_T(""))
	, Result(_T("Результат поиска:"))
{
}

fFindDlg::~fFindDlg()
{
}

void fFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_FIND, TextToFind);
	DDX_Text(pDX, IDC_STATIC_RESULT, Result);
	DDX_Control(pDX, IDC_EDIT_FIND, edFind);
}


BEGIN_MESSAGE_MAP(fFindDlg, CDialog)
	ON_BN_CLICKED(ID_FIND, OnBnClickedFind)
END_MESSAGE_MAP()


// fFindDlg message handlers

void fFindDlg::OnBnClickedFind()
{
	// TODO: Add your control notification handler code here
	UpdateData();
	if (TextToFind != "")
	{
		if (Interpreter->Find(TextToFind))
		{
			Result = _T("Результат поиска: строка найдена");
			UpdateData(FALSE);
		}
		else
		{
			Result = _T("Результат поиска: строка отсутствует");
			UpdateData(FALSE);
		}
	}
}

BOOL fFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	Result = _T("Результат поиска:");
	UpdateData(FALSE);
	edFind.SetFocus();
	return FALSE;
	//return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
