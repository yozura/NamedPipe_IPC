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
	// 널문자까지 읽기 위해 1바이트 추가 생성
	char* buffer = (char*)malloc(sizeof(char) * nLen + 1);
	wcstombs_s(&bytes, buffer, nLen + 1, target, nLen + 1);
	return buffer;
}

// char -> wchar_t 
wchar_t* ToWString(char* target)
{
	size_t bytes;
	// 널문자까지 생각해서 1 추가하기.
	size_t nLen = strlen(target) + 1;
	wchar_t* buffer = (wchar_t*)malloc(sizeof(wchar_t) * nLen);
	mbstowcs_s(&bytes, buffer, nLen, target, nLen);
	return buffer;
}