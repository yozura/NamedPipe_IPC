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
	TEXT("아스가르드"), TEXT("미드가르드")
};
static TCHAR cmdLines[][64] = {
	TEXT("SlaveServer.exe"), TEXT("SlaveServer.exe")
};
static LPCTSTR lpszPipenames[] = {
	TEXT("\\\\.\\pipe\\asgard"), TEXT("\\\\.\\pipe\\midgard")
};

// 서버마다 소유해야할 정보들 모음
typedef struct tag_pipe_server_info
{
	STARTUPINFO				si;				// 프로세스 생성시 필요
	PROCESS_INFORMATION		pi;				// 프로세스 생성 및 관리에 필요
	HANDLE					hPipe;			// 파이프
	LPCTSTR					lpszPipeName;	// 파이프 경로 이름
	TCHAR					cmdLine[64];	// 서버 실행 경로 
	TCHAR					lpTitle[64];	// 서버 이름

} PIPE_SERVER_INFO;