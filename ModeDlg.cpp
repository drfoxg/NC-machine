// ModeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NC mashine.h"
#include "FInterpreter.h"
#include ".\modedlg.h"

// fModeDlg dialog

IMPLEMENT_DYNAMIC(fModeDlg, CDialog)
fModeDlg::fModeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(fModeDlg::IDD, pParent)
{
}

fModeDlg::~fModeDlg()
{
}

void fModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_AUTO, rbtnAuto);
	DDX_Control(pDX, IDC_RADIO_SEMIAUTO, rbtnSemiauto);
	DDX_Control(pDX, IDC_RADIO_MANUAL, rbtnManual);
	DDX_Control(pDX, IDC_CHECK_M01, chkFrameSkip);
}


BEGIN_MESSAGE_MAP(fModeDlg, CDialog)
	ON_BN_CLICKED(IDC_RADIO_AUTO, OnBnClickedRadioAuto)
	ON_BN_CLICKED(IDC_RADIO_SEMIAUTO, OnBnClickedRadioSemiauto)
	ON_BN_CLICKED(IDC_RADIO_MANUAL, OnBnClickedRadioManual)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_M01, OnBnClickedCheckFrameSkip)
END_MESSAGE_MAP()


// fModeDlg message handlers

void fModeDlg::OnBnClickedRadioAuto()
{
	dlgMode = FInterpreter::_Mode::Automatic;
}

void fModeDlg::OnBnClickedRadioSemiauto()
{
	dlgMode = FInterpreter::_Mode::Semiautomatic;
}

void fModeDlg::OnBnClickedRadioManual()
{
	dlgMode = FInterpreter::_Mode::Manual;
}

BOOL fModeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	switch (dlgMode)
	{
	case FInterpreter::_Mode::Automatic:
		rbtnAuto.SetCheck(BST_CHECKED);
		rbtnAuto.SetFocus();
		break;
	case FInterpreter::_Mode::Semiautomatic:
		rbtnSemiauto.SetCheck(BST_CHECKED);
		rbtnSemiauto.SetFocus();
		break;
	case FInterpreter::_Mode::Manual:
		rbtnManual.SetCheck(BST_CHECKED);
		rbtnManual.SetFocus();
		break;
	}

	if (IsM01)
		chkFrameSkip.SetCheck(BST_CHECKED);
	else
		chkFrameSkip.SetCheck(BST_UNCHECKED);

	return FALSE;
	//return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void fModeDlg::OnBnClickedOk()
{
	// Без этого кода творится что-то для меня не понятное, dlgMode где-то
	// меняется на Automatic, хотя радио-кнопки не трогались
	if (rbtnAuto.GetState() == BST_CHECKED)
		dlgMode = FInterpreter::_Mode::Automatic;
	if (rbtnSemiauto.GetState() == BST_CHECKED)
		dlgMode = FInterpreter::_Mode::Semiautomatic;
	if (rbtnManual.GetState() == BST_CHECKED)
		dlgMode = FInterpreter::_Mode::Manual;

	if (chkFrameSkip.GetState() == BST_CHECKED)
		IsM01 = true;
	else
		IsM01 = false;

	OnOK();
}

void fModeDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	if (this->dlgMode == FInterpreter::_Mode::Manual)
		AfxMessageBox(_T("Нельзя закрыть окно оставив режим \"Ручной\"."));
	else
		CDialog::OnCancel();
}

void fModeDlg::OnBnClickedCheckFrameSkip()
{
	IsM01 = !IsM01;
}
