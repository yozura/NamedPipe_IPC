#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>

bool CreateMasterPipe(LPCTSTR pipeName, DWORD bufSize, HANDLE* hPipe)
{
    HANDLE tempPipe = CreateNamedPipe(
        pipeName,          // 파이프 이름
        PIPE_ACCESS_DUPLEX,         // 읽기/쓰기 모드(양방향)
        PIPE_TYPE_MESSAGE |
        PIPE_READMODE_MESSAGE |
        PIPE_WAIT,                  // 파이프 모드 설정
        PIPE_UNLIMITED_INSTANCES,   // 파이프 인스턴스 최대치
        bufSize,                    // 버퍼 사이즈(Out)
        bufSize,                    // 버퍼 사이즈(In)
        0, NULL);                   // 시큐리티 속성 기본값
    if (INVALID_HANDLE_VALUE == hPipe)
    {
        wprintf(TEXT("명명된 파이프 생성 실패 (에러 코드 = %d)\n"), GetLastError());
        return false;
    }

    *hPipe = tempPipe;
    return true;
}

/// <summary>
/// 하위 서버 프로세스를 생성합니다.
/// </summary>
/// <param name="psi">파이프 서버 정보 구조체입니다.</param>
/// <returns>
///     true일 경우 서버 프로세스 생성에 성공하여 파이프 서버 정보 구조체에 저장됩니다.
///     false일 경우 프로세스 생성에 실패했습니다.
/// </returns>
bool CreateSlaveProcess(PIPE_SERVER_INFO* psi);

/// <summary>
/// 하위 서버 파이프를 생성합니다.
/// </summary>
/// <param name="psi">파이프 서버 정보 구조체입니다.</param>
/// <returns>
///     true일 경우 서버 파이프 생성에 성공하여 파이프 서버 정보 구조체에 저장됩니다.
///     false일 경우 파이프 생성에 에러가 발생했습니다.
/// </returns>
bool CreateSlavePipe(PIPE_SERVER_INFO* psi, DWORD bufSize);

/// <summary>
/// 하위 서버 프로세스를 실행하고 하위 서버와 연결할 파이프를 생성합니다.
/// </summary>
/// <param name="psi">파이프 서버 정보 구조체입니다.</param>
/// <returns>
///     true일 경우 하위 파이프 생성, 하위 서버 프로세스 생성에 성공하여 파이프 서버 정보 구조체에 저장됩니다.
///     false일 경우 초기화에 실패한 사유를 메세지 박스로 출력합니다.
/// </returns>
bool InitializeSlave(PIPE_SERVER_INFO* psi, DWORD bufSize);

bool CreateSlaveProcess(PIPE_SERVER_INFO* psi)
{
    if (!psi) return false;
    if (!CreateProcess(
        NULL,
        psi->cmdLine,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &psi->si,
        &psi->pi))
    {
        wprintf(TEXT("하위 서버 프로세스 생성 실패 (에러 코드 = %d)\n"), GetLastError());
        return false;
    }

    CloseHandle(psi->pi.hProcess);
    CloseHandle(psi->pi.hThread);
    return true;
}

bool CreateSlavePipe(PIPE_SERVER_INFO* psi, DWORD bufSize)
{
    HANDLE hPipe = CreateNamedPipe(
        psi->lpszPipeName,          // 파이프 이름
        PIPE_ACCESS_DUPLEX,         // 읽기/쓰기 모드(양방향)
        PIPE_TYPE_MESSAGE |
        PIPE_READMODE_MESSAGE |
        PIPE_WAIT,                  // 파이프 모드 설정
        PIPE_UNLIMITED_INSTANCES,   // 파이프 인스턴스 최대치
        bufSize,                    // 버퍼 사이즈(Out)
        bufSize,                    // 버퍼 사이즈(In)
        0, NULL);                   // 시큐리티 속성 기본값
    if (INVALID_HANDLE_VALUE == hPipe)
    {
        wprintf(TEXT("명명된 파이프 생성 실패 (에러 코드 = %d)\n"), GetLastError());
        return false;
    }

    psi->hPipe = hPipe;
    return true;
}

bool InitializeSlave(PIPE_SERVER_INFO* psi, DWORD bufSize)
{
    bool result;

    // 파이프 생성
    result = CreateSlavePipe(psi, bufSize);
    if (!result)
    {
        MessageBox(NULL, TEXT("하위 서버 파이프 생성 실패"), TEXT("CreateSlavePipe()"), MB_ICONERROR);
        return false;
    }

    // 프로세스 실행
    result = CreateSlaveProcess(psi);
    if (!result)
    {
        MessageBox(NULL, TEXT("하위 서버 프로세스 생성 실패"), TEXT("CreateSalveProcess()"), MB_ICONERROR);  
        return false;
    }

    return true;
}

/// <summary>
/// 특정 파이프 메시지를 작성.
/// </summary>
/// <param name="hPipe">파이프 핸들</param>
/// <param name="buf">버퍼</param>
/// <param name="writeBytes">작성할 데이터 크기</param>
/// <param name="cbWritten">작성한 데이터 크기</param>
/// <returns></returns>
BOOL WritePipeMessage(HANDLE hPipe, LPVOID buf, DWORD writeBytes, DWORD cbWritten);

/// <summary>
/// 특정 파이프 메시지를 읽음.
/// </summary>
/// <param name="hPipe">파이프 핸들</param>
/// <param name="buf">버퍼</param>
/// <param name="readBytes">읽을 데이터 크기</param>
/// <param name="cbRead">읽은 데이터 크기</param>
/// <returns>실패/성공 여부 반환</returns>
BOOL ReadPipeMessage(HANDLE hPipe, LPVOID buf, DWORD readBytes, DWORD cbRead);