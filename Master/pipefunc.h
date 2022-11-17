#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>

/// <summary>
/// 하위 서버 프로세스를 생성합니다.
/// </summary>
/// <param name="psi">파이프 서버 정보 구조체입니다.</param>
/// <returns>
///     true일 경우 서버 프로세스 생성에 성공하여 파이프 서버 정보 구조체에 저장됩니다.
///     false일 경우 프로세스 생성에 실패했습니다.
/// </returns>
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

    return true;
}

/// <summary>
/// 하위 서버 파이프를 생성합니다.
/// </summary>
/// <param name="psi">파이프 서버 정보 구조체입니다.</param>
/// <returns>
///     true일 경우 서버 파이프 생성에 성공하여 파이프 서버 정보 구조체에 저장됩니다.
///     false일 경우 파이프 생성에 에러가 발생했습니다.
/// </returns>
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

/// <summary>
/// 하위 서버 프로세스를 실행하고 하위 서버와 연결할 파이프를 생성합니다.
/// </summary>
/// <param name="psi">파이프 서버 정보 구조체입니다.</param>
/// <returns>
///     true일 경우 하위 파이프 생성, 하위 서버 프로세스 생성에 성공하여 파이프 서버 정보 구조체에 저장됩니다.
///     false일 경우 초기화에 실패한 사유를 메세지 박스로 출력합니다.
/// </returns>
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
        MessageBox(NULL, TEXT("하위 서버 프로세스 생성 실패"), TEXT("CreateSalveProcess()"), MB_ICONERROR);        return false;
    }

    return true;
}


/// <summary>
/// 
/// </summary>
/// <param name="hPipe"></param>
/// <param name="buf"></param>
/// <param name="writeBytes"></param>
/// <param name="cbWritten"></param>
/// <returns></returns>
bool WritePipeMessage(HANDLE hPipe, const void* buf, DWORD writeBytes, DWORD cbWritten);

/// <summary>
/// 파이프에 작성된 메시지의 정보를 파악해 답변을 보냅니다.
/// </summary>
/// <param name="pchRequest"></param>
/// <param name="pchReply"></param>
/// <param name="pchBytes"></param>
void GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes, DWORD bufSize)
{
    wprintf(TEXT("클라이언트가 보낸 메시지 : %s\n"), pchRequest);
    if (FAILED(StringCchCopy(pchReply, bufSize, TEXT("파이프 연결 성공"))))
    {
        *pchBytes = 0;
        pchReply[0] = 0;
        printf("StringCchCopy failed, no outgoing msg\n");
        return;
    }

    *pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
}