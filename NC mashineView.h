// NC mashineView.h : interface of the CNCmashineView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_NCMASHINEVIEW_H__606C4013_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_)
#define AFX_NCMASHINEVIEW_H__606C4013_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CNCmashineView : public CListView
{
protected: // create from serialization only
		CNCmashineView();
	DECLARE_DYNCREATE(CNCmashineView)

// Attributes
public:
		CNCmashineDoc* GetDocument();

	// с параметрами по умолчанию строка добавляется в CCtrlList со стилем Report
	// при AddString = false выполянется объединение последней строки списка и Msg
	void PrnLog(CString Msg, bool AddString = true);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNCmashineView)
	public:
		virtual void OnDraw(CDC* pDC);  // overridden to draw this view
		virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
		virtual void OnInitialUpdate(); // called first time after construct
		virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
		virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
		virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
		virtual ~CNCmashineView();
#ifdef _DEBUG
		virtual void AssertValid() const;
		virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CNCmashineView)
		afx_msg void OnClearLog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in NC mashineView.cpp
inline CNCmashineDoc* CNCmashineView::GetDocument()
   { return (CNCmashineDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NCMASHINEVIEW_H__606C4013_97B6_11DA_9057_0050BF4A4CEB__INCLUDED_)
