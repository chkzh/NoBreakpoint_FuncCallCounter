#include "CmdRecv.h"

int ExecCmd();
void InitCmd();

int main()
{
	system("chcp 65001");
	int result;
	InitCmd();
	do
	{
		result = ExecCmd();
	} while (result >= 0);

	return 0;
}