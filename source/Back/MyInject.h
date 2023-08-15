#pragma once
#include <windows.h>
#include "DispatcherHolder.h"
#include "ModuleParser.h"
#include "MyProc.h"

//进程快照，用于搜索目标进程
typedef struct _PROCESS_SNAPSHOT
{
	UINT ProcessId;
	UINT ParentProcessId;
	std::string ExePath;
	std::string ExeName;
	std::string WinText;
}PROCESS_SNAPSHOT;


class MyInject : public MyProc
{
private:
	bool IsWow64;

	//DWORD HookTargetFlag;

	HANDLE hProc;
	UINT ProcId;
	PVOID ExeImageBase;

	StaticDispatcherHolder* holder;
	ModuleParser* parser;

	//static DWORD NeededProcessAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE;

public:
	std::vector<PROCESS_SNAPSHOT> snapshot;
	std::vector<PVOID> address;

	MyInject()
	{
		IsWow64 = false;

		HookTargetFlag = 0;

		hProc = NULL;
		ProcId = -1;
		ExeImageBase = NULL;
		holder = NULL;
		parser = NULL;
		
	}

	bool attach(UINT pid);

	//【接口】
	std::vector<CALL_DATA_ENTRY>* visitLatestRecords() override;
	void leaveCallRecords() override;

	void getDetails(int RecordIndex, CALL_RECORD_DETAILS& details) override;
	PVOID locateEntry(int RecordIndex) override;
	bool open(const char* lpExePath) override;
	void stop() override;
	void detach() override;	//【禁用】
	bool install() override;
	bool uninstall() override;
	//void setHookTarget(DWORD flag) override;

	DWORD checkProcessStatus() override;

private:

};