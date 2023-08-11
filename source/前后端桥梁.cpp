#include "bridge.h"
#include <QString>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")// 该指令仅支持VS环境
#endif

DataPicker picker;
MyDbg _my_dbg;
MyInject _my_inject;

MyProc* UsingProc;
PROC_MODE CurMode = PROC_MODE::PM_INJECT;
std::string CurExePath;

int CurDataTypeIndex = 0;
int CurFilterTypeIndex = -1;
int CurSortTypeIndex = -1;

int sort_count = 30;

QString CurExtraParam1;
QString CurExtraParam2;
QString CurExtraParam;
QString CurPrintCount;

bool flag_IsProcessing = false;

std::vector<RECORD_INFO_ENTRY> CurToShowRecords;
int CurChangedCount;

bool bkSwapMode(PROC_MODE mode)
{
	if (flag_IsProcessing)
		return false;

	//if (mode != CurMode)
	{
		if (mode == PM_INJECT)
		{
			UsingProc = &_my_inject;
			picker.bind(UsingProc);
			printf("已将模式设置为：Inject\n");
		}
		
		if (mode == PM_DBG)
		{
			UsingProc = &_my_dbg;
			picker.bind(UsingProc);
			printf("已将模式设置为：Debug\n");
		}

		CurMode = mode;
	}
	return true;
}

bool bkSetDataType(int index)
{

	PICK_DATA_TYPE to_check = Count_Recent;
	switch (index)
	{
	case DT_RECENT_COUNT:
		to_check = PICK_DATA_TYPE::Count_Recent;
		break;
	case DT_RECENT_FREQUENCY:
		to_check = PICK_DATA_TYPE::Frequency_Recent;
		break;
	case DT_ALL_TIME_COUNT:
		to_check = PICK_DATA_TYPE::Count_All_Time;
		break;
	}

	picker.setDataFlag(to_check);

	//CurDataTypeIndex = index;

	return true;
}

bool bkSetFilterType(int index, QString& param1, QString& param2)
{
	switch (index)
	{
	case FT_NO_FILTER:
	case FT_NON_ZERO_VALUE:
		break;

	case FT_SPECIFIC_VALUE:
	case FT_SPECIFIC_MULTIPLE:
		if (param1.isEmpty())
			return false;
		break;

	case FT_IN_RANGE:
		if (param1.isEmpty() || param2.isEmpty())
			return false;
		break;
	}

	FILTER_TYPE ft = No_Filter;
	switch (index)
	{
	case FT_NO_FILTER: ft = No_Filter; break;
	case FT_NON_ZERO_VALUE: ft = Non_Zero_Value; break;
	case FT_IN_RANGE:ft = In_Range_Value; break;
	case FT_SPECIFIC_VALUE:ft = Specific_Value; break;
	case FT_SPECIFIC_MULTIPLE:ft = Specific_Multiple; break;
	}

	if (CurDataTypeIndex == DT_RECENT_FREQUENCY)
	{
		float p1 = param1.toFloat();
		float p2 = param2.toFloat();
		picker.setFilterFlag(ft, (float)p1, p2);
	}
	else
	{
		int p1 = param1.toInt();
		int p2 = param2.toInt();
		picker.setFilterFlag(ft, (DWORD)p1, p2);
	}

	CurFilterTypeIndex = index;
	CurExtraParam1 = param1;
	CurExtraParam2 = param2;

	//picker.setFilterFlag(ft, )

	return true;
}

