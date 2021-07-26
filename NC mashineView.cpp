// NC mashineView.cpp : implementation of the CNCmashineView class
//

#include "stdafx.h"
#include "NC mashine.h"

#include "NC mashineDoc.h"
#include "NC mashineView.h"
#include "FInterpreter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCmashineView

IMPLEMENT_DYNCREATE(CNCmashineView, CListView)

BEGIN_MESSAGE_MAP(CNCmashineView, CListView)
	//{{AFX_MSG_MAP(CNCmashineView)
	ON_COMMAND(ID_CLEAR_LOG, OnClearLog)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CListView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCmashineView construction/destruction

CNCmashineView::CNCmashineView()
{
	// TODO: add construction code here

}

CNCmashineView::~CNCmashineView()
{
}

BOOL CNCmashineView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style |= LVS_REPORT | LVS_ALIGNLEFT | LVS_NOCOLUMNHEADER;

	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CNCmashineView drawing

void CNCmashineView::OnDraw(CDC* pDC)
{
	CNCmashineDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

void CNCmashineView::OnInitialUpdate()
{
	CListView::OnInitialUpdate();


	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().
	CListCtrl& theCtrl = GetListCtrl();

	theCtrl.DeleteColumn(0);

	LVCOLUMN lvc;
	lvc.mask = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 700; // не поместившиеся символы заменяются "..."
	lvc.cchTextMax = 255;
	lvc.pszText = _T("Протокол событий");
	lvc.iSubItem = 0;
	int new_index = theCtrl.InsertColumn(0, &lvc);

}

/////////////////////////////////////////////////////////////////////////////
// CNCmashineView printing

BOOL CNCmashineView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CNCmashineView::OnBeginPrinting(CDC *pDC /*pDC*/, CPrintInfo *pInfo /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CNCmashineView::OnEndPrinting(CDC *pDC /*pDC*/, CPrintInfo *pInfo /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CNCmashineView diagnostics

#ifdef _DEBUG
void CNCmashineView::AssertValid() const
{
	CListView::AssertValid();
}

void CNCmashineView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CNCmashineDoc* CNCmashineView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCmashineDoc)));
	return (CNCmashineDoc*)m_pDocument;
}
#endif //_DEBUG

///////////////////////////////////////////////////////////////////////////////
// с параметрами по умолчанию строка добавляется в CCtrlList со стилем Report
// при AddString = false выполянется объединение последней строки списка и Msg

void CNCmashineView::PrnLog(CString Msg, bool AddString)
{
	CListCtrl &ctrl = GetListCtrl();
	int item_pos;
	CString temp_msg;

	if (AddString)
	{
		ctrl.InsertItem(ctrl.GetItemCount(), Msg);
	}
	else
	{
		if (item_pos = ctrl.GetItemCount())
		{
			item_pos -= 1;
			temp_msg = ctrl.GetItemText(item_pos, 0) + Msg;
			ctrl.DeleteItem(item_pos);
			ctrl.InsertItem(item_pos, temp_msg);
		}
		else
			ctrl.InsertItem(item_pos, Msg);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCmashineView message handlers

void CNCmashineView::OnClearLog() 
{
	// TODO: Add your command handler code here
	CListCtrl &ctrl = GetListCtrl();
	ctrl.DeleteAllItems();	
}
