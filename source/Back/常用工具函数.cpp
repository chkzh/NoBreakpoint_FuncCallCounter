#include <windows.h>
#include <codecvt>
#include <algorithm>
#include <psapi.h>
#include <stdarg.h>
#include "GlobalFuncAndVar.h"
#include "x96.h"

//【开放】
void extractRemoteString(HANDLE hProc, PVOID address, std::string& str)
{
	int i = 0;
	str.clear();
	char ch;
	while (1)
	{
		ReadProcessMemory(hProc, (LPCVOID)((UPVOID)address + i), &ch, 1, NULL);
		if (!ch)
			break;
		str.push_back(ch);
		if (++i > 260)
		{
			str = "(error: remote string is too long)";
			//RemotePrintHex(hProc, address, 0x40);
			//printf_err("(error: remote string is too long)");
			break;
		}
	}
}

//【开放】
void extractRemoteStringW(HANDLE hProc, PVOID address, std::string& str)
{
	int i = 0;
	str.clear();
	WCHAR ch;
	std::wstring wstr;
	while (1)
	{
		ReadProcessMemory(hProc, (LPCVOID)((UPVOID)address + i * sizeof(WCHAR)), &ch, 2, NULL);
		if (!ch)
			break;
		wstr.push_back(ch);
		if (++i > 260)
		{
			str = "(error: remote string is too long)";
			//RemotePrintHex(hProc, address, 0x40);
			return;
		}
	}
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	str = converter.to_bytes(wstr);
}

//【开放】
void StringToLower(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void DisableSpecialChar(unsigned char* ptr, int size);
void PrintHex(char* raw, int num, PVOID address);

//【开放】
void RemotePrintHex(HANDLE hProc, PVOID address, int size)
{
	char* ptr = new char[size];
	ReadProcessMemory(hProc, (LPCVOID)address, ptr, size, NULL);
	PrintHex(ptr, size, address);
	delete[] ptr;
}


void PrintHex(char* raw, int num, PVOID address = NULL)		//以16进制的形式输出数据
{
	printf("\n");
	int i = 0;
	unsigned char w[16];
	while (1)
	{
		if (i && i % 16 == 0)
		{
			printf("|");
			DisableSpecialChar(w, 16);
			for (int j = 0; j < 16; j++)
			{
				printf("%c", w[j]);
			}
			memset(w, 0, 16);
			printf("\n");
		}
		if (i < num)
		{
			if (i % 16 == 0)
			{
				printf("[%p] | ", address);
				address = (PVOID)((UPVOID)address + 16);
			}
			w[i % 16] = raw[i];
			printf("%02X ", (unsigned char)raw[i]);
		}
		else break;
		i++;
	}
	printf("\n\n");
}

void DisableSpecialChar(unsigned char* ptr, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (ptr[i] > 126 || ptr[i] < 32)
			ptr[i] = '.';
	}
}

//加载未公开的API
PVOID LoadHiddenFuncA(const char* dll_name, const char* function_name)
{
	HMODULE hDll = LoadLibraryA(dll_name);
	if (hDll == INVALID_HANDLE_VALUE || hDll == 0)
	{
		printf("[ERROR] failed to load \"%s\" ...\n", dll_name);
		return NULL;
	}
	PVOID result = GetProcAddress(hDll, function_name);
	if (result == NULL)
	{
		printf("[ERROR] failed to find \"%s\" ...\n", function_name);
		return NULL;
	}
	//printf("Loaded Undocumented API:\n\t%s - %s -> %p\n", dll_name, function_name, result);
	return result;
}


PVOID GetProcImageBase(HANDLE hProc, PVOID* ppeb_slot)
{
	if (_NtQueryInformationProcess == NULL)
	{
		_NtQueryInformationProcess = (MyNtQueryInformationProcess)LoadHiddenFuncA("ntdll.dll", "NtQueryInformationProcess");
	}

	PROCESS_BASIC_INFORMATION pbi;
	_NtQueryInformationProcess(hProc, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);

	if (ppeb_slot)
		*ppeb_slot = pbi.PebBaseAddress;

	PVOID image_base;
	ReadProcessMemory(hProc, _VA(pbi.PebBaseAddress, RVA_PEB_PebBaseAddress), &image_base, sizeof(PVOID), NULL);

	WORD mz;
	ReadProcessMemory(hProc, image_base, &mz, 2, NULL);
	if (mz != 0x5A4D)
	{
		printf("[ERROR] 获取基地址失败？\n");
		return NULL;
	}

	return image_base;
}

