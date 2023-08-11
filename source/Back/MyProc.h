#pragma once
#include <string>
#include <windows.h>
#include <vector>
#include "GlobalStruct.h"

#define HM_EXE 0x1
#define HM_EXE_DIRECTORY_DLLS 0x2
#define HM_OTHER_DIRECTORY_DLLS 0x4
#define HM_SYSTEM_DIRECTORY_DLLS 0x8

#define HT_IMPORT_ADDRESS_TABLE 0x1
#define HT_GET_PROC_ADDRESS 0x2

class MyProc
{
protected:
	DWORD HookTargetFlag;
	DWORD HookTypeFlag;

public:
	
	MyProc()
	{
		HookTargetFlag = 0;
		HookTypeFlag = 0;
	}

	virtual ~MyProc()
	{
		;
	}

	//线程同步
	virtual std::vector<CALL_DATA_ENTRY>* visitLatestRecords() = 0;
	virtual void leaveCallRecords() = 0;

	virtual void getDetails(int RecordIndex, CALL_RECORD_DETAILS& details) = 0;
	virtual PVOID locateEntry(int RecordIndex) = 0;
	virtual bool open(const char* lpExePath) = 0;
	virtual void stop() = 0;
	virtual void detach() = 0;
	virtual bool install() = 0;
	virtual bool uninstall() = 0;
	virtual void setHookTarget(DWORD flag)
	{
		this->HookTargetFlag = flag;
	}
	virtual void setHookType(DWORD flag)
	{
		this->HookTypeFlag = flag;
	}


protected:
	virtual bool IsHookTarget(MODULE_INFO& info)
	{
		if ((HookTargetFlag & HM_EXE) && info.Source == Exe)
			return true;
		else if ((HookTargetFlag & HM_EXE_DIRECTORY_DLLS) && info.Source == Exe_Directory)
			return true;
		else if ((HookTargetFlag & HM_OTHER_DIRECTORY_DLLS) && info.Source == Other_Directory)
			return true;
		else if ((HookTargetFlag & HM_SYSTEM_DIRECTORY_DLLS) && info.Source == System_Directory)
			return true;

		return false;
	}
};

/*

	待实现功能：

	* 启动进程
	* 定位“分发项的地址”
	* 终止进程
	* 脱离调试
	* 设置钩取模块等级
	---------------------
	* 

*/


