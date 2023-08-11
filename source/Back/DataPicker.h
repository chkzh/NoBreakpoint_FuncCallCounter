#pragma once
#include "GlobalStruct.h"
#include "MyProc.h"
#include <vector>

class DataSorter
{
private:
	PICK_DATA_TYPE Type;
	SORT_FLAG flag;


public:

	DataSorter(PICK_DATA_TYPE type, SORT_FLAG& flag)
	{
		Type = type;
		this->flag = flag;
	}

	bool operator () (PICKED_DATA_ENTRY& a, PICKED_DATA_ENTRY& b)
	{
		return judgeSort(a, b);
	}

private:
	bool judgeSort(PICKED_DATA_ENTRY& new_ent, PICKED_DATA_ENTRY& old_ent);
};

//struct DATA_PICK_SOURCE
//{
//	PVOID p_source;
//	DWORD begin_index;
//	DWORD end_index;
//};


class DataPicker
{
private:
	PICK_DATA_TYPE Type;
	SORT_FLAG SortFlag;
	FILTER_FLAG FilterFlag;
	//std::vector<FILTER_FLAG> FilterFlags;

	//std::vector<DATA_PICK_SOURCE> DataSource;

	unsigned long long LastUpdateTime;
	unsigned long long CurUpdateTime;

	MyProc* proc;

public:
	std::vector<PICKED_DATA_ENTRY> DataArray;

	int ChangedCount;
	//std::vector<CALL_DATA_ENTRY>* p_records;


	DataPicker()
	{
		this->Type = Count_All_Time;
		this->SortFlag.type = Top_Down;
		this->FilterFlag.type = No_Filter;
		//this->proc = proc;

		//p_records = NULL;

		LastUpdateTime = NULL;
		CurUpdateTime = NULL;
		proc = NULL;

		this->ChangedCount = 0;
	}

	void bind(MyProc* pc)
	{
		this->proc = pc;
	}

	std::vector<PICKED_DATA_ENTRY>& data()
	{
		return this->DataArray;
	}

	//void setDataSource(std::vector<CALL_DATA_ENTRY>& source);

	//PICK_DATA_TYPE dataFlag()
	//{
	//	return this->Type;
	//}

	void setDataFlag(PICK_DATA_TYPE Type)
	{
		this->Type = Type;
	}

	//void setSortFlag(SORT_FLAG& Flag)
	//{
	//	SortFlag.type = Flag.type;
	//	SortFlag.extra_param_1 = Flag.extra_param_1;
	//}

	void setSortFlag(SORT_TYPE type, float param)
	{
		SortFlag.type = type;
		SortFlag.extra_param_1.frq = param;
	}

	void setSortFlag(SORT_TYPE type, DWORD param)
	{
		SortFlag.type = type;
		SortFlag.extra_param_1.cnt = param;
	}

	//void addFilterFlag(FILTER_FLAG& Flag)
	//{
	//	this->FilterFlags.push_back(Flag);
	//}

	void setFilterFlag(FILTER_TYPE type, float param_1, float param_2)
	{
		this->FilterFlag.type = type;
		this->FilterFlag.extra_param_1.frq = param_1;
		this->FilterFlag.extra_param_2.frq = param_2;
	}

	void setFilterFlag(FILTER_TYPE type, DWORD param_1, DWORD param_2)
	{
		//FILTER_FLAG flag;
		this->FilterFlag.type = type;
		this->FilterFlag.extra_param_1.cnt = param_1;
		this->FilterFlag.extra_param_2.cnt = param_2;
		//this->FilterFlags.push_back(flag);
	}

	void clear()
	{
		DataArray.clear();
		//DataSource.clear();
	}

	void clearFlag()
	{
		this->SortFlag.type = Top_Down;
		this->FilterFlag.type = No_Filter;
	}

	void sort(int LimitSize);

	//void pickWithFilters(std::vector<CALL_DATA_ENTRY>& Raw, DWORD time_cur, DWORD time_last);
	void pick();

	void print();

	//std::vector<CALL_DATA_ENTRY>* getDataSource(DWORD data_array_index);

private:
	bool judgeFilter(PICKED_DATA_ENTRY& ent, FILTER_FLAG& flag);

};
