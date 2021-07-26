// Copyright (C) 1991 - 1999 Rational Software Corporation

#include "stdafx.h"

#include "NC mashine.h"
#include "NC mashineDoc.h"
#include "NC mashineView.h"
#include "Resource.h"

#include "FInterpreter.h"
#include "TraceLastError.h"

using namespace std;
using namespace stdext;

// ��� E
HANDLE evnETimer;

FInterpreter::~FInterpreter()
{
	// ToDo: Add your specialized code here and/or call the base class
	delete [] ProgBuf;
	if (evnETimer)
	{
		CloseHandle(evnETimer);
		evnETimer = NULL;
	}
}

FInterpreter::FInterpreter()
{
	CallStatus = 0;
	//State = Initial;
	ProgBuf = NULL;
	pInterpreterThread = NULL;
	IsStop = false;
	IsRun = false;
	IsFrameSkip = false;
	IsM01 = false;
	evnETimer = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	// ������ ������
	pInterpreterThread = AfxBeginThread(InterpreterThread, (LPVOID)this);
	pInterpreterThread->m_bAutoDelete = false;
	// ������� ���� ����������� ������ ������ ��� ��� �������� �������
	InitNameTable();

} // FInterpreter(CString ManualProg, char dummy)

FInterpreter::FInterpreter(CString Path)
{
	CallStatus = 0;
	State = Initial;
	CString temp_msg;
	string temp_str;
	ProgBuf = NULL;
	pInterpreterThread = NULL;
	IsStop = false;
	IsRun = false;
	IsFrameSkip = false;
	IsM01 = false;
	evnETimer = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	FileName = Path;
	// �������� ��������� �� ����������
	try
	{
		CFile file(
				   Path,
				   CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary			   
				  );
		int len = (int)file.GetLength();
		// ��������� ������ ��� ���������
		if (!(ProgBuf = new char[(UINT)len + 1]))
		{
			temp_msg.LoadString(IDS_NOT_ENOUGH_MEMORY);
			TRACE(temp_msg);
			TRACE(_T("\n"));
			AfxMessageBox(temp_msg);
			ExitProcess(1); // ����� ������
		}

		file.Read(ProgBuf, (UINT)len);
		file.Close();

		ProgBuf[len] = 0; // ����� ������
		// �������� ������� ����� ����� ����� M30 � �02
		temp_str.append(ProgBuf + (len - 3), 3);
		if (temp_str != _T("M02") || temp_str != _T("M30") )
		{
			char *t = NULL;
			t = new char[len + 3]; // ����� ����� + \0
			if (!t)
			{
				temp_msg.LoadString(IDS_NOT_ENOUGH_MEMORY);
				TRACE(temp_msg);
				TRACE(_T("\n"));
				AfxMessageBox(temp_msg);
				ExitProcess(1); // ����� ������
			}
			memcpy(t, ProgBuf, len);
			delete [] ProgBuf;
			ProgBuf = NULL;
			ProgBuf = new char[len + 3]; // ����� ����� + \0
			if (!ProgBuf)
			{
				temp_msg.LoadString(IDS_NOT_ENOUGH_MEMORY);
				TRACE(temp_msg);
				TRACE(_T("\n"));
				AfxMessageBox(temp_msg);
				ExitProcess(1); // ����� ������
			}
			memcpy(ProgBuf, t, len);
			delete [] t;
			ProgBuf[len] = '\r';
			ProgBuf[len + 1] = '\n';
			ProgBuf[len + 2] = 0;
		}
		// ������ ������
		pInterpreterThread = AfxBeginThread(InterpreterThread, (LPVOID)this);
		pInterpreterThread->m_bAutoDelete = false;
		ToUpper();
		// ������� ���� ����������� ������ ������ ��� ��� �������� �������
		InitNameTable();
	}
	catch(CFileException *f_exc)
	{
		char ErrorMsg[512];
		f_exc->GetErrorMessage(ErrorMsg, 512);
		f_exc->Delete();
		if (ProgBuf)
			delete [] ProgBuf; // ������ ��� ���������
		ProgBuf = NULL;
		AfxMessageBox(ErrorMsg);
	}
} // FInterpreter(CString Path)

///////////////////////////////////////////////////////////////////////////////
// FInterpreter::tStart - ���������� ���������� (� ��������� ������)

