#include "ModuleParser.h"
#include "GlobalFuncAndVar.h"
#include <windows.h>
#include <psapi.h>


bool ModuleParser::walkIatTable(PVOID DllImageBase, MODULE_IAT_PACK* p_task)
{
	if (p_task)
		p_task->IatHookList.clear();

	if (this->IsWow64)
		return this->Wow64_WalkIatTableOfTheModule(DllImageBase, p_task);
	else
		return this->WalkIatTableOfTheModule(DllImageBase, p_task);
}

//内部实现了Wow64兼容
bool ModuleParser::walkAddressSpace()
{
	this->ModuleInfoBook.clear();

	MEMORY_BASIC_INFORMATION mem_basic_info;
	SIZE_T query_result;
	char lpFileName[MAX_PATH] = { 0 };

	//获取页面大小
	SYSTEM_INFO sys_info;
	ZeroMemory(&sys_info, sizeof(SYSTEM_INFO));
	GetSystemInfo(&sys_info);

	//查找内存模块
	PVOID address = sys_info.lpMinimumApplicationAddress;
	while (address < sys_info.lpMaximumApplicationAddress)
	{
		ZeroMemory(&mem_basic_info, sizeof(MEMORY_BASIC_INFORMATION));
		query_result = VirtualQueryEx(this->ProcHandle, address, &mem_basic_info, sizeof(MEMORY_BASIC_INFORMATION));
		if (query_result == 0)
		{
			printf("[ERROR] 地址查询出错，地址：%p，LastError：%d\n", address, GetLastError());
			address = (PVOID)(sys_info.dwPageSize + (UPVOID)address);
			continue;
		}

		//正常情况下：
		switch (mem_basic_info.State)
		{
		case MEM_FREE:
		case MEM_RESERVE:
			address = (PUCHAR)address + mem_basic_info.RegionSize;
			break;
		case MEM_COMMIT:

			//找到可执行文件的内存区段
			if (mem_basic_info.Type == MEM_IMAGE){

				MODULE_INFO* p_mod = this->tryAddModule(mem_basic_info.BaseAddress, NULL);

				if (p_mod){
					address = (PUCHAR)address + p_mod->ModuleSize;

					//【调试打印】
					//printf("  %p\t%s\n", p_mod->ModuleImageBase, p_mod->ModuleName.c_str());

					break;
				}
			}
			address = (PUCHAR)address + mem_basic_info.RegionSize;
			break;
		default:
			break;
		}
	}

	return true;
}

bool ModuleParser::WalkIatTableOfTheModule(PVOID ModuleBaseAddress, MODULE_IAT_PACK* p_task)
{
	auto ite_mod = this->ModuleInfoBook.find(ModuleBaseAddress);
	if (ite_mod == this->ModuleInfoBook.end())
	{
		printf("尝试解析导出表时出现异常：找不到指定模块。\n");
		return false;
	}

	if (p_task) {
		p_task->DllImageBase = ModuleBaseAddress;
		p_task->IatTableRva = (UPVOID)ite_mod->second.IATAddress - (UPVOID)ModuleBaseAddress;
		p_task->IatTableSize = ite_mod->second.IATSize;
	}

	PIMAGE_IMPORT_DESCRIPTOR p_iid = (PIMAGE_IMPORT_DESCRIPTOR)ite_mod->second.IDTAddress;
	IMAGE_IMPORT_DESCRIPTOR iid;
	do
	{

		ReadProcessMemory(this->ProcHandle, p_iid, &iid, sizeof(iid), NULL);
		if (iid.FirstThunk == 0)
			break;

		// 
		// 解析导入表模块
		//
		PIMAGE_THUNK_DATA p_iat = (PIMAGE_THUNK_DATA)(iid.FirstThunk + (UPVOID)ModuleBaseAddress);
		PIMAGE_THUNK_DATA p_int = (PIMAGE_THUNK_DATA)(iid.OriginalFirstThunk + (UPVOID)ModuleBaseAddress);
		IMAGE_THUNK_DATA _iat, _int;
		PVOID p_name = (PVOID)(iid.Name + (UPVOID)ModuleBaseAddress);
		std::string dll_name;
		this->readString(p_name, dll_name);
		//printf("\n\timported dll name: %s\n", dll_name.c_str());

		int index = dll_name.rfind('\\');
		if (index != std::string::npos)
			dll_name = dll_name.substr(index + 1);

		do {

			//
			// 开始解析导入表项
			//
			ReadProcessMemory(this->ProcHandle, p_iat, &_iat, sizeof(_iat), NULL);
			ReadProcessMemory(this->ProcHandle, p_int, &_int, sizeof(_int), NULL);
			if (_iat.u1.Function == 0)
				break;
			std::string func_name;
			unsigned int original_id = 0;
			if (_int.u1.Ordinal & IMAGE_ORDINAL_FLAG)
			{
				//按照Oridinal值导入
				original_id = _int.u1.Ordinal ^ IMAGE_ORDINAL_FLAG;
			}
			else
			{
				//按照名称导入
				this->readString((PVOID)((UPVOID)ModuleBaseAddress + _int.u1.AddressOfData + 2), func_name);
			}

			//更新数据：
			if (func_name.empty())
				this->tryAddApi((PVOID)_iat.u1.Function, NULL, original_id, NULL);
			else
				this->tryAddApi((PVOID)_iat.u1.Function, func_name.c_str(), -1, NULL);

			//printf("\t\t%I64X\t%s\n", _iat.u1.Function, func_name.c_str());

			if (p_task) {
				IAT_TASK_ENTRY pibd;
				pibd.OriFuncEntry = (PVOID)_iat.u1.Function;
				pibd.ToWriteAddress = p_iat;
				p_task->IatHookList.push_back(pibd);
			}

			p_int++;
			p_iat++;
		} while (1);

		p_iid++;
	} while (1);

	return true;
}

