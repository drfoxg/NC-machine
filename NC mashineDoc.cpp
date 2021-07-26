// NC mashineDoc.cpp : implementation of the CNCmashineDoc class
//

#include "stdafx.h"
#include "NC mashine.h"

#include "NC mashineDoc.h"
#include "NC mashineView.h"
#include "FInterpreter.h"
#include ".\nc mashinedoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CNCmashineView *pLog;

/////////////////////////////////////////////////////////////////////////////
// CNCmashineDoc

IMPLEMENT_DYNCREATE(CNCmashineDoc, CDocument)

BEGIN_MESSAGE_MAP(CNCmashineDoc, CDocument)
	//{{AFX_MSG_MAP(CNCmashineDoc)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_RUN_START, OnRunStart)
	ON_UPDATE_COMMAND_UI(ID_RUN_START, OnUpdateRunStart)
	ON_COMMAND(ID_RUN_MODE, OnRunMode)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_RUN_FIND, OnRunFind)
	ON_UPDATE_COMMAND_UI(ID_RUN_FIND, OnUpdateRunFind)
	ON_COMMAND(ID_RUN_STOP, OnRunStop)
	ON_UPDATE_COMMAND_UI(ID_RUN_STOP, OnUpdateRunStop)
	ON_UPDATE_COMMAND_UI(ID_RUN_MODE, OnUpdateRunMode)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CNCmashineDoc, CDocument)
	//{{AFX_DISPATCH_MAP(CNCmashineDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//      DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_INCmashine to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {606C4007-97B6-11DA-9057-0050BF4A4CEB}
static const IID IID_INCmashine =
{ 0x606c4007, 0x97b6, 0x11da, { 0x90, 0x57, 0x0, 0x50, 0xbf, 0x4a, 0x4c, 0xeb } };

BEGIN_INTERFACE_MAP(CNCmashineDoc, CDocument)
	INTERFACE_PART(CNCmashineDoc, IID_INCmashine, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCmashineDoc construction/destruction

CNCmashineDoc::CNCmashineDoc()
{
	EnableAutomation();
	AfxOleLockApp();

	FileName[0] = (TCHAR)NULL;
	Interpreter = NULL;
	docMode = FInterpreter::_Mode::Automatic;
	IsM01 = false;
	IsFrameSkip = false;
}

CNCmashineDoc::~CNCmashineDoc()
{
	AfxOleUnlockApp();
	if (Interpreter)
		Interpreter->CleanUp();
}

BOOL CNCmashineDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	CView *p_object;
	// обеспечить доступ к логу глобально
	POSITION pos = GetFirstViewPosition();
	while (pos != 0)
	{
		p_object = GetNextView(pos);
		if (p_object->IsKindOf(RUNTIME_CLASS(CNCmashineView)))
		{
			pLog = (CNCmashineView *)p_object;
			break;
		}
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNCmashineDoc serialization

void CNCmashineDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCmashineDoc diagnostics

#ifdef _DEBUG
void CNCmashineDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNCmashineDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCmashineDoc commands

void CNCmashineDoc::OnFileOpen() 
{
	if (Interpreter)
	{
		Interpreter->CleanUp();
		//delete Interpreter;
		Interpreter = NULL;
	}
	// TODO: Add your command handler code here
	CString tmp_msg;
	TCHAR tmp_path[_MAX_PATH];
	// открыть файл
	CFileDialog fd(true);
	fd.m_ofn.lStructSize = sizeof(OPENFILENAME);
	tmp_msg.LoadString(IDS_OPEN);
	fd.m_ofn.lpstrTitle = tmp_msg;
	strcpy(tmp_path, FileName);
	fd.m_ofn.lpstrFile = FileName;
	fd.m_ofn.nMaxFile = _MAX_PATH;
	FileName[0] = (TCHAR)NULL;
	fd.m_ofn.lpstrFilter = "Программы станка (*.ncm)\0*.ncm\0\0";
	fd.m_ofn.nFilterIndex = 1;
	if (fd.DoModal() == IDOK)
		SetTitle(FileName);	
	else
		strcpy(FileName, tmp_path);
}

void CNCmashineDoc::OnRunStart()
{
	// TODO: Add your command handler code here
	CString temp_str;

	temp_str.Format(_T("%s"), FileName);
	if (!Interpreter)
	{
		Interpreter = new FInterpreter(temp_str);
		Interpreter->SetpLog(pLog);
		Interpreter->SetCurrentMode(docMode);
		Interpreter->Start();
	}
	else
		// повторное выполнение одной и тойже программы
		Interpreter->Start();
} // OnRunStart()

void CNCmashineDoc::OnUpdateRunStart(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (FileName[0] != (TCHAR)NULL)
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(false);	
}
void CNCmashineDoc::OnCloseDocument()
{
	if (Interpreter)
	{
		Interpreter->CleanUp();
		Interpreter = NULL;
	}

	CDocument::OnCloseDocument();
}

void CNCmashineDoc::OnRunMode()
{
	ModeDlg.dlgMode = docMode;
	ModeDlg.IsM01 = IsM01;

	if (ModeDlg.DoModal() == IDOK)
	{
		docMode = ModeDlg.dlgMode;
		IsM01 = ModeDlg.IsM01;
		if (ModeDlg.dlgMode == FInterpreter::_Mode::Manual)
		{
			FileName[0] = (TCHAR)NULL;
			SetTitle(_T("БезНазвания"));	
			if (Interpreter)
			{
				Interpreter->CleanUp();
				Interpreter = NULL;
			}
			ManualInputDlg.DoModal();
			// Рекурсия
			OnRunMode();
		}
	}

	if(Interpreter)
	{
		Interpreter->SetCurrentMode(docMode);
		Interpreter->SetM01(IsM01);
	}
} // OnRunMode()

void CNCmashineDoc::OnRunFind()
{
	// TODO: Add your command handler code here
	CString temp_str;

	temp_str.Format(_T("%s"), FileName);
	if (!Interpreter)
	{
		Interpreter = new FInterpreter(temp_str);
		Interpreter->SetpLog(pLog);
		Interpreter->SetCurrentMode(docMode);
		FindDlg.Interpreter = Interpreter;
	}
	else
	{
		FindDlg.Interpreter = Interpreter;
		Interpreter->RestartFind();
	}
	FindDlg.DoModal();
}

void CNCmashineDoc::OnUpdateRunFind(CCmdUI *pCmdUI)
{
	if (FileName[0] != (TCHAR)NULL)
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(false);
}

void CNCmashineDoc::OnRunStop()
{
	if (Interpreter)
		Interpreter->Stop();
}

void CNCmashineDoc::OnUpdateRunStop(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (Interpreter)
		if (Interpreter->GetIsRun())
			pCmdUI->Enable(true);
		else
			pCmdUI->Enable(false);
	else
		pCmdUI->Enable(false);
}

void CNCmashineDoc::OnUpdateRunMode(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (Interpreter)
		// Переключение между режимами можно только при остановленной работе программы
		if (Interpreter->GetIsRun())
			pCmdUI->Enable(false);
		else
			pCmdUI->Enable(true);
	else
		pCmdUI->Enable(true);
}
