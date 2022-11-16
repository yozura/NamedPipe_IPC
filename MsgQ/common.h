#pragma once
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <locale.h>

#define PIPE_ASGARD TEXT("\\\\.\\pipe\\asgard")
#define PIPE_MIDGARD TEXT("\\\\.\\pipe\\midgard")

#define PIPE_SERVER_COUNT 2

static TCHAR lpTitles[][64] = {
	TEXT("�ƽ�������"), TEXT("�̵尡����")
};
static TCHAR cmdLines[][64] = {
	TEXT("SlaveServer.exe"), TEXT("SlaveServer.exe")
};
static LPCTSTR lpszPipenames[] = {
	TEXT("\\\\.\\pipe\\asgard"), TEXT("\\\\.\\pipe\\midgard")
};

// �������� �����ؾ��� ������ ����
typedef struct tag_pipe_server_info
{
	STARTUPINFO				si;				// ���μ��� ������ �ʿ�
	PROCESS_INFORMATION		pi;				// ���μ��� ���� �� ������ �ʿ�
	HANDLE					hPipe;			// ������
	LPCTSTR					lpszPipeName;	// ������ ��� �̸�
	TCHAR					cmdLine[64];	// ���� ���� ��� 
	TCHAR					lpTitle[64];	// ���� �̸�

} PIPE_SERVER_INFO;