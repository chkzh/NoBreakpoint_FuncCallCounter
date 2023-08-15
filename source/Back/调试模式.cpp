#include "MyDbg.h"
#include "CodeString.h"
#include "GlobalFuncAndVar.h"
#include "x96.h"

/*

	以安全的模式创建进程

细节：
	* 应用反反调试
	* 重新设置EXE文件路径

*/
bool MyDbg::open(const char* lpExePath)
{
	if (this->hProc)
		this->stop();

	this->ExeFilePath = lpExePath;
	this->LoopThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)this->_debug_thread_routine, this, NULL, &this->LoopThreadId);
	if (!LoopThreadHandle) {
		printf("创建线程异常！\n");
		return false;
	}

	//bool result;

	//if (!lpExePath)
	//	return false;

	//char lpCurDirectory[MAX_PATH] = { 0 };
	//char* lp_end = (char*)strrchr(lpExePath, '\\');
	//if (lp_end == NULL)
	//	return false;

	//int path_length = lp_end - lpExePath;
	//if (path_length <= 0)
	//	return false;

	//memcpy(lpCurDirectory, lpExePath, path_length);

	////反反调试第一招
	//STARTUPINFOA sia;
	//ZeroMemory(&sia, sizeof(STARTUPINFOA));

	//PROCESS_INFORMATION pi;
	////ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	//this->EnableDebugPrivilege(false);

	//result = CreateProcessA(lpExePath, NULL, NULL, NULL, false, DEBUG_ONLY_THIS_PROCESS, NULL, (LPCSTR)lpCurDirectory, &sia, &pi);

	//this->EnableDebugPrivilege(true);

	//if (!result)
	//	return result;

	//// 初始化各种重要对象
	////this->ExeFilePath = lpExePath;
	//this->hMainThread = pi.hThread;
	//this->hProc = pi.hProcess;
	//this->MainThreadId = pi.dwThreadId;
	//this->ProcId = pi.dwProcessId;
	//this->ExeImageBase = GetProcImageBase(this->hProc, &this->pPeb);
	//this->IsWow64 = IsWow64ModeOn(this->ProcId);

	//this->parser = new ModuleParser(hProc, IsWow64, ExeImageBase);
	//this->holder = new DynamicDispatcherHolder(hProc, IsWow64);

	//Thread th;
	//th.id = pi.dwThreadId;
	//th.handle = pi.hThread;
	//th.last_breakpoint_to_fix = NULL;
	//this->thread_book.insert(std::pair<UINT, Thread>(th.id, th));

	//EnterCriticalSection(&ModuleInfoLock);
	//MODULE_INFO* p_info = this->parser->tryAddModule(ExeImageBase, NULL);
	//LeaveCriticalSection(&ModuleInfoLock);

	//this->holder->init(0, 0);

	//setBreakPoint(p_info->MainEntry, "Exe Main()");
	//this->enableBreakPoint(p_info->MainEntry, true);

	////创建线程
	//LoopThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MyDbg::loop, this, NULL, (LPDWORD) & LoopThreadId);
	//if (!LoopThreadHandle) {
	//	printf("创建线程异常！\n");
	//	this->stop();
	//	return false;
	//}


	//printf("\nMyDbg:\n\t已创建进程。\n\t进程句柄：%I64X\n\t进程ID：%d\n\t进程路径：%s\n", (UPVOID)this->hProc, this->ProcId, this->ExeFilePath.c_str());

	//return result;
}

/*

	关闭进程，同时清理所有东西

*/
void MyDbg::stop()
{
	//if (!TerminateProcess(this->hProc, 0))
	//{
	//	printf("关闭进程失败！\n");
	//}

	if (LoopThreadHandle) {
		this->KillRequsetFlag = true;
		WaitForSingleObject(LoopThreadHandle, INFINITE);
		CloseHandle(LoopThreadHandle);
		this->KillRequsetFlag = false;
	}

	this->IsSysBreakPointReached = false;

	this->LoopThreadHandle = NULL;
	this->LoopThreadId = 0;

	this->MainThreadId = 0;
	this->hMainThread = NULL;
	this->ProcId = 0;
	this->hProc = NULL;
	this->ExeImageBase = NULL;
	this->pPeb = NULL;

	if (this->parser)
		delete this->parser;
	if (this->holder)
		delete this->holder;

	this->parser = NULL;
	this->holder = NULL;

	this->breakpoint_book.clear();
	this->thread_book.clear();
	this->GetProcAddr_RetnVals.clear();

}

