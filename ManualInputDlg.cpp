// ManualInputDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NC mashine.h"
#include ".\manualinputdlg.h"

//#include "FInterpreter.h"

extern CNCmashineView *pLog;

// fManualInputDlg dialog

IMPLEMENT_DYNAMIC(fManualInputDlg, CDialog)
fManualInputDlg::fManualInputDlg(CWnd* pParent /*=NULL*/)
	: CDialog(fManualInputDlg::IDD, pParent)
	, ManualProg(_T(""))
{
	Interpreter = NULL;
}

fManualInputDlg::~fManualInputDlg()
{
}

void fManualInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PROG, ManualProg);
	DDX_Control(pDX, IDC_EDIT_PROG, edManualProg);
}


BEGIN_MESSAGE_MAP(fManualInputDlg, CDialog)
	ON_BN_CLICKED(ID_START, OnBnClickedStart)
	ON_COMMAND(ID_START, OnBnClickedStart)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// fManualInputDlg message handlers

void fManualInputDlg::OnBnClickedStart()
{
	// TODO: Add your control notification handler code here
	UpdateData();

	if (Interpreter)
	{
		Interpreter->SetManualProg(ManualProg);
		Interpreter->Start();
	}
	else
	{
		Interpreter = new FInterpreter;
		Interpreter->SetpLog(pLog);
		Interpreter->SetCurrentMode(FInterpreter::_Mode::Manual);
		Interpreter->SetManualProg(ManualProg);
		Interpreter->Start();
	}
	//AfxMessageBox(ManualProg);
}

BOOL fManualInputDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	// Загружаем таблицу акселераторов.
	m_hAccel = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR_DLG));
	ASSERT(m_hAccel);
	edManualProg.SetFocus();
	//return TRUE;  // return TRUE unless you set the focus to a control
	return FALSE;
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL fManualInputDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if(TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
		return TRUE;

	return CDialog::PreTranslateMessage(pMsg);
}

void fManualInputDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: Add your message handler code here
	DestroyAcceleratorTable(m_hAccel);
	if (Interpreter)
	{
		Interpreter->CleanUp();
		Interpreter = NULL;
		//delete Interpreter;
	}
}
