#pragma once

#include "x96.h"
#include <vector>
#include <windows.h>

struct RelocationEntry
{
	int rva;
	int size;
};

//默认入口点为首地址
class CodeString
{
public:
	std::vector<unsigned char> the_data;
	std::vector<RelocationEntry> relocation_table;
	//PVOID entry_rva;

public:
	CodeString()
	{
		//entry_rva = 0;
	}

	unsigned char* data()
	{
		return the_data.data();
	}

	int size()
	{
		return the_data.size();
	}

	//重定位项最大为8个字节，默认传入最大值，然后该读几个字节就读几个字节
	void relocate(int index, unsigned long long value)
	{
		int i;
		int size = relocation_table[index].size;
		int rva = relocation_table[index].rva;
		for (i = 0; i < size; i++)
		{
			the_data[rva + i] = ((char*)&value)[i];
		}
		//relocation_table[index].fixed = true;
	}

	//bool isRelocationFinished()
	//{
	//	int i;
	//	for (i = 0; i < relocation_table.size(); i++)
	//	{
	//		if (relocation_table[i].fixed == false)
	//			return false;
	//	}
	//	return true;
	//}

	//void resetRelocationState()
	//{
	//	int i;
	//	for (i = 0; i < relocation_table.size(); i++)
	//	{
	//		relocation_table[i].fixed = false;
	//	}
	//}

};

//分发表表头
void InitAsDispatcherHead(CodeString& cs);
void RelocateDispatcherHead(CodeString& cs, UPVOID DataSegmentAddress);

//x64对x86版本
void Wow64_InitAsDispatcherHead(CodeString& cs);
void Wow64_RelocateDispatcherHead(CodeString& cs, UPVOID DataSegmentAddress);

//分发表表项
void InitAsDispatchingEntry(CodeString& cs);
void RelocateDispatchingEntry(CodeString& cs, DWORD FunctionNumber, DWORD EntryRva);

//GetProcAddress拦截钩子
void InitAsGetProcAddrHook(CodeString& cs);
void RelocateGetProcAddrHook(CodeString& cs, PVOID real_addr, PVOID hooked_addr, PVOID* breakpoint_addr);

void Wow64_InitAsGetProcAddrHook(CodeString& cs);
void Wow64_RelocateGetProcAddrHook(CodeString& cs, PVOID real_addr, PVOID hooked_addr, PVOID* breakpoint_addr);