bool MyDbg::EnableDebugPrivilege(bool enable_or_not)
{
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		printf("打开本进程令牌句柄失败！\n");
		return false;
	}

	LUID LuidPrivilege;
	LuidPrivilege.LowPart = 20;	//SeDebugPrivilege
	LuidPrivilege.HighPart = 0;

	TOKEN_PRIVILEGES Privileges;
	Privileges.PrivilegeCount = 1;
	Privileges.Privileges[0].Luid = LuidPrivilege;
	Privileges.Privileges[0].Attributes = enable_or_not ? SE_PRIVILEGE_ENABLED : 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &Privileges, sizeof(Privileges), NULL, NULL))
	{
		printf("调整本进程令牌失败！\n");
		return false;
	}

	return true;
}

//仅添加断点信息，不实际启用断点
BreakPoint* MyDbg::setBreakPoint(PVOID addr, const char* comment)
{
	BreakPoint* p_cc = NULL;

	auto ite = breakpoint_book.find(addr);
	if (ite != breakpoint_book.end())
		p_cc = &ite->second;

	BreakPoint cc;
	char bk = 0xCC;
	cc.is_enabled = false;
	if (comment)
		cc.comment = comment;
	ReadProcessMemory(this->hProc, addr, &cc.ori_byte, 1, NULL);

	breakpoint_book.insert(std::pair<PVOID, BreakPoint>(addr, cc));
	auto ite_2 = breakpoint_book.find(addr);
	if (ite_2 != breakpoint_book.end())
		p_cc = &ite_2->second;
	else
		printf("？？出现了不可能出现的错误？？\n");

	printf("已在 %p 处设下断点\n", ite_2->first);

	return p_cc;
}

//移除断点（修复）
void MyDbg::removeBreakPoint(PVOID addr)
{
	this->enableBreakPoint(addr, false);

	auto ite = this->breakpoint_book.find(addr);
	if (ite != this->breakpoint_book.end())
		this->breakpoint_book.erase(ite);
}

//步过断点（目前仅支持一次性断点）
void MyDbg::StepOverBreakPoint(DEBUG_EVENT& de, bool need_recovery)
{
	HANDLE hThread = thread_book[de.dwThreadId].handle;
	PVOID addr = de.u.Exception.ExceptionRecord.ExceptionAddress;
	auto ite = this->breakpoint_book.find(addr);
	if (ite == this->breakpoint_book.end())
		return;

	if (need_recovery)
		return;

	if (IsWow64)
	{
		WOW64_CONTEXT con;
		con.ContextFlags = WOW64_CONTEXT_CONTROL;
		Wow64GetThreadContext(hThread, &con);
		con.Eip = (DWORD)addr;
		Wow64SetThreadContext(hThread, &con);
	}
	else
	{
		CONTEXT con;
		con.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(hThread, &con);
		con.ip = (UPVOID)addr;
		SetThreadContext(hThread, &con);
	}

	this->enableBreakPoint(addr, false);
	this->breakpoint_book.erase(ite);

}

//实际的设置断点
void MyDbg::enableBreakPoint(PVOID addr, bool is_enable)
{
	auto ite = this->breakpoint_book.find(addr);
	if (ite == this->breakpoint_book.end())
		return;

	char bk = 0xCC;

	DWORD old_protect, new_protect = PAGE_EXECUTE_READWRITE;
	VirtualProtectEx(this->hProc, addr, 1, new_protect, &old_protect);
	if (is_enable)
		WriteProcessMemory(this->hProc, addr, &bk, 1, NULL);
	else
		WriteProcessMemory(this->hProc, addr, &ite->second.ori_byte, 1, NULL);
	VirtualProtectEx(this->hProc, addr, 1, new_protect, &old_protect);
	if (!FlushInstructionCache(hProc, addr, 1))
	{
		BugCheck("刷新指令失败！\n");
	}

	ite->second.is_enabled = is_enable;

}


