/* 포함 라이브러리 */
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <locale.h>
#include <stdio.h>

/* 사용자 정의 라이브러리 */
#include "../com/psi.h"
#include "../com/packet.h"
#include "strobj.h"

/* 라이브러리 링크 */
#pragma comment(lib, "ws2_32")

#define SERVERSTORAGE "E:\\Mark12\\testArchive\\sock\\"
#define BUFSIZE     2048

typedef struct tag_server_info {
    HANDLE      hPipe;
    LPCTSTR     pipeName;
    TCHAR       serverName[64];
    int         port;
    int         index;
} SERVER_INFO;

//LPCTSTR CheckServerType(char* type);
SERVER_INFO* CheckServerType(char* type);
HANDLE TryToConnectPipe(LPCTSTR lpszPipeName, int timeout);
BOOL ChangePipeMode(HANDLE hPipe, DWORD dwMode);
BOOL WritePipeMessage(HANDLE hPipe, void* buf, DWORD writeBytes, DWORD cbWritten);
BOOL ReadPipeMessage(HANDLE hPipe, void* buf, DWORD readBytes, DWORD cbRead);
DWORD SlaveMain(LPVOID arg);

void err_display(const char* title);
void err_quit(const char* title);

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "korean");

    HANDLE hPipe;
    SERVER_INFO* serverInfo = NULL;
    TCHAR buf[BUFSIZE];
    BOOL result = FALSE;
    DWORD writeBytes, readBytes;
    int timeout = 20000;

    // ---------------------------------
    /* 파이프 서버 초기화 작업 */
    // ---------------------------------

    // 어떤 서버를 담당하는지 체크하는 부분
    serverInfo = CheckServerType(argv[1]);
    if (NULL == serverInfo) return -1;

    // 파이프 연결 시도
    hPipe = TryToConnectPipe(serverInfo->pipeName, timeout);
    if (NULL == hPipe) return -1;

    // 파이프 연결에 성공했으므로 메시지 읽기 모드로 전환
    result = ChangePipeMode(hPipe, (PIPE_READMODE_MESSAGE));
    if (FALSE == result) return -1;
    
    // 서버 초기 세팅 메시지를 작성한다.
    lstrcpy(buf, serverInfo->pipeName);
    lstrcat(buf, TEXT(" 활성화 성공"));
    writeBytes = (lstrlen(buf) + 1) * sizeof(TCHAR);
    result = WritePipeMessage(hPipe, buf, writeBytes, 0);
    if (FALSE == result) return -1;

    // 서버 초기 세팅 메시지를 읽는다.
    readBytes = BUFSIZE;
    result = ReadPipeMessage(hPipe, buf, readBytes, 0);
    if (FALSE == result) return -1;

    // ---------------------------------
    /* 서버-클라이언트 루틴 시작 */
    // ---------------------------------

    wprintf(TEXT("%s 서버 시작!\n"), serverInfo->serverName);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) err_quit("WSAStartup()");

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) err_display("listener socket()");

    int retval;
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(serverInfo->port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_display("bind()");

    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_display("listen()");

    struct sockaddr_in clientaddr;
    HANDLE hThread;
    SOCKET client_sock;
    int addrlen;

    /* 서버 작업 쓰레드 처리 */
    while (true)
    {
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
        if (INVALID_SOCKET == client_sock)
        {
            err_display("accept()");
            break;
        }

        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
        printf("클라이언트 연결 성공! [%s:%d]\n", addr, ntohs(clientaddr.sin_port));

        hThread = CreateThread(NULL, 0, SlaveMain, (LPVOID)client_sock, 0, NULL);
        if (NULL == hThread)
        {
            printf("클라이언트 쓰레드 생성 실패! 접속 강제 종료!\n");
            closesocket(client_sock);
        }
        else CloseHandle(hThread);
    }

    /* 파이프 서버 종료 */
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    /* 소켓 서비스 종료 */
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}

