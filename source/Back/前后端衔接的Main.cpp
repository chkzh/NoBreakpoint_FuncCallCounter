#include "CmdRecv.h"
#include "DataPicker.h"
#include "ModuleParser.h"
#include "MyDbg.h"
#include "MyInject.h"
#include "MyProc.h"

enum PROC_MODE {
	Inject,
	Debug
};

bool IsProcessing = false;
std::string CurExePath;
MyProc* UsingProc;
MyDbg LoadedDbg;
MyInject LoadedInject;
DataPicker picker(UsingProc);

void UninstallHooks();
void InstallAndStart();
void ApplyDataPickSettings();

void RefreshData();
void GetMoreInfo(int index, int index_2);

//void ShowHelp();
void ExecuteCommand();

void OpenFile();
void StartProc();
void Restart();
void Detach();
void StopProc();

int main()
{
	return 0;
}

/*

	实现操作：选择一个可执行文件，获取路径

*/
void OpenFile()
{
	//获取路径字符串。
	
}

/*
	
	实现操作：恢复被挂起的进程

	* 锁死模式选择，代表出现的模式间的区别。
	* （仅调试模式）锁死除数据提取外的所有控件
	* （仅注入模式）安装与撤销其中会有一个锁死
*/
void StartProc()
{
	/* 进行前端操作，同时定好模式 */
	bool is_dbg_mode = false;

	if (is_dbg_mode)
		UsingProc = &LoadedDbg;
	else
		UsingProc = &LoadedInject;

	UsingProc->open(CurExePath.c_str());
}

/*

	实现操作：终止当前进程，按照之前的路径创建进程，并挂起。

	* 重启后，重新选择监视模式

*/
void Restart()
{
	UsingProc->open(CurExePath.c_str());
}

/*

	实现操作：断开调试器连接

	* 仅调试模式功能，注入模式请禁用

*/
void Detach()
{
	UsingProc->detach();
}

/*

	实现操作：终止进程

*/
void StopProc()
{
	UsingProc->stop();
}

/*

	执行代码，返还结果

*/
void ExecuteCommand()
{
	
}

/*
	
	需要能获取最大打印数

*/
void RefreshData()
{
	picker.pick();
	picker.sort(30);
}

/*

	获取更多信息

*/
void GetMoreInfo(int row, int column)
{
	PVOID Address = UsingProc->locateEntry(row - 1);
}

/*

	应用设置，没啥好说的

*/
void ApplyDataPickSettings()
{
	
}

//
void InstallHooksAndStart()
{
	UsingProc->install();
}

void UninstallHooks()
{
	UsingProc->uninstall();
}

void InstallAndStart()
{

}