bool MyDbg::_debug_thread_routine(MyDbg* _this)
{

	int index = _this->ExeFilePath.rfind('\\');
	if (index == std::string::npos)
		index = _this->ExeFilePath.rfind('/');
	if (index == std::string::npos)
		return false;

	std::string CurDirectory = _this->ExeFilePath.substr(0, index);

	//反反调试第一招
	STARTUPINFOA sia;
	ZeroMemory(&sia, sizeof(STARTUPINFOA));

	PROCESS_INFORMATION pi;
	//ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	bool result;
	_this->EnableDebugPrivilege(false);
	result = CreateProcessA(_this->ExeFilePath.c_str(), NULL, NULL, NULL, false, DEBUG_ONLY_THIS_PROCESS, NULL, CurDirectory.c_str(), &sia, &pi);
	_this->EnableDebugPrivilege(true);

	if (!result)
		return result;

	_this->ProcessStatusFlag |= PS_PROCESS_AVAILABLE;	//更新状态

	// 初始化各种重要对象
	//this->ExeFilePath = lpExePath;
	_this->hMainThread = pi.hThread;
	_this->hProc = pi.hProcess;
	_this->MainThreadId = pi.dwThreadId;
	_this->ProcId = pi.dwProcessId;
	_this->ExeImageBase = GetProcImageBase(_this->hProc, &_this->pPeb);
	_this->IsWow64 = IsWow64ModeOn(_this->ProcId);

	_this->parser = new ModuleParser(_this->hProc, _this->IsWow64, _this->ExeImageBase);
	_this->holder = new DynamicDispatcherHolder(_this->hProc, _this->IsWow64);

	Thread th;
	th.id = pi.dwThreadId;
	th.handle = pi.hThread;
	th.last_breakpoint_to_fix = NULL;
	_this->thread_book.insert(std::pair<UINT, Thread>(th.id, th));

	EnterCriticalSection(&_this->ModuleInfoLock);
	MODULE_INFO* p_info = _this->parser->tryAddModule(_this->ExeImageBase, NULL);
	LeaveCriticalSection(&_this->ModuleInfoLock);

	_this->holder->init(0, 0);

	_this->setBreakPoint(p_info->MainEntry, "Exe Main()");
	_this->enableBreakPoint(p_info->MainEntry, true);

	//首先设置好标识
	_this->KillRequsetFlag = false;
	_this->DetachRequestFlag = false;

	//
	// 进入调试器循环
	//
	int loop_result;
	do {
		if (_this->KillRequsetFlag || _this->DetachRequestFlag)
			break;

		loop_result = _this->_handle_debug_event(200);
	} while (loop_result != DE_EXIT);

	//
	// 进行收尾工作
	//
	if (_this->DetachRequestFlag)
	{
		SuspendThread(_this->hMainThread);
		DebugActiveProcessStop(_this->ProcId);
	}
	else if(_this->KillRequsetFlag)
		TerminateProcess(_this->hProc, 0x114514);
	//（如果满足不了上述的条件，说明调试过程出现问题）

	_this->ProcessStatusFlag &= ~PS_PROCESS_AVAILABLE;

	//
	// 关闭句柄
	//
	CloseHandle(_this->hMainThread);
	CloseHandle(_this->hProc);
	_this->hMainThread = NULL;
	_this->hProc = NULL;
}

