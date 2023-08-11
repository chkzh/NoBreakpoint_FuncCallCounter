#pragma once
#include "GlobalStruct.h"
#include "CodeString.h"
#include <windows.h>



typedef struct GET_PROC_ADDR_TASK_ENTRY
{
	PVOID CallerAddr;
	PVOID ProcAddr;
	PVOID HookedAddr;
	DWORD RecordIndex;	//CallRecord索引，绝对不可能超过65535

}GET_PROC_ADDR_TASK_ENTRY;

/*

	远端进程中一个完整的分发表

*/
class Dispatcher
{
private:
	HANDLE ProcHandle;
	PVOID DispatcherEntryPoint;
	PVOID TextSectionAddr;
	PVOID DataSectionAddr;

	PUCHAR CurTextPtr;
	PUCHAR CurDataPtr;

	DWORD TextSize;
	DWORD DataSize;
	DWORD CommittedCount;	//当前表单项的个数
	DWORD Capability;		//容量
	//DWORD ExpectedCount;

	bool IsWow64;

	CodeString DispatcherHead;
	CodeString DispatchingEntry;

public:

	Dispatcher(HANDLE proc_handle, bool is_wow_64)
	{
		this->IsWow64 = is_wow_64;
		
		this->ProcHandle = proc_handle;
		this->DispatcherEntryPoint = NULL;
		this->TextSectionAddr = NULL;
		this->DataSectionAddr = NULL;

		this->CurTextPtr = NULL;
		this->CurDataPtr = NULL;

		//this->ExpectedCount = desired_capability;

		this->CommittedCount = 0;
		this->Capability = 0;

		this->TextSize = 0;
		this->DataSize = 0;

		if (this->IsWow64)
			Wow64_InitAsDispatcherHead(DispatcherHead);
		else
			InitAsDispatcherHead(DispatcherHead);
		InitAsDispatchingEntry(DispatchingEntry);

	}

	~Dispatcher()
	{
		this->clear(true);
	}

	bool init(DWORD desired_capability);

	void clear(bool do_virtual_free_or_not)
	{
		if (do_virtual_free_or_not)
		{
			VirtualFreeEx(this->ProcHandle, this->DataSectionAddr, 0, MEM_RELEASE);
			VirtualFreeEx(this->ProcHandle, this->TextSectionAddr, 0, MEM_RELEASE);
			this->DataSectionAddr = NULL;
			this->TextSectionAddr = NULL;
			this->Capability = 0;
		}

		this->CurDataPtr = (PUCHAR)this->DataSectionAddr;
		this->CurTextPtr = (PUCHAR)this->TextSectionAddr;

		this->CommittedCount = 0;
	}

	//给处地址，返还分发项地址，NULL代表失败
	PVOID commit(PVOID function_address);

	DWORD free_count()
	{
		return this->Capability - this->CommittedCount;
	}

	DWORD committed_count()
	{
		return this->CommittedCount;
	}

	PVOID data()
	{
		return this->DataSectionAddr;
	}

	PVOID locateDispatchingEntry(DWORD index)
	{
		if (index >= CommittedCount)
			return NULL;

		return (PUCHAR)this->TextSectionAddr + DispatcherHead.size() + DispatchingEntry.size() * index;
	}
};


/*

	针对注入模式出现的分发器

	* 只能有一个分发器
	* 仅针对导入表钩子
	* 钩子可卸载

*/
class StaticDispatcherHolder
{
private:
	HANDLE ProcHandle;
	bool IsWow64;

	Dispatcher disp;
	

public:
	//DWORD LastUpdateTime;
	//DWORD CurUpdateTime;

	bool IsTaskCommitted;

	std::vector<CALL_DATA_ENTRY> CallRecords;	//代表记录，与远端对应
	std::vector<MODULE_IAT_PACK> IatHookTasks;	//代表待钩取的任务
	
	StaticDispatcherHolder(HANDLE ProcHandle, bool is_wow_64)
		:disp(ProcHandle, is_wow_64)
	{
		this->ProcHandle = ProcHandle;

		//this->LastUpdateTime = 0;
		//this->CurUpdateTime = GetTickCount();
		
		this->IsWow64 = is_wow_64;

		this->IsTaskCommitted = false;
	}

	~StaticDispatcherHolder()
	{
		clear(true);
	}

	bool addIatTask(MODULE_IAT_PACK& task)
	{
		if (this->IsTaskCommitted)
			return false;

		this->IatHookTasks.push_back(task);
		return true;
	}

	//提交IAT钩子任务
	bool commit();

	//更新本端数据
	void update();
	
	//不推荐使用！
	bool tryCommitExtraTask(MODULE_IAT_PACK& task);

	//重置，销毁任务
	void clear(bool remove_hooks_or_not = true);

	PVOID locateDispatchingEntry(DWORD index)
	{
		return this->disp.locateDispatchingEntry(index);
	}

private:

	void uninstallHooks(int TaskIndex);	//修复IAT钩子

	void installHooks(int TaskIndex);

	void installDispatchingEntry(int TaskIndex);
};

/*

	针对调试模式出现的分发器

使用情境：
	* 调试器环境
	* 需要快速处理单个任务

*/
class DynamicDispatcherHolder
{
private:
	bool IsWow64;
	
	DWORD GpaCount;
	DWORD IatCount;

	HANDLE ProcHandle;
	Dispatcher* CurDisp;

	std::vector<Dispatcher*> p_Disps;	//把一个填满了再创建下一个

public:
	//DWORD LastUpdateTime;
	//DWORD CurUpdateTime;

	std::vector<CALL_DATA_ENTRY> CallRecords;
	std::vector<GET_PROC_ADDR_TASK_ENTRY> GpaTasks;
	std::vector<MODULE_IAT_PACK> IatTasks;

	DynamicDispatcherHolder(HANDLE hProc, bool is_wow_64)
	{
		this->ProcHandle = hProc;
		this->IsWow64 = is_wow_64;

		this->GpaCount = 0;
		this->IatCount = 0;

		//this->LastUpdateTime = 0;
		//this->CurUpdateTime = 0;

		this->CurDisp = NULL;
	}

	void init(int expected_iat_count, int expected_gpa_count);

	bool commit(MODULE_IAT_PACK& task);

	bool commit(PVOID CallerAddr, PVOID OrigAddr, PVOID* HookedAddr);

	void update();

	PVOID locateDispatchingEntry(DWORD index);

private:
	bool spawnNewDispatcher(int desired_count);
};