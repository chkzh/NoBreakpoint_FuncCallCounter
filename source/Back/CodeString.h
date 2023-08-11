#pragma once

#include "x96.h"
#include <vector>
#include <windows.h>

struct RelocationEntry
{
	int rva;
	int size;
};

//Ĭ����ڵ�Ϊ�׵�ַ
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

	//�ض�λ�����Ϊ8���ֽڣ�Ĭ�ϴ������ֵ��Ȼ��ö������ֽھͶ������ֽ�
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

//�ַ����ͷ
void InitAsDispatcherHead(CodeString& cs);
void RelocateDispatcherHead(CodeString& cs, UPVOID DataSegmentAddress);

//x64��x86�汾
void Wow64_InitAsDispatcherHead(CodeString& cs);
void Wow64_RelocateDispatcherHead(CodeString& cs, UPVOID DataSegmentAddress);

//�ַ������
void InitAsDispatchingEntry(CodeString& cs);
void RelocateDispatchingEntry(CodeString& cs, DWORD FunctionNumber, DWORD EntryRva);

//GetProcAddress���ع���
void InitAsGetProcAddrHook(CodeString& cs);
void RelocateGetProcAddrHook(CodeString& cs, PVOID real_addr, PVOID hooked_addr, PVOID* breakpoint_addr);

void Wow64_InitAsGetProcAddrHook(CodeString& cs);
void Wow64_RelocateGetProcAddrHook(CodeString& cs, PVOID real_addr, PVOID hooked_addr, PVOID* breakpoint_addr);