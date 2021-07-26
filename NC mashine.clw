; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CNCmashineDoc
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "NC mashine.h"
ODLFile=NC mashine.odl
LastPage=0

ClassCount=5
Class1=CNCmashineApp
Class2=CNCmashineDoc
Class3=CNCmashineView
Class4=CMainFrame

ResourceCount=2
Resource1=IDR_MAINFRAME
Class5=CAboutDlg
Resource2=IDD_ABOUTBOX

[CLS:CNCmashineApp]
Type=0
HeaderFile=NC mashine.h
ImplementationFile=NC mashine.cpp
Filter=N
LastObject=CNCmashineApp
BaseClass=CWinApp

[CLS:CNCmashineDoc]
Type=0
HeaderFile=NC mashineDoc.h
ImplementationFile=NC mashineDoc.cpp
Filter=N
BaseClass=CDocument
VirtualFilter=DC
LastObject=ID_FILE_OPEN

[CLS:CNCmashineView]
Type=0
HeaderFile=NC mashineView.h
ImplementationFile=NC mashineView.cpp
Filter=C
BaseClass=CListView
VirtualFilter=VWC
LastObject=CNCmashineView


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=CMainFrame
BaseClass=CFrameWnd
VirtualFilter=fWC




[CLS:CAboutDlg]
Type=0
HeaderFile=NC mashine.cpp
ImplementationFile=NC mashine.cpp
Filter=D
BaseClass=CDialog

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_SAVE_AS
Command5=ID_FILE_PRINT
Command6=ID_FILE_PRINT_PREVIEW
Command7=ID_FILE_PRINT_SETUP
Command8=ID_FILE_MRU_FILE1
Command9=ID_APP_EXIT
Command10=ID_EDIT_UNDO
Command11=ID_EDIT_CUT
Command12=ID_EDIT_COPY
Command13=ID_EDIT_PASTE
Command14=ID_VIEW_TOOLBAR
Command15=ID_VIEW_STATUS_BAR
Command16=ID_RUN_START
Command17=ID_APP_ABOUT
CommandCount=17

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_CLEAR_LOG
Command2=ID_EDIT_COPY
Command3=ID_FILE_NEW
Command4=ID_FILE_OPEN
Command5=ID_FILE_PRINT
Command6=ID_FILE_SAVE
Command7=ID_EDIT_PASTE
Command8=ID_EDIT_UNDO
Command9=ID_EDIT_CUT
Command10=ID_RUN_START
Command11=ID_NEXT_PANE
Command12=ID_PREV_PANE
Command13=ID_EDIT_COPY
Command14=ID_EDIT_PASTE
Command15=ID_EDIT_CUT
Command16=ID_EDIT_UNDO
CommandCount=16

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
Command8=ID_APP_ABOUT
CommandCount=8