//宽字符转字符串
std::string WCharToAcsiiString(WCHAR* lpWString)
{
	char lpString[MAX_PATH];
	std::string AsciiString;
	int result = WideCharToMultiByte(CP_ACP, 0, lpWString, -1, lpString, MAX_PATH, 0, 0);
	if (result > 0)
	{
		AsciiString = lpString;
	}
	else
	{
		printf("[ERROR] Unicode转Ascii出错！\n");
	}
	return AsciiString;
}

//检查是否能获取特定权限的句柄
bool CheckProcessAccess(UINT ProcID, DWORD DesiredAccess)
{
	HANDLE hProc = OpenProcess(DesiredAccess, false, ProcID);
	if (hProc)
	{
		CloseHandle(hProc);
		return true;
	}
	else
		return false;
}

/*

	以模仿 Explorer.exe 的方式创建进程

*/
bool CreateProc(const char* lpExePath, HANDLE* lpProcHandle, UINT* lpProcID, HANDLE* lpThreadHandle, UINT* lpThreadID)
{
	bool result;

	if (!lpExePath)
		return false;

	char lpCurDirectory[MAX_PATH] = { 0 };
	char* lp_end = (char*)strrchr(lpExePath, '\\');
	if (lp_end == NULL)
		return false;

	int path_length =  lp_end - lpExePath;
	if (path_length <= 0)
		return false;

	memcpy(lpCurDirectory, lpExePath, path_length);

	STARTUPINFOA sia;
	ZeroMemory(&sia, sizeof(STARTUPINFOA));

	PROCESS_INFORMATION pi;
	//ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	result = CreateProcessA(lpExePath, NULL, NULL, NULL, false, NULL, NULL, (LPCSTR)lpCurDirectory, &sia, &pi);

	if (result)
	{
		if (lpProcHandle)
			*lpProcHandle = pi.hProcess;
		else
			CloseHandle(pi.hProcess);

		if (lpProcID)
			*lpProcID = pi.dwProcessId;

		if (lpThreadHandle)
			*lpThreadHandle = pi.hThread;
		else
			CloseHandle(pi.hThread);

		if (lpThreadID)
			*lpThreadID = pi.dwThreadId;
	}
	else
		printf("创建进程失败，文件路径：%s\n", lpExePath);

	return result;
}

bool SetKillChildProcOnClose(HANDLE hProc, HANDLE* lp_hJob)
{
	bool result;

	bool create_job_by_this_call = false;

	if (!lp_hJob)
		return false;

	if (*lp_hJob == NULL) {
		*lp_hJob = CreateJobObjectA(NULL, NULL);
		create_job_by_this_call = true;
	}

	if (*lp_hJob == NULL)
		return false;

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	result = SetInformationJobObject(*lp_hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
	if (!AssignProcessToJobObject(*lp_hJob, hProc))
		result = false;

	if (!result && create_job_by_this_call){
		CloseHandle(*lp_hJob);
		*lp_hJob = NULL;
	}

	return result;
}

void BugCheck(const char* fmt, ...)
{
	printf("\n【严重错误出现】\n");

	if (fmt) {
		printf("描述：\n");

		va_list arg;
		va_start(arg, fmt);
		vprintf(fmt, arg);
		va_end(arg);
	}

	getchar();
}

//返回值 -1 = 函数出错，0 = 不是，1 = 是
bool IsWow64ModeOn(UINT ProcessId)
{
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessId);
	if (hProc == NULL)
	{
		printf("[ERROR] 获取进程句柄失败。进程ID：%d，请求权限：QureyInformation\n", ProcessId);
		return -1;
	}

	if (_IsWow64Process == NULL)
	{
		_IsWow64Process = (MyIsWow64Process)LoadHiddenFuncA("kernel32.dll", "IsWow64Process");
	}

	BOOL result_1, result_2;
	if (_IsWow64Process(hProc, &result_1) && _IsWow64Process(GetCurrentProcess(), &result_2))
	{
		CloseHandle(hProc);
		return result_1 && !result_2;
	}
	else
	{
		CloseHandle(hProc);
		printf("[ERROR] Wow64 查询失败\n");
		return -1;
	}
}