#pragma once
#include <windows.h>
#include <string>
#include <map>
#include <unordered_set>
#include <thread>

#include "MyProc.h"
#include "ModuleParser.h";
#include "DispatcherHolder.h";


struct BreakPoint
{
	unsigned char ori_byte;
	//unsigned char replace_byte;
	bool is_enabled;
	//bool is_only_once;
	std::string comment;	//被用于
};

struct Thread
{
	UINT id;
	HANDLE handle;
	PVOID last_breakpoint_to_fix;
};

//struct DEBUG_THREAD_ROUTINE_PARAM
//{
//	char* lpExePath;
//	PVOID _this;
//};

/*

	调试器框架

作用：
	控制调试器模式的整个流程

细节：
	1. 使用导出表破坏的方式钩取 GetProcAddress，在GetProcAddress的返回处下断点，钩取地址值（需要考虑NULL的情况），可以使用一次性断点
	2. 在DLL加载时间中，在DllMain处下断点（一次性断点）

*/
class MyDbg : public MyProc
{
private:
	//X86 - 64兼容
	bool IsWow64;
	bool IsSysBreakPointReached;

	bool KillRequsetFlag;
	bool DetachRequestFlag;

	//循环线程
	HANDLE LoopThreadHandle;
	DWORD LoopThreadId;

	//基础进程信息
	HANDLE hProc;
	HANDLE hMainThread;
	UINT ProcId;
	UINT MainThreadId;
	PVOID ExeImageBase;
	PVOID pPeb;
	std::string ExeFilePath;

	//导出表破坏 - GetProcAddress
	PVOID CrashedGetProcAddr;
	PVOID OriGetProcAddr;

	//进程拓展信息
	std::map<PVOID, BreakPoint> breakpoint_book;
	std::map<UINT, Thread> thread_book;

	//Gpa识别跟踪
	PVOID GpaBreakPoint;
	std::unordered_set<PVOID> GetProcAddr_RetnVals;

	//辅助模块
	ModuleParser* parser;
	DynamicDispatcherHolder* holder;

	//必须的线程同步：
	CRITICAL_SECTION CallRecordLock;
	CRITICAL_SECTION ModuleInfoLock;

	

public:



	MyDbg()
		:MyProc()
	{
		IsWow64 = false;
		IsSysBreakPointReached = false;

		this->KillRequsetFlag = false;

		this->LoopThreadHandle = NULL;
		this->LoopThreadId = NULL;

		hProc = NULL;
		hMainThread = NULL;
		ProcId = 0;
		MainThreadId = 0;
		ExeImageBase = NULL;
		pPeb = NULL;

		parser = NULL;
		holder = NULL;

		CrashedGetProcAddr = NULL;
		OriGetProcAddr = NULL;

		InitializeCriticalSection(&CallRecordLock);
		InitializeCriticalSection(&ModuleInfoLock);

	}

	~MyDbg()
	{
		DeleteCriticalSection(&CallRecordLock);
		DeleteCriticalSection(&ModuleInfoLock);

		if (parser)
			delete parser;
		if (holder)
			delete holder;
	}

	BreakPoint* setBreakPoint(PVOID addr, const char* comment);
	void removeBreakPoint(PVOID addr);
	void enableBreakPoint(PVOID addr, bool is_enable);

	std::string getComment(PVOID addr);

	//【接口】：
	void getDetails(int RecordIndex, CALL_RECORD_DETAILS& details) override;//【同步】
	std::vector<CALL_DATA_ENTRY>* visitLatestRecords() override;//【同步】
	void leaveCallRecords() override;//【同步】
	PVOID locateEntry(int RecordIndex) override;//【同步】

	bool open(const char* lpExePath) override;
	void stop() override;
	void detach() override;

	bool install() override;	//【禁用】
	bool uninstall() override;	//【禁用】

private:

	//【线程函数】
	static bool _debug_thread_routine(MyDbg* p_param);
	int _handle_debug_event(int wait_time);

	void printCrashInfo(DEBUG_EVENT& de);

	bool EnableDebugPrivilege(bool enable_or_not);
	void StepOverBreakPoint(DEBUG_EVENT& de, bool need_to_recover);	//目前仅支持一次性断点

	//调试事件处理
	bool TryHandleGetProcAddrHook(DEBUG_EVENT& de);
	bool TryHandleDllMainBreakPoint(DEBUG_EVENT& de);

	//bool IsHookTarget(PVOID SusAddress);
	bool CrashExport(PVOID DllImageBase, const char* lpFunctionName, PVOID* CrashedAddr, PVOID* OriAddr);
	bool HookGetProcAddr(PVOID Kernel32BaseAddr);

	void HideDebugger(bool hide_or_unhide);

	////【接口】
	//void detach() override;
};

#define DE_HANDLED 2
#define DE_NOT_HANDLED 1
#define DE_TIME_OUT 0
#define DE_EXIT -1