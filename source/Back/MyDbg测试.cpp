#include "MyDbg.h"
#include "DataPicker.h"
#include "CmdRecv.h"
#include <thread>

//��������
const char* lpTestExe = "D:\\TraceMe.exe";
const char* lpTestExe_2 = "D:\\�½��ļ���(2)\\TR\\Terraria\\Terraria.exe";
const char* lpTestExe_3 = "D:\\NewFile(2)\\1264\\The Binding of Isaac Rebirth\\isaac-ng.exe";
const char* lpTestExe_4 = "C:\\Users\\86158\\source\\repos\\WIN32 API �о�ר��\\Release\\WIN32 API �о�ר��.exe";

/*

	���󣺵�����ѭ�������ݵ���ȡ��ͬһ���߳���ʵ�֡�

	loop{
	
		debug_proc();	//��ʱ

		if( one_time_flag || loop_flag )
		show_record();
	
	}

*/

bool one_time_flag = false;
bool loop_flag = false;
bool rip_flag = false;
bool sleep_flag = false;

void Shell()
{
	CmdRecv cd;
	std::string str;
	while (1)
	{
		cd.input();
		cd.get(0, str);

		if (str == "exit")
		{
			rip_flag = true;
			break;
		}
		else if (str == "loop")
		{
			loop_flag = !loop_flag;
		}
		else if (str == "a")
		{
			one_time_flag = true;
		}
		else if (str == "p")
		{
			sleep_flag = !sleep_flag;
		}
		else
		{
			system("cls");
			printf("HELP:\n exit - close everything\n loop - show data in every 2s\n a - show data one time\n p - pause");
		}
	}
}

int main()
{
	MyDbg dbg;
	DataPicker picker((MyProc*) & dbg);
	std::thread caller(Shell);
	caller.detach();

	picker.setFilterFlag(Non_Zero_Value, (DWORD)0, 0);
	picker.setDataFlag(Frequency_Recent);

	dbg.run(lpTestExe_2);

	while (1) {
		if (dbg.loop(1) < 0)
			break;

		if (loop_flag || one_time_flag) {
			picker.clear();
			picker.pick();
			picker.sort(28);
			system("cls");
			picker.print();

			if (one_time_flag)
				one_time_flag = false;

			if (loop_flag)
				Sleep(1500);
		}

		while (sleep_flag && !rip_flag)
		{
			Sleep(2000);
		}

		if (rip_flag)
		{
			break;
		}
	}

	dbg.close();

	//caller.detach();

	return 0;
}
