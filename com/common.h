#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <tchar.h>
#include <string.h>
#include <stdlib.h>

// wchar_t -> char 
char* ToString(wchar_t* target)
{
	size_t bytes;
	size_t nLen = wcslen(target);
	// �ι��ڱ��� �б� ���� 1����Ʈ �߰� ����
	char* buffer = (char*)malloc(sizeof(char) * nLen + 1);
	wcstombs_s(&bytes, buffer, nLen + 1, target, nLen + 1);
	return buffer;
}

// char -> wchar_t 
wchar_t* ToWString(char* target)
{
	size_t bytes;
	// �ι��ڱ��� �����ؼ� 1 �߰��ϱ�.
	size_t nLen = strlen(target) + 1;
	wchar_t* buffer = (wchar_t*)malloc(sizeof(wchar_t) * nLen);
	mbstowcs_s(&bytes, buffer, nLen, target, nLen);
	return buffer;
}