bool ModuleParser::Wow64_WalkIatTableOfTheModule(PVOID ModuleBaseAddress, MODULE_IAT_PACK* p_task)
{

	auto ite_mod = this->ModuleInfoBook.find(ModuleBaseAddress);
	if (ite_mod == this->ModuleInfoBook.end())
	{
		printf("\n尝试解析导出表时出现异常：找不到指定模块。\n");
		return false;
	}
	else
	{
		printf("\n正在解析模块：%s\n", ite_mod->second.ModuleName.c_str());
	}

	if (p_task) {
		p_task->DllImageBase = ModuleBaseAddress;
		p_task->IatTableRva = (UPVOID)ite_mod->second.IATAddress - (UPVOID)ModuleBaseAddress;
		p_task->IatTableSize = ite_mod->second.IATSize;
	}

	PIMAGE_IMPORT_DESCRIPTOR p_iid = (PIMAGE_IMPORT_DESCRIPTOR)ite_mod->second.IDTAddress;
	IMAGE_IMPORT_DESCRIPTOR iid;
	do
	{

		bool res = ReadProcessMemory(this->ProcHandle, p_iid, &iid, sizeof(iid), NULL);
		if (iid.FirstThunk == 0)
			break;

		// 
		// 解析导入表模块
		//
		PIMAGE_THUNK_DATA32 p_iat = (PIMAGE_THUNK_DATA32)(iid.FirstThunk + (UPVOID)ModuleBaseAddress);
		PIMAGE_THUNK_DATA32 p_int = (PIMAGE_THUNK_DATA32)(iid.OriginalFirstThunk + (UPVOID)ModuleBaseAddress);
		IMAGE_THUNK_DATA32 _iat, _int;
		PVOID p_name = (PVOID)(iid.Name + (UPVOID)ModuleBaseAddress);

		//RemotePrintHex(this->ProcHandle, pc_ExeImageBase, 260);

		std::string dll_name;
		this->readString(p_name, dll_name);
		printf("\tDLL Name: %s\n", dll_name.c_str());

		int index = dll_name.rfind('\\');
		if (index != std::string::npos)
			dll_name = dll_name.substr(index + 1);

		do {

			//
			// 开始解析导入表项
			//
			ReadProcessMemory(this->ProcHandle, p_iat, &_iat, sizeof(_iat), NULL);
			ReadProcessMemory(this->ProcHandle, p_int, &_int, sizeof(_int), NULL);
			if (_iat.u1.Function == 0)
				break;
			std::string func_name;
			unsigned int original_id = 0;
			if (_int.u1.Ordinal & IMAGE_ORDINAL_FLAG32)
			{
				//按照Oridinal值导入
				original_id = _int.u1.Ordinal & ~(IMAGE_ORDINAL_FLAG32);
				//func_name = "Oridinal #" + std::to_string(_int.u1.Ordinal);
			}
			else
			{
				//按照名称导入
				this->readString((PVOID)((UPVOID)ModuleBaseAddress + _int.u1.AddressOfData + 2), func_name);
			}

			//更新数据：
			if (func_name.empty())
				this->tryAddApi((PVOID)_iat.u1.Function, NULL, original_id, NULL);
			else
				this->tryAddApi((PVOID)_iat.u1.Function, func_name.c_str(), -1, NULL);

			printf("\t\t%X -> %X -> %s\n", (UPVOID)p_iat, _iat.u1.Function, func_name.c_str());

			if (p_task) {
				IAT_TASK_ENTRY pibd;
				pibd.OriFuncEntry = (PVOID)_iat.u1.Function;
				pibd.ToWriteAddress = p_iat;
				p_task->IatHookList.push_back(pibd);
			}

			p_int++;
			p_iat++;
		} while (1);

		p_iid++;
	} while (1);

	return true;
}

