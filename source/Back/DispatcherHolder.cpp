#include "DispatcherHolder.h"

/*

	第一次在远端构建分发表，完成对数据的初始化

	（尽量在一次将它们全部完成）

*/
bool StaticDispatcherHolder::commit()
{
	if (IsTaskCommitted)
		return false;

	DWORD entry_count = 0;
	auto ite_task = this->IatHookTasks.begin();
	while (ite_task != this->IatHookTasks.end())
	{
		entry_count += ite_task->IatHookList.size();
		ite_task++;
	}
	if (!entry_count)
		return false;

	this->disp.init(entry_count);

	for (int i = 0; i < this->IatHookTasks.size(); i++)
		installDispatchingEntry(i);

	for (int i = 0; i < this->IatHookTasks.size(); i++)
		installHooks(i);

	this->IsTaskCommitted = true;

	return true;
}

/*

	更新时间

*/
void StaticDispatcherHolder::update()
{
	if (!this->CallRecords.size())
		return;

	DWORD update_count = 0;

	//读取远端数据
	REMOTE_CALL_DATA_ENTRY* lp_buffer = new REMOTE_CALL_DATA_ENTRY[this->CallRecords.size()];
	ReadProcessMemory(this->ProcHandle, this->disp.data(), lp_buffer, sizeof(REMOTE_CALL_DATA_ENTRY) * this->CallRecords.size(), NULL);

	//修改本进程数据
	for (int i = 0; i < this->CallRecords.size(); i++){

		if (!this->CallRecords[i].FunctionAddress || !this->CallRecords[i].OverwrittenAddress)
			continue;

		this->CallRecords[i].LastCallCount = this->CallRecords[i].CurCallCount;
		this->CallRecords[i].CurCallCount = lp_buffer[i].CallCount;

		if (this->CallRecords[i].CurCallCount != this->CallRecords[i].LastCallCount)
			update_count++;
	}

	//this->LastUpdateTime = this->CurUpdateTime;
	//this->CurUpdateTime = GetTickCount();

	delete[] lp_buffer;

	printf("\n正在更新数据。\n");
	printf("\t累计 %d 项发生变化。\n", update_count);
}

void StaticDispatcherHolder::uninstallHooks(int TaskIndex)
{
	if (TaskIndex >= this->IatHookTasks.size())
		return;

	MODULE_IAT_PACK& task = this->IatHookTasks[TaskIndex];
	DWORD old_protect, new_protect = PAGE_READWRITE;
	PUCHAR iat_address = (PUCHAR)task.DllImageBase + task.IatTableRva;
	VirtualProtectEx(this->ProcHandle, iat_address, task.IatTableSize, new_protect, &old_protect);

	for (int i = task.BeginIndex; i <= task.EndIndex; i++)
	{
		WriteProcessMemory(this->ProcHandle, this->CallRecords[i].OverwrittenAddress, &this->CallRecords[i].FunctionAddress, sizeof(PVOID), NULL);
		this->CallRecords[i].OverwrittenAddress = NULL;
		this->CallRecords[i].FunctionAddress = NULL;
	}

	VirtualProtectEx(this->ProcHandle, iat_address, task.IatTableSize, old_protect, &new_protect);
}

void StaticDispatcherHolder::clear(bool remove_hooks_or_not)
{
	this->disp.clear(true);

	this->CallRecords.clear();
	this->IatHookTasks.clear();

	this->IsTaskCommitted = false;
}

bool StaticDispatcherHolder::tryCommitExtraTask(MODULE_IAT_PACK& task)
{
	if (task.IatHookList.size() > this->disp.free_count() || !this->IsTaskCommitted)
		return false;

	this->IatHookTasks.push_back(task);
	installDispatchingEntry(this->IatHookTasks.size() - 1);
	installHooks(this->IatHookTasks.size() - 1);

	return true;
}

