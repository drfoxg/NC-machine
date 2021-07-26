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

// Для E
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
	// Запуск потока
	pInterpreterThread = AfxBeginThread(InterpreterThread, (LPVOID)this);
	pInterpreterThread->m_bAutoDelete = false;
	// Таблица имен заполняется заново каждый раз при создании объекта
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
	// загрузка программы на выполнение
	try
	{
		CFile file(
				   Path,
				   CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary			   
				  );
		int len = (int)file.GetLength();
		// выделение памяти для программы
		if (!(ProgBuf = new char[(UINT)len + 1]))
		{
			temp_msg.LoadString(IDS_NOT_ENOUGH_MEMORY);
			TRACE(temp_msg);
			TRACE(_T("\n"));
			AfxMessageBox(temp_msg);
			ExitProcess(1); // конец работы
		}

		file.Read(ProgBuf, (UINT)len);
		file.Close();

		ProgBuf[len] = 0; // конец строки
		// Проверка наличия конца кадра после M30 и М02
		temp_str.append(ProgBuf + (len - 3), 3);
		if (temp_str != _T("M02") || temp_str != _T("M30") )
		{
			char *t = NULL;
			t = new char[len + 3]; // конец кадра + \0
			if (!t)
			{
				temp_msg.LoadString(IDS_NOT_ENOUGH_MEMORY);
				TRACE(temp_msg);
				TRACE(_T("\n"));
				AfxMessageBox(temp_msg);
				ExitProcess(1); // конец работы
			}
			memcpy(t, ProgBuf, len);
			delete [] ProgBuf;
			ProgBuf = NULL;
			ProgBuf = new char[len + 3]; // конец кадра + \0
			if (!ProgBuf)
			{
				temp_msg.LoadString(IDS_NOT_ENOUGH_MEMORY);
				TRACE(temp_msg);
				TRACE(_T("\n"));
				AfxMessageBox(temp_msg);
				ExitProcess(1); // конец работы
			}
			memcpy(ProgBuf, t, len);
			delete [] t;
			ProgBuf[len] = '\r';
			ProgBuf[len + 1] = '\n';
			ProgBuf[len + 2] = 0;
		}
		// Запуск потока
		pInterpreterThread = AfxBeginThread(InterpreterThread, (LPVOID)this);
		pInterpreterThread->m_bAutoDelete = false;
		ToUpper();
		// Таблица имен заполняется заново каждый раз при создании объекта
		InitNameTable();
	}
	catch(CFileException *f_exc)
	{
		char ErrorMsg[512];
		f_exc->GetErrorMessage(ErrorMsg, 512);
		f_exc->Delete();
		if (ProgBuf)
			delete [] ProgBuf; // память под программу
		ProgBuf = NULL;
		AfxMessageBox(ErrorMsg);
	}
} // FInterpreter(CString Path)

///////////////////////////////////////////////////////////////////////////////
// FInterpreter::tStart - выполнение трансляции (в отдельном потоке)

