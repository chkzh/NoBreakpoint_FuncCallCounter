#include "DataPicker.h"
#include <algorithm>

bool DataPicker::judgeFilter(PICKED_DATA_ENTRY& ent, FILTER_FLAG& flag)
{

	bool is_reserve = true;
	if (this->Type == Frequency_Recent)
	{
		switch (flag.type)
		{
		case FILTER_TYPE::Non_Zero_Value:
			is_reserve = (ent.CmpVal.frq != (float)0);
			break;
		case FILTER_TYPE::In_Range_Value:
			is_reserve = (ent.CmpVal.frq >= flag.extra_param_1.frq && ent.CmpVal.frq <= flag.extra_param_2.frq);
			break;
		case FILTER_TYPE::No_Filter:
			is_reserve = true;
			break;
		default:
			is_reserve = true;
			break;
		}
	}
	else
	{
		switch (flag.type)
		{
		case FILTER_TYPE::Non_Zero_Value:
			is_reserve = (ent.CmpVal.cnt != 0);
			break;
		case FILTER_TYPE::In_Range_Value:
			is_reserve = (ent.CmpVal.cnt >= flag.extra_param_1.cnt && ent.CmpVal.cnt <= flag.extra_param_2.cnt);
			break;
		case FILTER_TYPE::Specific_Value:
			is_reserve = (ent.CmpVal.cnt == flag.extra_param_1.cnt);
			break;
		case FILTER_TYPE::Specific_Multiple:
			is_reserve = (ent.CmpVal.cnt && ent.CmpVal.cnt % flag.extra_param_1.cnt == 0);
			break;
		case FILTER_TYPE::No_Filter:
			is_reserve = true;
			break;
		default:	//可能出现无效的过滤条件
			is_reserve = true;
			break;
		}
	}

	return is_reserve;
}

void DataPicker::sort(int limit_size)
{
	if (this->SortFlag.type == No_Sort)
		return;

	if (!limit_size)
		limit_size = -1;

	std::sort(DataArray.begin(), DataArray.end(), DataSorter(this->Type, this->SortFlag));

	if (limit_size < DataArray.size())
	{
		DataArray.erase(DataArray.begin() + limit_size, DataArray.end());
	}
}

void DataPicker::pick()
{
	if (!(proc->checkProcessStatus() & PS_PROCESS_AVAILABLE))
		return;

	std::vector<CALL_DATA_ENTRY>* p_records = proc->visitLatestRecords();

	this->LastUpdateTime = this->CurUpdateTime;
	this->CurUpdateTime = GetTickCount64();

	this->DataArray.clear();

	auto ite_ori = p_records->begin();
	while (ite_ori != p_records->end())
	{

		//生成比较项
		PICKED_DATA_ENTRY ent;
		ent.TblIndex = std::distance(p_records->begin(), ite_ori);

		if (this->Type == PICK_DATA_TYPE::Count_All_Time)
		{
			ent.CmpVal.cnt = ite_ori->CurCallCount;
		}
		else if (this->Type == PICK_DATA_TYPE::Count_Recent)
		{
			ent.CmpVal.cnt = ite_ori->CurCallCount - ite_ori->LastCallCount;
		}
		else if (this->Type == PICK_DATA_TYPE::Frequency_Recent)
		{
			ent.CmpVal.frq = (float)(ite_ori->CurCallCount - ite_ori->LastCallCount) / (CurUpdateTime - LastUpdateTime) * 1000;
		}

		//仅使用单一过滤条件，理由：根本用不到多条件
		if (this->judgeFilter(ent, this->FilterFlag))
			DataArray.push_back(ent);

		ite_ori++;
	}

	proc->leaveCallRecords();

	this->ChangedCount = this->DataArray.size();
}

void DataPicker::print()
{
	if (!proc){
		printf("未绑定处理过程！\n");
		return;
	}

	if (!this->DataArray.size())
		printf(" * 空空如也 * \n");
	else
		printf("共有 %d 条记录，如下：\n", (DWORD)this->DataArray.size());

	std::string ApiName;
	std::string ImporterName;
	std::string ExporterName;
	char source = 0;

	int i = 0;
	auto ite = this->DataArray.begin();
	while (ite != this->DataArray.end())
	{
		printf("%2d  ", i++);

		if (this->Type == Frequency_Recent)
			printf("%.2f  ", ite->CmpVal.frq);
		else
			printf("%d  ", ite->CmpVal.cnt);

		CALL_RECORD_DETAILS details;
		proc->getDetails(ite->TblIndex, details);

		//printf("%I64X  ", (UPVOID)details.address);
		if (source)
			printf("[GPA]");

		printf("%s - ", details.funcName.c_str());
		printf("%s - ", details.importerName.c_str());
		printf("%s", details.exporterName.c_str());

		printf("\n");

		ite++;
	}
}

////尝试查找数据来源
//std::vector<CALL_DATA_ENTRY>* DataPicker::getDataSource(DWORD data_array_index)
//{
//	//auto ite = this->DataSource.begin();
//	//while (ite != this->DataSource.end())
//	//{
//	//	if (data_array_index >= ite->begin_index && data_array_index <= ite->end_index)
//	//		return (std::vector<CALL_DATA_ENTRY>*)ite->p_source;
//	//	ite++;
//	//}
//
//	return NULL;
//}


bool DataSorter::judgeSort(PICKED_DATA_ENTRY& new_ent, PICKED_DATA_ENTRY& old_ent)
{
	bool is_first_one_better = true;
	if (this->Type == Frequency_Recent)
	{
		switch (flag.type)
		{
		case SORT_TYPE::No_Sort:
		case SORT_TYPE::Top_Down:
			is_first_one_better = (new_ent.CmpVal.frq > old_ent.CmpVal.frq);
			break;
		case SORT_TYPE::Bottom_Up:
			is_first_one_better = (new_ent.CmpVal.frq < old_ent.CmpVal.frq);
			break;
		case SORT_TYPE::Close_To_Specific_Value:
			is_first_one_better = (fabsf(new_ent.CmpVal.frq - flag.extra_param_1.frq) < fabsf(old_ent.CmpVal.frq - flag.extra_param_1.frq));
			break;
		case SORT_TYPE::Close_To_Specific_Multiple:
		{
			float new_delta = fabsf(round(new_ent.CmpVal.frq / flag.extra_param_1.frq) * flag.extra_param_1.frq - new_ent.CmpVal.frq);
			float old_delta = fabsf(round(old_ent.CmpVal.frq / flag.extra_param_1.frq) * flag.extra_param_1.frq - old_ent.CmpVal.frq);
			is_first_one_better = (new_delta < old_delta);
		}break;
		default:
			is_first_one_better = false;
			break;
		}
	}
	else
	{
		switch (flag.type)
		{
		case SORT_TYPE::No_Sort:
		case SORT_TYPE::Top_Down:
			is_first_one_better = (new_ent.CmpVal.cnt > old_ent.CmpVal.cnt);
			break;
		case SORT_TYPE::Bottom_Up:
			is_first_one_better = (new_ent.CmpVal.cnt < old_ent.CmpVal.cnt);
			break;
		default:
			is_first_one_better = false;
		}
	}

	return is_first_one_better;
}