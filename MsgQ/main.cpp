#include <windows.h>
#include <semaphore.h>
#include <pthread.h>

#include <stdio.h>
#include <tchar.h>

sem_t semp;
int num = 2;

int main(int argc)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    TCHAR title[] = L"아스가르드";
    TCHAR cmdLine[] = L"ProcessServer.exe";

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.lpTitle = title;
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process. 
    if (!CreateProcess(NULL,   // No module name (use command line)
        cmdLine,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return -1;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}