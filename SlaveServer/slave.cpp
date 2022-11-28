/* ���� ���̺귯�� */
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <locale.h>
#include <stdio.h>

/* ����� ���� ���̺귯�� */
#include "../com/psi.h"
#include "../com/packet.h"
#include "strobj.h"

/* ���̺귯�� ��ũ */
#pragma comment(lib, "ws2_32")

#define SERVERSTORAGE "E:\\Mark12\\testArchive\\sock\\"
#define BUFSIZE     2048
#define MAX_SOCKET  32

typedef struct tag_server_info 
{
    HANDLE      hPipe;
    LPCTSTR     pipeName;
    TCHAR       serverName[64];
    int         port;
    int         index;
} SERVER_INFO;

typedef struct tag_socket_info
{
    SOCKET              sock;
    struct sockaddr_in  addr;
    int                 addrlen;
    TCHAR               serverName[64];
} SOCKET_INFO;

SOCKET_INFO* g_SocketInfoArray[MAX_SOCKET];
int si_cursor;

bool AddSocketInfo(SOCKET sock, struct sockaddr_in& addr, int addrlen);
bool RemoveSocketInfo(TCHAR* name);
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
    setlocale(LC_ALL, "");

    si_cursor = 0;
    HANDLE hPipe;
    SERVER_INFO* serverInfo = NULL;
    TCHAR buf[BUFSIZE];
    BOOL result = FALSE;
    DWORD writeBytes, readBytes;
    int timeout = 20000;

    // ---------------------------------
    /* ������ ���� �ʱ�ȭ �۾� */
    // ---------------------------------

    // � ������ ����ϴ��� üũ�ϴ� �κ�
    serverInfo = CheckServerType(argv[1]);
    if (NULL == serverInfo) err_quit("CheckServerType()");

    // ������ ���� �õ�
    hPipe = TryToConnectPipe(serverInfo->pipeName, timeout);
    if (NULL == hPipe) err_quit("TryToConnectPipe()");

    // ������ ���ῡ ���������Ƿ� �޽��� �б� ���� ��ȯ
    result = ChangePipeMode(hPipe, (PIPE_READMODE_MESSAGE));
    if (FALSE == result) err_quit("ChangePipeMode()");
    
    // ���� �ʱ� ���� �޽����� �ۼ��Ѵ�.
    lstrcpy(buf, serverInfo->pipeName);
    lstrcat(buf, TEXT(" Ȱ��ȭ ����"));
    writeBytes = (lstrlen(buf) + 1) * sizeof(TCHAR);
    result = WritePipeMessage(hPipe, buf, writeBytes, 0);
    if (FALSE == result) err_quit("WritePipeMessage()");

    // ���� �ʱ� ���� �޽����� �д´�.
    readBytes = BUFSIZE;
    result = ReadPipeMessage(hPipe, buf, readBytes, 0);
    if (FALSE == result) err_quit("ReadPipeMessage");

    // ---------------------------------
    /* ����-Ŭ���̾�Ʈ ��ƾ ���� */
    // ---------------------------------

    wprintf(TEXT("%s ���� ����!\n"), serverInfo->serverName);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) err_quit("WSAStartup()");

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) err_quit("listener socket()");

    int retval;
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(serverInfo->port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    struct sockaddr_in clientaddr;
    HANDLE hThread;
    SOCKET client_sock;
    int addrlen;
    
    /* ���� �۾� ������ ó�� */
    while (true)
    {
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
        if (INVALID_SOCKET == client_sock)
        {
            err_display("accept()");
            break;
        }

        if (si_cursor >= MAX_SOCKET)
        {
            printf("Ŭ���̾�Ʈ �ִ� ���� ���� �Ѱ踦 �ʰ��߽��ϴ�.\n");
            continue;
        }

        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
        printf("[SLAVE SERVER] Ŭ���̾�Ʈ ���� [%s:%d]\n", addr, ntohs(clientaddr.sin_port));

        if (!AddSocketInfo(client_sock, clientaddr, sizeof(clientaddr)))
            closesocket(client_sock);

        hThread = CreateThread(NULL, 0, SlaveMain, (LPVOID)client_sock, 0, NULL);
        if (NULL == hThread)
        {
            printf("Ŭ���̾�Ʈ ������ ���� ����! ���� ���� ����!\n");
            closesocket(client_sock);
        }
        else CloseHandle(hThread);
    }

    /* ������ ���� ���� */
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    /* ���� ���� ���� */
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}

bool AddSocketInfo(SOCKET sock, struct sockaddr_in& addr, int addrlen)
{
    if (si_cursor >= MAX_SOCKET)
    {
        printf("[����] ���� ������ ���� �� �߰��� �� �����ϴ�.");
        return false;
    }

    SOCKET_INFO* ptr = new SOCKET_INFO;
    if (NULL == ptr)
    {
        printf("[����] �޸𸮰� �����մϴ�.");
        return false;
    }

    ptr->sock = sock;
    ptr->addr = addr;
    ptr->addrlen = addrlen;
    g_SocketInfoArray[si_cursor++] = ptr;
    return true;
}