void StaticDispatcherHolder::installDispatchingEntry(int TaskIndex)
{
	MODULE_IAT_PACK& task = this->IatHookTasks[TaskIndex];
	CALL_DATA_ENTRY ent;
	auto ite = task.IatHookList.begin();
	while (ite != task.IatHookList.end())
	{
		ent.CurCallCount = 0;
		ent.LastCallCount = 0;
		ent.FunctionAddress = ite->OriFuncEntry;
		ent.OverwrittenAddress = ite->ToWriteAddress;
		this->CallRecords.push_back(ent);

		ite->HookedAddress = this->disp.commit(ite->OriFuncEntry);
		ite++;
	}
}

void StaticDispatcherHolder::installHooks(int TaskIndex)
{
	DWORD old_protect, new_protect = PAGE_READWRITE;
	MODULE_IAT_PACK& task = this->IatHookTasks[TaskIndex];
	PUCHAR iat_address = (PUCHAR)task.DllImageBase + task.IatTableRva;
	
	VirtualProtectEx(this->ProcHandle, iat_address, task.IatTableSize, new_protect, &old_protect);

	auto ite = task.IatHookList.begin();
	while (ite != task.IatHookList.end())
	{
		bool result = false;
		if (IsWow64)
			result = WriteProcessMemory(this->ProcHandle, ite->ToWriteAddress, &ite->HookedAddress, sizeof(ULONG32), NULL);
		else
			result = WriteProcessMemory(this->ProcHandle, ite->ToWriteAddress, &ite->HookedAddress, sizeof(ULONG64), NULL);
		
		if (!result)
		{
			printf("严重错误：修改IAT失败！\n");
			getchar();
		}

		ite++;
	}

	VirtualProtectEx(this->ProcHandle, iat_address, task.IatTableSize, old_protect, &new_protect);
}

bool Dispatcher::init(DWORD desired_capability)
{
	this->DataSize = sizeof(REMOTE_CALL_DATA_ENTRY) * desired_capability;
	this->TextSize = this->DispatcherHead.size() + this->DispatchingEntry.size() * desired_capability;

	this->TextSectionAddr = VirtualAllocEx(ProcHandle, NULL, this->TextSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READ);
	this->DataSectionAddr = VirtualAllocEx(ProcHandle, NULL, this->DataSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!this->TextSectionAddr || !this->DataSectionAddr)
		return false;

	this->DispatcherEntryPoint = this->TextSectionAddr;

	this->CurTextPtr = (PUCHAR)this->TextSectionAddr;
	this->CurDataPtr = (PUCHAR)this->DataSectionAddr;

	this->TextSize = (this->TextSize + 0x1000 - 1) & ~(0x1000 - 1);
	this->DataSize = (this->DataSize + 0x1000 - 1) & ~(0x1000 - 1);

	DWORD text_cnt = (this->TextSize - this->DispatcherHead.size()) / this->DispatchingEntry.size();
	DWORD data_cnt = this->DataSize / sizeof(REMOTE_CALL_DATA_ENTRY);
	this->Capability = text_cnt < data_cnt ? text_cnt : data_cnt;

	if (IsWow64)
		Wow64_RelocateDispatcherHead(this->DispatcherHead, (UPVOID)DataSectionAddr);
	else
		RelocateDispatcherHead(this->DispatcherHead, (UPVOID)DataSectionAddr);

	/*
		[BUG RECORD 2023-8-15]: 
		In dllmain of comctl32.dll, "Call ds ptr: <GetVersionExW>" to an ERW-protect page will cause ACCESS_VIOLATION
	*/
	DWORD new_protect = PAGE_EXECUTE_READWRITE, old_protect;
	VirtualProtectEx(this->ProcHandle, this->CurTextPtr, this->DispatcherHead.size(), new_protect, &old_protect);
	WriteProcessMemory(this->ProcHandle, this->CurTextPtr, this->DispatcherHead.data(), this->DispatcherHead.size(), NULL);
	VirtualProtectEx(this->ProcHandle, this->CurTextPtr, this->DispatcherHead.size(), old_protect, &new_protect);
	FlushInstructionCache(this->ProcHandle, this->CurTextPtr, this->DispatcherHead.size());
	this->CurTextPtr += DispatcherHead.size();

	return true;
}