long FInterpreter::tStart()
{
	double value;
	double stack_value; // ��������� ���������� ��� ������ �� ������ �� ����� M99
	// ��������� ���������� ��� ������ �� ������ �� ����� L
	//double &l_stack_value = stack_value;
	NameTableIter = NameTable.find("L");
	NameTableIter->second.VarStack.push(0);
	double &l_stack_value = NameTableIter->second.VarStack.top();
	NameTableIter->second.VarStack.pop();

	int stack_size;
	CString temp_msg;
	if (ProgBuf)
		switch (CurrentMode)
		{
		case Automatic:
			//ProgBufTemp = ProgBuf;
			temp_msg.Format(_T("����� ��������(�) �� %s"), FileName);
			pLog->PrnLog(temp_msg);
			break;
		case Semiautomatic:
			//ProgBufTemp = ProgBuf;
			break;
		case Manual:
			ProgBufTemp = ProgBuf;
			State = EndFrame;
			break;
		}
	else
	{		
		temp_msg.LoadString(IDS_ERROR_PROGRAMM_NOT_IN_MEMORY);
		pLog->PrnLog(temp_msg);
		return 1; // ��������� ������ � ������������ FInterpreter
	}
	char c[2] = {0,0};
	int i, j;
	string temp_str;
	MyProc pMyProc;

	_TableData table_data ;
	table_data.Value = 0;
	table_data.MyProc = NULL;

	IsRun = true;

	while(*ProgBufTemp != 0) // ���� �� ����� �����
	{
		//c = toupper(*ProgBufTemp);
		switch (State)
		{
		case Initial:
			if (*ProgBufTemp == '%')
				State = ProgStart1;
			break; // case Initial
		case ProgStart1:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_PROG_START1);
			break; // case ProgStart1
		case ProgStart2:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_PROG_START2);
			break; // case ProgStart2
		case ReturnCarriage:
			State = EndFrame;
			break; // case ReturnCarriage
		case EndFrame:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_END_FRAME);
			if (IsStop)
			{
				IsStop = false;
				IsRun = false;
				temp_msg.LoadString(IDS_STOP);
				pLog->PrnLog(temp_msg);
				return 0;
			}
			switch (CurrentMode)
			{
			case Automatic:
				break;
			case Semiautomatic:
				temp_msg.LoadString(IDS_END_FRAME);
				pLog->PrnLog(temp_msg);
				ProgBufTemp++;
				IsRun = false;
				return 0;
			case Manual:
				break;
			}
			break; // case EndFrame
		case FrameNumber:
			if (isdigit(*ProgBufTemp) && isdigit(*(ProgBufTemp + 1)) && isdigit(*(ProgBufTemp + 2)) 
								      && isdigit(*(ProgBufTemp + 3)) && isdigit(*(ProgBufTemp + 4)))
			{
				ErrorMsg.LoadString(IDS_ERR_FRAME_NUMBER);
				State = Error;
			}
			else
			{
				while (isdigit(*ProgBufTemp))
				{
					ProgBufTemp++;
				}
				temp_str = *ProgBufTemp;
				State = GetNextState(temp_str, IDS_ERR_FRAME_NUMBER);
			}
			break; // case FrameNumber
		case SubProgNumber:
			temp_str.clear();
			if (isdigit(*ProgBufTemp) && isdigit(*(ProgBufTemp + 1)) && isdigit(*(ProgBufTemp + 2)))
			{
				ErrorMsg.LoadString(IDS_ERR_SUB_PROG_NUMBER);
				State = Error;		
			}
			else
			{
				temp_str.append(ProgBufTemp - 1, 3); // Pxx
				ProgBufTemp += 2; // ProgBufTemp == '\r' ��� ���������� ���������
				// ��� �������� ���������� ������������ - �������� �������
				if (!(*ProgBufTemp == '\r' && *(ProgBufTemp + 1) == '\n' &&
					*(ProgBufTemp + 2) == '\r' && *(ProgBufTemp + 3) == '\n'))
				{
					if (*ProgBufTemp == '\r' && *(ProgBufTemp + 1) == '\n')
					{
						// ��������� � ����� �� ����� �99 ����� ��������
						NameTableIter = NameTable.find(_T("M99"));
						NameTableIter->second.VarStack.push((double)((int)ProgBufTemp));
						// ���������� ����� ������ ����� �� ����� �99
						stack_size = NameTableIter->second.VarStack.size();
						// ����� ������������
						NameTableIter = NameTable.find(temp_str);
						if (NameTableIter == NameTable.end())
						{
							ErrorMsg.LoadString(IDS_ERR_SUB_PROG_NUMBER);
							State = Error;
						}
						else
						{
							pMyProc = NameTableIter->second.MyProc;
							if (pMyProc)
								(this->*pMyProc)();
							value = NameTableIter->second.Value;
							ProgBufTemp = (char*)((int)value);
							State = NameTableIter->second.State;
							// ��������� ������� ����� M99 � L
							NameTableIter = NameTable.find(_T("L"));
							if (stack_size != NameTableIter->second.VarStack.size())
							{
								// ������� ���������� ���������� ������������
								// �� ��������� - 1
								NameTableIter->second.VarStack.push(1);
							}
						}				
					}
					else
					{
						ErrorMsg.LoadString(IDS_ERR_SUB_PROG_NUMBER);
						State = Error;
					}
				}
				else
				{
					// ���������� ������������
					// ��������� ��������� �� �������. (������ ������ '\n')
					// �.�. ����� ���������� ������������ ���� ��� �������� ������
					table_data.Value = (double)((int)(ProgBufTemp + 3));
					table_data.State = SubProg;
					NameTable.insert(Pair(temp_str, table_data));
					State = SubProgEnd;
				}
			} // else (isdigit(*ProgBufTemp) && isdigit(*(ProgBufTemp + 1)) && isdigit(*(ProgBufTemp + 2)))
			break; // case SubProgNumber
		case LoopNumber:
			temp_str.clear();
			while(isdigit(*ProgBufTemp))
			{
				temp_str += *ProgBufTemp;
				ProgBufTemp++;
			}
			stack_value = atof(temp_str.c_str());
			NameTableIter = NameTable.find(_T("L"));
			NameTableIter->second.VarStack.push(stack_value);
			// ���������������� ������� �����
			if (*ProgBufTemp != 'P')
			{
				// ��������� ����� ��� ������� �����
				NameTableIter->second.Value = (double)((int)ProgBufTemp);
			}
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_LOOP_NUMBER);
			break; // case LoopNumber
		case SubProgEnd:
			while(*ProgBufTemp != 'M')
			{
				ProgBufTemp++;
			}
			ProgBufTemp++;
			if (isdigit(*ProgBufTemp) && (isdigit(*(ProgBufTemp + 1))))
			{
				c[0] = *ProgBufTemp;
				i = atoi(c);
				ProgBufTemp++;
				c[0] = *ProgBufTemp;
				j = atoi(c);
				if (i == 9 && j == 9 && *(ProgBufTemp + 1) == '\r')
				{
					ProgBufTemp++;
					State = ReturnCarriage; // ������ ����� ������������
				}
				else
				{
					State = SubProgEnd;
				}
			}
			else
			{
				ErrorMsg.LoadString(IDS_ERR_SUB_PROG_END);
				State = Error;			
			}
			break; // case SubProgEnd
		case SubProg:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_SUB_PROG);
			break; // case SubProg
		case ProgNumber:
			if (isdigit(*ProgBufTemp) && (isdigit(*(ProgBufTemp + 1))))
			{
				ProgBufTemp += 2;
				if (*ProgBufTemp == '\r' && (*(ProgBufTemp + 1)) == '\n' &&
					(*(ProgBufTemp + 2)) == '\r' && (*(ProgBufTemp + 3)) == '\n')
				{
					ProgBufTemp += 3;
					State = EndFrame;
				}
				else
				{
					ErrorMsg.LoadString(IDS_ERR_PROG_NUMBER);
					State = Error;
				}
			}
			else
			{
				if (isdigit(*ProgBufTemp))
				{
					ProgBufTemp += 1;
					if (*ProgBufTemp == '\r' && (*(ProgBufTemp + 1)) == '\n' &&
						(*(ProgBufTemp + 2)) == '\r' && (*(ProgBufTemp + 3)) == '\n')
					{
						ProgBufTemp += 3;
						State = EndFrame;
					}
					else
					{
						ErrorMsg.LoadString(IDS_ERR_PROG_NUMBER);
						State = Error;
					}
				}
				else
				{
					ErrorMsg.LoadString(IDS_ERR_PROG_NUMBER);
					State = Error;
				}
			}
			break; // case ProgNumber
		case FrameSkip:
			//temp_str = *ProgBufTemp;
			//NameTableIter = NameTable.find(temp_str);
			//State = GetNextState(temp_str, IDS_ERR_AUX_FUNCTION_M02);
			//break; // case FrameSkip
		case MainFrame:
			State = Error;
			ErrorMsg.LoadString(IDS_ERR_UNKNOWN);
			break;
		case AuxFunction:
			if (isdigit(*ProgBufTemp) && isdigit(*(ProgBufTemp + 1)))
			{
				temp_str.clear();
				temp_str.append(ProgBufTemp - 1, 3);
				ProgBufTemp++;
				State = GetNextState(temp_str, IDS_ERR_AUX_FUNCTION);
				// ����� �02 ��� �30 ����� ���� ����� ����� � ����� ������� �
				// EndInfo �� ���������� ��-�� ���������� �������� �����
				// ��������� ��������
				//if (State == AuxFunctionM02 || State == AuxFunctionM30)
				//	ProgBufTemp--;
			}
			else
			{
				ErrorMsg.LoadString(IDS_ERR_AUX_FUNCTION);
				State = Error;
			}
			break; // case AuxFunction
		case AuxFunctionM00:
			State = Error;
			ErrorMsg.LoadString(IDS_ERR_UNKNOWN);
			break; // case AuxFunctionM00
		case AuxFunctionM01:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_AUX_FUNCTION_M01);
			if (IsM01)
			{
				ProgBufTemp++;
				return 0;
			}
			break; // case AuxFunctionM01
		case AuxFunctionM02:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_AUX_FUNCTION_M02);
			ProgBufTemp++;
			return 0;
			//break; // case AuxFunctionM02
		case AuxFunctionM05:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_AUX_FUNCTION_M05);
			break; // case AuxFunctionM05
		case AuxFunctionM30: // ����������� ��������� ������ ����, ������� � %%
			State = EndInfo;
			ProgBufTemp = ProgBuf;
			break; // case AuxFunctionM30
		case AuxFunctionM99:
			stack_value = NameTableIter->second.VarStack.top();
			NameTableIter->second.VarStack.pop();
			ProgBufTemp = (char*)((int)stack_value);
			// �������� ������������ ���������� ������ ������������
			NameTableIter = NameTable.find(_T("L"));
			l_stack_value = NameTableIter->second.VarStack.top();
			if (l_stack_value == 1)// L = 1 - ���������� ����������� ���������
			{
				NameTableIter->second.VarStack.pop(); // ���� �� ����� L
				temp_str = *ProgBufTemp;
				State = GetNextState(temp_str, IDS_ERR_AUX_FUNCTION_M99);
			}			
			else // L > 1
			{
				l_stack_value--;
				ProgBufTemp -= 3;
				State = SubProgNumber;
			}
			break; // AuxFunctionM99
		case DimWordX:
			temp_str = *ProgBufTemp;
			NameTableIter = NameTable.find(temp_str);
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_X);
			break; // case DimWordX
		case DimWordY:
			temp_str = *ProgBufTemp;
			NameTableIter = NameTable.find(temp_str);
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_Y);
			break; // case DimWordY
		case DimWordZ:
			temp_str = *ProgBufTemp;
			NameTableIter = NameTable.find(temp_str);
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_Z);
			break; // case DimWordZ
		case DimWordA:
			temp_str = *ProgBufTemp;
			NameTableIter = NameTable.find(temp_str);
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_A);
			break; // case DimWordA
		case DimWordB:
			temp_str = *ProgBufTemp;
			NameTableIter = NameTable.find(temp_str);
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_B);
			break; // case DimWordB
		case DimWordC:
			temp_str = *ProgBufTemp;
			NameTableIter = NameTable.find(temp_str);
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_C);
			break; // case DimWordC
		case G:
			if (isdigit(*ProgBufTemp) && isdigit(*(ProgBufTemp + 1)))
			{
				temp_str.clear();
				temp_str.append(ProgBufTemp - 1, 3);
				ProgBufTemp++;
				State = GetNextState(temp_str, IDS_ERR_G);				
			}
			else
			{
				ErrorMsg.LoadString(IDS_ERR_G);
				State = Error;
			}
			break; // case G
		case G_00:
		case G_01:
		case G_02:
		case G_03:
		case G_04:
		case G_09:
		case G_10:
		case G_17:
		case G_18:
		case G_19:
		case G_27:
		case G_28:
		case G_29:
		case G_30:
		case G_31:
		case G_40:
		case G_41:
		case G_42:
		case G_43:
		case G_44:
		case G_49:
		case G_45:
		case G_46:
		case G_47:
		case G_48:
		case G_53:
		case G_54:
		case G_55:
		case G_56:
		case G_57:
		case G_58:
		case G_59:
		case G_80:
		case G_81:
		case G_82:
		case G_83:
		case G_84:
		case G_85:
		case G_86:
		case G_87:
		case G_88:
		case G_89:
		case G_90:
		case G_91:
		case G_92:
		case G_94:
		case G_60:
		case G_37:
		case G_36:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_GXX);
			break; // G_XX
		case DimWordI:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_I);
			break; // DimWordI
		case DimWordJ:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_J);
			break; // DimWordJ
		case DimWordK:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_DIM_WORD_K);
			break; // DimWordK
		case Presenting:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_PRESENTING);
			break; // Presenting
		case Spindel:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_SPINDEL);
			break; // Spindel
		case Pause:
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_PAUSE);
			break; // Pause
		case Tool:
			if (CallStatus != CALL_ERROR)
			{
				temp_str = *ProgBufTemp;
				State = GetNextState(temp_str, IDS_ERR_TOOL);
			}
			else
			{
				ErrorMsg.LoadString(IDS_ERR_NONFORMAT);
				State = Error;
			}
			break; // Tool
		case OffsetD:
			if (CallStatus != CALL_ERROR)
			{
				temp_str = *ProgBufTemp;
				State = GetNextState(temp_str, IDS_ERR_OFFSETD);
			}
			else
			{
				ErrorMsg.LoadString(IDS_ERR_NONFORMAT);
				State = Error;
			}
			break; // OffsetD
		case OffsetH:
			if (CallStatus != CALL_ERROR)
			{
				temp_str = *ProgBufTemp;
				State = GetNextState(temp_str, IDS_ERR_OFFSETH);
			}
			else
			{
				ErrorMsg.LoadString(IDS_ERR_NONFORMAT);
				State = Error;
			}
			break; // OffsetH
		case Macro:
			temp_str.clear();
			temp_str.append(ProgBufTemp - 1, 2);
			State = GetNextState(temp_str, IDS_ERR_MACRO);
			break; // case Macro
		case Print:
			Message = "";
			while(*ProgBufTemp != '&')
			{
				Message += *ProgBufTemp;
				ProgBufTemp++;
			}
			ProgBufTemp++;
			PrintMsg();
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_MACRO);
			break; // case Print
		case EndInfo:
			State = Initial;
			pLog->PrnLog(_T("������ �������� �������"));
			IsRun = false;
			return 0; // case EndInfo
		case Error:
			State = Initial;
			// ���������� ������ ������
			char *temp = ProgBuf;
			int lines = 0;
			for(temp; temp != ProgBufTemp; temp++)
				if (*temp == '\r')
					lines++;
			temp_msg.Format(_T(" (c����� %d)"), lines + 1);
			// ����� ���������
			pLog->PrnLog(ErrorMsg);
			pLog->PrnLog(temp_msg, false);
			IsRun = false;
			return 1;
		}
		ProgBufTemp++;
	} // while(ProgBuf)

	State = Initial;
	pLog->PrnLog(_T("������ �������� �������")/*, false*/);
	IsRun = false;
	return 0;
} // tStart()

