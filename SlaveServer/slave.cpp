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

    // � ������ ����ϴ��� üũ�ϴ� �κ�, ���� NULL�� ��ȯ�Ѵٸ� ��� ����
    lpszPipename = CheckServerType(argv[1]);
    if (NULL == lpszPipename) return -1;

    // ������ ���� �õ�
    hPipe = TryToConnectPipe(lpszPipename, timeout);
    if (NULL == hPipe) return -1;

    // ������ ���ῡ ���������Ƿ� �޽��� �б� ���� ��ȯ
    result = ChangePipeMode(hPipe, (PIPE_READMODE_MESSAGE));
    if (FALSE == result) return -1;
    
    // ���� �ʱ� ���� �޽����� �ۼ��Ѵ�.
    lstrcpy(buf, lpszPipename);
    lstrcat(buf, TEXT(" activate"));
    writeBytes = (lstrlen(buf) + 1) * sizeof(TCHAR);
    result = WritePipeMessage(hPipe, buf, writeBytes, 0);
    if (FALSE == result) return -1;

    // ���� �ʱ� ���� �޽����� �д´�.
    readBytes = BUFSIZE;
    result = ReadPipeMessage(hPipe, buf, readBytes, 0);
    if (FALSE == result) return -1;

    // ���� ��ƾ�� �����. (���� ��)
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
            _tprintf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
            exit(1);
        }

        // �������� �����ϱ� ���� ������ �ð� ���� ����ϴٰ� ������� ���� �� ���� ����
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
