#if defined (_MSC_VER) && (_MSC_VER >= 1000)
#pragma once
#endif
#ifndef _INC_FINTERPRETER_43E83C3102DA_INCLUDED
#define _INC_FINTERPRETER_43E83C3102DA_INCLUDED

#include <stack>
#include <hash_map>
#include <string>
#include <ctype.h>

using namespace std;
using namespace stdext;

///////////////////////////////////////////////////////////////////////////////
// ���������������� ���������

const FM_START = WM_APP + 100;				// ����������

class CNCmashineView;

//���������� ��������� ����������� � ������������� ������ ��� 
//������ ���������
class FInterpreter 
{
public:
	// ������ � ��������� ������� "���������� ����"
	static const int CALL_ERROR = 1000000000;
	static const int E_TIMER = 1;

	enum _Mode 
	{
		Automatic,
		Semiautomatic,
		Manual
	};
public:
	FInterpreter();
	FInterpreter(CString Path);
	virtual ~FInterpreter();
	// �������� ��������� �� ���������� ����������
	void Start()
	{
		CString temp_msg;
		if (!ProgBuf)
		{		
			temp_msg.LoadString(IDS_ERROR_PROGRAMM_NOT_IN_MEMORY);
			AfxMessageBox(temp_msg);
		}
		else
			::PostThreadMessage(pInterpreterThread->m_nThreadID,
								FM_START,
								0, 0);	
	}
	// ���������� ���������� (� ��������� ������)
	long tStart();
	const CNCmashineView* GetpLog() const;
	void SetpLog(CNCmashineView* value);
	void Stop(void );
	const _Mode GetCurrentMode() const {return CurrentMode;}
	void SetCurrentMode(_Mode value)
	{
		if (!IsRun)
			CurrentMode = value;
	}
	// �������� ������ ����� ��������� ������, ��������� ����������������� ������� ����
	long SetFileName(CString Path);
	void SetManualProg(CString ManualProg)
	{
		int len = ManualProg.GetLength();
		if (ProgBuf)
			delete [] ProgBuf;
		ProgBuf = new char[len + 3];
		strcpy(ProgBuf, ManualProg.GetBuffer(len));
		ManualProg.ReleaseBuffer();
		ProgBuf[len] = '\r';
		ProgBuf[len + 1] = '\n';
		ProgBuf[len + 2] = 0;
		ToUpper();
	}
	// ����� ������ � ��������� � ��������� ��������� �� ��� ����� ���� �������
	bool Find(CString TextToFind);
	// ����� ������ � ��������� c ������ (ProgBufTemp = ProgBuf)
	void RestartFind()
	{
		ProgBufTemp = ProgBuf;
	}
	// ����������� ������� � ����� ����������� (�� ���������)
	void CleanUp(bool del = true);
	const bool GetIsStop() {return IsStop;}
	void SetIsStop(bool stop = true) {IsStop = stop;}
	const bool GetIsRun() {return IsRun;}
	void SetIsRun(bool run = true) {IsRun = run;}
	const bool GetFrameSkip() {return IsFrameSkip;}
	void SetFrameSkip(bool frame_skip = true) {IsFrameSkip = frame_skip;}
	const bool GetM01() {return IsM01;}
	void SetM01(bool m_01 = true) {IsM01 = m_01;}

private:
	// ����� ���������
	char* ProgBuf;
	// ��������� ��� ������ ���������
	char *ProgBufTemp;
	// C�������� ��������� �� ����� ����
	CString Message;
	// ���� ��������� � ������ �����
	CString FileName;
	CNCmashineView* pLog;
	enum _State
	{
		Initial,
		ProgStart1,
		ProgStart2,
		EndFrame,
		EndInfo,		// ����������� ������� �30
		Error,
		ProgNumber,
		LoopNumber,
		SubProgNumber,	// P
		SubProgEnd,		// M99
		SubProg,		// Pxx
		FrameNumber,	// Nxxxx
		FrameSkip,		// '/'
		DimWordX,
		DimWordY,
		DimWordZ,
		DimWordA,
		DimWordB,
		DimWordC,
		DimWordI,
		DimWordJ,
		DimWordK,
		AuxFunction,
		AuxFunctionM00,
		AuxFunctionM01,
		AuxFunctionM02,
		AuxFunctionM05,
		AuxFunctionM30,
		AuxFunctionM99,
		G,
		G_00,
		G_01,
		G_02,
		G_03,
		G_04,
		G_09,
		G_10,
		G_17,
		G_18,
		G_19,
		G_27,
		G_28,
		G_29,
		G_30,
		G_31,		
		G_40,
		G_41,
		G_42,
		G_43,
		G_44,
		G_49,
		G_45,
		G_46,
		G_47,
		G_48,
		G_53,
		G_54,
		G_55,
		G_56,
		G_57,
		G_58,
		G_59,
		G_80,
		G_81,
		G_82,
		G_83,
		G_84,
		G_85,
		G_86,
		G_87,
		G_88,
		G_89,
		G_90,
		G_91,
		G_92,
		G_94,
		G_60,
		G_37,
		G_36,
		Macro,			// #
		MainFrame,
		Print,
		ReturnCarriage,	// '\r'
		Presenting,		// ������
		Spindel,		// ��������
		Pause,
		Tool,			// ����������
		OffsetH,		// �������� ��� ��������� �� �����
		OffsetD			// �������� ��� ��������� �� �������
	};

	struct _TableData
	{
		//_TableData(){};
		//_TableData(const _TableData &table_data);
		//~_TableData(){};
		//_TableData operator=(_TableData &table_data);

