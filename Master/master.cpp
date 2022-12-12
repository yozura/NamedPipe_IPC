/* 포함 라이브러리 */
#include <locale.h>

/* 사용자 정의 라이브러리 */
#include "../com/psi.h"
#include "../com/packet.h"
#include "../com/common.h"
#include "pipefunc.h"

#define BUFSIZE     512

/// <summary>
/// 프로세스 파이프를 생성 한 뒤 해당 파이프의 읽기/쓰기를 전담하는 쓰레드에 사용됩니다.
/// </summary>
/// <param name="arg">생성된(초기화된) 파이프를 전달해야합니다.</param>
DWORD WINAPI PipeInstanceThread(LPVOID arg);
DWORD WINAPI MasterPipeThread(LPVOID arg);
BOOL WritePipeMessage(HANDLE hPipe, void* buf, DWORD writeBytes, DWORD cbWritten);

/* 전역 변수 */
HANDLE g_MasterPipe;
PIPE_SERVER_INFO g_Servers[PIPE_SERVER_COUNT]; 

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "korean");

    bool result;
    HANDLE hThread;

    CreateMasterPipe(PIPE_MASTER, BUFSIZE, &g_MasterPipe);
    if (!ConnectNamedPipe(g_MasterPipe, NULL))
    {
        if (GetLastError() == ERROR_PIPE_CONNECTED)
        {
            MessageBox(NULL, TEXT("마스터 파이프 연결 실패"), TEXT("ConnectNamedPipe()"), MB_ICONERROR);
            return -1;
        }
    }

    hThread = CreateThread(NULL, 0, MasterPipeThread, 0, 0, NULL);
    if (NULL == hThread)
    {
        wprintf(TEXT("쓰레드 생성 실패 (에러 코드 = %d)\n"), GetLastError());
        MessageBox(NULL, TEXT("쓰레드 생성 실패"), TEXT("CreateThread()"), MB_ICONERROR);
        return -1;
    }
    else CloseHandle(hThread);

    for (int i = 0; i < PIPE_SERVER_COUNT; ++i)
    {
        PIPE_SERVER_INFO psi;
        ZeroMemory(&psi.si, sizeof(psi.si));
        ZeroMemory(&psi.pi, sizeof(psi.pi));

        psi.si.cb = sizeof(psi.si);
        psi.si.lpTitle = lpTitles[i];

        lstrcpy(psi.cmdLine, cmdLines[i]);
        psi.lpszPipeName = lpszPipeNames[i];

        result = InitializeSlave(&psi, BUFSIZE);
        if (!result) return -1;

        // 프로세스의 파이프와 연결 상태 확인
        result = ConnectNamedPipe(psi.hPipe, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!result)
        {
            MessageBox(NULL, TEXT("파이프 연결 실패"), TEXT("ConnectNamedPipe()"), MB_ICONERROR);
            return -1;
        }

        // 여기까지 왔으면 파이프 연결 성공했으므로 각 파이프 마다 쓰레드로 종속 처리
        hThread = CreateThread(NULL, 0, PipeInstanceThread, (LPVOID)psi.hPipe, 0, NULL);
        if (NULL == hThread)
        {
            wprintf(TEXT("쓰레드 생성 실패 (에러 코드 = %d)\n"), GetLastError());
            MessageBox(NULL, TEXT("쓰레드 생성 실패"), TEXT("CreateThread()"), MB_ICONERROR);
            return -1;
        }
        else CloseHandle(hThread);

        g_Servers[i] = psi;
        wprintf(TEXT("%s, %s 서버 생성\n"), g_Servers[i].lpszPipeName, g_Servers[i].si.lpTitle);
    }

    WaitForSingleObject(g_Servers[(int)PIPE_SERVER::ASGARD].hPipe, INFINITE);
    WaitForSingleObject(g_Servers[(int)PIPE_SERVER::MIDGARD].hPipe, INFINITE);

    printf("마스터 서버 종료\n");
    return 0;
}

