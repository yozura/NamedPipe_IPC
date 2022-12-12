/* ���� ���̺귯�� */
#include <locale.h>

/* ����� ���� ���̺귯�� */
#include "../com/psi.h"
#include "../com/packet.h"
#include "../com/common.h"
#include "pipefunc.h"

#define BUFSIZE     512

/// <summary>
/// ���μ��� �������� ���� �� �� �ش� �������� �б�/���⸦ �����ϴ� �����忡 ���˴ϴ�.
/// </summary>
/// <param name="arg">������(�ʱ�ȭ��) �������� �����ؾ��մϴ�.</param>
DWORD WINAPI PipeInstanceThread(LPVOID arg);
DWORD WINAPI MasterPipeThread(LPVOID arg);
BOOL WritePipeMessage(HANDLE hPipe, void* buf, DWORD writeBytes, DWORD cbWritten);

/* ���� ���� */
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
            MessageBox(NULL, TEXT("������ ������ ���� ����"), TEXT("ConnectNamedPipe()"), MB_ICONERROR);
            return -1;
        }
    }

    hThread = CreateThread(NULL, 0, MasterPipeThread, 0, 0, NULL);
    if (NULL == hThread)
    {
        wprintf(TEXT("������ ���� ���� (���� �ڵ� = %d)\n"), GetLastError());
        MessageBox(NULL, TEXT("������ ���� ����"), TEXT("CreateThread()"), MB_ICONERROR);
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

        // ���μ����� �������� ���� ���� Ȯ��
        result = ConnectNamedPipe(psi.hPipe, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!result)
        {
            MessageBox(NULL, TEXT("������ ���� ����"), TEXT("ConnectNamedPipe()"), MB_ICONERROR);
            return -1;
        }

        // ������� ������ ������ ���� ���������Ƿ� �� ������ ���� ������� ���� ó��
        hThread = CreateThread(NULL, 0, PipeInstanceThread, (LPVOID)psi.hPipe, 0, NULL);
        if (NULL == hThread)
        {
            wprintf(TEXT("������ ���� ���� (���� �ڵ� = %d)\n"), GetLastError());
            MessageBox(NULL, TEXT("������ ���� ����"), TEXT("CreateThread()"), MB_ICONERROR);
            return -1;
        }
        else CloseHandle(hThread);

        g_Servers[i] = psi;
        wprintf(TEXT("%s, %s ���� ����\n"), g_Servers[i].lpszPipeName, g_Servers[i].si.lpTitle);
    }

    WaitForSingleObject(g_Servers[(int)PIPE_SERVER::ASGARD].hPipe, INFINITE);
    WaitForSingleObject(g_Servers[(int)PIPE_SERVER::MIDGARD].hPipe, INFINITE);

    printf("������ ���� ����\n");
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
    while (flag == FALSE)
    {
        // 1 ~ 2 ~ 3 �ݺ�, ������ ������ �޽����� ������ ����
        // �ʱ� ���� ���ð� ���� Ȯ���� ��� �̿ܿ� ����.

        // 1. �޽��� �б�
        result = ReadFile(
            hPipe,                      // ������ �ڵ�
            pchRequest,                 // ���� ������ ����
            PIPE_MSG_SIZE,              // ���� ������
            &cbBytesRead,               // ����Ʈ ���ŷ�
            NULL);                      // ��ø I/O ���� �Ķ����
        if (!result || 0 == cbBytesRead)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE) printf("Ŭ���̾�Ʈ ���� ����\n");
            else wprintf(TEXT("ReadFile() (���� �ڵ� = %d)\n"), GetLastError());
            break;
        }

        // 2. �޽��� ó��
        PIPE_MSG* pm = (PIPE_MSG*)pchRequest;
        printf("[MASTER SERVER] %s : %s\n", pm->userName, pm->msg);
        switch (pm->type)
        {
        case PIPE_TYPE_INIT:
            // �ʱ� �޽��� ����
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

DWORD WINAPI MasterPipeThread(LPVOID arg)
{
    return 0;
}

BOOL WritePipeMessage(HANDLE hPipe, void* buf, DWORD writeBytes, DWORD cbWritten)
{
    BOOL result;
    result = WriteFile(
        hPipe,                      // ������ �ڵ�
        buf,                        // �۽� ������ ����
        writeBytes,                 // �ۼ��� ����Ʈ �۽ŷ�
        &cbWritten,                 // �ۼ� ����Ʈ��
        NULL);                      // ��ø I/O ���� �Ķ����
    if (!result || writeBytes != cbWritten)
    {
        // �ۼ��� �����߰ų� �ۼ��� �����Ͱ� �߷��� ���
        wprintf(TEXT("WriteFile() (���� �ڵ� = %d)\n"), GetLastError());
        return false;
    }

    return true;
}