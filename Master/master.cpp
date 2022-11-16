#include "common.h"

#define BUFSIZE     1024

DWORD WINAPI PipeInstanceThread(LPVOID arg);
void GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes);

bool InitializeSlave(PIPE_SERVER_INFO* psi);
bool CreateSlaveProcess(PIPE_SERVER_INFO* psi);
bool CreateSlavePipe(PIPE_SERVER_INFO* psi);

BOOL WritePipeMessage(HANDLE hPipe, const void* buf, DWORD writeBytes, DWORD cbWritten);

PIPE_SERVER_INFO g_Servers[PIPE_SERVER_COUNT];

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "korean");

    bool result;

    // 1. Slave 서버 초기화
    for (int i = 0; i < PIPE_SERVER_COUNT; ++i)
    {
        PIPE_SERVER_INFO psi;
        ZeroMemory(&psi.si, sizeof(psi.si));
        ZeroMemory(&psi.pi, sizeof(psi.pi));

        psi.si.cb = sizeof(psi.si);
        psi.si.lpTitle = lpTitles[i];

        lstrcpy(psi.cmdLine, cmdLines[i]);
        psi.lpszPipeName = lpszPipenames[i];
        
        result = InitializeSlave(&psi);
        if (!result) return -1;

        g_Servers[i] = psi;
        wprintf(TEXT("%s, %s 서버 생성\n"), g_Servers[i].lpszPipeName, g_Servers[i].si.lpTitle);
    }

    WaitForSingleObject(g_Servers[0].hPipe, INFINITE);

    return 0;
}

bool InitializeSlave(PIPE_SERVER_INFO* psi)
{
    bool result;
    HANDLE hThread;

    result = CreateSlavePipe(psi);
    if (!result) return false;

    result = CreateSlaveProcess(psi);
    if (!result) return false;

    // 파이프 연결 상태 확인
    result = ConnectNamedPipe(psi->hPipe, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!result) return false;

    // 여기까지 왔으면 파이프 연결 성공했으므로 각 파이프 마다 쓰레드로 종속 처리
    hThread = CreateThread(NULL, 0, PipeInstanceThread, (LPVOID)psi->hPipe, 0, NULL);
    if (NULL == hThread)
    {
        wprintf(TEXT("쓰레드 생성 실패 (에러 코드 = %d)\n"), GetLastError());
        return false;
    }
    else CloseHandle(hThread);

    return true;
}

bool CreateSlavePipe(PIPE_SERVER_INFO* psi)
{
    HANDLE hPipe = CreateNamedPipe(
        psi->lpszPipeName,          // 파이프 이름
        PIPE_ACCESS_DUPLEX,         // 읽기/쓰기 모드(양방향)
        PIPE_TYPE_MESSAGE |
        PIPE_READMODE_MESSAGE |
        PIPE_WAIT,                  // 파이프 모드 설정
        PIPE_UNLIMITED_INSTANCES,   // 파이프 인스턴스 최대치
        BUFSIZE,                    // 버퍼 사이즈(Out)
        BUFSIZE,                    // 버퍼 사이즈(In)
        0, NULL);                   // 시큐리티 속성 기본값
    if (INVALID_HANDLE_VALUE == hPipe)
    {
        wprintf(TEXT("명명된 파이프 생성 실패 (에러 코드 = %d)\n"), GetLastError());
        return false;
    }

    psi->hPipe = hPipe;
    return true;
}

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
        return false;
    }

    return true;
}

DWORD WINAPI PipeInstanceThread(LPVOID arg)
{
    HANDLE hHeap = GetProcessHeap();
    TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
    TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));

    DWORD cbBytesRead, cbReplyBytes, cbWritten;
    cbBytesRead = cbReplyBytes = cbWritten = 0;

    BOOL result = FALSE;
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
    while (true)
    {
        // 1 ~ 2 ~ 3 반복, 마스터 서버에 메시지가 들어오는 경우는
        // 초기 서버 세팅과 이후 확성기 모드 이외엔 없다.

        // 1. 메시지 읽기
        result = ReadFile(
            hPipe,                      // 파이프 핸들
            pchRequest,                 // 수신 데이터 버퍼
            BUFSIZE * sizeof(TCHAR),    // 버퍼 사이즈
            &cbBytesRead,               // 바이트 수신량
            NULL);                      // 중첩 I/O 전용 파라미터
        if (!result || 0 == cbBytesRead)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE) printf("클라이언트 연결 종료\n");
            else wprintf(TEXT("ReadFile() (에러 코드 = %d)\n"), GetLastError());
            break;
        }

        // 2. 메시지 처리
        GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);

        // 3. 메시지 작성
        result = WriteFile(
            hPipe,                      // 파이프 핸들
            pchReply,                   // 송신 데이터 버퍼
            cbReplyBytes,               // 작성할 바이트 송신량
            &cbWritten,                 // 작성 바이트량
            NULL);                      // 중첩 I/O 전용 파라미터
        if (!result || cbReplyBytes != cbWritten)
        {
            // 작성에 실패했거나 작성한 데이터가 올바르지 않을 경우
            wprintf(TEXT("WriteFile() (에러 코드 = %d)\n"), GetLastError());
            break;
        }
    }

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    HeapFree(hHeap, 0, pchRequest);
    HeapFree(hHeap, 0, pchReply);
    printf("스레드 종료\n");
    return 0;
}

void GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes)
{
    wprintf(TEXT("클라이언트가 보낸 메시지 : %s\n"), pchRequest);
    if (FAILED(StringCchCopy(pchReply, BUFSIZE, TEXT("deafult answer from server"))))
    {
        *pchBytes = 0;
        pchReply[0] = 0;
        printf("StringCchCopy failed, no outgoing msg\n");
        return;
    }

    *pchBytes = (lstrlen((pchReply)+1) * sizeof(TCHAR));
}
