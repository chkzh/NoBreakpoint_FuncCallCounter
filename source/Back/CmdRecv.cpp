#include "CmdRecv.h"
#include <iostream>
#include <stdlib.h>

int CmdRecv::input()
{
	printf("\n>>");
	fgets(this->lpCmdBuffer, MAX_BUFFER_SIZE, stdin);

	int result = this->parse(this->lpCmdBuffer);
End:
	rewind(stdin);
	return result;
}

void CmdRecv::print()
{
	printf("\n");

	if (!argc)
	{
		printf("* 没有参数 *\n");
		return;
	}
	else if (argc == -1)
	{
		printf("* 参数溢出 *\n");
		return;
	}

	printf("参数如下：\n");
	for (int i = 0; i < this->argc; i++)
	{
		printf("   %2d\t%s\n", i, argv[i]);
	}

	printf("\n");
}

bool CmdRecv::get(int index, std::string& str)
{
	if (index < this->argc)
		str = this->argv[index];
	else
		str = "($ _ $)";

	return index < this->argc;
}

bool CmdRecv::get(int index, int& val)
{
	char* str = NULL;
	if (index < this->argc)
		str = this->argv[index];

	if (!str)
		return false;

	return sscanf_s(str, "%d", &val);
}

bool CmdRecv::get(int index, float& val)
{
	char* str = NULL;
	if (index < this->argc)
		str = this->argv[index];

	if (!str)
		return false;

	return sscanf_s(str, "%f", &val);
}

/*

	指令解析要求

	1. 正常情况下，以空格作为分隔符，遇到第一个'\0'结束。
	2. ""内部的禁用空格分隔符，作为完整的参数。
	3. 最多解析16个参数，超过的部分不予考虑

	返回值：
	>0 - 正常解析的参数个数
	0 - 出错
*/
int CmdRecv::parse(char* lpCmd)
{
	//int result;
	this->argc = 0;

	if (lpCmd != this->lpCmdBuffer)
		strcpy_s(this->lpCmdBuffer, MAX_BUFFER_SIZE - 1, lpCmd);

	char* p_head = this->lpCmdBuffer;	//字符串的头
	char* p_end = this->lpCmdBuffer;	//字符串的终止符

	bool loop_result;

	do{
		////理论上不应该出现的错误
		//if (p_end - this->lpCmdBuffer > MAX_BUFFER_SIZE)
		//	return 0;

		//首先判断是否能够继续添加参数
		if (argc == MAX_ARG_COUNT)
			break;

		loop_result = *p_end != '\0';

		//当遇到分隔符，尝试添加参数
		if (*p_end == '\0' || *p_end == ' ' || *p_end == '\n' || *p_end == '}')
		{
			if (p_head < p_end)
				this->argv[argc++] = p_head;

			p_head = p_end + 1;
			*p_end = '\0';
		}

		//遇到特殊符号，进行特殊的处理
		else if (*p_end == '{')
		{
			//首先保存当前的字符串，在最后会将'{'和'}'置零
			if (p_head < p_end)
				this->argv[argc++] = p_head;

			char* p_head_spc = p_end;

			//搜索与之对应的'}'字符
			char* p_end_spc = p_end;
			do {
				if (*p_end_spc == '}')
					break;
			} while (*p_end_spc++);

			//找不到'}'，格式非法，停止解析
			if (*(p_end_spc - 1) == '\0')
				break;

			//head从'{'开始找到第一个非分隔符字符
			p_head = p_end;
			p_head++;
			do {
				if (*p_head != ' ')
					break;
			} while (*p_head++ != '}');

			//end从'}'开始倒着寻找第一个非分隔符字符
			p_end = p_end_spc;
			p_end--;
			do {
				if (*p_end != ' ')
					break;
			} while (*p_end-- != '{');

			if (p_head < p_end)
				this->argv[argc++] = p_head;
			*(p_end + 1) = '\0';

			p_head = p_end_spc + 1;
			p_end = p_end_spc;

			//将括号置零
			*p_end_spc = '\0';
			*p_head_spc = '\0';
		}

	} while (p_end++, loop_result);

	return this->argc;


}

bool CmdRecv::_cmp(char* lpA, char* lpB, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (lpA[i] != lpB[i])
			return false;
	}
}

