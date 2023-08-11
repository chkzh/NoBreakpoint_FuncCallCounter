#pragma once

#include <windows.h>
#include <winternl.h>
#include <vector>
#include <string>
#include "x96.h"


typedef NTSTATUS(NTAPI* MyNtQueryInformationProcess)(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength
	);

typedef NTSTATUS(NTAPI* MyNtQueryInformationThread)(
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	IN OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength
	);

typedef NTSTATUS(WINAPI* MyIsWow64Process)(
	IN HANDLE ProcessHandle,
	OUT PBOOL ResultSlot
	);

/*

	函数调用记录的重要数据结构。

成员：
	CallCount - 调用次数，源于远端进程的数据段。需要及时更新。

	FunctionAddress - 原函数地址。
		可以代表此项是否有效。
		可以用于定位此项的导出DLL。

	OverwrittenAddress - 远端进程中，数据被修改的地方。
		每一项的特有成员，不会重复。
		可以用于定位此项的导入模块。

注意点：
	该列表的元素顺序极为重要，顺序与远端进程一一对应，不可以随意删除元素。删除元素请将那一项置零。

*/
typedef struct _CALL_DATA_ENTRY
{
	//远端进程中只有以下2个成员：
	unsigned long long CurCallCount;
	PVOID FunctionAddress;

	//本进程独有成员：
	unsigned long long LastCallCount;
	PVOID OverwrittenAddress;	//IAT -> 导入模块对应的导入表地址。GetProcAddr -> 调用者地址
}CALL_DATA_ENTRY;

typedef struct _REMOTE_CALL_DATA_ENTRY
{
	unsigned long long CallCount;
	PVOID FunctionAddress;
}REMOTE_CALL_DATA_ENTRY;

enum PROCESS_TYPE
{
	PE_32,
	PE_64
};

enum MODULE_SOURCE
{
	Exe,
	Exe_Directory,		//Exe文件夹下的DLL
	System_Directory,	//系统路径下的DLL，包括WOW64的路径
	Other_Directory		//其他路径？？
};

//用于查询DLL基地址对应的DLL的详细信息
typedef struct _MODULE_INFO
{
	PVOID ModuleImageBase;
	PVOID IatTableAddress;
	PVOID EatTableAddress;
	PVOID MainEntry;
	PVOID NtHeaderAddress;
	PVOID SectionHeadersAddress;
	MODULE_SOURCE Source;
	DWORD ModuleSize;
	DWORD IatTableSize;
	DWORD EatTableSize;
	DWORD SectionHeaderCount;
	std::string ModulePath;
	std::string ModuleName;
}MODULE_INFO;

//用于查询函数地址对应的API的详细信息，以函数地址区分函数地址，可能出现重名
typedef struct _API_INFO
{
	PVOID ApiEntryAddress;
	PVOID ExportingDllImageBase;	//导出此API的模块
	DWORD Oridinal;
	std::string ApiName;
}API_INFO;

union DBL_USE_DWORD
{
	DWORD cnt;
	float frq;
};

/*

	【数据清洗模块】

*/

//函数调用模块数据
typedef struct _PICKED_DATA_ENTRY
{
	DBL_USE_DWORD CmpVal;
	DWORD TblIndex;
}PICKED_DATA_ENTRY;

//数据类型，影响数据的处理方式
enum PICK_DATA_TYPE
{
	Count_All_Time,
	Count_Recent,
	Frequency_Recent
};

//过滤方式，旨在于去除元素
enum FILTER_TYPE
{
	No_Filter,
	In_Range_Value,		//取值在某个范围内
	Non_Zero_Value,		//只存在非零值
	Specific_Value,		//仅限计数
	Specific_Multiple	//仅限计数
};

//排序方式，影响数据的排列方式
enum SORT_TYPE
{
	No_Sort,
	Top_Down,
	Bottom_Up,
	Close_To_Specific_Value,	//仅限频率
	Close_To_Specific_Multiple	//仅限频率
};

typedef struct _FILTER_FLAG
{
	FILTER_TYPE type;
	DBL_USE_DWORD extra_param_1;
	DBL_USE_DWORD extra_param_2;
}FILTER_FLAG;

typedef struct _SORT_FLAG
{
	SORT_TYPE type;
	DBL_USE_DWORD extra_param_1;
}SORT_FLAG;


typedef struct IAT_TASK_ENTRY
{
	PVOID ToWriteAddress;
	PVOID OriFuncEntry;
	PVOID HookedAddress;
};

//模块IAT数据包。用于提供钩取的目标
typedef struct _MODULE_IAT_PACK
{
	PVOID DllImageBase;
	DWORD IatTableRva;
	DWORD IatTableSize;
	DWORD BeginIndex;
	DWORD EndIndex;
	std::vector<IAT_TASK_ENTRY> IatHookList;
}MODULE_IAT_PACK;

enum RECORD_SOURCE
{
	RS_ImportAddressTable,
	RS_GetProcAddress
};

typedef struct _CALL_RECORD_DETAILS
{
	PVOID func_address;
	PVOID overwritten_address;
	PVOID dispatching_entry_address;
	std::string funcName;
	std::string importerName;
	std::string exporterName;
	RECORD_SOURCE source;
}CALL_RECORD_DETAILS;