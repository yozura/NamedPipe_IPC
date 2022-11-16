#include "common.h"

#define BUFSIZE     1024

DWORD WINAPI PipeInstanceThread(LPVOID arg);
void GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD);

int main(int argc)
{
    setlocale(LC_ALL, "korean");

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    TCHAR lpTitle[] = TEXT("�����̺�");
    TCHAR cmdLine[] = TEXT("SlaveServer.exe");
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.lpTitle = lpTitle;
    ZeroMemory(&pi, sizeof(pi));

    // ä�� ���� ���μ��� ����
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
    wprintf(TEXT("%s ���μ��� ���� ����\n"), lpTitle);

    BOOL fConnected = FALSE;
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    LPCTSTR lpszPipeName = PIPE_ASGARD;
    HANDLE hThread;
    
    while (true)
    {
        wprintf(TEXT("%s ������ ���� ���� ��\n"), lpszPipeName);
        hPipe = CreateNamedPipe(
            lpszPipeName,               // ������ �̸�
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
            return -1;
        }


        fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (fConnected)
        {
            printf(">>> ������ ���� Ȯ�� �Ϸ�\n");
            hThread = CreateThread(NULL, 0, PipeInstanceThread, (LPVOID)hPipe, 0, NULL);
            if (NULL == hThread)
            {
                wprintf(TEXT("������ ���� ���� (���� �ڵ� = %d)\n"), GetLastError());
                return -1;
            }
            else CloseHandle(hThread);
        }
        else
        {
            // Ŭ���̾�Ʈ ������ ������ ��� ������ �ݱ�
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
        fSuccess = ReadFile(
            hPipe,                      // ������ �ڵ�
            pchRequest,                 // ���� ������ ����
            BUFSIZE * sizeof(TCHAR),    // ���� ������
            &cbBytesRead,               // ����Ʈ ���ŷ�
            NULL);                      // ��ø I/O ���� �Ķ����
        if (!fSuccess || 0 == cbBytesRead)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE) printf("Ŭ���̾�Ʈ ���� ����\n");
            else wprintf(TEXT("ReadFile() (���� �ڵ� = %d)\n"), GetLastError());
            break;
        }

        // �޽��� ó��
        GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);

        // �������� ���� �ۼ�
        fSuccess = WriteFile(
            hPipe,                      // ������ �ڵ�
            pchReply,                   // �۽� ������ ����
            cbReplyBytes,               // �ۼ��� ����Ʈ �۽ŷ�
            &cbWritten,                 // �ۼ� ����Ʈ��
            NULL);                      // ��ø I/O ���� �Ķ����
        if (!fSuccess || cbReplyBytes != cbWritten)
        {
            // �ۼ��� �����߰ų� �ۼ��� �����Ͱ� �ùٸ��� ���� ���
            wprintf(TEXT("WriteFile() (���� �ڵ� = %d)\n"), GetLastError());
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

    *pchBytes = (lstrlen((pchReply) + 1) * sizeof(TCHAR));
}
