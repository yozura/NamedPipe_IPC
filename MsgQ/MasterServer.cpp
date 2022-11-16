#include "common.h"

#define BUFSIZE     1024

DWORD WINAPI PipeInstanceThread(LPVOID arg);
void GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD);

int main(int argc)
{
    setlocale(LC_ALL, "korean");

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    TCHAR lpTitle[] = TEXT("슬레이브");
    TCHAR cmdLine[] = TEXT("SlaveServer.exe");
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.lpTitle = lpTitle;
    ZeroMemory(&pi, sizeof(pi));

    // 채팅 서버 프로세스 생성
    if (!CreateProcess(
        NULL,                   // No module name (use command line)
        cmdLine,                // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,     // No creation flags
        NULL,                   // Use parent's environment block
        NULL,                   // Use parent's starting directory 
        &si,                    // Pointer to STARTUPINFO structure
        &pi)                    // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return -1;
    }
    wprintf(TEXT("%s 프로세스 생성 성공\n"), lpTitle);

    BOOL fConnected = FALSE;
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    LPCTSTR lpszPipeName = PIPE_ASGARD;
    HANDLE hThread;
    
    while (true)
    {
        wprintf(TEXT("%s 파이프 서버 생성 중\n"), lpszPipeName);
        hPipe = CreateNamedPipe(
            lpszPipeName,               // 파이프 이름
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
            return -1;
        }


        fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (fConnected)
        {
            printf(">>> 파이프 연결 확인 완료\n");
            hThread = CreateThread(NULL, 0, PipeInstanceThread, (LPVOID)hPipe, 0, NULL);
            if (NULL == hThread)
            {
                wprintf(TEXT("쓰레드 생성 실패 (에러 코드 = %d)\n"), GetLastError());
                return -1;
            }
            else CloseHandle(hThread);
        }
        else
        {
            // 클라이언트 접속이 실패할 경우 파이프 닫기
            CloseHandle(hPipe);
        }
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}

DWORD WINAPI PipeInstanceThread(LPVOID arg)
{
    HANDLE hHeap        = GetProcessHeap();
    TCHAR* pchRequest   = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
    TCHAR* pchReply     = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));

    DWORD cbBytesRead, cbReplyBytes, cbWritten;
    cbBytesRead = cbReplyBytes = cbWritten = 0;

    BOOL fSuccess = FALSE;
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
        fSuccess = ReadFile(
            hPipe,                      // 파이프 핸들
            pchRequest,                 // 수신 데이터 버퍼
            BUFSIZE * sizeof(TCHAR),    // 버퍼 사이즈
            &cbBytesRead,               // 바이트 수신량
            NULL);                      // 중첩 I/O 전용 파라미터
        if (!fSuccess || 0 == cbBytesRead)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE) printf("클라이언트 연결 종료\n");
            else wprintf(TEXT("ReadFile() (에러 코드 = %d)\n"), GetLastError());
            break;
        }

        // 메시지 처리
        GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);

        // 파이프에 응답 작성
        fSuccess = WriteFile(
            hPipe,                      // 파이프 핸들
            pchReply,                   // 송신 데이터 버퍼
            cbReplyBytes,               // 작성할 바이트 송신량
            &cbWritten,                 // 작성 바이트량
            NULL);                      // 중첩 I/O 전용 파라미터
        if (!fSuccess || cbReplyBytes != cbWritten)
        {
            // 작성에 실패했거나 작성한 데이터가 올바르지 않을 경우
            wprintf(TEXT("WriteFile() (에러 코드 = %d)\n"), GetLastError());
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

    *pchBytes = (lstrlen((pchReply) + 1) * sizeof(TCHAR));
}