void ModuleParser::printModules()
{
	printf("\n已解析模块如下：\n------------------\n");
	auto ite = this->ModuleInfoBook.begin();
	while (ite != this->ModuleInfoBook.end())
	{

		printf("%p\t", ite->first);
		printf("%s", ite->second.ModulePath.c_str());

		printf("\n");
		ite++;
	}
}

PVOID ModuleParser::getProcAddr(PVOID dll_image_base, std::string& function_name, PVOID* export_locate)
{
	if (function_name.empty())
		return NULL;

	MODULE_INFO* p_mod = this->tryAddModule(dll_image_base, NULL);
	if (!p_mod)
		return NULL;

	PIMAGE_EXPORT_DIRECTORY p_export_table = (PIMAGE_EXPORT_DIRECTORY)p_mod->EDTAddress;
	IMAGE_EXPORT_DIRECTORY export_table;
	PDWORD p_function_table_base;
	PDWORD p_name_table_base;	//名称数组
	PWORD p_name_oridinal_base;	//名称转索引数组

	ReadProcessMemory(this->ProcHandle, p_export_table, &export_table, sizeof(export_table), NULL);

	p_function_table_base = (PDWORD)((UPVOID)dll_image_base + export_table.AddressOfFunctions);
	p_name_table_base = (PDWORD)((UPVOID)dll_image_base + export_table.AddressOfNames);
	p_name_oridinal_base = (PWORD)((UPVOID)dll_image_base + export_table.AddressOfNameOrdinals);
	DWORD high = export_table.NumberOfNames - 1;
	DWORD low = 0;
	DWORD middle;
	std::string cur_function_name;
	PVOID p_cur_function;
	DWORD cur_function_rva;
	while (high >= low)
	{
		middle = (high + low) >> 1;

		ReadProcessMemory(this->ProcHandle, p_name_table_base + middle, &cur_function_rva, sizeof(DWORD), NULL);

		p_cur_function = (PVOID)((UPVOID)dll_image_base + cur_function_rva);
		this->readString((PUCHAR)p_cur_function, cur_function_name);
		int result = strcmp(function_name.c_str(), cur_function_name.c_str());
		if (result < 0)
			high = middle - 1;
		else if (result > 0)
			low = middle + 1;
		else
			break;
	}

	if (high >= low)
	{
		WORD cur_index;
		//this->read(p_name_oridinal_base + middle, &cur_index, sizeof(WORD));
		ReadProcessMemory(this->ProcHandle, p_name_oridinal_base + middle, &cur_index, sizeof(WORD), NULL);

		PVOID location = p_function_table_base + cur_index;
		DWORD function_rva;
		ReadProcessMemory(this->ProcHandle, location, &function_rva, sizeof(DWORD), NULL);
		if (export_locate)
			*export_locate = location;

		PUCHAR real_address = function_rva + (PUCHAR)dll_image_base;

		//更新信息：
		this->tryAddApi(real_address, (char*)function_name.c_str(), -1, NULL);

		return real_address;
	}
	else
	{
		printf("【ERR】GetProcAddress 查找失败！\n");
	}

	if (export_locate)
		*export_locate = NULL;
	return NULL;
}


