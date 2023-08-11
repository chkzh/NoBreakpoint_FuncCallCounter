#pragma once
#include "GlobalFuncAndVar.h"
#include <map>

/*

	模块下的自带全局变量

	（这些属于私有变量）

*/

extern char lpExeDirectory[MAX_PATH];
extern char lpSystemDirectory[MAX_PATH];
extern char lpWow64SystemDirectory[MAX_PATH];

/*

	 进行对远端内存模块的解析，获取模块相关的任何信息

	 * 兼容x86 x64

	 * 一个进程对应一个对象

*/
class ModuleParser
{
private:
	bool IsWow64;
	HANDLE ProcHandle;

public:

	PVOID ExeImageBase;
	std::map<PVOID, API_INFO> ApiInfoBook;
	std::map<PVOID, MODULE_INFO> ModuleInfoBook;

public:
	ModuleParser(HANDLE ProcHandle, bool IsWow64, PVOID ExeImageBase)
	{
		this->IsWow64 = IsWow64;
		this->ProcHandle = ProcHandle;
		this->ExeImageBase = ExeImageBase;

		this->tryAddModule(ExeImageBase, NULL);
	}

	~ModuleParser()
	{
		lpExeDirectory[0] = 0;
	}

	std::map<PVOID, MODULE_INFO>& modules()
	{
		return this->ModuleInfoBook;
	}

	std::map<PVOID, API_INFO>& apis()
	{
		return this->ApiInfoBook;
	}

	MODULE_INFO* queryModule(PVOID ModuleBaseAddress)
	{
		auto ite = this->ModuleInfoBook.find(ModuleBaseAddress);
		if (ite == this->ModuleInfoBook.end())
			return NULL;
		return &(ite->second);
	}

	MODULE_INFO* querySource(PVOID ApiEntry)
	{
		auto ite = this->ModuleInfoBook.begin();
		while (ite != this->ModuleInfoBook.end())
		{
			PVOID ModuleEndAddr = (PUCHAR)ite->first + ite->second.ModuleSize;
			if (ApiEntry >= ite->first && ApiEntry <= ModuleEndAddr)
				return &(ite->second);
			ite++;
		}

		return NULL;
	}

	API_INFO* queryApi(PVOID ApiEntry)
	{
		auto ite = this->ApiInfoBook.find(ApiEntry);
		if (ite == this->ApiInfoBook.end())
			return NULL;
		return &(ite->second);
	}
	
	//遍历模块
	bool walkAddressSpace();

	//遍历导入表
	bool walkIatTable(PVOID ModuleBaseAddress, MODULE_IAT_PACK* p_ToHook);

	//打印模块
	void printModules();

	//模仿GetProcAddress的函数
	PVOID getProcAddr(PVOID dll_image_base, std::string& func_name, PVOID* export_table_addr);
	PVOID getProcAddr(PVOID dll_image_base, DWORD oridinal, PVOID* export_table_addr);

	//提取远端的字符串
	void readString(PVOID addr, std::string& str);

	//尝试添加模块，同时对模块进行解析
	MODULE_INFO* tryAddModule(PVOID DllImageBase, bool* is_added_this_time);
	void tryAddApi(PVOID ApiEntry, const char* lpName, DWORD Oridinal, bool* is_added_this_time);

	bool getComment(PVOID address, std::string& str);

	//转向新的模块
	void reset(HANDLE hProc, bool is_wow_64, PVOID ImageBase)
	{
		this->ApiInfoBook.clear();
		this->ModuleInfoBook.clear();

		this->ProcHandle = hProc;
		this->IsWow64 = is_wow_64;
		this->ExeImageBase = ImageBase;
	}

	MODULE_SOURCE FigureOutModuleSource(std::string& ModulePath);

	bool getModuleExecutionGap(PVOID DllImageBase, PVOID* GapPtrSlot, DWORD* GapSizeSlot);

private:

	//遍历IAT表的两个版本的函数
	bool WalkIatTableOfTheModule(PVOID ModuleBaseAddress, MODULE_IAT_PACK* p_ToHook);
	bool Wow64_WalkIatTableOfTheModule(PVOID ModuleBaseAddress, MODULE_IAT_PACK* p_ToHook);

};