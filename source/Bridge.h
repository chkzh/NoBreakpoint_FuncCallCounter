#pragma once

#include "back/MyProc.h"
#include "back/MyInject.h"
#include "back/MyDbg.h"
#include "back/DataPicker.h"

#include <QString>

extern DataPicker picker;
extern MyDbg _my_dbg;
extern MyInject _my_inject;
extern MyProc* UsingProc;
extern std::string CurExePath;

extern int CurDataTypeIndex;
extern int CurFilterTypeIndex;
extern int CurSortTypeIndex;

extern QString CurExtraParam1;
extern QString CurExtraParam2;
extern QString CurExtraParam;
extern QString CurPrintCount;

extern bool flag_IsProcessing;

extern int CurChangedCount;

typedef struct _RECORD_INFO_ENTRY
{
	union {
		DWORD cnt;
		float frq;
	}val;

	std::string func_name;
	std::string importer_name;
	std::string exporter_name;

	//∂ÓÕ‚–≈œ¢
	PVOID func_address;
	PVOID overwritten_address;
	PVOID dispatching_entry_address;

}RECORD_INFO_ENTRY;

enum PROC_MODE
{
	PM_INJECT,
	PM_DBG
};

extern PROC_MODE CurMode;

extern std::vector<RECORD_INFO_ENTRY> CurToShowRecords;

#define DT_RECENT_COUNT 0
#define DT_RECENT_FREQUENCY 1
#define DT_ALL_TIME_COUNT 2

#define FT_NO_FILTER 0
#define FT_NON_ZERO_VALUE 1
#define FT_IN_RANGE 2
#define FT_SPECIFIC_VALUE 3
#define FT_SPECIFIC_MULTIPLE 4

#define ST_TOP_DOWN 0
#define ST_BOTTOM_UP 1
#define ST_CLOSE_TO_SPECIFIC_VALUE 2
#define ST_CLOSE_TO_SPECIFIC_MULTIPLE 3

bool bkSwapMode(PROC_MODE mode);

bool bkSetDataType(int index);
bool bkSetSortType(int index, QString& param, QString& count);
bool bkSetFilterType(int index, QString& param1, QString& param2);

void bkClose();
bool bkOpen();
bool bkStart();
bool bkRestart();
bool bkDetach();
bool bkInstall();
bool bkUninstall();

void bkRefresh();

void bkQurey(int index, QString& extra_info);