bool RemoveSocketInfo(TCHAR* name)
{
    for (int i = 0; i < si_cursor; ++i)
    {
        if (lstrcmp(g_SocketInfoArray[i]->serverName, name) == 0)
        {
            char addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &g_SocketInfoArray[i]->addr, addr, sizeof(addr));
            printf("[SLAVE SERVER] Ŭ���̾�Ʈ ���� [%s:%d]\n", addr, ntohs(g_SocketInfoArray[i]->addr.sin_port));

            closesocket(g_SocketInfoArray[i]->sock);
            delete g_SocketInfoArray[i];

            if (i != (si_cursor - 1))
                g_SocketInfoArray[i] = g_SocketInfoArray[si_cursor - 1];

            --si_cursor;
            return true;
        }
    }

    return false;
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
            wprintf(TEXT("���� �̸� : %s, ������ �̸� : %s, ��Ʈ ��ȣ = %d\n"), si->serverName, si->pipeName, si->port);
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
        // ������ ���� �õ�
        hPipe = CreateFile(
            lpszPipeName,   // ������ �̸�
            GENERIC_READ |
            GENERIC_WRITE,  // �б� & ���� ���� �ޱ�
            0,              // ���� ��� ���� (�� ��� �������� ����)
            NULL,           // �⺻ ��ť��Ƽ �Ӽ���
            OPEN_EXISTING,  // �������� �̹� ������ ��쿡�� ������ ����
            0,              // �������� ��Ÿ �Ӽ� �ο�
            NULL);          // ���ø� ���� ����

        // ���� �������� �޾ƿ��µ� �������� ��� ���� ������ Ż���Ѵ�.
        if (INVALID_HANDLE_VALUE != hPipe)
            break;

        // ���� �ڵ尡 ERROR_PIPE_BUSY �� ��� �����Ѵ�.
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            wprintf(TEXT("������ ���� ����. (���� �ڵ� = %d)\n"), GetLastError());
            exit(1);
        }

        // �������� �����ϱ� ���� ������ �ð� ���� ����ϴٰ� ������� ���� �� ���� ����
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
        hPipe,    // ������ �ڵ�
        &dwMode,  // �������� ���� ���ο� ��� 
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

    // ����� ������ ������ �޽����� ����
    wprintf(TEXT("Sending %d byte message: \"%s\"\n"), writeBytes, (TCHAR*)buf);
    result = WriteFile(
            hPipe,           // ������ �ڵ�
            buf,             // �޽��� ����
            writeBytes,      // �޽��� ����
            &cbWritten,      // �����µ� �������� ��� ����Ʈ ���� ��ϵ� 
            NULL);           // ��ø I/O ����
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

    // ������ �б�
    result = ReadFile(
            hPipe,      // ������ �ڵ�
            buf,        // ������ ���� ����
            readBytes,  // ���� ������
            &cbRead,    // ���ſ� �������� ��� ����Ʈ ���� ��ϵ�
            NULL);      // ��ø I/O ����
    if (!result && GetLastError() != ERROR_MORE_DATA)
    {
        wprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
        return FALSE;
    }

    // �о���� ������ ���
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

        TCHAR* bbuf = (TCHAR*)calloc(received->length + 1, sizeof(TCHAR));
        if (NULL == bbuf)
        {
            err_display("Failed allocat body buffer\n");
            break;
        }

        int idx;
        String cut, savepath;
        FILE* fp;

        // ������ �����ϴ� ������ Ŭ���̾�Ʈ ������ ���� ��Ʈ������ �ø� ������ ����
        // ��Ʈ������ �����̺� ������ ���, �װ� �������� recv()�� ������ �ȴ�.
        switch (received->type) 
        {
        case TYPE_CHAT:
        {
            retval = recv(sock, (char*)bbuf, received->length * 2, 0);
            if (retval == SOCKET_ERROR) err_quit("body recv()");

            int len = (int)lstrlen(bbuf);
            bbuf[len] = '\0';
            printf("[SLAVE SERVER] %d����Ʈ�� �޾ҽ��ϴ�.\n", retval);
            printf("[SLAVE SERVER] [%s:%d] %ls\n", addr, ntohs(clientaddr.sin_port), bbuf);

            /* ��� Ŭ�� ������ */
            for (int i = 0; i < si_cursor; ++i)
            {
                SOCKET_INFO* ptr = g_SocketInfoArray[i];
                retval = send(ptr->sock, (char*)hbuf, sizeof(MSG_HEADER), 0);
                if (SOCKET_ERROR == retval)
                {
                    printf("[SLAVE SERVER] ä�� �޽��� ��� ���ۿ� �����߽��ϴ�.\n");
                    if (!RemoveSocketInfo(ptr->serverName)) err_display("send() header");
                    break;
                }

                retval = send(ptr->sock, (char*)bbuf, len, 0);
                if (SOCKET_ERROR == retval)
                {
                    printf("[SLAVE SERVER] ä�� �޽��� ���ۿ� �����߽��ϴ�.\n");
                    if (!RemoveSocketInfo(ptr->serverName)) err_display("send() body");
                    break;
                }
            }

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
            printf("������ ����Ǿ����ϴ�.\n");
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
            printf("�̹����� ����Ǿ����ϴ�.\n");
        }
            break;
        }
        
        if (bbuf)
        {
            free(bbuf);
            bbuf = 0;
        }
        
        if (hbuf)
        {
            free(hbuf);
            hbuf = 0;
        }
    }

    printf("[SLAVE SERVER]Ŭ���̾�Ʈ ����! [%s:%d]\n", addr, ntohs(clientaddr.sin_port));

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