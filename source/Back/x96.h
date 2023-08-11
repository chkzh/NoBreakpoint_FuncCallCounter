#pragma once
#include <windows.h>

/*

	WINDOWS下的各种重要常数值

*/

#ifdef _WIN64	//for x64 program

typedef ULONG64 UPVOID;
#define RVA_PEB_PebBaseAddress 0x10
#define RVA_PEB_BeingDebugged 0x2
#define RVA_PEB_NtGlobalFlag 0xBC
#define RVA_PEB_ProcessHeaps 0xF0

#define RVA_ProcessHeap_HeapFlag 0x70
#define RVA_ProcessHeap_HeapForceFlag 0x74

#define RVA_NT_EntryPoint 0x28
#define RVA_NT_ImportTable 0x90
#define RVA_NT_BoundTable 0xE0

#define ip Rip
#define sp Rsp
#define ax Rax

#else			//for x86 program

typedef ULONG32 UPVOID;
#define RVA_PEB_PebBaseAddress 0x8
#define RVA_PEB_BeingDebugged 0x2
#define RVA_PEB_NtGlobalFlag 0x68
#define RVA_PEB_ProcessHeaps 0x90

#define RVA_ProcessHeap_HeapFlag 0x40
#define RVA_ProcessHeap_HeapForceFlag 0x44

#define RVA_NT_EntryPoint 0x28
#define RVA_NT_ImportTable 0x80
#define RVA_NT_BoundTable 0xD0

#define ip Eip
#define sp Esp
#define ax Eax

#endif

typedef ULONG64 QWORD;

#define DOS_MAGIC 0x5A4D
#define NT_MAGIC 0x4550
#define RVA_DOS_NtHeader 0x3C

#define WOW64_EXCEPTION_BREAKPOINT 0x4000001F