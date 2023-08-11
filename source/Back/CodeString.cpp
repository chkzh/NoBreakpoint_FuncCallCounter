#include "CodeString.h"
#include "x96.h"

/*

	分发器补丁（X64）

重定位项：
	1. 数据基地址

*/

#ifdef _WIN64
void InitAsDispatcherHead(CodeString& cs)
{
	cs.the_data = {
	/*++
		+  0		push rax
		+  1		push rdx
		+  2		push rcx
		+  3		mov rcx, {数据段基址}
		+ 13		mov rdx, qword ptr ss: [rsp + 0x18]
		+ 18		shl rdx, 4
		+ 22		lock inc qword ptr ds: [rdx + rcx]
		+ 27		mov rax, qword ptr ds: [rcx + rdx + 0x8]
		+ 32		qword ptr ds: [rsp + 0x18], rax
		+ 37		pop rcx
		+ 38		pop rdx
		+ 39		pop rax
		+ 40		ret
	--*/
		0x50,
		0x52,
		0x51,
		0x48, 0xB9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x48, 0x8B, 0x54, 0x24, 0x18,
		0x48, 0xC1, 0xE2, 0x04,
		0xF0, 0x48, 0xFF, 0x04, 0x0A,
		0x48, 0x8B, 0x44, 0x11, 0x08,
		0x48, 0x89, 0x44, 0x24, 0x18,
		0x59,
		0x5A,
		0x58,
		0xC3
	};

	RelocationEntry re_data_seg;
	re_data_seg.size = 8;
	re_data_seg.rva = 5;
	cs.relocation_table.push_back(re_data_seg);
}

void RelocateDispatcherHead(CodeString& cs, UPVOID data_segment_address)
{
	cs.relocate(0, data_segment_address);
}

void Wow64_InitAsDispatcherHead(CodeString& cs)
{
	cs.the_data = {
		/*++
			+  0	push eax
			+  1	push edx
			+  2	mov edx, dword ptr ss: [esp + 0x8]
			+  6	shl edx, 4
			+  9	lock add dword ptr ds: [edx + {数据段基址}], 1
			+ 17	lock adc dword ptr ds: [edx + {数据段基址 + 4}], 0
			+ 25	mov eax, dword ptr ds: [edx + {数据段基址 + 8}]
			+ 31	mov dword ptr ds: ss: [esp + 0x8], eax
			+ 35	pop edx
			+ 36	pop eax
			+ 37	ret
		--*/
			0x50,
			0x52,
			0x8B, 0x54, 0x24, 0x08,
			0xC1, 0xE2, 0x04,
			0xF0, 0x83, 0x82, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
			0xF0, 0x83, 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
			0x8B, 0x82, 0xFF, 0xFF, 0xFF, 0xFF,
			0x89, 0x44, 0x24, 0x08,
			0x5A,
			0x58,
			0xC3
	};

	RelocationEntry re_0, re_1, re_2;
	re_0.size = 4;
	re_0.rva = 12;
	re_1.size = 4;
	re_1.rva = 20;
	re_2.size = 4;
	re_2.rva = 27;
	cs.relocation_table.push_back(re_0);
	cs.relocation_table.push_back(re_1);
	cs.relocation_table.push_back(re_2);
}

void Wow64_RelocateDispatcherHead(CodeString& cs, UPVOID DataSegmentAddress)
{
	cs.relocate(0, DataSegmentAddress);
	cs.relocate(1, DataSegmentAddress + 4);
	cs.relocate(2, DataSegmentAddress + 8);
}

#else

/*

	分发器补丁（X86）

重定位项：
	1. 数据基地址（计数的低位）
	2. 数据基地址+4（计数的高位）
	3. 数据基地址+8（函数原地址）

*/
void InitAsDispatcherHead(CodeString& cs)
{
	cs.the_data = {
	/*++
		+  0	push eax
		+  1	push edx
		+  2	mov edx, dword ptr ss: [esp + 0x8]
		+  6	shl edx, 4
		+  9	lock add dword ptr ds: [edx + {数据段基址}], 1
		+ 17	lock adc dword ptr ds: [edx + {数据段基址 + 4}], 0
		+ 25	mov eax, dword ptr ds: [edx + {数据段基址 + 8}]
		+ 31	mov dword ptr ds: ss: [esp + 0x8], eax
		+ 35	pop edx
		+ 36	pop eax
		+ 37	ret
	--*/
		0x50,
		0x52,
		0x8B, 0x54, 0x24, 0x08,
		0xC1, 0xE2, 0x04,
		0xF0, 0x83, 0x82, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
		0xF0, 0x83, 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
		0x8B, 0x82, 0xFF, 0xFF, 0xFF, 0xFF,
		0x89, 0x44, 0x24, 0x08,
		0x5A,
		0x58,
		0xC3
	};

	RelocationEntry re_0, re_1, re_2;
	re_0.size = 4;
	re_0.rva = 12;
	re_1.size = 4;
	re_1.rva = 20;
	re_2.size = 4;
	re_2.rva = 27;
	cs.relocation_table.push_back(re_0);
	cs.relocation_table.push_back(re_1);
	cs.relocation_table.push_back(re_2);
}

