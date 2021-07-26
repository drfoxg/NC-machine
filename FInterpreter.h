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
// Пользовательские сообщения

const FM_START = WM_APP + 100;				// трансляция

class CNCmashineView;

//Транслятор выполняет лексический и синтаксически анализ над 
//файлом программы
class FInterpreter 
{
public:
	// Ошибка в вызванной функции "генератора кода"
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
	// Отправка сообщения на выполнение трансляции
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
	// Выполнение трансляции (в отдельном потоке)
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
	// Открытие нового файла программы станка, частичная переинициализация таблицы имен
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
	// Поиск строки в программе и установка указателя на это место если найдено
	bool Find(CString TextToFind);
	// Поиск строки в программе c начала (ProgBufTemp = ProgBuf)
	void RestartFind()
	{
		ProgBufTemp = ProgBuf;
	}
	// Необходимая очистка и вызов деструктора (по умолчанию)
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
	// Текст программы
	char* ProgBuf;
	// Временная для текста программы
	char *ProgBufTemp;
	// Cообщение выводимое на экран УЧПУ
	CString Message;
	// Файл программы с полным путем
	CString FileName;
	CNCmashineView* pLog;
	enum _State
	{
		Initial,
		ProgStart1,
		ProgStart2,
		EndFrame,
		EndInfo,		// встретилась функция М30
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
		Presenting,		// подача
		Spindel,		// шпиндель
		Pause,
		Tool,			// инструмент
		OffsetH,		// смещение для коррекции по длине
		OffsetD			// смещение для коррекции по радиусу
	};

	struct _TableData
	{
		//_TableData(){};
		//_TableData(const _TableData &table_data);
		//~_TableData(){};
		//_TableData operator=(_TableData &table_data);

		double (FInterpreter::* MyProc)(void);		
		double Value;
		// стек переменной, используется в подпрограмме
		stack<double> VarStack;
		_State State;
	};
	// Команда управляющему модулю
	struct _Command
	{
		_State ID;
		double Data;
	};
	typedef double (FInterpreter::* MyProc)(void);
	typedef pair <string, _TableData> Pair;
	hash_map<string, _TableData> NameTable;
	hash_map<string, _TableData>::iterator NameTableIter;
	// Результат вызова функции "генератора кода"
	int CallStatus;
	// Сообщение об ошибке трансляции
	CString ErrorMsg;
	// Флаг остановки
	bool IsStop; 
	// Флаг - идет трансляция
	bool IsRun;
	// Флаг технологической остановки
	bool IsM01;
	// Флаг пропуска кадра
	bool IsFrameSkip;
private:
	_Mode CurrentMode;
	_State State;
	_Command Command;
	CWinThread *pInterpreterThread;
private:	
	void InitNameTable(void);	// Инициализация таблицы имен
	void ToUpper();				// Преобразование программы (лат. символов) к верх. регистру
	//
	// Aux function - вспомогательные функции
	//
	double afM00(void);	// M00
	double afM01(void);	// M01
	double afM02(void);	// M02
	double afM05(void);	// M05
	double afM30(void);	// M30
	double afM99(void);	// M99
	//
	// Dim words - размерные слова
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
	// Пропуск кадра
	double fFrameSkip(void);
	// Вывод текстового сообщения на экран УЧПУ
	double PrintMsg(void);
	// Значение размерного слова
	double GetDimValue(void);
	// Значение адреса, 2-ух значный формат
	int FInterpreter::GetDimValue2(void);
	// Выделяет лексему и возвращает соответсвующее ей состояние  
	_State GetNextState(string Lexeme, int StrId);
	// Задание скорости подачи
	double fPresenting(void);
	// Скорость вращения шпинделя
	double fSpindel(void);
	// Пауза в выполнении программы
	double fPause(void);
	// Смена инструмента
	double fTool(void);
	// Смещение для коррекции по длине
	double fOffsetH(void);
	// Смещение для коррекции по радиусу
	double fOffsetD(void);
	//
	// G-функции
	//
	// группа 01
	double G00(); // Позиционирование (ускоренное перемещение)
	double G01(); // Линейная интерполяция (рабочая подача)
	double G02(); // Круговая интерполяция по часовой стрелке
	double G03(); // Круговая интерполяция против часовой стрелки
	// группа 00
	double G04(); // Пауза
	double G09(); // Торможение в конце кадра
	double G10(); // Линейно-круговая интерполяция
	// группа 02
	double G17(); // Задание плоскости XY
	double G18(); // Задание плоскости ZX
	double G19(); // Задание плоскости YZ
	// группа 00
	double G27(); // Вход в "ноль" координат станка по путевым ограничителям хода
	double G28(); // Возврат в "ноль" координат станка через промежуточную точку
	double G29(); // Выход из ноля в заданную точку через промежуточную точку
	double G30(); // Выход в заданную точку
	double G31(); // через промежуточную точку
	// группа 03
	double G40(); // Отмена коррекции инструмента по радиусу
	double G41(); // Коррекция инструмента по радиусу слева
	double G42(); // Коррекция инструмента по радиусу справа
	// группа 04
	double G43(); // Коррекция длины инструмента "+"
	double G44(); // Коррекция длины инструмента "-"
	double G49(); // Отмена коррекции длины инструмента
	// группа 00
	double G45(); // Смещение инструмента в направлении расширенеия
	double G46(); // Смещение инструмента в направлении сокращения
	double G47(); // Двойное смещение инструмента в направлении расширенеия
	double G48(); // Двойное смещение инструмента в направлении сокращения
	// группа 05
	double G53(); // Возврат к абсолютной системе координат
	double G54(); // Выбор координатной системы заготовки 1
	double G55(); // Выбор координатной системы заготовки 2
	double G56(); // Выбор координатной системы заготовки 3
	double G57(); // Выбор координатной системы заготовки 4
	double G58(); // Выбор координатной системы заготовки 5
	double G59(); // Выбор координатной системы заготовки 6
	// группа 06
	double G80(); // Отмена постоянного цикла 
	double G81(); // Сверление, зацентровка, быстрый отвод
	double G82(); // Сверление, зенковка, быстрый отвод
	double G83(); // Глубокое сверление, быстрый отвод
	double G84(); // Нарезание резьбы метчиком, отвод на рабочей подаче
	double G85(); // Растачивание, развертывание, отвод на рабочей подаче
	double G86(); // Растачивание, рабочая подача, быстрый отвод
	double G87(); // Растачивание, рабочая подача, отвод в вручную
	double G88(); // Растачивание, отвод в вручную
	double G89(); // Растачивание, развертывание, отвод на рабочей подаче
	// группа 07
	double G90(); // Задание в абсолютных величинах
	double G91(); // Задание в приращениях
	// группа 00
	double G92(); // Задание системы координат
	// группа 08
	double G94(); // Задание подачи в мм/мин
	// группа 00
	double G60(); // Одностороннее позиционирование
	// группа 09
	double G37(); // Задание токарного режима
	double G36(); // Отмена токарного режима
};

// Рабочий поток транслятора
UINT InterpreterThread(LPVOID data);
// Функция таймера E_TIMER
void CALLBACK TimerProc(HWND hWnd,		// handle of CWnd that called SetTimer
					    UINT nMsg,		// WM_TIMER
						UINT nIDEvent,	// timer identification
						DWORD dwTime);	// system time
							  

#endif /* _INC_FINTERPRETER_43E83C3102DA_INCLUDED */

//	Конец файла FInterpreter.h
///////////////////////////////////////////////////////////////////////////////