const CNCmashineView* FInterpreter::GetpLog() const
{
	return pLog;
}

void FInterpreter::SetpLog(CNCmashineView* value)
{
	pLog = value;
	return;
}

// ������������� ������� ����
void FInterpreter::InitNameTable(void)
{
	_TableData table_data;
	table_data.Value = 0;
	table_data.MyProc = NULL;
	table_data.State = ProgStart1;
	NameTable.insert(Pair(_T("%"), table_data));
	table_data.State = ReturnCarriage;
	NameTable.insert(Pair(_T("\r"), table_data));
	table_data.State = EndFrame;
	NameTable.insert(Pair(_T("\r\n"), table_data));
	table_data.State = G;
	NameTable.insert(Pair(_T("G"), table_data));
	table_data.State = ProgNumber;
	NameTable.insert(Pair(_T("O"), table_data));
	table_data.State = SubProgNumber;
	NameTable.insert(Pair(_T("P"), table_data));
	table_data.State = LoopNumber;
	NameTable.insert(Pair(_T("L"), table_data));
	table_data.State = FrameNumber;
	NameTable.insert(Pair(_T("N"), table_data));
	table_data.State = MainFrame;
	NameTable.insert(Pair(_T(":"), table_data));
	table_data.State = FrameSkip;
	NameTable.insert(Pair(_T("/"), table_data));
	table_data.State = Macro;
	NameTable.insert(Pair(_T("#"), table_data));
	table_data.State = Print;
	NameTable.insert(Pair(_T("#&"), table_data));
	//
	// AuxFunction
	//
	table_data.State = AuxFunction;
	NameTable.insert(Pair(_T("M"), table_data));
	table_data.MyProc = afM00;
	table_data.State = AuxFunctionM00;
	NameTable.insert(Pair(_T("M00"), table_data));
	table_data.MyProc = afM01;
	table_data.State = AuxFunctionM01;
	NameTable.insert(Pair(_T("M01"), table_data));
	table_data.MyProc = afM02;
	table_data.State = AuxFunctionM02;
	NameTable.insert(Pair(_T("M02"), table_data));
	table_data.MyProc = afM05;
	table_data.State = AuxFunctionM05;
	NameTable.insert(Pair(_T("M05"), table_data));
	table_data.MyProc = afM30;
	table_data.State = AuxFunctionM30;
	NameTable.insert(Pair(_T("M30"), table_data));
	table_data.MyProc = afM99;
	table_data.State = AuxFunctionM99;
	NameTable.insert(Pair(_T("M99"), table_data));
	//
	// DimWords
	//
	table_data.MyProc = dwX;
	table_data.State = DimWordX;
	NameTable.insert(Pair(_T("X"), table_data));
	table_data.MyProc = dwY;
	table_data.State = DimWordY;
	NameTable.insert(Pair(_T("Y"), table_data));
	table_data.MyProc = dwZ;
	table_data.State = DimWordZ;
	NameTable.insert(Pair(_T("Z"), table_data));
	table_data.MyProc = dwA;
	table_data.State = DimWordA;
	NameTable.insert(Pair(_T("A"), table_data));
	table_data.MyProc = dwB;
	table_data.State = DimWordB;
	NameTable.insert(Pair(_T("B"), table_data));
	table_data.MyProc = dwC;
	table_data.State = DimWordC;
	NameTable.insert(Pair(_T("C"), table_data));
	table_data.MyProc = dwI;
	table_data.State = DimWordI;
	NameTable.insert(Pair(_T("I"), table_data));
	table_data.MyProc = dwJ;
	table_data.State = DimWordJ;
	NameTable.insert(Pair(_T("J"), table_data));
	table_data.MyProc = dwK;
	table_data.State = DimWordK;
	NameTable.insert(Pair(_T("K"), table_data));
	//
	// G
	//
	table_data.MyProc = G00;
	table_data.State = G_00;
	NameTable.insert(Pair(_T("G00"), table_data));
	table_data.MyProc = G01;
	table_data.State = G_01;
	NameTable.insert(Pair(_T("G01"), table_data));
	table_data.MyProc = G02;
	table_data.State = G_02;
	NameTable.insert(Pair(_T("G02"), table_data));
	table_data.MyProc = G03;
	table_data.State = G_03;
	NameTable.insert(Pair(_T("G03"), table_data));
	table_data.MyProc = G04;
	table_data.State = G_04;
	NameTable.insert(Pair(_T("G04"), table_data));
	table_data.MyProc = G09;
	table_data.State = G_09;
	NameTable.insert(Pair(_T("G09"), table_data));
	table_data.MyProc = G10;
	table_data.State = G_10;
	NameTable.insert(Pair(_T("G10"), table_data));
	table_data.MyProc = G17;
	table_data.State = G_17;
	NameTable.insert(Pair(_T("G17"), table_data));
	table_data.MyProc = G18;
	table_data.State = G_18;
	NameTable.insert(Pair(_T("G18"), table_data));
	table_data.MyProc = G19;
	table_data.State = G_19;
	NameTable.insert(Pair(_T("G19"), table_data));
	table_data.MyProc = G27;
	table_data.State = G_27;
	NameTable.insert(Pair(_T("G27"), table_data));
	table_data.MyProc = G28;
	table_data.State = G_28;
	NameTable.insert(Pair(_T("G28"), table_data));
	table_data.MyProc = G29;
	table_data.State = G_29;
	NameTable.insert(Pair(_T("G29"), table_data));
	table_data.MyProc = G30;
	table_data.State = G_30;
	NameTable.insert(Pair(_T("G30"), table_data));
	table_data.MyProc = G31;
	table_data.State = G_31;
	NameTable.insert(Pair(_T("G31"), table_data));
	table_data.MyProc = G36;
	table_data.State = G_36;
	NameTable.insert(Pair(_T("G36"), table_data));
	table_data.MyProc = G37;
	table_data.State = G_37;
	NameTable.insert(Pair(_T("G37"), table_data));
	table_data.MyProc = G40;
	table_data.State = G_40;
	NameTable.insert(Pair(_T("G40"), table_data));
	table_data.MyProc = G41;
	table_data.State = G_41;
	NameTable.insert(Pair(_T("G41"), table_data));
	table_data.MyProc = G42;
	table_data.State = G_42;
	NameTable.insert(Pair(_T("G42"), table_data));
	table_data.MyProc = G43;
	table_data.State = G_43;
	NameTable.insert(Pair(_T("G43"), table_data));
	table_data.MyProc = G44;
	table_data.State = G_44;
	NameTable.insert(Pair(_T("G44"), table_data));
	table_data.MyProc = G45;
	table_data.State = G_45;
	NameTable.insert(Pair(_T("G45"), table_data));
	table_data.MyProc = G46;
	table_data.State = G_46;
	NameTable.insert(Pair(_T("G46"), table_data));
	table_data.MyProc = G47;
	table_data.State = G_47;
	NameTable.insert(Pair(_T("G47"), table_data));
	table_data.MyProc = G48;
	table_data.State = G_48;
	NameTable.insert(Pair(_T("G48"), table_data));
	table_data.MyProc = G49;
	table_data.State = G_49;
	NameTable.insert(Pair(_T("G49"), table_data));
	table_data.MyProc = G53;
	table_data.State = G_53;
	NameTable.insert(Pair(_T("G53"), table_data));
	table_data.MyProc = G54;
	table_data.State = G_54;
	NameTable.insert(Pair(_T("G54"), table_data));
	table_data.MyProc = G55;
	table_data.State = G_55;
	NameTable.insert(Pair(_T("G55"), table_data));
	table_data.MyProc = G56;
	table_data.State = G_56;
	NameTable.insert(Pair(_T("G56"), table_data));
	table_data.MyProc = G57;
	table_data.State = G_57;
	NameTable.insert(Pair(_T("G57"), table_data));
	table_data.MyProc = G58;
	table_data.State = G_58;
	NameTable.insert(Pair(_T("G58"), table_data));
	table_data.MyProc = G59;
	table_data.State = G_59;
	NameTable.insert(Pair(_T("G59"), table_data));
	table_data.MyProc = G60;
	table_data.State = G_60;
	NameTable.insert(Pair(_T("G60"), table_data));
	table_data.MyProc = G80;
	table_data.State = G_80;
	NameTable.insert(Pair(_T("G80"), table_data));
	table_data.MyProc = G81;
	table_data.State = G_81;
	NameTable.insert(Pair(_T("G81"), table_data));
	table_data.MyProc = G82;
	table_data.State = G_82;
	NameTable.insert(Pair(_T("G82"), table_data));
	table_data.MyProc = G83;
	table_data.State = G_83;
	NameTable.insert(Pair(_T("G83"), table_data));
	table_data.MyProc = G84;
	table_data.State = G_84;
	NameTable.insert(Pair(_T("G84"), table_data));
	table_data.MyProc = G85;
	table_data.State = G_85;
	NameTable.insert(Pair(_T("G85"), table_data));
	table_data.MyProc = G86;
	table_data.State = G_86;
	NameTable.insert(Pair(_T("G86"), table_data));
	table_data.MyProc = G87;
	table_data.State = G_87;
	NameTable.insert(Pair(_T("G87"), table_data));
	table_data.MyProc = G88;
	table_data.State = G_88;
	NameTable.insert(Pair(_T("G88"), table_data));
	table_data.MyProc = G89;
	table_data.State = G_89;
	NameTable.insert(Pair(_T("G89"), table_data));
	table_data.MyProc = G90;
	table_data.State = G_90;
	NameTable.insert(Pair(_T("G90"), table_data));
	table_data.MyProc = G91;
	table_data.State = G_91;
	NameTable.insert(Pair(_T("G91"), table_data));
	table_data.MyProc = G92;
	table_data.State = G_92;
	NameTable.insert(Pair(_T("G92"), table_data));
	table_data.MyProc = G94;
	table_data.State = G_94;
	NameTable.insert(Pair(_T("G94"), table_data));
	//
	table_data.MyProc = fPresenting;
	table_data.State = Presenting;
	NameTable.insert(Pair(_T("F"), table_data));
	table_data.MyProc = fSpindel;
	table_data.State = Spindel;
	NameTable.insert(Pair(_T("S"), table_data));
	table_data.MyProc = fPause;
	table_data.State = Pause;
	NameTable.insert(Pair(_T("E"), table_data));
	table_data.MyProc = fTool;
	table_data.State = Tool;
	NameTable.insert(Pair(_T("T"), table_data));
	table_data.MyProc = fOffsetH;
	table_data.State = OffsetH;
	NameTable.insert(Pair(_T("H"), table_data));
	table_data.MyProc = fOffsetD;
	table_data.State = OffsetD;
	NameTable.insert(Pair(_T("D"), table_data));

}

