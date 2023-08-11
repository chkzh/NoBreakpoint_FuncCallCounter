#include "ModuleParser.h"
#include "ȫ�ֺ�����ȫ�ֱ���.h"

const char* lpTestExe = "D:\\TraceMe.exe";

void test_1();
void test_2();

int main()
{
	test_1();
	return 0;
}

//��϶���ҹ���
void test_1()
{
	//HANDLE hJob;
	HANDLE hProc;
	UINT Pid;
	CreateProc(lpTestExe, &hProc, &Pid, NULL, NULL);
	bool IsWow64 = IsWow64ModeOn(Pid);
	PVOID ExeImageBase = GetProcImageBase(hProc, NULL);
	//SetKillChildProcOnClose(hProc, &hJob);
	ModuleParser parser(hProc, IsWow64, ExeImageBase);
	Sleep(200);

	parser.walkAddressSpace();
	auto ite = parser.ModuleInfoBook.begin();
	while (ite != parser.ModuleInfoBook.end())
	{
		printf("  %p\t", ite->first);
		printf("%s\t", ite->second.ModuleName.c_str());

		PVOID free_ptr;
		DWORD free_size;

		bool result = parser.getModuleExecutionGap(ite->first, &free_ptr, &free_size);

		if (!result)
			printf("������ʧ�ܣ�\n");
		else {
			printf("%p\t", free_ptr);
			printf("%d\n", free_size);
		}

		ite++;
	}
}

void test_2()
{
	HANDLE hProc;
	UINT Pid;
	CreateProc(lpTestExe, &hProc, &Pid, NULL, NULL);
	bool IsWow64 = IsWow64ModeOn(Pid);
	PVOID ExeImageBase = GetProcImageBase(hProc, NULL);
	ModuleParser parser(hProc, IsWow64, ExeImageBase);
	Sleep(2000);
}