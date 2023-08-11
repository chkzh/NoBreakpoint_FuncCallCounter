#pragma once
#include <windows.h>
#include <string>

#define MAX_BUFFER_SIZE 1024
#define MAX_ARG_COUNT 16

//union UNKNOWN_TYPE
//{
//	char chr;
//	float flt;
//	DWORD dw;
//	ULONG64 qw;
//	PVOID ptr;
//};

class CmdRecv
{
private:
	char* lpCmdBuffer;

public:
	char* argv[MAX_ARG_COUNT];
	DWORD argc;

	CmdRecv()
	{
		lpCmdBuffer = new char[MAX_BUFFER_SIZE];
		lpCmdBuffer[MAX_BUFFER_SIZE - 1] = '\0';

		argc = 0;

		memset(argv, 0, sizeof(char*) * MAX_ARG_COUNT);
	}

	~CmdRecv()
	{
		delete[] lpCmdBuffer;
	}

	//代表
	int input();

	//std::string _get(int index);

	bool get(int index, std::string& str);

	bool get(int index, int& val);

	bool get(int index, float& val);

	void print();

	int parse(char* lpCmd);

private:
	bool _cmp(char* lpA, char* lpB, int cmp_length);
	char* _tryGetNextStr(char*);
};