PVOID ModuleParser::getProcAddr(PVOID dll_image_base, DWORD oridinal, PVOID* export_table_addr)
{
	MODULE_INFO* p_mod = this->tryAddModule(dll_image_base, NULL);
	if (!p_mod)
		return NULL;

	PIMAGE_EXPORT_DIRECTORY p_export_table = (PIMAGE_EXPORT_DIRECTORY)p_mod->EDTAddress;
	IMAGE_EXPORT_DIRECTORY export_table;
	PDWORD p_function_table_base;
	PDWORD p_name_table_base;
	PWORD p_name_oridinal_base;

	ReadProcessMemory(this->ProcHandle, p_export_table, &export_table, sizeof(IMAGE_EXPORT_DIRECTORY), NULL);

	p_function_table_base = (PDWORD)((UPVOID)dll_image_base + export_table.AddressOfFunctions);
	DWORD index = oridinal - export_table.Base;

	if (index >= export_table.AddressOfFunctions)
	{
		printf("【ERR】GetProcAddr出错：Oridinal索引越界。\n");
		return NULL;
	}

	PVOID entry_location = p_function_table_base + index;
	DWORD func_rva;
	ReadProcessMemory(this->ProcHandle, entry_location, &func_rva, sizeof(DWORD), NULL);

	if (export_table_addr)
		*export_table_addr = entry_location;

	PUCHAR real_address = func_rva + (PUCHAR)dll_image_base;

	//更新信息：
	this->tryAddApi(real_address, NULL, oridinal, NULL);

	return real_address;

	//return (PUCHAR)dll_image_base + func_rva;
}

void ModuleParser::readString(PVOID StrAddr, std::string& str)
{
	str.clear();

	int i;
	char temp[0x20] = { 0 };
	do {
		ReadProcessMemory(this->ProcHandle, StrAddr, temp, 0x20, NULL);

		for (i = 0; i < 0x20; i++)
		{
			if (temp[i] > 126 && temp[i] < 32 && str.size() >= MAX_PATH)
			{
				str.clear();
				goto StopRead;
			}

			if (temp[i])
				str.push_back(temp[i]);
			else
				goto StopRead;
		}

		StrAddr = (PUCHAR)StrAddr + 0x20;

	} while (1);

StopRead:
	if (str.size() == 0)
		str = "[That isn't a string]";

	return;
}