void RelocateDispatcherHead(CodeString& cs, UPVOID DataSegmentAddress)
{
	cs.relocate(0, DataSegmentAddress);
	cs.relocate(1, DataSegmentAddress + 4);
	cs.relocate(2, DataSegmentAddress + 8);
}

#endif

/*++

	分发表项补丁（X96）

重定位项：
	1. 函数序号
	2. 分发表偏移地址

--*/
void InitAsDispatchingEntry(CodeString& cs)
{
	cs.the_data = {
	/*
		+  0	push {函数序号}
		+  5	jmp {分发表入口相对地址}
	*/
		0x68, 0xFF, 0xFF, 0xFF, 0xFF,
		0xE9, 0xFF, 0xFF, 0xFF, 0xFF
	};

	RelocationEntry re_function_number, re_entry_rva;
	re_function_number.rva = 1;
	re_function_number.size = 4;
	re_entry_rva.rva = 6;
	re_entry_rva.size = 4;
	cs.relocation_table.push_back(re_function_number);
	cs.relocation_table.push_back(re_entry_rva);
}

void RelocateDispatchingEntry(CodeString& cs, DWORD FunctionNumber, DWORD EntryRva)
{
	cs.relocate(0, FunctionNumber);
	cs.relocate(1, EntryRva);
}

void InitAsGetProcAddrHook(CodeString& cs)
{
	cs.the_data = {
		0x48, 0x89, 0x4C, 0x24, 0x08,
		0x48, 0x89, 0x54, 0x24, 0x10,
		0x48, 0x83, 0xEC, 0x20,
		0xE8, 0xF2, 0xBB, 0xF9, 0xFF,
		0x48, 0x83, 0xC4, 0x20,
		0xCC,
		0xC3
	};
	
	RelocationEntry GetProcAddr_Rva;
	GetProcAddr_Rva.rva = 15;
	GetProcAddr_Rva.size = 4;

	cs.relocation_table.push_back(GetProcAddr_Rva);
}

void RelocateGetProcAddrHook(CodeString& cs, PVOID real_addr, PVOID install_addr, PVOID* breakpoint_address)
{
	DWORD rva = (UPVOID)real_addr - (UPVOID)install_addr - 19;
	cs.relocate(0, rva);

	if (breakpoint_address)
		*breakpoint_address = 23 + (PUCHAR)install_addr;
}

void Wow64_InitAsGetProcAddrHook(CodeString& cs)
{
	cs.the_data = {
	0xFF, 0x74, 0x24, 0x08,
	0xFF, 0x74, 0x24, 0x08,
	0xE8, 0xF5, 0xEC, 0xFE, 0xFD,
	0xCC,
	0xC2, 0x08, 0x00
	};

	RelocationEntry GetProcAddr_Rva;
	GetProcAddr_Rva.rva = 9;
	GetProcAddr_Rva.size = 4;

	cs.relocation_table.push_back(GetProcAddr_Rva);
}

void Wow64_RelocateGetProcAddrHook(CodeString& cs, PVOID real_addr, PVOID install_addr, PVOID* breakpoint_address)
{
	DWORD rva = (UPVOID)real_addr - (UPVOID)install_addr - 13;
	cs.relocate(0, rva);

	if (breakpoint_address)
		*breakpoint_address = (PUCHAR)install_addr + 13;
}

/*

	地址对齐，理论上，函数首地址要求是0x10（也就是16）

*/
void RoundUpCodeStringWithBytes(CodeString& cs, unsigned char byte_to_fill, DWORD round_up_size)
{
	while (cs.size() % round_up_size)
	{
		cs.the_data.push_back(byte_to_fill);
	}
}