		double (FInterpreter::* MyProc)(void);		
		double Value;
		// ���� ����������, ������������ � ������������
		stack<double> VarStack;
		_State State;
	};
	// ������� ������������ ������
	struct _Command
	{
		_State ID;
		double Data;
	};
	typedef double (FInterpreter::* MyProc)(void);
	typedef pair <string, _TableData> Pair;
	hash_map<string, _TableData> NameTable;
	hash_map<string, _TableData>::iterator NameTableIter;
	// ��������� ������ ������� "���������� ����"
	int CallStatus;
	// ��������� �� ������ ����������
	CString ErrorMsg;
	// ���� ���������
	bool IsStop; 
	// ���� - ���� ����������
	bool IsRun;
	// ���� ��������������� ���������
	bool IsM01;
	// ���� �������� �����
	bool IsFrameSkip;
private:
	_Mode CurrentMode;
	_State State;
	_Command Command;
	CWinThread *pInterpreterThread;
private:	
	void InitNameTable(void);	// ������������� ������� ����
	void ToUpper();				// �������������� ��������� (���. ��������) � ����. ��������
	//
	// Aux function - ��������������� �������
	//
	double afM00(void);	// M00
	double afM01(void);	// M01
	double afM02(void);	// M02
	double afM05(void);	// M05
	double afM30(void);	// M30
	double afM99(void);	// M99
	//
	// Dim words - ��������� �����
	//
	double dwX(void);	// X
	double dwY(void);	// Y
	double dwZ(void);	// Z
	double dwA(void);	// A
	double dwB(void);	// B
	double dwC(void);	// C
	double dwI(void);	// I
	double dwJ(void);	// J
	double dwK(void);	// K
	// ������� �����
	double fFrameSkip(void);
	// ����� ���������� ��������� �� ����� ����
	double PrintMsg(void);
	// �������� ���������� �����
	double GetDimValue(void);
	// �������� ������, 2-�� ������� ������
	int FInterpreter::GetDimValue2(void);
	// �������� ������� � ���������� �������������� �� ���������  
	_State GetNextState(string Lexeme, int StrId);
	// ������� �������� ������
	double fPresenting(void);
	// �������� �������� ��������
	double fSpindel(void);
	// ����� � ���������� ���������
	double fPause(void);
	// ����� �����������
	double fTool(void);
	// �������� ��� ��������� �� �����
	double fOffsetH(void);
	// �������� ��� ��������� �� �������
	double fOffsetD(void);
	//
	// G-�������
	//
	// ������ 01
	double G00(); // ���������������� (���������� �����������)
	double G01(); // �������� ������������ (������� ������)
	double G02(); // �������� ������������ �� ������� �������
	double G03(); // �������� ������������ ������ ������� �������
	// ������ 00
	double G04(); // �����
	double G09(); // ���������� � ����� �����
	double G10(); // �������-�������� ������������
	// ������ 02
	double G17(); // ������� ��������� XY
	double G18(); // ������� ��������� ZX
	double G19(); // ������� ��������� YZ
	// ������ 00
	double G27(); // ���� � "����" ��������� ������ �� ������� ������������� ����
	double G28(); // ������� � "����" ��������� ������ ����� ������������� �����
	double G29(); // ����� �� ���� � �������� ����� ����� ������������� �����
	double G30(); // ����� � �������� �����
	double G31(); // ����� ������������� �����
	// ������ 03
	double G40(); // ������ ��������� ����������� �� �������
	double G41(); // ��������� ����������� �� ������� �����
	double G42(); // ��������� ����������� �� ������� ������
	// ������ 04
	double G43(); // ��������� ����� ����������� "+"
	double G44(); // ��������� ����� ����������� "-"
	double G49(); // ������ ��������� ����� �����������
	// ������ 00
	double G45(); // �������� ����������� � ����������� �����������
	double G46(); // �������� ����������� � ����������� ����������
	double G47(); // ������� �������� ����������� � ����������� �����������
	double G48(); // ������� �������� ����������� � ����������� ����������
	// ������ 05
	double G53(); // ������� � ���������� ������� ���������
	double G54(); // ����� ������������ ������� ��������� 1
	double G55(); // ����� ������������ ������� ��������� 2
	double G56(); // ����� ������������ ������� ��������� 3
	double G57(); // ����� ������������ ������� ��������� 4
	double G58(); // ����� ������������ ������� ��������� 5
	double G59(); // ����� ������������ ������� ��������� 6
	// ������ 06
	double G80(); // ������ ����������� ����� 
	double G81(); // ���������, �����������, ������� �����
	double G82(); // ���������, ��������, ������� �����
	double G83(); // �������� ���������, ������� �����
	double G84(); // ��������� ������ ��������, ����� �� ������� ������
	double G85(); // ������������, �������������, ����� �� ������� ������
	double G86(); // ������������, ������� ������, ������� �����
	double G87(); // ������������, ������� ������, ����� � �������
	double G88(); // ������������, ����� � �������
	double G89(); // ������������, �������������, ����� �� ������� ������
	// ������ 07
	double G90(); // ������� � ���������� ���������
	double G91(); // ������� � �����������
	// ������ 00
	double G92(); // ������� ������� ���������
	// ������ 08
	double G94(); // ������� ������ � ��/���
	// ������ 00
	double G60(); // ������������� ����������������
	// ������ 09
	double G37(); // ������� ��������� ������
	double G36(); // ������ ��������� ������
};

// ������� ����� �����������
UINT InterpreterThread(LPVOID data);
// ������� ������� E_TIMER
void CALLBACK TimerProc(HWND hWnd,		// handle of CWnd that called SetTimer
					    UINT nMsg,		// WM_TIMER
						UINT nIDEvent,	// timer identification
						DWORD dwTime);	// system time
							  

#endif /* _INC_FINTERPRETER_43E83C3102DA_INCLUDED */

//	����� ����� FInterpreter.h
///////////////////////////////////////////////////////////////////////////////