DWORD WINAPI PipeInstanceThread(LPVOID arg)
{
    HANDLE hHeap = GetProcessHeap();
    LPVOID pchRequest = (LPVOID)HeapAlloc(hHeap, 0, PIPE_MSG_SIZE);
    LPVOID pchReply = (LPVOID)HeapAlloc(hHeap, 0, PIPE_MSG_SIZE);

    DWORD cbBytesRead, cbReplyBytes, cbWritten;
    cbBytesRead = cbReplyBytes = cbWritten = 0;

    BOOL result = FALSE;
    BOOL flag = FALSE;
    HANDLE hPipe = NULL;

    // 파이프가 전달되지 않은 경우
    if (NULL == arg)
    {
        printf("파이프 전달 실패\n");
        if (NULL == pchRequest) HeapFree(hHeap, 0, pchRequest);
        if (NULL == pchReply) HeapFree(hHeap, 0, pchReply);
        return -1;
    }

    if (NULL == pchRequest)
    {
        printf("Request 힙 할당 실패\n");
        if (NULL == pchReply) HeapFree(hHeap, 0, pchReply);
        return -1;
    }

    if (NULL == pchReply)
    {
        printf("Reply 힙 할당 실패\n");
        if (NULL == pchRequest) HeapFree(hHeap, 0, pchRequest);
        return -1;
    }

    hPipe = (HANDLE)arg;
    while (flag == FALSE)
    {
        // 1 ~ 2 ~ 3 반복, 마스터 서버에 메시지가 들어오는 경우는
        // 초기 서버 세팅과 이후 확성기 모드 이외엔 없다.

        // 1. 메시지 읽기
        result = ReadFile(
            hPipe,                      // 파이프 핸들
            pchRequest,                 // 수신 데이터 버퍼
            PIPE_MSG_SIZE,              // 버퍼 사이즈
            &cbBytesRead,               // 바이트 수신량
            NULL);                      // 중첩 I/O 전용 파라미터
        if (!result || 0 == cbBytesRead)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE) printf("클라이언트 연결 종료\n");
            else wprintf(TEXT("ReadFile() (에러 코드 = %d)\n"), GetLastError());
            break;
        }

        // 2. 메시지 처리
        PIPE_MSG* pm = (PIPE_MSG*)pchRequest;
        printf("[MASTER SERVER] %s : %s\n", pm->userName, pm->msg);
        switch (pm->type)
        {
        case PIPE_TYPE_INIT:
            // 초기 메시지 세팅
            if (!WritePipeMessage(hPipe, (LPVOID)pm, PIPE_MSG_SIZE, cbWritten))
                flag = TRUE;
            break;
        case PIPE_TYPE_MP:
        {
            for (int i = 0; i < PIPE_SERVER_COUNT; ++i)
            {
                if (!WritePipeMessage(g_Servers[i].hPipe, (LPVOID)pm, PIPE_MSG_SIZE, cbWritten))
                    flag = TRUE;
            }
            break;
        }
        }
    }

    // 힙 할당 해제
    HeapFree(hHeap, 0, pchRequest);
    HeapFree(hHeap, 0, pchReply);

    // 파이프 닫기
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    printf("쓰레드 종료\n");
    return 0;
}

DWORD WINAPI MasterPipeThread(LPVOID arg)
{
    return 0;
}

BOOL WritePipeMessage(HANDLE hPipe, void* buf, DWORD writeBytes, DWORD cbWritten)
{
    BOOL result;
    result = WriteFile(
        hPipe,                      // 파이프 핸들
        buf,                        // 송신 데이터 버퍼
        writeBytes,                 // 작성할 바이트 송신량
        &cbWritten,                 // 작성 바이트량
        NULL);                      // 중첩 I/O 전용 파라미터
    if (!result || writeBytes != cbWritten)
    {
        // 작성에 실패했거나 작성한 데이터가 잘렸을 경우
        wprintf(TEXT("WriteFile() (에러 코드 = %d)\n"), GetLastError());
        return false;
    }

    return true;
}