double FInterpreter::afM00(void)
{
	TRACE(_T("afM00 was call\n"));
	return 0;
}

double FInterpreter::afM01(void)
{
	if (IsM01)
		pLog->PrnLog(_T("��������������� ��������� �� �01"));
	return 0;
}

double FInterpreter::afM02(void)
{
	pLog->PrnLog(_T("��������� �� �02"));
	return 0;
}

double FInterpreter::afM05(void)
{
	TRACE(_T("afM05 was call\n"));
	return 0;
}

double FInterpreter::afM30(void)
{
	pLog->PrnLog(_T("��������� �� �30"));
	return 0;
}

double FInterpreter::afM99(void)
{
	TRACE(_T("afM99 was call\n"));
	return 0;
}

double FInterpreter::dwX(void)
{
	TRACE(_T("dwX was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("X: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = DimWordX;
	Command.Data = value;

	return 0;
}

double FInterpreter::dwY(void)
{
	TRACE(_T("dwY was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("Y: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = DimWordY;
	Command.Data = value;

	return 0;
}

double FInterpreter::dwZ(void)
{
	TRACE(_T("dwZ was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("Z: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = DimWordZ;
	Command.Data = value;

	return 0;
}

double FInterpreter::dwA(void)
{
	TRACE(_T("dwA was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("A: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = DimWordA;
	Command.Data = value;

	return 0;
}

double FInterpreter::dwB(void)
{
	TRACE(_T("dwB was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("B: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = DimWordB;
	Command.Data = value;

	return 0;
}

double FInterpreter::dwC(void)
{
	TRACE(_T("dwC was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("C: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = DimWordC;
	Command.Data = value;

	return 0;
}

double FInterpreter::dwI(void)
{
	TRACE(_T("dwI was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("I: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = DimWordI;
	Command.Data = value;

	return 0;
}

double FInterpreter::dwJ(void)
{
	TRACE(_T("dwJ was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("J: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = DimWordJ;
	Command.Data = value;

	return 0;
}

double FInterpreter::dwK(void)
{
	TRACE(_T("dwK was call: "));
	double value;
	value = GetDimValue();
	NameTableIter->second.Value = value;
	TRACE(_T("%.5f\n"), value);
	CString str;
	str.Format(_T("K: %.5f"), value);
	pLog->PrnLog(str);

	Command.ID = DimWordK;
	Command.Data = value;

	return 0;
}
// ������� �����
double FInterpreter::fFrameSkip(void)
{
	return 0;
} // fFrameSkip

// �������� ������ ����� ��������� ������, ��������� ����������������� ������� ����
long FInterpreter::SetFileName(CString Path)
{
	return 0;
}

// ����� ���������� ��������� �� ����� ����
double FInterpreter::PrintMsg(void)
{
	pLog->PrnLog(_T("��������� ���������: ") + Message);
	return 0;
}

void FInterpreter::Stop(void )
{
	IsStop = true;
	IsRun = false;
	::SetEvent(evnETimer);
	KillTimer(AfxGetApp()->GetMainWnd()->GetSafeHwnd(), FInterpreter::E_TIMER);
}

// ����� ������ � ��������� � ��������� ��������� �� ��� ����� ���� �������
bool FInterpreter::Find(CString TextToFind)
{
	char *temp;
	string temp_str;
	int i;
	int len = TextToFind.GetLength();

	CString temp_msg;
	if (!ProgBuf)
	{		
		temp_msg.LoadString(IDS_ERROR_PROGRAMM_NOT_IN_MEMORY);
		AfxMessageBox(temp_msg);
		return false;
	}
	else
	{
		temp = TextToFind.GetBuffer(len);
		for (i = 0; i < len; i++)
			temp[i] = toupper(TextToFind[i]);
		TextToFind.ReleaseBuffer();
		temp = NULL;
		temp = strstr(ProgBufTemp, (LPCTSTR)TextToFind);
		if (temp)
		{
			ProgBufTemp = temp;
			temp_str = *ProgBufTemp;
			State = GetNextState(temp_str, IDS_ERR_UNKNOWN);
			if (State == Error)
			{
				State = Initial;
				ProgBufTemp = ProgBuf;
				pLog->PrnLog(ErrorMsg);
			}
			else
			{
				ProgBufTemp++;
			}
			return true;
		}
		else
		{
			return false;
		}
	}
} // Find(CString TextToFind)

// �������� ���������� �����
double FInterpreter::GetDimValue(void)
{
	ProgBufTemp++;
	string number;	
	if (*ProgBufTemp == '-' || *ProgBufTemp == '+')
	{
		number = *ProgBufTemp;
		ProgBufTemp++;
	}
	if (*ProgBufTemp == '.') // ����� � ������� .xxx, ������� ���� ������
	{
		number += '0';
		number += *ProgBufTemp;//',';
		ProgBufTemp++;
	}
	while(isdigit(*ProgBufTemp))
	{
		number += *ProgBufTemp;
		ProgBufTemp++;
	}
	if (*ProgBufTemp == '.')
	{
		number += *ProgBufTemp;//',';
		ProgBufTemp++;
		while (isdigit(*ProgBufTemp))
		{
			number += *ProgBufTemp;
			ProgBufTemp++;
		}
	}
	else
	{
		ProgBufTemp--;
		return atof(number.c_str());
	}

	ProgBufTemp--;
	return atof(number.c_str());
} // GetDimValue

// �������� ������, 2-�� ������� ������
int FInterpreter::GetDimValue2(void)
{
	ProgBufTemp++;
	string number;
	if 	(isdigit(*ProgBufTemp) && isdigit(*(ProgBufTemp + 1)))
	{
		number += *ProgBufTemp;
		ProgBufTemp++;
		number += *ProgBufTemp;
	}
	else
	{
		// ������ ������� ���� ��� ������� 1-9
		return CALL_ERROR;
	}

	return atoi(number.c_str());
} //GetDimValue2

// �������� ������� � ���������� �������������� �� ���������
inline FInterpreter::_State FInterpreter::GetNextState(string Lexeme, int StrId)
{
	MyProc pMyProc;
	NameTableIter = NameTable.find(Lexeme);
	if (NameTableIter == NameTable.end())
	{
		ErrorMsg.LoadString(StrId);
		State = Error;
	}
	else
	{
		pMyProc = NameTableIter->second.MyProc;
		if (pMyProc)
			CallStatus = (int)(this->*pMyProc)();
		State = NameTableIter->second.State;
	}
	return State;
} // GetNextState(string Lexeme, int StrId)

// ������� �������� ������
double FInterpreter::fPresenting(void)
{
	double value;
	CString str;
	value = GetDimValue();
	if (value >= 0 && value <= 15000.0)
		NameTableIter->second.Value = value;
	else
	{
		str.LoadString(IDS_ERR_PRESENTING_VALUE);
		pLog->PrnLog(str); // �� ������ �������� ������
	}
	TRACE(_T("F: %.5f\n"), value);
	str = "";
	str.Format(_T("F: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = Presenting;
	Command.Data = value;

	return 0;
}

// �������� �������� ��������
double FInterpreter::fSpindel(void)
{
	double value;
	value = GetDimValue();
	if (value >= 0 && value <= 9999.0)
		NameTableIter->second.Value = value;
	else
		; // �� ������ �������� �������� ��������
	TRACE(_T("S: %.5f\n"), value);
	CString str;
	str.Format(_T("S: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = Spindel;
	Command.Data = value;

	return 0;
}

// ����� � ���������� ���������
double FInterpreter::fPause(void)
{
	double value;
	int trunc;
	value = GetDimValue();
	if (value > 0)
	{
		trunc = (int)(value * 1000);
		NameTableIter->second.Value = trunc/1000; // � �������
		if(!::SetTimer(AfxGetApp()->GetMainWnd()->GetSafeHwnd(),
					   E_TIMER,
				       UINT(NameTableIter->second.Value * 1000), // ����� �����������
				       (TIMERPROC)TimerProc))
		{
			pLog->PrnLog(_T("�� ������� ������� ������"));
			TraceLastError();
		}
		else
		{
			// evnETimer - ���� Auto
			WaitForSingleObject(evnETimer, INFINITE);
		}
	}
	else
		; // ����� �� ����� ���� �������������
	TRACE(_T("E: %.3f\n"), value);
	CString str;
	str.Format(_T("E: %.3f"), value);
	pLog->PrnLog(str);
	return 0;
}

// ����� �����������
double FInterpreter::fTool(void)
{
	int value;
	value = GetDimValue2();
	if (!(value <= 0))
		NameTableIter->second.Value = value;
	else 
		return CALL_ERROR;// ����� ����������� �� ����� ���� �������������
    if (value == CALL_ERROR) // GetDimValue2 ���������� ������
		return value;

	TRACE(_T("T%.2d\n"), value);
	CString str;
	str.Format(_T("T: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = Tool;
	Command.Data = value;

	return 0;
}

// �������� ��� ��������� �� �����
double FInterpreter::fOffsetH(void)
{
	int value;
	value = GetDimValue2();
	if (!(value <= 0))
		NameTableIter->second.Value = value;
	else 
		return CALL_ERROR;// �������� �� ����� ���� �������������
    if (value == CALL_ERROR) // GetDimValue2 ���������� ������
		return value;

	TRACE(_T("H%.2d\n"), value);
	CString str;
	str.Format(_T("H: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = OffsetH;
	Command.Data = value;

	return 0;
}

// �������� ��� ��������� �� �������
double FInterpreter::fOffsetD(void)
{
	int value;
	value = GetDimValue2();
	if (!(value <= 0))
		NameTableIter->second.Value = value;
	else 
		return CALL_ERROR;// �������� �� ����� ���� �������������
    if (value == CALL_ERROR) // GetDimValue2 ���������� ������
		return value;

	TRACE(_T("D%.2d\n"), value);
	CString str;
	str.Format(_T("D: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = OffsetD;
	Command.Data = value;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//	G00 - ���������������� (���������� �����������)

double FInterpreter::G00()
{
	// ����� G00 ��� ���������� - ����������� � ������� �����
	TRACE(_T("G00 - ���������������� (���������� �����������)\n"));
	pLog->PrnLog(_T("G00 - ���������������� (���������� �����������)"));
	Command.ID = G_00;
	Command.Data = 0;

	return 0;
} // G00()

///////////////////////////////////////////////////////////////////////////////
//	G01 - �������� ������������ (������� ������) 

double FInterpreter::G01() 
{
	TRACE(_T("G01 - �������� ������������ (������� ������)\n"));
	pLog->PrnLog(_T("G01 - �������� ������������ (������� ������)"));
	Command.ID = G_01;
	Command.Data = 0;

	return 0;
}//int G01()

///////////////////////////////////////////////////////////////////////////////
//	G02 - �������� ������������ �� ������� �������

double FInterpreter::G02() 
{
	TRACE(_T("G02 - �������� ������������ �� ������� �������\n"));
	pLog->PrnLog(_T("G02 - �������� ������������ �� ������� �������"));
	Command.ID = G_02;
	Command.Data = 0;

	return 0;
}//int G02()

///////////////////////////////////////////////////////////////////////////////
//	G03 - �������� ������������ ������ ������� �������

double FInterpreter::G03() 
{
	TRACE(_T("G03 - �������� ������������ ������ ������� �������\n"));
	pLog->PrnLog(_T("G03 - �������� ������������ ������ ������� �������"));
	Command.ID = G_03;
	Command.Data = 0;

	return 0;
}//int G03()

///////////////////////////////////////////////////////////////////////////////
//	G04 - �����

double FInterpreter::G04() 
{
	TRACE(_T("G04 - �����\n"));
	pLog->PrnLog(_T("G04 - �����"));
	Command.ID = G_04;
	Command.Data = 0;

	return 0;
} // int G04()

///////////////////////////////////////////////////////////////////////////////
//	G09 - ���������� � ����� �����

double FInterpreter::G09()
{
	TRACE(_T("G09 - ���������� � ����� �����\n"));
	pLog->PrnLog(_T("G09 - ���������� � ����� �����"));
	Command.ID = G_09;
	Command.Data = 0;

	return 0;
} // G09()

///////////////////////////////////////////////////////////////////////////////
//	G10 - �������-�������� ������������

double FInterpreter::G10()
{
	TRACE(_T("G10 - �������-�������� ������������\n"));
	pLog->PrnLog(_T("G10 - �������-�������� ������������"));
	Command.ID = G_10;
	Command.Data = 0;

	return 0;
} // G10()

//---------------------------------------------------------------------------------------------
// ������ 02

///////////////////////////////////////////////////////////////////////////////
//	G17 - ������� ��������� XY

double FInterpreter::G17()
{
	TRACE(_T("G17 - ������� ��������� XY\n"));
	pLog->PrnLog(_T("G17 - ������� ��������� XY"));
	Command.ID = G_17;
	Command.Data = 0;

	return 0;
} // G17()

///////////////////////////////////////////////////////////////////////////////
//	G18 - ������� ��������� ZX

double FInterpreter::G18()
{
	TRACE(_T("G18 - ������� ��������� ZX\n"));
	pLog->PrnLog(_T("G18 - ������� ��������� ZX"));
	Command.ID = G_18;
	Command.Data = 0;

	return 0;
} // G18()

///////////////////////////////////////////////////////////////////////////////
//	G19 - ������� ��������� YZ

double FInterpreter::G19()
{
	TRACE(_T("G19 - ������� ��������� YZ\n"));
	pLog->PrnLog(_T("G19 - ������� ��������� YZ"));
	Command.ID = G_19;
	Command.Data = 0;

	return 0;
} // G19

//-----------------------------------------------------------------------------
// ������ 00

///////////////////////////////////////////////////////////////////////////////
//	G27 - ���� � "����" ��������� ������ �� ������� ������������� ����

double FInterpreter::G27()
{
	TRACE(_T("G27 - ���� � \"����\" ��������� ������ �� ������� ������������� ����\n"));
	pLog->PrnLog(_T("G27 - ���� � \"����\" ��������� ������ �� ������� ������������� ����"));
	Command.ID = G_27;
	Command.Data = 0;

	return 0;
} // G27()

///////////////////////////////////////////////////////////////////////////////
//	G28 - ������� � "����" ��������� ������ ����� ������������� �����

double FInterpreter::G28()
{
	TRACE(_T("G28 - ������� � \"����\" ��������� ������ ����� ������������� �����\n"));
	pLog->PrnLog(_T("G28 - ������� � \"����\" ��������� ������ ����� ������������� �����"));
	Command.ID = G_28;
	Command.Data = 0;

	return 0;
} // G28()

///////////////////////////////////////////////////////////////////////////////
//	G29 - ����� �� ���� � �������� ����� ����� ������������� �����

double FInterpreter::G29()
{
	CString msg;
	TRACE(_T("G29 - ����� �� ���� � �������� ����� ����� ������������� �����\n"));
	pLog->PrnLog(_T("G29 - ����� �� ���� � �������� ����� ����� ������������� �����"));
	Command.ID = G_29;
	Command.Data = 0;

	return 0;
} // G29()

///////////////////////////////////////////////////////////////////////////////
//	G30(31) - ����� � �������� ����� ����� ������������� �����

double FInterpreter::G30()
{
	TRACE(_T("G30(31) - ����� � �������� ����� ����� ������������� �����\n"));
	pLog->PrnLog(_T("G30(31) - ����� � �������� ����� ����� ������������� �����"));
	Command.ID = G_30;
	Command.Data = 0;

	return 0;
} // G30()

double FInterpreter::G31()
{
	G30();
	return 0;
}

//-----------------------------------------------------------------------------
// ������ 03

///////////////////////////////////////////////////////////////////////////////
//	G40 - ������ ��������� ����������� �� �������

double FInterpreter::G40()
{
	TRACE(_T("G40 - ������ ��������� ����������� �� �������\n"));
	pLog->PrnLog(_T("G40 - ������ ��������� ����������� �� �������"));
	Command.ID = G_40;
	Command.Data = 0;

	return 0;
} // G40()

///////////////////////////////////////////////////////////////////////////////
//	G41 - ��������� ����������� �� ������� �����

double FInterpreter::G41()
{
	TRACE(_T("G41 - ��������� ����������� �� ������� �����\n"));
	pLog->PrnLog(_T("G41 - ��������� ����������� �� ������� �����"));
	Command.ID = G_41;
	Command.Data = 0;

	return 0;
} // G41()

///////////////////////////////////////////////////////////////////////////////
//	G42 - ��������� ����������� �� ������� ������

double FInterpreter::G42()
{
	TRACE(_T("G42 - ��������� ����������� �� ������� ������\n"));
	pLog->PrnLog(_T("G42 - ��������� ����������� �� ������� ������"));
	Command.ID = G_42;
	Command.Data = 0;

	return 0;
} // G42()

//-----------------------------------------------------------------------------
// ������ 04

///////////////////////////////////////////////////////////////////////////////
//	G43 - ��������� ����� ����������� "+"

double FInterpreter::G43()
{
	TRACE(_T("G43 - ��������� ����� ����������� \"+\"\n"));
	pLog->PrnLog(_T("G43 - ��������� ����� ����������� \"+\""));
	Command.ID = G_43;
	Command.Data = 0;

	return 0;
} // G43()

///////////////////////////////////////////////////////////////////////////////
//	G44 - ��������� ����� ����������� "-"

double FInterpreter::G44()
{
	TRACE(_T("G44 - ��������� ����� ����������� \"-\"\n"));
	pLog->PrnLog(_T("G44 - ��������� ����� ����������� \"-\""));
	Command.ID = G_44;
	Command.Data = 0;

	return 0;
} // G44()

///////////////////////////////////////////////////////////////////////////////
//	G49 - ������ ��������� ����� �����������

double FInterpreter::G49()
{
	TRACE(_T("G49 - ������ ��������� ����� �����������\n"));
	pLog->PrnLog(_T("G49 - ������ ��������� ����� �����������"));
	Command.ID = G_49;
	Command.Data = 0;

	return 0;
} // G49

//-----------------------------------------------------------------------------
// ������ 00

///////////////////////////////////////////////////////////////////////////////
//	G45 - �������� ����������� � ����������� �����������

double FInterpreter::G45()
{
	TRACE(_T("G45 - �������� ����������� � ����������� �����������\n"));
	pLog->PrnLog(_T("G45 - �������� ����������� � ����������� �����������"));
	Command.ID = G_45;
	Command.Data = 0;

	return 0;
} // G45()

///////////////////////////////////////////////////////////////////////////////
//	G46 - �������� ����������� � ����������� ����������

double FInterpreter::G46()
{
	TRACE(_T("G46 - �������� ����������� � ����������� ����������\n"));
	pLog->PrnLog(_T("G46 - �������� ����������� � ����������� ����������"));
	Command.ID = G_46;
	Command.Data = 0;

	return 0;
} // G46()

///////////////////////////////////////////////////////////////////////////////
//	G47 - ������� �������� ����������� � ����������� �����������

double FInterpreter::G47()
{
	TRACE(_T("G47 - ������� �������� ����������� � ����������� �����������\n"));
	pLog->PrnLog(_T("G47 - ������� �������� ����������� � ����������� �����������"));
	Command.ID = G_47;
	Command.Data = 0;

	return 0;
} // G47()

///////////////////////////////////////////////////////////////////////////////
//	G48 - ������� �������� ����������� � ����������� ����������

double FInterpreter::G48()
{
	TRACE(_T("G48 - ������� �������� ����������� � ����������� ����������\n"));
	pLog->PrnLog(_T("G48 - ������� �������� ����������� � ����������� ����������"));
	Command.ID = G_48;
	Command.Data = 0;

	return 0;
} // G48

//-----------------------------------------------------------------------------
// ������ 05

///////////////////////////////////////////////////////////////////////////////
//	G53 - ������� � ���������� ������� ���������

double FInterpreter::G53()
{
	TRACE(_T("G53 - ������� �������� ����������� � ����������� �����������\n"));
	pLog->PrnLog(_T("G53 - ������� �������� ����������� � ����������� �����������"));
	Command.ID = G_53;
	Command.Data = 0;

	return 0;
} // G53()

///////////////////////////////////////////////////////////////////////////////
//	G54 - ����� ������������ ������� ��������� 1

double FInterpreter::G54()
{
	TRACE(_T("G54 - ����� ������������ ������� ��������� 1\n"));
	pLog->PrnLog(_T("G54 - ����� ������������ ������� ��������� 1"));
	Command.ID = G_54;
	Command.Data = 0;

	return 0;
} // G54()
///////////////////////////////////////////////////////////////////////////////
//	G54 - ����� ������������ ������� ��������� 2

double FInterpreter::G55()
{
	TRACE(_T("G55 - ����� ������������ ������� ��������� 2\n"));
	pLog->PrnLog(_T("G55 - ����� ������������ ������� ��������� 2"));
	Command.ID = G_55;
	Command.Data = 0;

	return 0;
} // G55()

///////////////////////////////////////////////////////////////////////////////
//	G56 - ����� ������������ ������� ��������� 3

double FInterpreter::G56()
{
	TRACE(_T("G56 - ����� ������������ ������� ��������� 3\n"));
	pLog->PrnLog(_T("G56 - ����� ������������ ������� ��������� 3"));
	Command.ID = G_56;
	Command.Data = 0;

	return 0;
} // G56()

///////////////////////////////////////////////////////////////////////////////
//	G57 - ����� ������������ ������� ��������� 1

double FInterpreter::G57()
{
	TRACE(_T("G57 - ����� ������������ ������� ��������� 4\n"));
	pLog->PrnLog(_T("G57 - ����� ������������ ������� ��������� 4"));
	Command.ID = G_57;
	Command.Data = 0;

	return 0;
} // G57()

///////////////////////////////////////////////////////////////////////////////
//	G58 - ����� ������������ ������� ��������� 5

double FInterpreter::G58()
{
	TRACE(_T("G58 - ����� ������������ ������� ��������� 5\n"));
	pLog->PrnLog(_T("G58 - ����� ������������ ������� ��������� 5"));
	Command.ID = G_58;
	Command.Data = 0;

	return 0;
} // G58()

///////////////////////////////////////////////////////////////////////////////
//	G59 - ����� ������������ ������� ��������� 6

double FInterpreter::G59()
{
	TRACE(_T("G59 - ����� ������������ ������� ��������� 6\n"));
	pLog->PrnLog(_T("G59 - ����� ������������ ������� ��������� 6"));
	Command.ID = G_59;
	Command.Data = 0;

	return 0;
} // G58()

//-----------------------------------------------------------------------------
// ������ 06

///////////////////////////////////////////////////////////////////////////////
//	G80 - ������ ����������� �����

double FInterpreter::G80()
{
	TRACE(_T("G80 - ������ ����������� �����\n"));
	pLog->PrnLog(_T("G80 - ������ ����������� �����"));
	Command.ID = G_80;
	Command.Data = 0;

	return 0;
} // G80()

///////////////////////////////////////////////////////////////////////////////
//	G81 - ���������, �����������, ������� �����

double FInterpreter::G81()
{
	TRACE(_T("G81 - ���������, �����������, ������� �����\n"));
	pLog->PrnLog(_T("G81 - ���������, �����������, ������� �����"));
	Command.ID = G_81;
	Command.Data = 0;

	return 0;
} // G81()

///////////////////////////////////////////////////////////////////////////////
//	G82 - ���������, ��������, ������� �����

double FInterpreter::G82()
{
	TRACE(_T("G82 - ���������, ��������, ������� �����\n"));
	pLog->PrnLog(_T("G82 - ���������, ��������, ������� �����"));
	Command.ID = G_82;
	Command.Data = 0;

	return 0;
} // G82()

///////////////////////////////////////////////////////////////////////////////
//	G83 - �������� ���������, ������� �����

double FInterpreter::G83()
{
	TRACE(_T("G83 - �������� ���������, ������� �����\n"));
	pLog->PrnLog(_T("G83 - �������� ���������, ������� �����"));
	Command.ID = G_83;
	Command.Data = 0;

	return 0;
} // G83()

///////////////////////////////////////////////////////////////////////////////
//	G84 - ��������� ������ ��������, ����� �� ������� ������

double FInterpreter::G84()
{
	TRACE(_T("G84 - ��������� ������ ��������, ����� �� ������� ������\n"));
	pLog->PrnLog(_T("G84 - ��������� ������ ��������, ����� �� ������� ������"));
	Command.ID = G_84;
	Command.Data = 0;

	return 0;
} // G84()

///////////////////////////////////////////////////////////////////////////////
//	G85 - ������������, �������������, ����� �� ������� ������

double FInterpreter::G85()
{
	TRACE(_T("G85 - ������������, �������������, ����� �� ������� ������\n"));
	pLog->PrnLog(_T("G85 - ������������, �������������, ����� �� ������� ������"));
	Command.ID = G_85;
	Command.Data = 0;

	return 0;
} // G85()

///////////////////////////////////////////////////////////////////////////////
//	G86 - ������������, ������� ������, ������� �����

double FInterpreter::G86()
{
	TRACE(_T("G86 - ������������, ������� ������, ������� �����\n"));
	pLog->PrnLog(_T("G86 - ������������, ������� ������, ������� �����"));
	Command.ID = G_86;
	Command.Data = 0;

	return 0;
} // G86()

///////////////////////////////////////////////////////////////////////////////
//	G87 - ������������, ������� ������, ����� � �������

double FInterpreter::G87()
{
	TRACE(_T("G87 - ������������, ������� ������, ����� � �������\n"));
	pLog->PrnLog(_T("G87 - ������������, ������� ������, ����� � �������"));
	Command.ID = G_87;
	Command.Data = 0;

	return 0;
} // G87()

///////////////////////////////////////////////////////////////////////////////
//	G88 - ������������, ����� � �������

double FInterpreter::G88()
{
	TRACE(_T("G88 - ������������, ����� � �������\n"));
	pLog->PrnLog(_T("G88 - ������������, ����� � �������"));
	Command.ID = G_88;
	Command.Data = 0;

	return 0;
} // G88()

///////////////////////////////////////////////////////////////////////////////
//	G89 - ������������, �������������, ����� �� ������� ������

double FInterpreter::G89()
{
	TRACE(_T("G89 - ������������, �������������, ����� �� ������� ������\n"));
	pLog->PrnLog(_T("G89 - ������������, �������������, ����� �� ������� ������"));
	Command.ID = G_89;
	Command.Data = 0;

	return 0;
} // G89()

//-----------------------------------------------------------------------------
// ������ 07

///////////////////////////////////////////////////////////////////////////////
//	G90 - ������� � ���������� ���������

double FInterpreter::G90()
{
	TRACE(_T("G90 - ������� � ���������� ���������\n"));
	pLog->PrnLog(_T("G90 - ������� � ���������� ���������"));
	Command.ID = G_90;
	Command.Data = 0;

	return 0;
} // G90()

///////////////////////////////////////////////////////////////////////////////
//	G91 - ������� � �����������

double FInterpreter::G91()
{
	TRACE(_T("G91 - ������� � �����������\n"));
	pLog->PrnLog(_T("G91 - ������� � �����������"));
	Command.ID = G_91;
	Command.Data = 0;

	return 0;
} // G91()

//-----------------------------------------------------------------------------
// ������ 00

///////////////////////////////////////////////////////////////////////////////
//	G92 - ������� ������� ���������

double FInterpreter::G92()
{
	TRACE(_T("G92 - ������� ������� ���������\n"));
	pLog->PrnLog(_T("G92 - ������� ������� ���������"));
	Command.ID = G_92;
	Command.Data = 0;

	return 0;
} // G92()

//-----------------------------------------------------------------------------
// ������ 08

///////////////////////////////////////////////////////////////////////////////
//	G94 - ������� ������ � ��/���

double FInterpreter::G94()
{
	TRACE(_T("G94 - ������� ������ � ��/���\n"));
	pLog->PrnLog(_T("G94 - ������� ������ � ��/���"));
	Command.ID = G_94;
	Command.Data = 0;

	return 0;
} // G94()

//-----------------------------------------------------------------------------
// ������ 00

///////////////////////////////////////////////////////////////////////////////
//	G60 - ������������� ����������������

double FInterpreter::G60()
{
	TRACE(_T("G60 - ������������� ����������������\n"));
	pLog->PrnLog(_T("G60 - ������������� ����������������"));
	Command.ID = G_60;
	Command.Data = 0;

	return 0;
} // G60()

//-----------------------------------------------------------------------------
// ������ 09

///////////////////////////////////////////////////////////////////////////////
//	G37 - ������� ��������� ������

double FInterpreter::G37()
{
	TRACE(_T("G37 - ������� ��������� ������\n"));
	pLog->PrnLog(_T("G37 - ������� ��������� ������"));
	Command.ID = G_37;
	Command.Data = 0;

	return 0;
} // G37()

///////////////////////////////////////////////////////////////////////////////
//	G36 - ������ ��������� ������

double FInterpreter::G36()
{
	TRACE(_T("G36 - ������ ��������� ������\n"));
	pLog->PrnLog(_T("G36 - ������ ��������� ������"));
	Command.ID = G_36;
	Command.Data = 0;

	return 0;
} // G36()

///////////////////////////////////////////////////////////////////////////////
// CleanUp - ����������� ������� � ����� ����������� (�� ���������)

void FInterpreter::CleanUp(bool del)
{
	ULONG exit_code;
	// ���������� �����, ���� �� ��� �������
	if (pInterpreterThread)
	{
		::PostThreadMessage(pInterpreterThread->m_nThreadID, WM_QUIT, 0, 0);
		GetExitCodeThread(pInterpreterThread->m_hThread, &exit_code);
		for(;;)
		{
			if (exit_code != STILL_ACTIVE)
			{
				delete pInterpreterThread;
				break;
			}
			else
			{
				Sleep(3);
				GetExitCodeThread(pInterpreterThread->m_hThread, &exit_code);		
			}
		}
	}
	if (del)
		delete this;
} // CleanUp()

///////////////////////////////////////////////////////////////////////////////
// ToUpper - �������������� ��������� (���. ��������) � �������� ��������

void FInterpreter::ToUpper()
{
	ProgBufTemp = ProgBuf;
	// Converts all programm to upper case
	while(*ProgBufTemp != 0)
	{
		*ProgBufTemp = toupper(*ProgBufTemp);
		ProgBufTemp++;
	}
	ProgBufTemp = ProgBuf;
} // ToUpper()

///////////////////////////////////////////////////////////////////////////////
//	InterpreterThread - ������� ����� �����������

UINT InterpreterThread(LPVOID data)
{
	int bRet;				// �������� ������������ GetMessage()
	MSG msg;				// ���������� ���������
	FInterpreter *pInterp = (FInterpreter*)data;

	TRACE(_T("InterpreterThread was call\n"));

    while((bRet = GetMessage(&msg, NULL, 0, 0)))
    { 
        if (bRet == -1)
        {
			TraceLastError();
			break;
        }
		switch (msg.message) 
		{
			case FM_START:
				pInterp->tStart();
				break; // FM_START
		} // switch (msg.message) 
	} // while((bRet = GetMessage...
	if (!bRet)
	{
		TRACE(_T("InterpreterThread get WM_QUIT\n"));
		return 0;
	}
	return 1;
} // InterpreterThread(LPVOID data)

///////////////////////////////////////////////////////////////////////////////
//	TimerProc - ������� ������� E_TIMER

void CALLBACK TimerProc(HWND hWnd,		// handle of CWnd that called SetTimer
						UINT nMsg,		// WM_TIMER
						UINT nIDEvent,	// timer identification
						DWORD dwTime)	// system time
							  
{
	TRACE(_T("TimerProc was call\n"));
	KillTimer(AfxGetApp()->GetMainWnd()->GetSafeHwnd(), FInterpreter::E_TIMER);
	::SetEvent(evnETimer);
} // TimerProc(...

//	����� ����� FInterpreter.cpp
///////////////////////////////////////////////////////////////////////////////