SERVER_INFO* CheckServerType(char* type)
{
    const char* types[PIPE_SERVER_COUNT] = {
        "asgard", "midgard"
    };

    for (int i = 0; i < PIPE_SERVER_COUNT; ++i)
    {
        if (strcmp(type, types[i]) == 0)
        {
            SERVER_INFO* si = new SERVER_INFO;
            si->pipeName = lpszPipeNames[i];
            lstrcpy(si->serverName, lpTitles[i]);
            si->port = ((int)PIPE_SERVER_PORT::ASGARD) + i;
            si->index = i;
            wprintf(TEXT("서버 이름 : %s, 파이프 이름 : %s, 포트 번호 = %d\n"), si->serverName, si->pipeName, si->port);
            return si;
        }
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
            wprintf(TEXT("파이프 열기 실패. (에러 코드 = %d)\n"), GetLastError());
            exit(1);
        }

        // 파이프를 연결하기 위해 지정한 시간 동안 대기하다가 연결되지 않을 시 강제 종료
        if (!WaitNamedPipe(lpszPipeName, timeout))
        {
            MessageBox(NULL, TEXT(""), TEXT(""), MB_ICONERROR);
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

DWORD SlaveMain(LPVOID arg)
{
    SOCKET sock = (SOCKET)arg;
    struct sockaddr_in clientaddr;
    int addrlen;
    int retval;

    addrlen = sizeof(clientaddr);
    getpeername(sock, (struct sockaddr*)&clientaddr, &addrlen);
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

    while (true) 
    {
        char* hbuf = (char*)calloc(1, sizeof(MSG_HEADER));
        retval = recv(sock, hbuf, sizeof(MSG_HEADER), 0);
        if (SOCKET_ERROR == retval) err_display("header recv()");
        else if (0 == retval) break;

        MSG_HEADER* received = (MSG_HEADER*)hbuf;
        printf("type = %d\nlength = %d\npath = %s\n", received->type, received->length, received->path);

        char* bbuf = (char*)calloc(received->length + 1, sizeof(char));
        if (NULL == bbuf)
        {
            err_display("Failed allocat body buffer\n");
            break;
        }

        int idx;
        String cut, savepath;
        FILE* fp;

        // 파일을 전송하는 로직은 클라이언트 측에서 파일 스트림으로 올린 파일을 열어
        // 스트림으로 슬레이브 서버로 쏘고, 그걸 서버에서 recv()로 받으면 된다.
        switch (received->type) 
        {
        case TYPE_CHAT:
        {
            retval = recv(sock, bbuf, received->length, 0);
            if (retval == SOCKET_ERROR) err_quit("body recv()");

            int len = (int)strlen(bbuf);
            bbuf[len] = '\0';
            printf("%d바이트를 받았습니다.\n", retval);
            printf("[클라이언트] %s\n", bbuf);
        }
            break;
        case TYPE_TXT:
        {
            idx = lastindexof(received->path, String("\\"));
            cut = substring(received->path, idx + 1, length(received->path) + 1);
            savepath = concat(String(SERVERSTORAGE), cut);
            printf("com path : %s\n", savepath);

            fp = fopen(savepath, "w");
            do {
                char headBuf[HEADSIZE];
                retval = recv(sock, headBuf, sizeof(MSG_HEADER), 0);
                MSG_HEADER* temphd = (MSG_HEADER*)headBuf;
                char* strbuf = (char*)calloc(temphd->length + 1, sizeof(char));
                if (NULL == strbuf)
                {
                    err_display("Failed allocat strbuf.\n");
                    break;
                }

                retval = recv(sock, strbuf, temphd->length, 0);
                printf("%s", strbuf);
                fputs(strbuf, fp);

                free(strbuf);
            } while (retval != 0);
            fclose(fp);
            printf("파일이 저장되었습니다.\n");
        }
            break;
        case TYPE_IMG:
        {
            idx = lastindexof(received->path, String("\\"));
            cut = substring(received->path, idx + 1, length(received->path) + 1);
            savepath = concat(String(SERVERSTORAGE), cut);
            printf("com path : %s\n", savepath);

            fp = fopen(savepath, "wb");
            do {
                char tempbuf[HEADSIZE];
                retval = recv(sock, tempbuf, sizeof(MSG_HEADER), 0);
                MSG_HEADER* temphd = (MSG_HEADER*)tempbuf;
                char* imgbuf = (char*)calloc(temphd->length, sizeof(char));
                if (NULL == imgbuf)
                {
                    err_display("Failed allocate imgbuf\n");
                    break;
                }

                retval = recv(sock, imgbuf, temphd->length, 0);
                fwrite(imgbuf, sizeof(char), temphd->length, fp);
                free(imgbuf);
            } while (retval != 0);
            fclose(fp);
            printf("이미지가 저장되었습니다.\n");
        }
            break;
        }

        free(bbuf);
        free(hbuf);
        break;
    }

    printf("클라이언트 종료! [%s:%d]\n", addr, ntohs(clientaddr.sin_port));

    return 0;
}

void err_quit(const char* title) {
    LPVOID lpMsgBuf;
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&lpMsgBuf, 0, NULL);
    MessageBoxA(NULL, (char*)&lpMsgBuf, title, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

void err_display(const char* title) {
    LPVOID lpMsgBuf;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&lpMsgBuf, 0, NULL);
    printf("[%s] %s\n", title, (char*)&lpMsgBuf);
    LocalFree(lpMsgBuf);
    exit(1);
}