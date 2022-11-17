#include "../com/psi.h"
#include "pipefunc.h"

#define BUFSIZE     1024

/// <summary>
/// ���μ��� �������� ���� �� �� �ش� �������� �б�/���⸦ �����ϴ� �����忡 ���˴ϴ�.
/// </summary>
/// <param name="arg">������(�ʱ�ȭ��) �������� �����ؾ��մϴ�.</param>
DWORD WINAPI PipeInstanceThread(LPVOID arg);

/* ���� ���� */
PIPE_SERVER_INFO g_Servers[PIPE_SERVER_COUNT]; 

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "korean");

    bool result;
    HANDLE hThread;

    for (int i = 0; i < PIPE_SERVER_COUNT; ++i)
    {
        PIPE_SERVER_INFO psi;
        ZeroMemory(&psi.si, sizeof(psi.si));
        ZeroMemory(&psi.pi, sizeof(psi.pi));

        psi.si.cb = sizeof(psi.si);
        psi.si.lpTitle = lpTitles[i];

        lstrcpy(psi.cmdLine, cmdLines[i]);
        psi.lpszPipeName = lpszPipenames[i];

        result = InitializeSlave(&psi, BUFSIZE);
        if (!result) return -1;

        // ���μ����� �������� ���� ���� Ȯ��
        result = ConnectNamedPipe(psi.hPipe, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!result)
        {
            MessageBox(NULL, TEXT("������ ���� ����"), TEXT("ConnectNamedPipe()"), MB_ICONERROR);
            return false;
        }

        // ������� ������ ������ ���� ���������Ƿ� �� ������ ���� ������� ���� ó��
        hThread = CreateThread(NULL, 0, PipeInstanceThread, (LPVOID)psi.hPipe, 0, NULL);
        if (NULL == hThread)
        {
            wprintf(TEXT("������ ���� ���� (���� �ڵ� = %d)\n"), GetLastError());
            MessageBox(NULL, TEXT("������ ���� ����"), TEXT("CreateThread()"), MB_ICONERROR);
            return false;
        }
        else CloseHandle(hThread);

        g_Servers[i] = psi;
        wprintf(TEXT("%s, %s ���� ����\n"), g_Servers[i].lpszPipeName, g_Servers[i].si.lpTitle);
    }

    WaitForSingleObject(g_Servers[(int)PIPE_SERVER::ASGARD].hPipe, INFINITE);
    WaitForSingleObject(g_Servers[(int)PIPE_SERVER::MIDGARD].hPipe, INFINITE);

    for (int i = 0; i < PIPE_SERVER_COUNT; ++i)
    {
        CloseHandle(g_Servers[i].pi.hProcess);
        CloseHandle(g_Servers[i].pi.hThread);
    }

    printf("������ ���� ����\n");
    return 0;
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
        GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes, BUFSIZE);

        // 3. �޽��� �ۼ�
        result = WriteFile(
            hPipe,                      // ������ �ڵ�
            pchReply,                   // �۽� ������ ����
            cbReplyBytes,               // �ۼ��� ����Ʈ �۽ŷ�
            &cbWritten,                 // �ۼ� ����Ʈ��
            NULL);                      // ��ø I/O ���� �Ķ����
        if (!result || cbReplyBytes != cbWritten)
        {
            // �ۼ��� �����߰ų� �ۼ��� �����Ͱ� �߷��� ���
            wprintf(TEXT("WriteFile() (���� �ڵ� = %d)\n"), GetLastError());
            break;
        }
    }

    // �� �Ҵ� ����
    HeapFree(hHeap, 0, pchRequest);
    HeapFree(hHeap, 0, pchReply);

    // ������ �ݱ�
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    printf("������ ����\n");
    return 0;
}