PVOID Dispatcher::commit(PVOID function_address)
{
	if (this->CommittedCount >= this->Capability)
		return NULL;

	PVOID return_address = this->CurTextPtr;

	DWORD entry_rva = (PUCHAR)this->DispatcherEntryPoint - this->CurTextPtr - DispatchingEntry.size();
	RelocateDispatchingEntry(this->DispatchingEntry, this->CommittedCount, entry_rva);

	DWORD new_protect = PAGE_EXECUTE_READWRITE, old_protect;
	bool result = false;
	VirtualProtectEx(this->ProcHandle, this->CurTextPtr, this->DispatchingEntry.size(), new_protect, &old_protect);
	result = WriteProcessMemory(this->ProcHandle, this->CurTextPtr, this->DispatchingEntry.data(), this->DispatchingEntry.size(), NULL);
	VirtualProtectEx(this->ProcHandle, this->CurTextPtr, this->DispatchingEntry.size(), old_protect, &new_protect);
	FlushInstructionCache(this->ProcHandle, this->CurTextPtr, this->DispatchingEntry.size());
	this->CurTextPtr += this->DispatchingEntry.size();

	if (!result)
	{
		printf("严重错误：安装分发项失败！\n");
		getchar();
	}

	REMOTE_CALL_DATA_ENTRY rcde;
	rcde.CallCount = 0;
	rcde.FunctionAddress = function_address;
	WriteProcessMemory(this->ProcHandle, this->CurDataPtr, &rcde, sizeof(rcde), NULL);
	this->CurDataPtr += sizeof(rcde);

	this->CommittedCount++;

	return return_address;
}

void DynamicDispatcherHolder::init(int expected_iat_count, int expected_gpa_count)
{
	DWORD expected_max_count;
	if (!expected_iat_count && !expected_gpa_count)
		expected_max_count = 4096;
	else
		expected_max_count = expected_gpa_count + expected_iat_count;

	this->spawnNewDispatcher(expected_max_count);
}

//默认增长速度：1.5 倍
bool DynamicDispatcherHolder::spawnNewDispatcher(int desired_count)
{
	if (desired_count == 0)
		desired_count = this->CallRecords.size() / 2;

	Dispatcher* disp = new Dispatcher(this->ProcHandle, this->IsWow64);
	bool result = disp->init(desired_count);
	if (result) {
		CurDisp = disp;
		this->p_Disps.push_back(disp);
	}
	else{
		delete disp;
		CurDisp = NULL;
	}

	printf("\nDynamic_Dispatcher_Holder:\n\t已生成新的分发器，大小：%d\n", CurDisp->free_count());

	return result;
}

