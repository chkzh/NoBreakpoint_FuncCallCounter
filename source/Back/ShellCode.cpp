#include "CmdRecv.h"
#include "DataPicker.h"
#include "MyProc.h"
#include "MyDbg.h"
#include "MyInject.h"

CmdRecv cr;

bool IsProcessing = false;
std::string CurExePath;
MyDbg mydbg;
MyInject myinject;
MyProc* UsingProc = &myinject;
DataPicker picker;

/*

	【指令大全】

[1]	start - 启动程序
[1]	restart - 重启程序
[1]	close - 关闭程序
[1]	exit - 退出程序
[1]	install - 安装钩子
[1]	uninstall - 卸载钩子
[2]	open {file} - 打开文件

[3]	set mode {enum} - 模式切换

[5]	set filter {enum} {param1} {param2} - 设置过滤条件
[5]	set sort {enum} {param} {limit_size} - 设置排序条件
[3]	set data-type {enum} - 设置数据类型
[4]	set hook {enum} {bool} - 启用/禁用钩子
[3]	set hook-level {int} - 设置钩子等级


[1]	(refresh / r) - 刷新一次数据
[1]	loop - 循环刷新
[1]	(pause / p) - 循环刷新数据

[2]	(query / q) {int} - 获取更多信息


*/
int ExecCmd()
{
	cr.input();

	//【一个参数】
	std::string arg_0;
	if (!cr.get(0, arg_0))
	{
		printf("指令无效");
		return 0;
	}

	if (arg_0 == "start")
	{
		//启动
		if (!IsProcessing)
			UsingProc->open(CurExePath.c_str());

		IsProcessing = true;

		printf("尝试启动进程\n");
		return 1;
	}

	else if (arg_0 == "detach")
	{
		UsingProc->detach();

		printf("已脱离调试!\n");
		return 1;
	}

	else if (arg_0 == "restart")
	{
		//重启
		UsingProc->open(CurExePath.c_str());
		IsProcessing = true;

		printf("尝试重启进程\n");
		return 1;
	}

	else if (arg_0 == "close")
	{
		//关闭进程
		UsingProc->stop();

		printf("已终止进程\n");
		return 1;
	}

	else if (arg_0 == "exit")
	{
		//退出程序
		printf("准备退出\n");
		return -1;
	}

	else if (arg_0 == "install")
	{
		//安装钩子
		UsingProc->install();
		printf("已安装钩子\n");
		return 1;
	}

	else if (arg_0 == "uninstall")
	{
		//卸载钩子
		UsingProc->uninstall();

		printf("已卸载钩子\n");
		return 1;
	}

	else if (arg_0 == "r" || arg_0 == "refresh")
	{
		//刷新
		picker.pick();
		picker.sort(30);
		picker.print();

		printf("刷新中");
		return 1;
	}

	else if (arg_0 == "p" || arg_0 == "pause")
	{
		//暂停
		;
		return 1;
	}

	else if (arg_0 == "loop")
	{
		//循环
		;
		return 1;
	}

	else if (arg_0 == "test_1")
	{
		//设置模式：
		UsingProc = &mydbg;
		
		UsingProc->setHookType(HT_IMPORT_ADDRESS_TABLE | HT_GET_PROC_ADDRESS);
		UsingProc->setHookTarget(HM_EXE | HM_OTHER_DIRECTORY_DLLS);

		return 1;
	}

	else if (arg_0 == "test_2")
	{
		UsingProc == &myinject;

		UsingProc->setHookType(HT_IMPORT_ADDRESS_TABLE);
		UsingProc->setHookTarget(HM_EXE | HM_EXE_DIRECTORY_DLLS | HM_OTHER_DIRECTORY_DLLS);

		return 1;
	}

	else if (arg_0 == "test_3")
	{
		picker.setDataFlag(Frequency_Recent);
		picker.setFilterFlag(Non_Zero_Value, (DWORD)0, 0);
		picker.setSortFlag(Close_To_Specific_Value, (DWORD)60);

		return 1;
	}

	else if (arg_0 == "test_4")
	{
		picker.setDataFlag(Count_Recent);
		picker.setFilterFlag(In_Range_Value, (DWORD)0, 10000);
		picker.setSortFlag(Top_Down, (DWORD)60);

		return 1;
	}

	//【两个参数】
	std::string arg_1;
	if (!cr.get(1, arg_1))
	{
		//Reply = "无效的指令";
		return 0;
	}

	if (arg_0 == "open")
	{
		//尝试用arg1打开文件
		CurExePath = arg_1;
		printf("CurPath = \"%s\"\n", CurExePath.c_str());

		return 1;
	}

	else if (arg_0 == "query")
	{
		int val;
		if (!cr.get(1, val))
			return 0;

		//尝试解析信息
		int ind = picker.DataArray[val].TblIndex;
		PVOID locate = UsingProc->locateEntry(ind);
		printf("Entry Address = %p\n", locate);

		return 1;
	}

	return 0;
	//std::string arg_2;
	//if (!cr.get(2, arg_2))
	//{
	//	Reply = "无效的指令";
	//	return;
	//}
	//
	//if (arg_0 == "set")
	//{
	//	if (arg_1 == "hook-level")
	//	{
	//		//
	//	}

	//	else if (arg_1 == "data-type")
	//	{

	//	}

	//	else if (arg_1 == "mode")
	//	{

	//	}
	//}

	////【四个参数】
	//std::string arg_3;
	//if (!cr.get(3, arg_3))
	//{
	//	Reply = "无效的指令";
	//	return;
	//}
	//
	//if (arg_0 == "set" && arg_1 == "Hook");
	//{
	//	
	//}

	////【五个参数】
	//std::string arg_4;
	//if (!cr.get(4, arg_4))
	//{
	//	Reply = "无效的指令";
	//	return;
	//}

	//if (arg_0 == "set");
	//{
	//	if (arg_1 == "filter")
	//	{

	//	}

	//	else if (arg_1 == "sort")
	//	{

	//	}
	//}
}

void InitCmd()
{
	CurExePath = "D:\\TraceMe.exe";

	UsingProc = &mydbg;
	UsingProc->setHookType(HT_IMPORT_ADDRESS_TABLE | HT_GET_PROC_ADDRESS);
	UsingProc->setHookTarget(HM_EXE | HM_OTHER_DIRECTORY_DLLS);

	picker.bind(UsingProc);
	picker.setDataFlag(Frequency_Recent);
	picker.setFilterFlag(Non_Zero_Value, (DWORD)0, 0);
	picker.setSortFlag(Close_To_Specific_Value, (DWORD)60);

	UsingProc->open(CurExePath.c_str());
}