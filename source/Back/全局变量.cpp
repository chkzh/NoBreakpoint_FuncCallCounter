#include "GlobalFuncAndVar.h"
#include "GlobalStruct.h"
//#include "远端内存管理.h"
#include <vector>
#include <list>


//要用到的API函数
MyNtQueryInformationProcess _NtQueryInformationProcess = NULL;
MyNtQueryInformationThread _NtQueryInformationThread = NULL;
MyIsWow64Process _IsWow64Process = NULL;

//【重点】进程选定相关（Process Choosing）
UINT pc_ProcID = 0;
HANDLE pc_ProcHandle = NULL;
PVOID pc_ExeImageBase = NULL;
PROCESS_TYPE pc_ProcType;
PVOID pc_PebAddr = NULL;

//进程快照组
//std::map<UINT, PROCESS_SNAPSHOT> ProcessSnapshotBook;

//进程快照权限
DWORD ProcessSnapshotAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE;

//窗口搜索相关
HWND TargetHwnd = NULL;		//plan - A
HWND ForgroundHwnd = NULL;	//plan - B
UINT ProcessingProcID = NULL;

//函数调用记录信息查询（具有时效性！）
std::map<PVOID, API_INFO> g_ApiInfoBook;
std::map<PVOID, MODULE_INFO> g_ModuleInfoBook;

char lpExeFilePath[MAX_PATH] = { 0 };
char* lpExeName = NULL;

HANDLE JobHandle = NULL;