//提交IAT包
bool DynamicDispatcherHolder::commit(MODULE_IAT_PACK& task)
{
	printf("\nDynamic_Dispatcher_Holder:\n\tIAT任务数目：%d\n\t总表：%d -> %d", 
		(DWORD)task.IatHookList.size(), (DWORD)this->CallRecords.size(), (DWORD)this->CallRecords.size() + (DWORD)task.IatHookList.size());

	CALL_DATA_ENTRY ent;

	this->IatTasks.push_back(task);

	DWORD new_protect = PAGE_READWRITE, old_protect;
	PVOID iat_addr = (PUCHAR)task.DllImageBase + task.IatTableRva;
	VirtualProtectEx(this->ProcHandle, iat_addr, task.IatTableSize, new_protect, &old_protect);

	auto ite = this->IatTasks.back().IatHookList.begin();
	while (ite != this->IatTasks.back().IatHookList.end())
	{
		if (!this->CurDisp->free_count())
			this->spawnNewDispatcher(0);

		ite->HookedAddress = CurDisp->commit(ite->OriFuncEntry);

		printf("\t%p: %p -> %p\n", ite->ToWriteAddress, ite->HookedAddress, ite->OriFuncEntry);

		if (ite->ToWriteAddress == (PVOID)0x83C45C)
		{
			printf("oops!\n");
		}

		bool result = false;

		if(IsWow64)
			result = WriteProcessMemory(this->ProcHandle, ite->ToWriteAddress, &ite->HookedAddress, sizeof(ULONG32), NULL);
		else
			result = WriteProcessMemory(this->ProcHandle, ite->ToWriteAddress, &ite->HookedAddress, sizeof(ULONG64), NULL);

		if (!result)
		{
			printf("严重错误：修改IAT失败！\n");
			getchar();
		}
		
		ent.CurCallCount = 0;
		ent.LastCallCount = 0;
		ent.FunctionAddress = ite->OriFuncEntry;
		ent.OverwrittenAddress = ite->ToWriteAddress;
		this->CallRecords.push_back(ent);

		ite++;
	}
	VirtualProtectEx(this->ProcHandle, iat_addr, task.IatTableSize, old_protect, &new_protect);

	return true;
}

bool DynamicDispatcherHolder::commit(PVOID CallerAddr, PVOID OrigAddr, PVOID* HookedAddr)
{
	GET_PROC_ADDR_TASK_ENTRY gpa;
	CALL_DATA_ENTRY cde;

	if (!this->CurDisp->free_count())
		this->spawnNewDispatcher(0);

	*HookedAddr = CurDisp->commit(OrigAddr);

	gpa.CallerAddr = CallerAddr;
	gpa.ProcAddr = OrigAddr;
	gpa.HookedAddr = *HookedAddr;
	gpa.RecordIndex = this->CallRecords.size();
	this->GpaTasks.push_back(gpa);

	cde.CurCallCount = 0;
	cde.LastCallCount = 0;
	cde.FunctionAddress = OrigAddr;
	cde.OverwrittenAddress = CallerAddr;
	this->CallRecords.push_back(cde);

	printf("\nDynamic_Dispatcher_Holder:\n\t提交一条GetProcAddress Hook。\n");

	return true;
}

void DynamicDispatcherHolder::update()
{
	int index = 0;

	auto ite = this->p_Disps.begin();
	while (ite != this->p_Disps.end())
	{
		Dispatcher& disp = **ite;	//愚蠢的解指针
		REMOTE_CALL_DATA_ENTRY* p_buff = new REMOTE_CALL_DATA_ENTRY[disp.committed_count()];

		ReadProcessMemory(this->ProcHandle, disp.data(), p_buff, disp.committed_count() * sizeof(REMOTE_CALL_DATA_ENTRY), NULL);

		for (int i = 0; i < disp.committed_count(); i++)
		{
			this->CallRecords[index].LastCallCount = this->CallRecords[index].CurCallCount;
			this->CallRecords[index].CurCallCount = p_buff[i].CallCount;
			index++;
		}

		delete[] p_buff;

		ite++;
	}

	//this->LastUpdateTime = this->CurUpdateTime;
	//this->CurUpdateTime = GetTickCount();
}

PVOID DynamicDispatcherHolder::locateDispatchingEntry(DWORD index)
{
	Dispatcher* p_targ = NULL;
	DWORD range_1 = 0, range_2 = 0;
	DWORD r_index = 0;
	for (int i = 0; i < this->p_Disps.size(); i++)
	{
		range_2 = p_Disps[i]->committed_count();
		if (index >= range_1 && index < range_2)
		{
			r_index = index - range_1;
			p_targ = this->p_Disps[i];
			break;
		}

		range_1 = range_2;
	}

	if (!p_targ)
		return NULL;

	return p_targ->locateDispatchingEntry(r_index);
}