bool bkSetSortType(int index, QString& param, QString& count)
{
	if (count.isEmpty())
		return false;

	switch (index)
	{
	case ST_BOTTOM_UP:
	case ST_TOP_DOWN:
		break;
	case ST_CLOSE_TO_SPECIFIC_VALUE:
	case ST_CLOSE_TO_SPECIFIC_MULTIPLE:
		if (param.isEmpty())
			return false;
	}

	SORT_TYPE st = Top_Down;
	switch (index)
	{
	case ST_BOTTOM_UP:st = Bottom_Up; break;
	case ST_TOP_DOWN:st = Top_Down; break;
	case ST_CLOSE_TO_SPECIFIC_VALUE:st = Close_To_Specific_Value; break;
	case ST_CLOSE_TO_SPECIFIC_MULTIPLE:st = Close_To_Specific_Multiple; break;
	}

	if (CurDataTypeIndex == DT_RECENT_FREQUENCY)
	{
		float p = param.toFloat();
		picker.setSortFlag(st, (float)p);
	}
	else
	{
		int p = param.toInt();
		picker.setSortFlag(st, (DWORD)p);
	}

	CurSortTypeIndex = index;
	CurExtraParam = param;
	CurPrintCount = count;

	sort_count = count.toInt();

	return true;
}

bool bkOpen()
{
	if (CurExePath.empty())
		return false;

	if (flag_IsProcessing)
		return false;

	if (!UsingProc->open(CurExePath.c_str()))
	{
		printf("打开文件失败！，文件名：%s\n", CurExePath.c_str());
		return false;
	}

	flag_IsProcessing = true;
	return true;
}

void bkClose()
{
	if (!flag_IsProcessing)
		return;

	UsingProc->stop();
	return;
}

bool bkStart()
{
	if (flag_IsProcessing)
		return false;

	if (CurExePath.empty())
		return false;

	if (!UsingProc->open(CurExePath.c_str()))
	{
		printf("打开文件失败！文件名：%s\n", CurExePath.c_str());
		return false;
	}

	flag_IsProcessing = true;
	return true;
}

bool bkRestart()
{
	if (flag_IsProcessing)
		UsingProc->stop();

	if (!UsingProc->open(CurExePath.c_str()))
	{
		printf("打开文件失败！，文件名：%s\n", CurExePath.c_str());
		return false;
	}

	flag_IsProcessing = true;
	return true;
}

bool bkDetach()
{
	if (flag_IsProcessing)
		UsingProc->detach();

	return true;
}

bool bkInstall()
{
	UsingProc->install();
	return true;
}

bool bkUninstall()
{
	UsingProc->uninstall();
	return true;
}

void bkRefresh()
{
	CurToShowRecords.clear();

	picker.pick();
	picker.sort(sort_count);
	auto list = picker.data();
	CALL_RECORD_DETAILS details;
	RECORD_INFO_ENTRY info;
	for (int i = 0; i < list.size(); i++)
	{
		UsingProc->getDetails(list[i].TblIndex, details);
		if (CurDataTypeIndex == DT_RECENT_FREQUENCY)
			info.val.frq = list[i].CmpVal.frq;
		else
			info.val.cnt = list[i].CmpVal.cnt;

		info.func_name = details.funcName;
		info.importer_name = details.importerName;
		info.exporter_name = details.exporterName;
		info.func_address = details.func_address;
		info.overwritten_address = details.overwritten_address;
		info.dispatching_entry_address = details.dispatching_entry_address;

		CurToShowRecords.push_back(info);
	}

	CurChangedCount = picker.ChangedCount;
}

void bkQurey(int index, QString& extra_info)
{
	if (index + 1 > CurToShowRecords.size())
	{
		extra_info = "? ? ?";
		return;
	}

	PVOID func_addr = CurToShowRecords[index].func_address;
	PVOID entry_locate = CurToShowRecords[index].dispatching_entry_address;
	PVOID overwritten_addr = CurToShowRecords[index].overwritten_address;

	QString str_func_addr = "0x" + QString::number((UPVOID)func_addr, 16);
	QString str_entry_locate = "0x" + QString::number((UPVOID)entry_locate, 16);
	QString str_overwritten_addr = "0x" + QString::number((UPVOID)overwritten_addr, 16);

	extra_info = "函数地址：" + str_func_addr + "\n" + "分发表地址：" + str_entry_locate + "\n" + "钩取发生位置：" + str_func_addr;
	return;
}