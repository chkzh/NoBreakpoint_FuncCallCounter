#include "MyInject.h"
#include "GlobalFuncAndVar.h"

bool MyInject::attach(UINT pid)
{
	HANDLE result = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, false, pid);
	if (!result)
		return false;

	this->hProc = result;
	this->ProcId = pid;
	this->ExeImageBase = GetProcImageBase(this->hProc, NULL);
	this->IsWow64 = IsWow64ModeOn(this->ProcId);
	
	parser = new ModuleParser(this->hProc, this->IsWow64, this->ExeImageBase);
	holder = new StaticDispatcherHolder(this->hProc, this->IsWow64);

	return true;
}

void MyInject::detach()
{
	return;
}

//bool MyInject::install()
//{
//	return false;
//}

//void MyInject::setHookTarget(DWORD flag)
//{
//	this->HookTargetFlag = flag;
//}

bool MyInject::open(const char* lpExePath)
{
	if (this->hProc)
		this->stop();

	bool result;

	if (!lpExePath)
		return false;

	char lpCurDirectory[MAX_PATH] = { 0 };
	char* lp_end = (char*)strrchr(lpExePath, '\\');
	if (lp_end == NULL)
		lp_end = (char*)strrchr(lpExePath, '/');
	if (lp_end == NULL)
		return false;

	int path_length = lp_end - lpExePath;
	if (path_length <= 0)
		return false;

	memcpy(lpCurDirectory, lpExePath, path_length);

	STARTUPINFOA sia;
	ZeroMemory(&sia, sizeof(STARTUPINFOA));

	PROCESS_INFORMATION pi;
	//ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	result = CreateProcessA(lpExePath, NULL, NULL, NULL, false, NULL, NULL, (LPCSTR)lpCurDirectory, &sia, &pi);

	if (!result)
		return false;

	this->ProcessStatusFlag |= PS_PROCESS_AVAILABLE;

	this->hProc = pi.hProcess;
	this->ProcId = pi.dwProcessId;
	this->ExeImageBase = GetProcImageBase(this->hProc, NULL);
	this->IsWow64 = IsWow64ModeOn(this->ProcId);
	CloseHandle(pi.hThread);

	parser = new ModuleParser(this->hProc, this->IsWow64, this->ExeImageBase);
	holder = new StaticDispatcherHolder(this->hProc, this->IsWow64);

	return true;
}

void MyInject::stop()
{
	if (hProc) {
		TerminateProcess(this->hProc, 0);
		CloseHandle(this->hProc);
		this->hProc = NULL;

		this->ProcessStatusFlag &= ~PS_PROCESS_AVAILABLE;
	}
	this->ProcId = -1;
	this->ExeImageBase = NULL;
	this->IsWow64 = false;

	delete parser;
	delete holder;
	parser = NULL;
	holder = NULL;
}

//void MyInject::close()
//{
//	if (hProc) {
//		CloseHandle(this->hProc);
//		TerminateProcess(this->hProc, 0);
//		this->hProc = NULL;
//	}
//	this->ProcId = -1;
//	this->ExeImageBase = NULL;
//	this->IsWow64 = false;
//
//	delete parser;
//	delete holder;
//	parser = NULL;
//	holder = NULL;
//}


bool MyInject::uninstall()
{
	holder->clear(true);
	return true;
}

DWORD MyInject::checkProcessStatus()
{
	WORD magic;
	if (!ReadProcessMemory(this->hProc, this->ExeImageBase, &magic, sizeof(WORD), NULL))
	{
		this->ProcessStatusFlag &= ~PS_PROCESS_AVAILABLE;
	}

	return this->ProcessStatusFlag;
}

//void MyInject::getSnapshot()
//{
//	;
//}

//bool MyInject::IsHookTarget(MODULE_INFO& info)
//{
//	if ((HookTargetFlag & HM_EXE) && info.Source == Exe)
//		return true;
//	else if ((HookTargetFlag & HM_EXE_DIRECTORY_DLLS) && info.Source == Exe_Directory)
//		return true;
//	else if ((HookTargetFlag & HM_OTHER_DIRECTORY_DLLS) && info.Source == Other_Directory)
//		return true;
//	else if ((HookTargetFlag & HM_SYSTEM_DIRECTORY_DLLS) && info.Source == System_Directory)
//		return true;
//
//	return false;
//}

bool MyInject::install()
{
	//if hook IAT is disabled;
	if ((this->HookTypeFlag & HT_IMPORT_ADDRESS_TABLE) == 0)
		return true;

	parser->walkAddressSpace();

	auto ite = this->parser->ModuleInfoBook.begin();
	while (ite != this->parser->ModuleInfoBook.end())
	{
		if (IsHookTarget(ite->second))
		{
			MODULE_IAT_PACK task;
			parser->walkIatTable(ite->first, &task);
			holder->addIatTask(task);
		}

		ite++;
	}

	holder->commit();

	return true;
}

//std::string MyInject::getComment(PVOID address)
//{
//	std::string str;
//	parser->getComment(address, str);
//	return str;
//}

std::vector<CALL_DATA_ENTRY>* MyInject::visitLatestRecords()
{
	//if (!(this->ProcessStatusFlag & PS_PROCESS_AVAILABLE))
	//	return NULL;

	this->holder->update();
	return &(this->holder->CallRecords);
}

void MyInject::leaveCallRecords()
{
	return;
}

void MyInject::getDetails(int RecordIndex, CALL_RECORD_DETAILS& details)
{
	CALL_DATA_ENTRY& ent = this->holder->CallRecords[RecordIndex];

	details.func_address = ent.FunctionAddress;
	details.overwritten_address = ent.OverwrittenAddress;
	details.dispatching_entry_address = this->locateEntry(RecordIndex);

	auto ite_1 = parser->ApiInfoBook.find(ent.FunctionAddress);
	if (ite_1 != parser->ApiInfoBook.end())
	{
		if (ite_1->second.ApiName.empty())
			details.funcName = "Oridinal_#" + std::to_string(ite_1->second.Oridinal);
		else
			details.funcName = ite_1->second.ApiName;
	}
	else
		details.funcName = "( ? )";

	MODULE_INFO* p_info = parser->querySource(ent.OverwrittenAddress);
	if (p_info)
		details.importerName = p_info->ModuleName;
	else
		details.importerName = "( ? )";

	p_info = parser->querySource(ent.FunctionAddress);
	if (p_info)
		details.exporterName = p_info->ModuleName;
	else
		details.exporterName = "( ? )";

	details.source = RECORD_SOURCE::RS_ImportAddressTable;
}

PVOID MyInject::locateEntry(int RecordIndex)
{
	if (holder)
		return this->holder->locateDispatchingEntry(RecordIndex);
	else
		return NULL;
}
