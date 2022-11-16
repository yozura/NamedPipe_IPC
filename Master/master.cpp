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

    // 1. Slave ���� �ʱ�ȭ
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
        wprintf(TEXT("%s, %s ���� ����\n"), g_Servers[i].lpszPipeName, g_Servers[i].si.lpTitle);
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

    // ������ ���� ���� Ȯ��
    result = ConnectNamedPipe(psi->hPipe, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!result) return false;

    // ������� ������ ������ ���� ���������Ƿ� �� ������ ���� ������� ���� ó��
    hThread = CreateThread(NULL, 0, PipeInstanceThread, (LPVOID)psi->hPipe, 0, NULL);
    if (NULL == hThread)
    {
        wprintf(TEXT("������ ���� ���� (���� �ڵ� = %d)\n"), GetLastError());
        return false;
    }
    else CloseHandle(hThread);

    return true;
}

bool CreateSlavePipe(PIPE_SERVER_INFO* psi)
{
    HANDLE hPipe = CreateNamedPipe(
        psi->lpszPipeName,          // ������ �̸�
        PIPE_ACCESS_DUPLEX,         // �б�/���� ���(�����)
        PIPE_TYPE_MESSAGE |
        PIPE_READMODE_MESSAGE |
        PIPE_WAIT,                  // ������ ��� ����
        PIPE_UNLIMITED_INSTANCES,   // ������ �ν��Ͻ� �ִ�ġ
        BUFSIZE,                    // ���� ������(Out)
        BUFSIZE,                    // ���� ������(In)
        0, NULL);                   // ��ť��Ƽ �Ӽ� �⺻��
    if (INVALID_HANDLE_VALUE == hPipe)
    {
        wprintf(TEXT("���� ������ ���� ���� (���� �ڵ� = %d)\n"), GetLastError());
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

    // �������� ���޵��� ���� ���
    if (NULL == arg)
    {
        printf("������ ���� ����\n");
        if (NULL == pchRequest) HeapFree(hHeap, 0, pchRequest);
        if (NULL == pchReply) HeapFree(hHeap, 0, pchReply);
        return -1;
    }

    if (NULL == pchRequest)
    {
        printf("Request �� �Ҵ� ����\n");
        if (NULL == pchReply) HeapFree(hHeap, 0, pchReply);
        return -1;
    }

    if (NULL == pchReply)
    {
        printf("Reply �� �Ҵ� ����\n");
        if (NULL == pchRequest) HeapFree(hHeap, 0, pchRequest);
        return -1;
    }

    hPipe = (HANDLE)arg;
    while (true)
    {
        // 1 ~ 2 ~ 3 �ݺ�, ������ ������ �޽����� ������ ����
        // �ʱ� ���� ���ð� ���� Ȯ���� ��� �̿ܿ� ����.

        // 1. �޽��� �б�
        result = ReadFile(
            hPipe,                      // ������ �ڵ�
            pchRequest,                 // ���� ������ ����
            BUFSIZE * sizeof(TCHAR),    // ���� ������
            &cbBytesRead,               // ����Ʈ ���ŷ�
            NULL);                      // ��ø I/O ���� �Ķ����
        if (!result || 0 == cbBytesRead)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE) printf("Ŭ���̾�Ʈ ���� ����\n");
            else wprintf(TEXT("ReadFile() (���� �ڵ� = %d)\n"), GetLastError());
            break;
        }

        // 2. �޽��� ó��
        GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);

        // 3. �޽��� �ۼ�
        result = WriteFile(
            hPipe,                      // ������ �ڵ�
            pchReply,                   // �۽� ������ ����
            cbReplyBytes,               // �ۼ��� ����Ʈ �۽ŷ�
            &cbWritten,                 // �ۼ� ����Ʈ��
            NULL);                      // ��ø I/O ���� �Ķ����
        if (!result || cbReplyBytes != cbWritten)
        {
            // �ۼ��� �����߰ų� �ۼ��� �����Ͱ� �ùٸ��� ���� ���
            wprintf(TEXT("WriteFile() (���� �ڵ� = %d)\n"), GetLastError());
            break;
        }
    }

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    HeapFree(hHeap, 0, pchRequest);
    HeapFree(hHeap, 0, pchReply);
    printf("������ ����\n");
    return 0;
}

void GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes)
{
    wprintf(TEXT("Ŭ���̾�Ʈ�� ���� �޽��� : %s\n"), pchRequest);
    if (FAILED(StringCchCopy(pchReply, BUFSIZE, TEXT("deafult answer from server"))))
    {
        *pchBytes = 0;
        pchReply[0] = 0;
        printf("StringCchCopy failed, no outgoing msg\n");
        return;
    }

    *pchBytes = (lstrlen((pchReply)+1) * sizeof(TCHAR));
}
