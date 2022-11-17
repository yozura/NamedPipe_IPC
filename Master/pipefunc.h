#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>

/// <summary>
/// ���� ���� ���μ����� �����մϴ�.
/// </summary>
/// <param name="psi">������ ���� ���� ����ü�Դϴ�.</param>
/// <returns>
///     true�� ��� ���� ���μ��� ������ �����Ͽ� ������ ���� ���� ����ü�� ����˴ϴ�.
///     false�� ��� ���μ��� ������ �����߽��ϴ�.
/// </returns>
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
        wprintf(TEXT("���� ���� ���μ��� ���� ���� (���� �ڵ� = %d)\n"), GetLastError());
        return false;
    }

    return true;
}

/// <summary>
/// ���� ���� �������� �����մϴ�.
/// </summary>
/// <param name="psi">������ ���� ���� ����ü�Դϴ�.</param>
/// <returns>
///     true�� ��� ���� ������ ������ �����Ͽ� ������ ���� ���� ����ü�� ����˴ϴ�.
///     false�� ��� ������ ������ ������ �߻��߽��ϴ�.
/// </returns>
bool CreateSlavePipe(PIPE_SERVER_INFO* psi, DWORD bufSize)
{
    HANDLE hPipe = CreateNamedPipe(
        psi->lpszPipeName,          // ������ �̸�
        PIPE_ACCESS_DUPLEX,         // �б�/���� ���(�����)
        PIPE_TYPE_MESSAGE |
        PIPE_READMODE_MESSAGE |
        PIPE_WAIT,                  // ������ ��� ����
        PIPE_UNLIMITED_INSTANCES,   // ������ �ν��Ͻ� �ִ�ġ
        bufSize,                    // ���� ������(Out)
        bufSize,                    // ���� ������(In)
        0, NULL);                   // ��ť��Ƽ �Ӽ� �⺻��
    if (INVALID_HANDLE_VALUE == hPipe)
    {
        wprintf(TEXT("���� ������ ���� ���� (���� �ڵ� = %d)\n"), GetLastError());
        return false;
    }

    psi->hPipe = hPipe;
    return true;
}

/// <summary>
/// ���� ���� ���μ����� �����ϰ� ���� ������ ������ �������� �����մϴ�.
/// </summary>
/// <param name="psi">������ ���� ���� ����ü�Դϴ�.</param>
/// <returns>
///     true�� ��� ���� ������ ����, ���� ���� ���μ��� ������ �����Ͽ� ������ ���� ���� ����ü�� ����˴ϴ�.
///     false�� ��� �ʱ�ȭ�� ������ ������ �޼��� �ڽ��� ����մϴ�.
/// </returns>
bool InitializeSlave(PIPE_SERVER_INFO* psi, DWORD bufSize)
{
    bool result;

    // ������ ����
    result = CreateSlavePipe(psi, bufSize);
    if (!result)
    {
        MessageBox(NULL, TEXT("���� ���� ������ ���� ����"), TEXT("CreateSlavePipe()"), MB_ICONERROR);
        return false;
    }

    // ���μ��� ����
    result = CreateSlaveProcess(psi);
    if (!result)
    {
        MessageBox(NULL, TEXT("���� ���� ���μ��� ���� ����"), TEXT("CreateSalveProcess()"), MB_ICONERROR);        return false;
    }

    return true;
}


/// <summary>
/// 
/// </summary>
/// <param name="hPipe"></param>
/// <param name="buf"></param>
/// <param name="writeBytes"></param>
/// <param name="cbWritten"></param>
/// <returns></returns>
bool WritePipeMessage(HANDLE hPipe, const void* buf, DWORD writeBytes, DWORD cbWritten);

/// <summary>
/// �������� �ۼ��� �޽����� ������ �ľ��� �亯�� �����ϴ�.
/// </summary>
/// <param name="pchRequest"></param>
/// <param name="pchReply"></param>
/// <param name="pchBytes"></param>
void GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes, DWORD bufSize)
{
    wprintf(TEXT("Ŭ���̾�Ʈ�� ���� �޽��� : %s\n"), pchRequest);
    if (FAILED(StringCchCopy(pchReply, bufSize, TEXT("������ ���� ����"))))
    {
        *pchBytes = 0;
        pchReply[0] = 0;
        printf("StringCchCopy failed, no outgoing msg\n");
        return;
    }

    *pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
}