// * * 线程循环函数 * *
int MyDbg::_handle_debug_event(int wait_time)
{
	//printf("循环已启动\n");
	int loop_result = 0;
	bool wait_result = false;
	HANDLE hThread;
	DEBUG_EVENT de;
	if (!WaitForDebugEvent(&de, wait_time))
	{
		if (GetLastError() == ERROR_SEM_TIMEOUT)
			return DE_TIME_OUT;
		else
			return DE_EXIT;
	}

	//
	// ============ 开始正常处理 =============
	//
	auto ite_thr = this->thread_book.find(de.dwThreadId);
	if (ite_thr == this->thread_book.end() && de.dwDebugEventCode != CREATE_THREAD_DEBUG_EVENT)
	{
		BugCheck("【严重错误】出现未记录的线程！\n");
	}

	//
	// ============ 异常调试事件 ============
	//
	if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
	{
		DWORD code = de.u.Exception.ExceptionRecord.ExceptionCode;
		PVOID address = de.u.Exception.ExceptionRecord.ExceptionAddress;

		//
		// ====== 断点异常 ======
		//
		if ((code == EXCEPTION_BREAKPOINT && this->IsWow64 == false)
			|| (code == WOW64_EXCEPTION_BREAKPOINT && this->IsWow64 == true))
		{
			printf("检测到断点，位置：%p\n", de.u.Exception.ExceptionRecord.ExceptionAddress);

			//
			// 处理两种特殊异常
			//
			if ((this->HookTypeFlag & HT_GET_PROC_ADDRESS) && this->TryHandleGetProcAddrHook(de))
				goto Continue;
			if ((this->HookTypeFlag & HT_IMPORT_ADDRESS_TABLE) && this->TryHandleDllMainBreakPoint(de))
				goto Continue;

			//
			// 记录系统断点
			//
			if (this->IsSysBreakPointReached == false)
			{
				this->IsSysBreakPointReached = true;
				this->HideDebugger(true);
				goto Continue;
			}

			printf("（断点未处理）\n");

		}
		else
		{
			//
			// 来自被调试器的严重错误
			//
			printf("出现了意料之外的异常，异常代码：%X，地址：%p，问题线程：%d\n", de.u.Exception.ExceptionRecord.ExceptionCode, address, de.dwThreadId);
			this->printCrashInfo(de);

			//SuspendThread(this->thread_book[de.dwThreadId].handle);
			//this->detach();
			//loop_result = -1;
			//getchar();
			//break;

			ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
			goto Skip_Continue;
		}

	}

	//
	// ============ DLL加载事件 ============
	// 
	else if (de.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
	{
		bool is_addedthis_time;

		EnterCriticalSection(&this->ModuleInfoLock);
		MODULE_INFO* p_info = this->parser->tryAddModule(de.u.LoadDll.lpBaseOfDll, &is_addedthis_time);
		LeaveCriticalSection(&this->ModuleInfoLock);

		if (p_info)
		{
			//【】设置可疑的断点
			if ((this->HookTypeFlag & HT_IMPORT_ADDRESS_TABLE) && this->IsHookTarget(*p_info) && is_addedthis_time) {
				this->setBreakPoint(p_info->MainEntry, "DllMain for Iat Hook");
				this->enableBreakPoint(p_info->MainEntry, true);
			}

			//首次加载kernel32.dll
			if (p_info->ModuleName == "kernel32.dll" && is_addedthis_time)
			{
				this->HideDebugger(false);

				//【可疑的操作点】
				if (this->HookTypeFlag & HT_GET_PROC_ADDRESS)
					this->HookGetProcAddr(de.u.LoadDll.lpBaseOfDll);
			}

			//首次加载ntdll.dll
			else if (p_info->ModuleName == "ntdll.dll" && is_addedthis_time)
			{
				this->HideDebugger(true);
			}

			printf("检测到加载模块，模块地址：%p，模块名称：%s\n", p_info->ModuleImageBase, p_info->ModuleName.c_str());
		}

		else
		{
			printf("【WARN】出现了解析失败的模块：%p\n", de.u.UnloadDll.lpBaseOfDll);
		}
	}

	//
	// ============ DLL卸载事件 ============
	// 
	else if (de.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT)	//真的需要排除那些不重要的DLL
	{
		printf("【WARN】出现了卸载的模块：%p\n", de.u.UnloadDll.lpBaseOfDll);

		MODULE_INFO* p_info = this->parser->querySource(de.u.UnloadDll.lpBaseOfDll);
		if (p_info == NULL)
			printf("\t（该模块未被解析）\n");
		else
			printf("\t模块名称：%s\n", p_info->ModuleName.c_str());

		this->parser->walkAddressSpace();
	}

	//
	// ============ 线程创建事件 ============
	// 
	else if (de.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
	{
		Thread th;
		th.id = de.dwThreadId;
		th.handle = de.u.CreateThread.hThread;
		th.last_breakpoint_to_fix = NULL;

		auto ite = this->thread_book.find(de.dwThreadId);
		if (ite != this->thread_book.end())
			this->thread_book.erase(ite);

		this->thread_book.insert(std::pair<UINT, Thread>(th.id, th));

		printf("线程已创建，线程ID：%d\n", th.id);
	}

	//
	// ============ 线程退出事件 ============
	// 
	else if (de.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
	{
		auto ite = this->thread_book.find(de.dwThreadId);
		if (ite != this->thread_book.end())
			this->thread_book.erase(ite);

		printf("线程已退出，线程ID：%d，退出码：%X\n", de.dwThreadId, de.u.ExitThread.dwExitCode);
	}

	//
	// ============ 进程退出事件 ============
	// 
	else if (de.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
	{
		printf("进程已退出。\n");

		loop_result = DE_EXIT;

	}
	// 事件处理从此结束
	// ========================================
	// 恢复调试事件措施从此开始


	//
	// 常规例程
	//
Continue:
	ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
Skip_Continue:

	//printf("LastError = %d", GetLastError());

	return loop_result;
}

bool MyDbg::TryHandleGetProcAddrHook(DEBUG_EVENT& de)
{	
	//
	// 导出表钩
	//
	HANDLE hThread = thread_book[de.dwThreadId].handle;
	PVOID addr = de.u.Exception.ExceptionRecord.ExceptionAddress;
	if (addr != this->GpaBreakPoint)
		return false;

	//
	// 获取信息
	//
	std::string FuncName;
	PVOID CallerAddress = NULL;	//函数调用方
	PVOID lpFuncName = NULL;
	PVOID DllImageBase = NULL;
	PVOID RetnFuncAddress = NULL;	//GetProcAddress查找的最终结果
	PVOID HookedAddr = NULL;
	if (IsWow64)
	{
		WOW64_CONTEXT con;
		con.ContextFlags = CONTEXT_ALL;
		Wow64GetThreadContext(hThread, &con);
		ReadProcessMemory(this->hProc, (PUCHAR)con.Esp, &CallerAddress, sizeof(DWORD), NULL);
		ReadProcessMemory(this->hProc, (PUCHAR)con.Esp + 4, &DllImageBase, sizeof(DWORD), NULL);
		ReadProcessMemory(this->hProc, (PUCHAR)con.Esp + 8, &lpFuncName, sizeof(DWORD), NULL);
		RetnFuncAddress = (PVOID)con.Eax;

		MODULE_INFO* p_info = parser->querySource(CallerAddress);

		if (!p_info || !this->IsHookTarget(*p_info))
			return true;

		if (!RetnFuncAddress) {
			printf("远端的GetProcAddress调用失败。\n");
			return true;
		}

		EnterCriticalSection(&CallRecordLock);
		this->holder->commit(CallerAddress, RetnFuncAddress, &HookedAddr);
		LeaveCriticalSection(&CallRecordLock);

		con.Eax = (DWORD)HookedAddr;
		Wow64SetThreadContext(hThread, &con);
	}
	else
	{
		CONTEXT con;
		con.ContextFlags = CONTEXT_ALL;
		GetThreadContext(hThread, &con);
		ReadProcessMemory(this->hProc, (PUCHAR)con.Rsp, &CallerAddress, sizeof(PVOID), NULL);
		DllImageBase = (PVOID)con.Rcx;
		lpFuncName = (PVOID)con.Rdx;
		RetnFuncAddress = (PVOID)con.Rax;

		MODULE_INFO* p_info = parser->querySource(CallerAddress);

		if (!p_info || !this->IsHookTarget(*p_info))
			return true;

		if (!RetnFuncAddress) {
			printf("远端的GetProcAddress调用失败。\n");
			return true;
		}

		EnterCriticalSection(&CallRecordLock);
		this->holder->commit(CallerAddress, RetnFuncAddress, &HookedAddr);
		LeaveCriticalSection(&CallRecordLock);

		con.Rax = (UPVOID)HookedAddr;
		SetThreadContext(hThread, &con);
	}

	EnterCriticalSection(&ModuleInfoLock);
	if (lpFuncName < (PVOID)0xFFFF)
		this->parser->tryAddApi(RetnFuncAddress, NULL, (DWORD)lpFuncName, NULL);
	else{
		parser->readString(lpFuncName, FuncName);
		this->parser->tryAddApi(RetnFuncAddress, FuncName.c_str(), -1, NULL);
	}
	LeaveCriticalSection(&ModuleInfoLock);

	this->GetProcAddr_RetnVals.insert(CallerAddress);
	return true;
}

bool MyDbg::TryHandleDllMainBreakPoint(DEBUG_EVENT& de)
{
	PVOID addr = de.u.Exception.ExceptionRecord.ExceptionAddress;
	MODULE_INFO* p_info = parser->querySource(addr);
	if (!p_info || p_info->MainEntry != (PUCHAR)addr)
		return false;

	MODULE_IAT_PACK task;
	parser->walkIatTable(p_info->ModuleImageBase, &task);

	EnterCriticalSection(&CallRecordLock);
	holder->commit(task);
	LeaveCriticalSection(&CallRecordLock);

	StepOverBreakPoint(de, false);

	//if (p_info->ModuleName == "comctl32.dll")
	//{
	//	printf("err address = %p\n", de.u.Exception.ExceptionRecord.ExceptionAddress);
	//	printf("dll main entry = %p\n", p_info->MainEntry);
	//	printf("dll path = %s\n", p_info->ModulePath.c_str());
	//	HANDLE hThread = this->thread_book[de.dwThreadId].handle;
	//	SuspendThread(this->thread_book[de.dwThreadId].handle);
	//	this->DetachRequestFlag = true;
	//	//getchar();
	//}

	printf("\nMyDbg:\t检测到 DllMain断点。\n\t所属模块：%s\n", p_info->ModuleName.c_str());

	return true;
}

bool MyDbg::CrashExport(PVOID DllImageBase, const char* lpFunctionName, PVOID* CrashedAddr, PVOID* OriAddr)
{
	std::string func_name = lpFunctionName;

	PVOID export_location, ori_addr;
	ori_addr = this->parser->getProcAddr(DllImageBase, func_name, &export_location);
	if (!ori_addr)
		return false;

	//查找0xCC字符
	unsigned char lpBuffer[0x20];
	PUCHAR ptr = (PUCHAR)ori_addr;
	bool result = false;
	int i;
	while (1)
	{
		if (!ReadProcessMemory(this->hProc, ptr, lpBuffer, 0x20, NULL))
		{
			return false;
		}
		for (i = 0; i < 0x20; i++)
		{
			if (lpBuffer[i] == 0xCC)
			{
				result = true;
				break;
			}
		}
		ptr += i;
		if (result)
			break;
	}

	if (!result)
		return false;

	//破坏导出表
	DWORD crashed_rva = ptr - DllImageBase;
	DWORD new_protect = PAGE_READWRITE, old_protect;
	VirtualProtectEx(this->hProc, export_location, sizeof(DWORD), new_protect, &old_protect);
	WriteProcessMemory(this->hProc, export_location, &crashed_rva, sizeof(DWORD), NULL);
	VirtualProtectEx(this->hProc, export_location, sizeof(DWORD), old_protect, &new_protect);

	if (CrashedAddr)
		*CrashedAddr = crashed_rva + (PUCHAR)DllImageBase;

	if (OriAddr)
		*OriAddr = ori_addr;


	printf("【钩子】已破坏导出表。\n");
	return true;
}

void MyDbg::HideDebugger(bool hide_or_unhide)
{
	unsigned char ori_value, new_value;
	if (hide_or_unhide)
		new_value = 0;
	else
		new_value = 1;

	DWORD old_protect, new_protect = PAGE_READWRITE;
	VirtualProtectEx(this->hProc, (PUCHAR)pPeb + RVA_PEB_BeingDebugged, 1, new_protect, &old_protect);
	if (IsWow64) {
		ReadProcessMemory(this->hProc, (PUCHAR)pPeb + RVA_PEB_BeingDebugged, &ori_value, 1, NULL);
		WriteProcessMemory(this->hProc, (PUCHAR)pPeb + RVA_PEB_BeingDebugged, &new_value, 1, NULL);
	}
	else{
		ReadProcessMemory(this->hProc, (PUCHAR)pPeb + RVA_PEB_BeingDebugged, &ori_value, 1, NULL);
		WriteProcessMemory(this->hProc, (PUCHAR)pPeb + RVA_PEB_BeingDebugged, &new_value, 1, NULL);
	}
	VirtualProtectEx(this->hProc, (PUCHAR)pPeb + RVA_PEB_BeingDebugged, 1, old_protect, &new_protect);

	printf("【反反调试】BeingDebugged: %d -> %d\n", ori_value, new_value);

}

void MyDbg::detach()
{
	//DebugActiveProcessStop(this->ProcId);

	if (this->LoopThreadHandle) {
		this->DetachRequestFlag = true;
		WaitForSingleObject(this->LoopThreadHandle, INFINITE);
		CloseHandle(this->LoopThreadHandle);
		this->DetachRequestFlag = false;
	}

	this->ProcessStatusFlag &= ~PS_PROCESS_AVAILABLE;

	printf("---------------------------------------\n");
	printf("已脱离调试，被调试进程ID：%d\n", this->ProcId);

	this->IsSysBreakPointReached = false;

	this->LoopThreadHandle = NULL;
	this->LoopThreadId = 0;

	this->MainThreadId = 0;
	this->hMainThread = NULL;
	this->ProcId = 0;
	this->hProc = NULL;
	this->ExeImageBase = NULL;
	this->pPeb = NULL;

	if (this->parser)
		delete this->parser;
	if (this->holder)
		delete this->holder;

	this->parser = NULL;
	this->holder = NULL;

	this->breakpoint_book.clear();
	this->thread_book.clear();
	this->GetProcAddr_RetnVals.clear();
}

bool MyDbg::install()
{
	return true;
}

bool MyDbg::uninstall()
{
	return true;
}

//void MyDbg::setHookTarget(DWORD flag)
//{
//	this->HookTargetFlag = flag;
//}

std::string MyDbg::getComment(PVOID addr)
{
	std::string str;
	parser->getComment(addr, str);

	auto ite = this->GetProcAddr_RetnVals.find(addr);
	if (ite != this->GetProcAddr_RetnVals.end())
		str = "[GPA]" + str;

	return str;
}

std::vector<CALL_DATA_ENTRY>* MyDbg::visitLatestRecords()
{
	//if (!(this->ProcessStatusFlag & PS_PROCESS_AVAILABLE))
	//	return NULL;

	EnterCriticalSection(&CallRecordLock);
	this->holder->update();
	return &(this->holder->CallRecords);
}

void MyDbg::leaveCallRecords()
{
	LeaveCriticalSection(&CallRecordLock);
	return;
}

PVOID MyDbg::locateEntry(int RecordIndex)
{
	if (holder)
	{
		EnterCriticalSection(&CallRecordLock);
		PVOID result = this->holder->locateDispatchingEntry(RecordIndex);
		LeaveCriticalSection(&CallRecordLock);
		return result;
	}
	else
		return NULL;
}

DWORD MyDbg::checkProcessStatus()
{

	//检查是否能够正常读取
	WORD magic;
	if (!ReadProcessMemory(this->hProc, this->ExeImageBase, &magic, sizeof(WORD), NULL))
	{
		this->ProcessStatusFlag &= ~PS_PROCESS_AVAILABLE;
	}

	return this->ProcessStatusFlag;
}

void MyDbg::getDetails(int RecordIndex, CALL_RECORD_DETAILS& details)
{
	EnterCriticalSection(&ModuleInfoLock);

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

	auto ite_2 = this->GetProcAddr_RetnVals.find(ent.OverwrittenAddress);
	if (ite_2 != this->GetProcAddr_RetnVals.end())
		details.source = RECORD_SOURCE::RS_GetProcAddress;
	else
		details.source = RECORD_SOURCE::RS_ImportAddressTable;

	LeaveCriticalSection(&ModuleInfoLock);
}

void MyDbg::printCrashInfo(DEBUG_EVENT& de)
{

	static DWORD depth = 10;
	HANDLE hThread = this->thread_book[de.dwThreadId].handle;
	printf("\n====================\n崩溃存储信息如下：\n--------------------\n");
	PVOID Rip;
	PVOID Rsp;
	
	if (IsWow64)
	{
		WOW64_CONTEXT con;
		con.ContextFlags = CONTEXT_ALL;
		Wow64GetThreadContext(hThread, &con);
		Rip = (PVOID)con.Eip;
		Rsp = (PVOID)con.Esp;
	}
	else
	{
		CONTEXT con;
		con.ContextFlags = CONTEXT_ALL;
		GetThreadContext(hThread, &con);
		Rip = (PVOID)con.ip;
		Rsp = (PVOID)con.sp;
	}

	printf("\t指令指针ip：%p\n", Rip);
	printf("\t栈指针sp：%p\n", Rsp);

	printf("\n\t栈内容：\n");
	for (int i = 0; i < depth; i++)
	{
		PVOID val = 0;
		if (IsWow64)
			ReadProcessMemory(this->hProc, (PDWORD)Rsp+i, &val, sizeof(DWORD), NULL);
		else
			ReadProcessMemory(this->hProc, (UPVOID*)Rsp+i, &val, sizeof(UPVOID), NULL);

		printf("\t[%p] = %I64X\n", (PDWORD)Rsp + i, (UPVOID)val);
	}


}

//钩GetProcAddress，只会触发一次断点，同时保证能获取所有的信息
bool MyDbg::HookGetProcAddr(PVOID Kernel32BaseAddr)
{
	std::string gpa = "GetProcAddress";
	PVOID export_rva_location;
	PVOID real_addr = parser->getProcAddr(Kernel32BaseAddr, gpa, &export_rva_location);
	if (!real_addr)
	{
		BugCheck("无法找到GetProcAddress函数？\n");
		return false;
	}

	MODULE_INFO* p_info = parser->queryModule(Kernel32BaseAddr);
	if (!p_info) {
		printf("？？不可能的错误？\n");
		return false;
	}

	//
	// 初始化补丁
	//
	CodeString cs;
	if (this->IsWow64)
		Wow64_InitAsGetProcAddrHook(cs);
	else
		InitAsGetProcAddrHook(cs);

	//
	// 尝试搜索合适的内存空间
	//
	DWORD FreeSize;
	PVOID SuitableAddress;
	if (!parser->getModuleExecutionGap(Kernel32BaseAddr, &SuitableAddress, &FreeSize) || FreeSize < cs.size())
	{
		BugCheck("无法找到合适的空位？\n");
		return false;
	}

	//
	// 修正钩子，同时给定地址。
	//
	if (IsWow64)
		Wow64_RelocateGetProcAddrHook(cs, real_addr, SuitableAddress, &this->GpaBreakPoint);
	else
		RelocateGetProcAddrHook(cs, real_addr, SuitableAddress, &this->GpaBreakPoint);

	//
	// 安装钩子
	//
	DWORD old_protect, new_protect = PAGE_EXECUTE_READWRITE;
	VirtualProtectEx(this->hProc, SuitableAddress, cs.size(), new_protect, &old_protect);
	WriteProcessMemory(this->hProc, SuitableAddress, cs.data(), cs.size(), NULL);
	VirtualProtectEx(this->hProc, SuitableAddress, cs.size(), old_protect, &new_protect);
	FlushInstructionCache(this->hProc, SuitableAddress, cs.size());

	//修改导出表
	DWORD func_rva = (UPVOID)SuitableAddress - (UPVOID)Kernel32BaseAddr;
	VirtualProtectEx(this->hProc, export_rva_location, sizeof(DWORD), new_protect, &old_protect);
	WriteProcessMemory(this->hProc, export_rva_location, &func_rva, sizeof(DWORD), NULL);
	VirtualProtectEx(this->hProc, export_rva_location, sizeof(DWORD), old_protect, &new_protect);

	return true;
}