MODULE_INFO* ModuleParser::tryAddModule(PVOID DllImageBase, bool* is_added_this_time)
{
	MODULE_INFO* targ;

	auto ite = this->ModuleInfoBook.find(DllImageBase);
	if (ite != this->ModuleInfoBook.end()) {
		if (is_added_this_time)
			*is_added_this_time = false;
		targ = &ite->second;
		return targ;
	}

	WORD dos_magic;
	ReadProcessMemory(this->ProcHandle, DllImageBase, &dos_magic, sizeof(WORD), NULL);
	if (dos_magic != DOS_MAGIC){
		if (is_added_this_time)
			*is_added_this_time = false;
		printf("模块解析出错：DOS头不匹配\n");
		return NULL;
	}


	char lpFileName[MAX_PATH] = { 0 };
	if (!GetMappedFileNameA(this->ProcHandle, DllImageBase, lpFileName, MAX_PATH)) {
		if (is_added_this_time)
			*is_added_this_time = false;
		printf("模块解析出错：无法获取映射文件名称\n");
		return NULL;
	}

	//尝试解析模块基本信息
	MODULE_INFO mod_info;
	mod_info.ModuleImageBase = DllImageBase;
	mod_info.ModulePath = std::string(lpFileName);	//是设备路径。暂时没办法获取到DOS路径
	if (DllImageBase == this->ExeImageBase)
		mod_info.Source = MODULE_SOURCE::Exe;
	else
		mod_info.Source = FigureOutModuleSource(mod_info.ModulePath);
	int name_index = mod_info.ModulePath.rfind('\\') + 1;
	if (name_index != std::string::npos)
		mod_info.ModuleName = mod_info.ModulePath.substr(name_index);


	WORD nt_rva;
	ReadProcessMemory(this->ProcHandle, (PVOID)((UPVOID)mod_info.ModuleImageBase + RVA_DOS_NtHeader), &nt_rva, sizeof(WORD), NULL);
	mod_info.NtHeaderAddress = (PUCHAR)mod_info.ModuleImageBase + nt_rva;

	if (this->IsWow64) {
		IMAGE_NT_HEADERS32 nt_header;
		ReadProcessMemory(this->ProcHandle, (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_rva), &nt_header, sizeof(IMAGE_NT_HEADERS32), NULL);

		mod_info.ModuleSize = nt_header.OptionalHeader.SizeOfImage;
		mod_info.IDTAddress = (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		mod_info.EDTAddress = (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		mod_info.IATAddress = (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress);
		mod_info.IDTSize = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		mod_info.EDTSize = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
		mod_info.IATSize = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
		mod_info.MainEntry = nt_header.OptionalHeader.AddressOfEntryPoint + (PUCHAR)DllImageBase;
		mod_info.SectionHeadersAddress = (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_rva + sizeof(IMAGE_NT_HEADERS32));
		mod_info.SectionHeaderCount = nt_header.FileHeader.NumberOfSections;
	}
	else {
		IMAGE_NT_HEADERS nt_header;
		ReadProcessMemory(this->ProcHandle, (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_rva), &nt_header, sizeof(IMAGE_NT_HEADERS), NULL);

		mod_info.ModuleSize = nt_header.OptionalHeader.SizeOfImage;
		mod_info.IDTAddress = (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		mod_info.EDTAddress = (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		mod_info.IATAddress = (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress);
		mod_info.IDTSize = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		mod_info.EDTSize = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
		mod_info.IATSize = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
		mod_info.MainEntry = nt_header.OptionalHeader.AddressOfEntryPoint + (PUCHAR)DllImageBase;
		mod_info.SectionHeadersAddress = (PVOID)((UPVOID)mod_info.ModuleImageBase + nt_rva + sizeof(IMAGE_NT_HEADERS));
		mod_info.SectionHeaderCount = nt_header.FileHeader.NumberOfSections;
	}

	this->ModuleInfoBook.insert(std::pair<PVOID, MODULE_INFO>(DllImageBase, mod_info));

	auto ite_2 = this->ModuleInfoBook.find(DllImageBase);
	if (ite_2 != this->ModuleInfoBook.end()) {
		targ = &ite_2->second;
		if (is_added_this_time)
			*is_added_this_time = true;
		return targ;
	}
	else
	{
		printf("？？ 出现了不可能的错误？？\n");
		if (is_added_this_time)
			*is_added_this_time = true;
		return NULL;
	}
}

void ModuleParser::tryAddApi(PVOID ApiEntry, const char* lpName, DWORD Oridinal, bool* is_added_this_time)
{
	auto ite = this->ApiInfoBook.find(ApiEntry);
	if (ite != this->ApiInfoBook.end()){

		//检测一下这次尝试添加的信息是否更加全面：
		if (ite->second.ApiName.empty() && lpName)
			ite->second.ApiName = lpName;

		if (is_added_this_time)
			*is_added_this_time = false;
		return;
	}

	API_INFO info;
	info.ApiEntryAddress = ApiEntry;
	if (lpName)
		info.ApiName = std::string(lpName);
	info.Oridinal = Oridinal;
	info.ExportingDllImageBase = this->querySource(ApiEntry) ? this->querySource(ApiEntry)->ModuleImageBase : NULL;

	this->ApiInfoBook.insert(std::pair<PVOID, API_INFO>(ApiEntry, info));

	if (is_added_this_time)
		*is_added_this_time = true;
}


void DosPathToDevicePath(char* lpDosPath)
{
	std::string DosPath = lpDosPath;
	char lp_dos[32] = { 0 };
	char lp_device_name[32] = { 0 };

	int index = DosPath.find('\\');
	if (index == std::string::npos)
		return;

	memcpy(lp_dos, DosPath.c_str(), index);
	QueryDosDeviceA(lp_dos, lp_device_name, 32);

	if (lp_device_name[0])
	{
		DosPath.replace(0, index, lp_device_name);
		memcpy(lpDosPath, DosPath.c_str(), DosPath.length());
	}
}



//三大系统路径，用于确定DLL的来源。
char lpExeDirectory[MAX_PATH] = { 0 };
char lpSystemDirectory[MAX_PATH] = { 0 };
char lpWow64SystemDirectory[MAX_PATH] = { 0 };

//获取模块的来源
MODULE_SOURCE ModuleParser::FigureOutModuleSource(std::string& ModulePath)
{
	char lpModulePath[MAX_PATH] = { 0 };
	int index = ModulePath.rfind('\\');
	if (index == std::string::npos)
		goto ERR_EXIT;

	memcpy(lpModulePath, ModulePath.c_str(), index);
	_strlwr_s(lpModulePath);

	//补充特殊路径
	if (!lpExeDirectory[0]) {
		GetMappedFileNameA(this->ProcHandle, this->ExeImageBase, lpExeDirectory, MAX_PATH);
		_strlwr_s(lpExeDirectory);
		char* p_ch = strrchr(lpExeDirectory, '\\');
		if (p_ch)
			*p_ch = '\0';
	}
	if (!lpSystemDirectory[0]) {
		GetSystemDirectoryA(lpSystemDirectory, MAX_PATH);
		DosPathToDevicePath(lpSystemDirectory);
		_strlwr_s(lpSystemDirectory);
	}
	if (!lpWow64SystemDirectory[0]) {
		GetSystemWow64DirectoryA(lpWow64SystemDirectory, MAX_PATH);
		DosPathToDevicePath(lpWow64SystemDirectory);
		_strlwr_s(lpWow64SystemDirectory);
	}

	//printf("%s\n", lpExeDirectory);
	//printf("%s\n", lpSystemDirectory);
	//printf("%s\n", lpWow64SystemDirectory);

	if (strstr(lpModulePath, lpExeDirectory))
		return Exe_Directory;
	else if (strstr(lpModulePath, lpSystemDirectory) || strstr(lpModulePath, lpWow64SystemDirectory))
		return System_Directory;

ERR_EXIT:
	return Other_Directory;
}

bool ModuleParser::getComment(PVOID address, std::string& str)
{
	auto ite = this->ApiInfoBook.find(address);
	if (ite != this->ApiInfoBook.end()){
		str = ite->second.ApiName;
		return true;
	}

	MODULE_INFO* p_info = this->querySource(address);
	if (p_info) {
		str = p_info->ModuleName;
		return true;
	}

	str = "( ? )";
	return false;
}

//
// 尽可能地去搜索出最大的空间（一般的模块只有一个代码段）
// 理论上代码段的最后一条指令一定是跳转指令：（jmp、call，ret），需要保留4字节
// jmp ds:[addr]，此处最好保留4字节
//
bool ModuleParser::getModuleExecutionGap(PVOID DllImageBase, PVOID* GapPtrSlot, DWORD* GapSizeSlot)
{
	static unsigned char reserve_zero_cnt = 4;
	//static unsigned char read_size = 0x20;
	//char read_buffer[0x20] = { 0 };

	MODULE_INFO* p_info = tryAddModule(DllImageBase, NULL);

	//
	// 定位代码区段的范围
	//
	PVOID TextSectionPtr = NULL;
	DWORD TextSectionSize;
	IMAGE_SECTION_HEADER* p_header = (IMAGE_SECTION_HEADER*)p_info->SectionHeadersAddress;
	IMAGE_SECTION_HEADER sec_header;
	for (int i = 0; i < p_info->SectionHeaderCount; i++)
	{
		ReadProcessMemory(this->ProcHandle, p_header + i, &sec_header, sizeof(IMAGE_SECTION_HEADER), NULL);
		if (sec_header.Characteristics & IMAGE_SCN_MEM_EXECUTE)
		{
			//DWORD VirtualSize;
			MEMORY_BASIC_INFORMATION mbi;
			ZeroMemory(&mbi, sizeof(MEMORY_BASIC_INFORMATION));
			if (!VirtualQueryEx(this->ProcHandle, sec_header.VirtualAddress + (PUCHAR)DllImageBase, &mbi, sizeof(mbi)))
			{
				printf("kernel32内存区段解析失败？\n");
				return false;
			}
			TextSectionSize = mbi.RegionSize;
			TextSectionPtr = sec_header.VirtualAddress + (PUCHAR)DllImageBase;
			break;
		}
	}
	if (!TextSectionPtr)
	{
		printf("代码段查找失败\n");
		return false;
	}

	//
	// 从后往前搜索第一个非0x00字节
	//
	DWORD zero_cnt;
	DWORD total_zero_cnt = 0;
	bool check_result = false;
	char read_buffer[0x20] = { 0 };
	PUCHAR read_ptr = (PUCHAR)TextSectionPtr + TextSectionSize;
	while (1)
	{
		read_ptr -= 0x20;
		if (!ReadProcessMemory(this->ProcHandle, read_ptr, read_buffer, 0x20, NULL))
			break;

		for (zero_cnt = 0; zero_cnt < 0x20; zero_cnt++)
		{
			if (read_buffer[0x20 - 1 - zero_cnt]) {
				check_result = true;
				break;
			}
		}
		total_zero_cnt += zero_cnt;

		if (check_result)
			break;
	}

	//填充信息
	if (GapSizeSlot)
		*GapSizeSlot = total_zero_cnt - reserve_zero_cnt;
	if (GapPtrSlot)
		*GapPtrSlot = (PUCHAR)TextSectionPtr + TextSectionSize - (total_zero_cnt - reserve_zero_cnt);

	return true;
}