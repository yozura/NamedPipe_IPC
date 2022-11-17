#include "../com/psi.h"

#define BUFSIZE 1024

LPCTSTR CheckServerType(char* type);
HANDLE TryToConnectPipe(LPCTSTR lpszPipeName, int timeout);
BOOL ChangePipeMode(HANDLE hPipe, DWORD dwMode);
BOOL WritePipeMessage(HANDLE hPipe, void* buf, DWORD writeBytes, DWORD cbWritten);
BOOL ReadPipeMessage(HANDLE hPipe, void* buf, DWORD readBytes, DWORD cbRead);

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "korean");

    HANDLE hPipe;
    TCHAR buf[BUFSIZE];
    BOOL result = FALSE;
    DWORD writeBytes, readBytes;
    LPCTSTR lpszPipename = NULL;
    int timeout = 20000;

    // 어떤 서버를 담당하는지 체크하는 부분, 만약 NULL을 반환한다면 즉시 종료
    lpszPipename = CheckServerType(argv[1]);
    if (NULL == lpszPipename) return -1;

    // 파이프 연결 시도
    hPipe = TryToConnectPipe(lpszPipename, timeout);
    if (NULL == hPipe) return -1;

    // 파이프 연결에 성공했으므로 메시지 읽기 모드로 전환
    result = ChangePipeMode(hPipe, (PIPE_READMODE_MESSAGE));
    if (FALSE == result) return -1;
    
    // 서버 초기 세팅 메시지를 작성한다.
    lstrcpy(buf, lpszPipename);
    lstrcat(buf, TEXT(" activate"));
    writeBytes = (lstrlen(buf) + 1) * sizeof(TCHAR);
    result = WritePipeMessage(hPipe, buf, writeBytes, 0);
    if (FALSE == result) return -1;

    // 서버 초기 세팅 메시지를 읽는다.
    readBytes = BUFSIZE;
    result = ReadPipeMessage(hPipe, buf, readBytes, 0);
    if (FALSE == result) return -1;

    // 서버 루틴을 만든다. (게임 룸)
    while (true)
    {
        if (getc(stdin) == 'e') break;
    }

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    return 0;
}

LPCTSTR CheckServerType(char* type)
{
    const char* types[PIPE_SERVER_COUNT] = {
        "asgard", "midgard"
    };

    for (int i = 0; i < PIPE_SERVER_COUNT; ++i)
    {
        if (strcmp(type, types[i]) == 0)
            return lpszPipenames[i];
    }

    return NULL;
}

HANDLE TryToConnectPipe(LPCTSTR lpszPipeName, int timeout)
{
    HANDLE hPipe = NULL;
    while (true)
    {
        // 파이프 연결 시도
        hPipe = CreateFile(
            lpszPipeName,   // 파이프 이름
            GENERIC_READ |
            GENERIC_WRITE,  // 읽기 & 쓰기 권한 받기
            0,              // 공유 방식 지정 (이 경우 공유하지 않음)
            NULL,           // 기본 시큐리티 속성값
            OPEN_EXISTING,  // 파이프가 이미 존재할 경우에만 열도록 설정
            0,              // 파이프의 기타 속성 부여
            NULL);          // 템플릿 파일 없이

        // 만약 파이프를 받아오는데 성공했을 경우 무한 루프를 탈출한다.
        if (INVALID_HANDLE_VALUE != hPipe)
            break;

        // 에러 코드가 ERROR_PIPE_BUSY 일 경우 종료한다.
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            _tprintf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
            exit(1);
        }

        // 파이프를 연결하기 위해 지정한 시간 동안 대기하다가 연결되지 않을 시 강제 종료
        if (!WaitNamedPipe(lpszPipeName, timeout))
        {
            printf("Could not open pipe: 20 second wait timed out.");
            exit(1);
        }
    }

    return hPipe;
}

BOOL ChangePipeMode(HANDLE hPipe, DWORD dwMode)
{
    BOOL result;

    result = SetNamedPipeHandleState(
        hPipe,    // 파이프 핸들
        &dwMode,  // 파이프에 대한 새로운 모드 
        NULL,     
        NULL);    
    if (!result)
    {
        wprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
        return FALSE;
    }

    return TRUE;
}

BOOL WritePipeMessage(HANDLE hPipe, void* buf, DWORD writeBytes, DWORD cbWritten)
{
    BOOL result;

    // 연결된 파이프 서버에 메시지를 전송
    wprintf(TEXT("Sending %d byte message: \"%s\"\n"), writeBytes, (TCHAR*)buf);
    result = WriteFile(
            hPipe,           // 파이프 핸들
            buf,             // 메시지 버퍼
            writeBytes,      // 메시지 길이
            &cbWritten,      // 보내는데 성공했을 경우 바이트 수가 기록됨 
            NULL);           // 중첩 I/O 전용
    if (!result)
    {
        wprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
        return FALSE;
    }
    return TRUE;
}

BOOL ReadPipeMessage(HANDLE hPipe, void* buf, DWORD readBytes, DWORD cbRead)
{
    BOOL result;

    // 파이프 읽기
    result = ReadFile(
            hPipe,      // 파이프 핸들
            buf,        // 답장을 받을 버퍼
            readBytes,  // 버퍼 사이즈
            &cbRead,    // 수신에 성공했을 경우 바이트 수가 기록됨
            NULL);      // 중첩 I/O 전용
    if (!result && GetLastError() != ERROR_MORE_DATA)
    {
        wprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
        return FALSE;
    }

    // 읽어들인 데이터 출력
    wprintf(TEXT("\"%s\"\n"), (TCHAR*)buf);
    return TRUE;
}
