#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>

bool CreateMasterPipe(LPCTSTR pipeName, DWORD bufSize, HANDLE* hPipe)
{
    HANDLE tempPipe = CreateNamedPipe(
        pipeName,          // ������ �̸�
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

    *hPipe = tempPipe;
    return true;
}

/// <summary>
/// ���� ���� ���μ����� �����մϴ�.
/// </summary>
/// <param name="psi">������ ���� ���� ����ü�Դϴ�.</param>
/// <returns>
///     true�� ��� ���� ���μ��� ������ �����Ͽ� ������ ���� ���� ����ü�� ����˴ϴ�.
///     false�� ��� ���μ��� ������ �����߽��ϴ�.
/// </returns>
bool CreateSlaveProcess(PIPE_SERVER_INFO* psi);

/// <summary>
/// ���� ���� �������� �����մϴ�.
/// </summary>
/// <param name="psi">������ ���� ���� ����ü�Դϴ�.</param>
/// <returns>
///     true�� ��� ���� ������ ������ �����Ͽ� ������ ���� ���� ����ü�� ����˴ϴ�.
///     false�� ��� ������ ������ ������ �߻��߽��ϴ�.
/// </returns>
bool CreateSlavePipe(PIPE_SERVER_INFO* psi, DWORD bufSize);

/// <summary>
/// ���� ���� ���μ����� �����ϰ� ���� ������ ������ �������� �����մϴ�.
/// </summary>
/// <param name="psi">������ ���� ���� ����ü�Դϴ�.</param>
/// <returns>
///     true�� ��� ���� ������ ����, ���� ���� ���μ��� ������ �����Ͽ� ������ ���� ���� ����ü�� ����˴ϴ�.
///     false�� ��� �ʱ�ȭ�� ������ ������ �޼��� �ڽ��� ����մϴ�.
/// </returns>
bool InitializeSlave(PIPE_SERVER_INFO* psi, DWORD bufSize);

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

    CloseHandle(psi->pi.hProcess);
    CloseHandle(psi->pi.hThread);
    return true;
}

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
        MessageBox(NULL, TEXT("���� ���� ���μ��� ���� ����"), TEXT("CreateSalveProcess()"), MB_ICONERROR);  
        return false;
    }

    return true;
}

/// <summary>
/// Ư�� ������ �޽����� �ۼ�.
/// </summary>
/// <param name="hPipe">������ �ڵ�</param>
/// <param name="buf">����</param>
/// <param name="writeBytes">�ۼ��� ������ ũ��</param>
/// <param name="cbWritten">�ۼ��� ������ ũ��</param>
/// <returns></returns>
BOOL WritePipeMessage(HANDLE hPipe, LPVOID buf, DWORD writeBytes, DWORD cbWritten);

/// <summary>
/// Ư�� ������ �޽����� ����.
/// </summary>
/// <param name="hPipe">������ �ڵ�</param>
/// <param name="buf">����</param>
/// <param name="readBytes">���� ������ ũ��</param>
/// <param name="cbRead">���� ������ ũ��</param>
/// <returns>����/���� ���� ��ȯ</returns>
BOOL ReadPipeMessage(HANDLE hPipe, LPVOID buf, DWORD readBytes, DWORD cbRead);