long FInterpreter::tStart()
{
	double value;
	double stack_value; // временная переменная для работы со стеком по ключу M99
	// временная переменная для работы со стеком по ключу L
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
			temp_msg.Format(_T("Старт программ(ы) из %s"), FileName);
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
		return 1; // произошла ошибка в конструкторе FInterpreter
	}
	char c[2] = {0,0};
	int i, j;
	string temp_str;
	MyProc pMyProc;

	_TableData table_data ;
	table_data.Value = 0;
	table_data.MyProc = NULL;

	IsRun = true;

	while(*ProgBufTemp != 0) // пока не конец файла
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
				ProgBufTemp += 2; // ProgBufTemp == '\r' для корректной программы
				// при введении параметров подпрограммы - изменить условия
				if (!(*ProgBufTemp == '\r' && *(ProgBufTemp + 1) == '\n' &&
					*(ProgBufTemp + 2) == '\r' && *(ProgBufTemp + 3) == '\n'))
				{
					if (*ProgBufTemp == '\r' && *(ProgBufTemp + 1) == '\n')
					{
						// запомнить в стеке по ключу М99 адрес возврата
						NameTableIter = NameTable.find(_T("M99"));
						NameTableIter->second.VarStack.push((double)((int)ProgBufTemp));
						// определить новый размер стека по ключу М99
						stack_size = NameTableIter->second.VarStack.size();
						// вызов подпрограммы
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
							// сравнение размера стека M99 и L
							NameTableIter = NameTable.find(_T("L"));
							if (stack_size != NameTableIter->second.VarStack.size())
							{
								// занести количество повторений подпрогарммы
								// по умолчанию - 1
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
					// объявление подпрограммы
					// запомнить указатель на подпрог. (второй символ '\n')
					// т.к. после объявление подпрограммы идет два перевода строки
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
			// программирование повтора кадра
			if (*ProgBufTemp != 'P')
			{
				// запомнить адрес для повтора кадра
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
					State = ReturnCarriage; // найден конец подпрограммы
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
				// после М02 или М30 может быть конец файла и тогда переход в
				// EndInfo не выполнится из-за завершения главного цикла
				// обработки символов
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
		case AuxFunctionM30: // Выполняется остановка работы УЧПУ, переход к %%
			State = EndInfo;
			ProgBufTemp = ProgBuf;
			break; // case AuxFunctionM30
		case AuxFunctionM99:
			stack_value = NameTableIter->second.VarStack.top();
			NameTableIter->second.VarStack.pop();
			ProgBufTemp = (char*)((int)stack_value);
			// проверка неоходимости повторения вызова подпрограммы
			NameTableIter = NameTable.find(_T("L"));
			l_stack_value = NameTableIter->second.VarStack.top();
			if (l_stack_value == 1)// L = 1 - нормальное продолжение программы
			{
				NameTableIter->second.VarStack.pop(); // стек по ключу L
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
			pLog->PrnLog(_T("Анализ закончен успешно"));
			IsRun = false;
			return 0; // case EndInfo
		case Error:
			State = Initial;
			// вычисление номера строки
			char *temp = ProgBuf;
			int lines = 0;
			for(temp; temp != ProgBufTemp; temp++)
				if (*temp == '\r')
					lines++;
			temp_msg.Format(_T(" (cтрока %d)"), lines + 1);
			// вывод сообщения
			pLog->PrnLog(ErrorMsg);
			pLog->PrnLog(temp_msg, false);
			IsRun = false;
			return 1;
		}
		ProgBufTemp++;
	} // while(ProgBuf)

	State = Initial;
	pLog->PrnLog(_T("Анализ закончен успешно")/*, false*/);
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

// Инициализация таблицы имен
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
		pLog->PrnLog(_T("Технологическая остановка по М01"));
	return 0;
}

double FInterpreter::afM02(void)
{
	pLog->PrnLog(_T("Остановка по М02"));
	return 0;
}

double FInterpreter::afM05(void)
{
	TRACE(_T("afM05 was call\n"));
	return 0;
}

double FInterpreter::afM30(void)
{
	pLog->PrnLog(_T("Остановка по М30"));
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
// Пропуск кадра
double FInterpreter::fFrameSkip(void)
{
	return 0;
} // fFrameSkip

// Открытие нового файла программы станка, частичная переинициализация таблицы имен
long FInterpreter::SetFileName(CString Path)
{
	return 0;
}

// Вывод текстового сообщения на экран УЧПУ
double FInterpreter::PrintMsg(void)
{
	pLog->PrnLog(_T("Сообщение программы: ") + Message);
	return 0;
}

void FInterpreter::Stop(void )
{
	IsStop = true;
	IsRun = false;
	::SetEvent(evnETimer);
	KillTimer(AfxGetApp()->GetMainWnd()->GetSafeHwnd(), FInterpreter::E_TIMER);
}

// Поиск строки в программе и установка указателя на это место если найдено
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

// Значение размерного слова
double FInterpreter::GetDimValue(void)
{
	ProgBufTemp++;
	string number;	
	if (*ProgBufTemp == '-' || *ProgBufTemp == '+')
	{
		number = *ProgBufTemp;
		ProgBufTemp++;
	}
	if (*ProgBufTemp == '.') // число в формате .xxx, ведущий ноль опущен
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

// Значение адреса, 2-ух значный формат
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
		// опущен ведущий ноль для номеров 1-9
		return CALL_ERROR;
	}

	return atoi(number.c_str());
} //GetDimValue2

// Выделяет лексему и возвращает соответсвующее ей состояние
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

// Задание скорости подачи
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
		pLog->PrnLog(str); // не менять скорость подачи
	}
	TRACE(_T("F: %.5f\n"), value);
	str = "";
	str.Format(_T("F: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = Presenting;
	Command.Data = value;

	return 0;
}

// Скорость вращения шпинделя
double FInterpreter::fSpindel(void)
{
	double value;
	value = GetDimValue();
	if (value >= 0 && value <= 9999.0)
		NameTableIter->second.Value = value;
	else
		; // не менять скорость вращения шпинделя
	TRACE(_T("S: %.5f\n"), value);
	CString str;
	str.Format(_T("S: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = Spindel;
	Command.Data = value;

	return 0;
}

// Пауза в выполнении программы
double FInterpreter::fPause(void)
{
	double value;
	int trunc;
	value = GetDimValue();
	if (value > 0)
	{
		trunc = (int)(value * 1000);
		NameTableIter->second.Value = trunc/1000; // в секудах
		if(!::SetTimer(AfxGetApp()->GetMainWnd()->GetSafeHwnd(),
					   E_TIMER,
				       UINT(NameTableIter->second.Value * 1000), // нужны милисекунды
				       (TIMERPROC)TimerProc))
		{
			pLog->PrnLog(_T("Не удалось создать таймер"));
			TraceLastError();
		}
		else
		{
			// evnETimer - типа Auto
			WaitForSingleObject(evnETimer, INFINITE);
		}
	}
	else
		; // время не может быть отрицательным
	TRACE(_T("E: %.3f\n"), value);
	CString str;
	str.Format(_T("E: %.3f"), value);
	pLog->PrnLog(str);
	return 0;
}

// Смена инструмента
double FInterpreter::fTool(void)
{
	int value;
	value = GetDimValue2();
	if (!(value <= 0))
		NameTableIter->second.Value = value;
	else 
		return CALL_ERROR;// номер инструмента не может быть отрицательным
    if (value == CALL_ERROR) // GetDimValue2 обнаружила ошибку
		return value;

	TRACE(_T("T%.2d\n"), value);
	CString str;
	str.Format(_T("T: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = Tool;
	Command.Data = value;

	return 0;
}

// Смещение для коррекции по длине
double FInterpreter::fOffsetH(void)
{
	int value;
	value = GetDimValue2();
	if (!(value <= 0))
		NameTableIter->second.Value = value;
	else 
		return CALL_ERROR;// смещение не может быть отрицательным
    if (value == CALL_ERROR) // GetDimValue2 обнаружила ошибку
		return value;

	TRACE(_T("H%.2d\n"), value);
	CString str;
	str.Format(_T("H: %.5f"), value);
	pLog->PrnLog(str);
	Command.ID = OffsetH;
	Command.Data = value;

	return 0;
}

// Смещение для коррекции по радиусу
double FInterpreter::fOffsetD(void)
{
	int value;
	value = GetDimValue2();
	if (!(value <= 0))
		NameTableIter->second.Value = value;
	else 
		return CALL_ERROR;// смещение не может быть отрицательным
    if (value == CALL_ERROR) // GetDimValue2 обнаружила ошибку
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
//	G00 - позиционирование (ускоренное перемещение)

double FInterpreter::G00()
{
	// вызов G00 без параметров - перемещение в нулевую точку
	TRACE(_T("G00 - Позиционирование (ускоренное перемещение)\n"));
	pLog->PrnLog(_T("G00 - Позиционирование (ускоренное перемещение)"));
	Command.ID = G_00;
	Command.Data = 0;

	return 0;
} // G00()

///////////////////////////////////////////////////////////////////////////////
//	G01 - линейная интерполяция (рабочая подача) 

double FInterpreter::G01() 
{
	TRACE(_T("G01 - Линейная интерполяция (рабочая подача)\n"));
	pLog->PrnLog(_T("G01 - Линейная интерполяция (рабочая подача)"));
	Command.ID = G_01;
	Command.Data = 0;

	return 0;
}//int G01()

///////////////////////////////////////////////////////////////////////////////
//	G02 - Круговая интерполяция по часовой стрелке

double FInterpreter::G02() 
{
	TRACE(_T("G02 - Круговая интерполяция по часовой стрелке\n"));
	pLog->PrnLog(_T("G02 - Круговая интерполяция по часовой стрелке"));
	Command.ID = G_02;
	Command.Data = 0;

	return 0;
}//int G02()

///////////////////////////////////////////////////////////////////////////////
//	G03 - Круговая интерполяция против часовой стрелки

double FInterpreter::G03() 
{
	TRACE(_T("G03 - Круговая интерполяция против часовой стрелки\n"));
	pLog->PrnLog(_T("G03 - Круговая интерполяция против часовой стрелки"));
	Command.ID = G_03;
	Command.Data = 0;

	return 0;
}//int G03()

///////////////////////////////////////////////////////////////////////////////
//	G04 - Пауза

double FInterpreter::G04() 
{
	TRACE(_T("G04 - Пауза\n"));
	pLog->PrnLog(_T("G04 - Пауза"));
	Command.ID = G_04;
	Command.Data = 0;

	return 0;
} // int G04()

///////////////////////////////////////////////////////////////////////////////
//	G09 - Торможение в конце кадра

double FInterpreter::G09()
{
	TRACE(_T("G09 - Торможение в конце кадра\n"));
	pLog->PrnLog(_T("G09 - Торможение в конце кадра"));
	Command.ID = G_09;
	Command.Data = 0;

	return 0;
} // G09()

///////////////////////////////////////////////////////////////////////////////
//	G10 - Линейно-круговая интерполяция

double FInterpreter::G10()
{
	TRACE(_T("G10 - Линейно-круговая интерполяция\n"));
	pLog->PrnLog(_T("G10 - Линейно-круговая интерполяция"));
	Command.ID = G_10;
	Command.Data = 0;

	return 0;
} // G10()

//---------------------------------------------------------------------------------------------
// группа 02

///////////////////////////////////////////////////////////////////////////////
//	G17 - Задание плоскости XY

double FInterpreter::G17()
{
	TRACE(_T("G17 - Задание плоскости XY\n"));
	pLog->PrnLog(_T("G17 - Задание плоскости XY"));
	Command.ID = G_17;
	Command.Data = 0;

	return 0;
} // G17()

///////////////////////////////////////////////////////////////////////////////
//	G18 - Задание плоскости ZX

double FInterpreter::G18()
{
	TRACE(_T("G18 - Задание плоскости ZX\n"));
	pLog->PrnLog(_T("G18 - Задание плоскости ZX"));
	Command.ID = G_18;
	Command.Data = 0;

	return 0;
} // G18()

///////////////////////////////////////////////////////////////////////////////
//	G19 - Задание плоскости YZ

double FInterpreter::G19()
{
	TRACE(_T("G19 - Задание плоскости YZ\n"));
	pLog->PrnLog(_T("G19 - Задание плоскости YZ"));
	Command.ID = G_19;
	Command.Data = 0;

	return 0;
} // G19

//-----------------------------------------------------------------------------
// группа 00

///////////////////////////////////////////////////////////////////////////////
//	G27 - Вход в "ноль" координат станка по путевым ограничителям хода

double FInterpreter::G27()
{
	TRACE(_T("G27 - Вход в \"ноль\" координат станка по путевым ограничителям хода\n"));
	pLog->PrnLog(_T("G27 - Вход в \"ноль\" координат станка по путевым ограничителям хода"));
	Command.ID = G_27;
	Command.Data = 0;

	return 0;
} // G27()

///////////////////////////////////////////////////////////////////////////////
//	G28 - Возврат в "ноль" координат станка через промежуточную точку

double FInterpreter::G28()
{
	TRACE(_T("G28 - Возврат в \"ноль\" координат станка через промежуточную точку\n"));
	pLog->PrnLog(_T("G28 - Возврат в \"ноль\" координат станка через промежуточную точку"));
	Command.ID = G_28;
	Command.Data = 0;

	return 0;
} // G28()

///////////////////////////////////////////////////////////////////////////////
//	G29 - Выход из ноля в заданную точку через промежуточную точку

double FInterpreter::G29()
{
	CString msg;
	TRACE(_T("G29 - Выход из ноля в заданную точку через промежуточную точку\n"));
	pLog->PrnLog(_T("G29 - Выход из ноля в заданную точку через промежуточную точку"));
	Command.ID = G_29;
	Command.Data = 0;

	return 0;
} // G29()

///////////////////////////////////////////////////////////////////////////////
//	G30(31) - Выход в заданную точку через промежуточную точку

double FInterpreter::G30()
{
	TRACE(_T("G30(31) - Выход в заданную точку через промежуточную точку\n"));
	pLog->PrnLog(_T("G30(31) - Выход в заданную точку через промежуточную точку"));
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
// группа 03

///////////////////////////////////////////////////////////////////////////////
//	G40 - Отмена коррекции инструмента по радиусу

double FInterpreter::G40()
{
	TRACE(_T("G40 - Отмена коррекции инструмента по радиусу\n"));
	pLog->PrnLog(_T("G40 - Отмена коррекции инструмента по радиусу"));
	Command.ID = G_40;
	Command.Data = 0;

	return 0;
} // G40()

///////////////////////////////////////////////////////////////////////////////
//	G41 - Коррекция инструмента по радиусу слева

double FInterpreter::G41()
{
	TRACE(_T("G41 - Коррекция инструмента по радиусу слева\n"));
	pLog->PrnLog(_T("G41 - Коррекция инструмента по радиусу слева"));
	Command.ID = G_41;
	Command.Data = 0;

	return 0;
} // G41()

///////////////////////////////////////////////////////////////////////////////
//	G42 - Коррекция инструмента по радиусу справа

double FInterpreter::G42()
{
	TRACE(_T("G42 - Коррекция инструмента по радиусу справа\n"));
	pLog->PrnLog(_T("G42 - Коррекция инструмента по радиусу справа"));
	Command.ID = G_42;
	Command.Data = 0;

	return 0;
} // G42()

//-----------------------------------------------------------------------------
// группа 04

///////////////////////////////////////////////////////////////////////////////
//	G43 - Коррекция длины инструмента "+"

double FInterpreter::G43()
{
	TRACE(_T("G43 - Коррекция длины инструмента \"+\"\n"));
	pLog->PrnLog(_T("G43 - Коррекция длины инструмента \"+\""));
	Command.ID = G_43;
	Command.Data = 0;

	return 0;
} // G43()

///////////////////////////////////////////////////////////////////////////////
//	G44 - Коррекция длины инструмента "-"

double FInterpreter::G44()
{
	TRACE(_T("G44 - Коррекция длины инструмента \"-\"\n"));
	pLog->PrnLog(_T("G44 - Коррекция длины инструмента \"-\""));
	Command.ID = G_44;
	Command.Data = 0;

	return 0;
} // G44()

///////////////////////////////////////////////////////////////////////////////
//	G49 - Отмена коррекции длины инструмента

double FInterpreter::G49()
{
	TRACE(_T("G49 - Отмена коррекции длины инструмента\n"));
	pLog->PrnLog(_T("G49 - Отмена коррекции длины инструмента"));
	Command.ID = G_49;
	Command.Data = 0;

	return 0;
} // G49

//-----------------------------------------------------------------------------
// группа 00

///////////////////////////////////////////////////////////////////////////////
//	G45 - Смещение инструмента в направлении расширенеия

double FInterpreter::G45()
{
	TRACE(_T("G45 - Смещение инструмента в направлении расширенеия\n"));
	pLog->PrnLog(_T("G45 - Смещение инструмента в направлении расширенеия"));
	Command.ID = G_45;
	Command.Data = 0;

	return 0;
} // G45()

///////////////////////////////////////////////////////////////////////////////
//	G46 - Смещение инструмента в направлении сокращения

double FInterpreter::G46()
{
	TRACE(_T("G46 - Смещение инструмента в направлении сокращения\n"));
	pLog->PrnLog(_T("G46 - Смещение инструмента в направлении сокращения"));
	Command.ID = G_46;
	Command.Data = 0;

	return 0;
} // G46()

///////////////////////////////////////////////////////////////////////////////
//	G47 - Двойное смещение инструмента в направлении расширенеия

double FInterpreter::G47()
{
	TRACE(_T("G47 - Двойное смещение инструмента в направлении расширенеия\n"));
	pLog->PrnLog(_T("G47 - Двойное смещение инструмента в направлении расширенеия"));
	Command.ID = G_47;
	Command.Data = 0;

	return 0;
} // G47()

///////////////////////////////////////////////////////////////////////////////
//	G48 - Двойное смещение инструмента в направлении сокращения

double FInterpreter::G48()
{
	TRACE(_T("G48 - Двойное смещение инструмента в направлении сокращения\n"));
	pLog->PrnLog(_T("G48 - Двойное смещение инструмента в направлении сокращения"));
	Command.ID = G_48;
	Command.Data = 0;

	return 0;
} // G48

//-----------------------------------------------------------------------------
// группа 05

///////////////////////////////////////////////////////////////////////////////
//	G53 - Возврат к абсолютной системе координат

double FInterpreter::G53()
{
	TRACE(_T("G53 - Двойное смещение инструмента в направлении расширенеия\n"));
	pLog->PrnLog(_T("G53 - Двойное смещение инструмента в направлении расширенеия"));
	Command.ID = G_53;
	Command.Data = 0;

	return 0;
} // G53()

///////////////////////////////////////////////////////////////////////////////
//	G54 - Выбор координатной системы заготовки 1

double FInterpreter::G54()
{
	TRACE(_T("G54 - Выбор координатной системы заготовки 1\n"));
	pLog->PrnLog(_T("G54 - Выбор координатной системы заготовки 1"));
	Command.ID = G_54;
	Command.Data = 0;

	return 0;
} // G54()
///////////////////////////////////////////////////////////////////////////////
//	G54 - Выбор координатной системы заготовки 2

double FInterpreter::G55()
{
	TRACE(_T("G55 - Выбор координатной системы заготовки 2\n"));
	pLog->PrnLog(_T("G55 - Выбор координатной системы заготовки 2"));
	Command.ID = G_55;
	Command.Data = 0;

	return 0;
} // G55()

///////////////////////////////////////////////////////////////////////////////
//	G56 - Выбор координатной системы заготовки 3

double FInterpreter::G56()
{
	TRACE(_T("G56 - Выбор координатной системы заготовки 3\n"));
	pLog->PrnLog(_T("G56 - Выбор координатной системы заготовки 3"));
	Command.ID = G_56;
	Command.Data = 0;

	return 0;
} // G56()

///////////////////////////////////////////////////////////////////////////////
//	G57 - Выбор координатной системы заготовки 1

double FInterpreter::G57()
{
	TRACE(_T("G57 - Выбор координатной системы заготовки 4\n"));
	pLog->PrnLog(_T("G57 - Выбор координатной системы заготовки 4"));
	Command.ID = G_57;
	Command.Data = 0;

	return 0;
} // G57()

///////////////////////////////////////////////////////////////////////////////
//	G58 - Выбор координатной системы заготовки 5

double FInterpreter::G58()
{
	TRACE(_T("G58 - Выбор координатной системы заготовки 5\n"));
	pLog->PrnLog(_T("G58 - Выбор координатной системы заготовки 5"));
	Command.ID = G_58;
	Command.Data = 0;

	return 0;
} // G58()

///////////////////////////////////////////////////////////////////////////////
//	G59 - Выбор координатной системы заготовки 6

double FInterpreter::G59()
{
	TRACE(_T("G59 - Выбор координатной системы заготовки 6\n"));
	pLog->PrnLog(_T("G59 - Выбор координатной системы заготовки 6"));
	Command.ID = G_59;
	Command.Data = 0;

	return 0;
} // G58()

//-----------------------------------------------------------------------------
// группа 06

///////////////////////////////////////////////////////////////////////////////
//	G80 - Отмена постоянного цикла

double FInterpreter::G80()
{
	TRACE(_T("G80 - Отмена постоянного цикла\n"));
	pLog->PrnLog(_T("G80 - Отмена постоянного цикла"));
	Command.ID = G_80;
	Command.Data = 0;

	return 0;
} // G80()

///////////////////////////////////////////////////////////////////////////////
//	G81 - Сверление, зацентровка, быстрый отвод

double FInterpreter::G81()
{
	TRACE(_T("G81 - Сверление, зацентровка, быстрый отвод\n"));
	pLog->PrnLog(_T("G81 - Сверление, зацентровка, быстрый отвод"));
	Command.ID = G_81;
	Command.Data = 0;

	return 0;
} // G81()

///////////////////////////////////////////////////////////////////////////////
//	G82 - Сверление, зенковка, быстрый отвод

double FInterpreter::G82()
{
	TRACE(_T("G82 - Сверление, зенковка, быстрый отвод\n"));
	pLog->PrnLog(_T("G82 - Сверление, зенковка, быстрый отвод"));
	Command.ID = G_82;
	Command.Data = 0;

	return 0;
} // G82()

///////////////////////////////////////////////////////////////////////////////
//	G83 - Глубокое сверление, быстрый отвод

double FInterpreter::G83()
{
	TRACE(_T("G83 - Глубокое сверление, быстрый отвод\n"));
	pLog->PrnLog(_T("G83 - Глубокое сверление, быстрый отвод"));
	Command.ID = G_83;
	Command.Data = 0;

	return 0;
} // G83()

///////////////////////////////////////////////////////////////////////////////
//	G84 - Нарезание резьбы метчиком, отвод на рабочей подаче

double FInterpreter::G84()
{
	TRACE(_T("G84 - Нарезание резьбы метчиком, отвод на рабочей подаче\n"));
	pLog->PrnLog(_T("G84 - Нарезание резьбы метчиком, отвод на рабочей подаче"));
	Command.ID = G_84;
	Command.Data = 0;

	return 0;
} // G84()

///////////////////////////////////////////////////////////////////////////////
//	G85 - Растачивание, развертывание, отвод на рабочей подаче

double FInterpreter::G85()
{
	TRACE(_T("G85 - Растачивание, развертывание, отвод на рабочей подаче\n"));
	pLog->PrnLog(_T("G85 - Растачивание, развертывание, отвод на рабочей подаче"));
	Command.ID = G_85;
	Command.Data = 0;

	return 0;
} // G85()

///////////////////////////////////////////////////////////////////////////////
//	G86 - Растачивание, рабочая подача, быстрый отвод

double FInterpreter::G86()
{
	TRACE(_T("G86 - Растачивание, рабочая подача, быстрый отвод\n"));
	pLog->PrnLog(_T("G86 - Растачивание, рабочая подача, быстрый отвод"));
	Command.ID = G_86;
	Command.Data = 0;

	return 0;
} // G86()

///////////////////////////////////////////////////////////////////////////////
//	G87 - Растачивание, рабочая подача, отвод в вручную

double FInterpreter::G87()
{
	TRACE(_T("G87 - Растачивание, рабочая подача, отвод в вручную\n"));
	pLog->PrnLog(_T("G87 - Растачивание, рабочая подача, отвод в вручную"));
	Command.ID = G_87;
	Command.Data = 0;

	return 0;
} // G87()

///////////////////////////////////////////////////////////////////////////////
//	G88 - Растачивание, отвод в вручную

double FInterpreter::G88()
{
	TRACE(_T("G88 - Растачивание, отвод в вручную\n"));
	pLog->PrnLog(_T("G88 - Растачивание, отвод в вручную"));
	Command.ID = G_88;
	Command.Data = 0;

	return 0;
} // G88()

///////////////////////////////////////////////////////////////////////////////
//	G89 - Растачивание, развертывание, отвод на рабочей подаче

double FInterpreter::G89()
{
	TRACE(_T("G89 - Растачивание, развертывание, отвод на рабочей подаче\n"));
	pLog->PrnLog(_T("G89 - Растачивание, развертывание, отвод на рабочей подаче"));
	Command.ID = G_89;
	Command.Data = 0;

	return 0;
} // G89()

//-----------------------------------------------------------------------------
// группа 07

///////////////////////////////////////////////////////////////////////////////
//	G90 - Задание в абсолютных величинах

double FInterpreter::G90()
{
	TRACE(_T("G90 - Задание в абсолютных величинах\n"));
	pLog->PrnLog(_T("G90 - Задание в абсолютных величинах"));
	Command.ID = G_90;
	Command.Data = 0;

	return 0;
} // G90()

///////////////////////////////////////////////////////////////////////////////
//	G91 - Задание в приращениях

double FInterpreter::G91()
{
	TRACE(_T("G91 - Задание в приращениях\n"));
	pLog->PrnLog(_T("G91 - Задание в приращениях"));
	Command.ID = G_91;
	Command.Data = 0;

	return 0;
} // G91()

//-----------------------------------------------------------------------------
// группа 00

///////////////////////////////////////////////////////////////////////////////
//	G92 - Задание системы координат

double FInterpreter::G92()
{
	TRACE(_T("G92 - Задание системы координат\n"));
	pLog->PrnLog(_T("G92 - Задание системы координат"));
	Command.ID = G_92;
	Command.Data = 0;

	return 0;
} // G92()

//-----------------------------------------------------------------------------
// группа 08

///////////////////////////////////////////////////////////////////////////////
//	G94 - Задание подачи в мм/мин

double FInterpreter::G94()
{
	TRACE(_T("G94 - Задание подачи в мм/мин\n"));
	pLog->PrnLog(_T("G94 - Задание подачи в мм/мин"));
	Command.ID = G_94;
	Command.Data = 0;

	return 0;
} // G94()

//-----------------------------------------------------------------------------
// группа 00

///////////////////////////////////////////////////////////////////////////////
//	G60 - Одностороннее позиционирование

double FInterpreter::G60()
{
	TRACE(_T("G60 - Одностороннее позиционирование\n"));
	pLog->PrnLog(_T("G60 - Одностороннее позиционирование"));
	Command.ID = G_60;
	Command.Data = 0;

	return 0;
} // G60()

//-----------------------------------------------------------------------------
// группа 09

///////////////////////////////////////////////////////////////////////////////
//	G37 - Задание токарного режима

double FInterpreter::G37()
{
	TRACE(_T("G37 - Задание токарного режима\n"));
	pLog->PrnLog(_T("G37 - Задание токарного режима"));
	Command.ID = G_37;
	Command.Data = 0;

	return 0;
} // G37()

///////////////////////////////////////////////////////////////////////////////
//	G36 - Отмена токарного режима

double FInterpreter::G36()
{
	TRACE(_T("G36 - Отмена токарного режима\n"));
	pLog->PrnLog(_T("G36 - Отмена токарного режима"));
	Command.ID = G_36;
	Command.Data = 0;

	return 0;
} // G36()

///////////////////////////////////////////////////////////////////////////////
// CleanUp - необходимая очистка и вызов деструктора (по умолчанию)

void FInterpreter::CleanUp(bool del)
{
	ULONG exit_code;
	// Остановить поток, если он был запущен
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
// ToUpper - преобразование программы (лат. символов) к верхнему регистру

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
//	InterpreterThread - рабочий поток транслятора

UINT InterpreterThread(LPVOID data)
{
	int bRet;				// значение возвращаемое GetMessage()
	MSG msg;				// получаемое сообщение
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
//	TimerProc - функция таймера E_TIMER

void CALLBACK TimerProc(HWND hWnd,		// handle of CWnd that called SetTimer
						UINT nMsg,		// WM_TIMER
						UINT nIDEvent,	// timer identification
						DWORD dwTime)	// system time
							  
{
	TRACE(_T("TimerProc was call\n"));
	KillTimer(AfxGetApp()->GetMainWnd()->GetSafeHwnd(), FInterpreter::E_TIMER);
	::SetEvent(evnETimer);
} // TimerProc(...

//	Конец файла FInterpreter.cpp
///////////////////////////////////////////////////////////////////////////////