#pragma once

#include <windows.h>
#include <string>
#include <winternl.h>
#include <map>
#include "GlobalStruct.h"

/*++

	全局使用的函数、全局变量
	（所有的模块都需要使用到这些）
	（只声明，不定义）
	（定义全部写在对应的 .cpp 文件中）

--*/

// 远端字符串提取
void extractRemoteString(HANDLE hProc, PVOID address, std::string& str);
void extractRemoteStringW(HANDLE hProc, PVOID address, std::string& str);//<----可能需要测试。

// 简易字符串操作
void StringToLower(std::string& str);

// 远端打印二进制数据
void RemotePrintHex(HANDLE hProc, PVOID address, int size);

//加载未公开函数
PVOID LoadHiddenFuncA(const char* dll_name, const char* function_name);

//只是语法糖 >_<;
#define _VA(x,y) (PVOID)((UPVOID)x + y)

//未公开的函数们
extern MyNtQueryInformationProcess _NtQueryInformationProcess;
extern MyNtQueryInformationThread _NtQueryInformationThread;
extern MyIsWow64Process _IsWow64Process;

//依靠句柄查出进程的基地址
PVOID GetProcImageBase(HANDLE hProc, PVOID* ppeb_slot);

//宽字符转字符串
std::string WCharToAcsiiString(WCHAR* lpWString);

//检查进程操作权限
bool CheckProcessAccess(UINT ProcID, DWORD DesiredAccess);

//【重点】进程选定相关
extern UINT pc_ProcID;
extern HANDLE pc_ProcHandle;
extern PVOID pc_ExeImageBase;
extern PVOID pc_PebAddr;
extern PROCESS_TYPE pc_ProcType;

//DOS路径转设备路径
void DosPathToDevicePath(char* lpDosPath);

//查询路径的来源
MODULE_SOURCE FigureOutModuleSource(std::string& ModulePath);

//以"explore.exe"的方式创建进程
bool CreateProc(const char* lpExePath, HANDLE* lpProcHandle, UINT* lpProcID, HANDLE* lpThreadHandle, UINT* lpThreadID);

//绑定子进程
bool SetKillChildProcOnClose(HANDLE hProc, HANDLE* lp_hJob);

//程序崩溃函数
void BugCheck(const char* fmt, ...);

//检查是否需要启用Wow64兼容功能
bool IsWow64ModeOn(UINT ProcessId);