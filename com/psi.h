/* 
	�� ��� ������ ������ ���� ���� ����ü�� ���õǾ� �ֽ��ϴ�.
	���� �� �ڵ忡 ���� �ּ��� �����ϰ� �����ּ���.

*/
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* ������ ��� �� ���Ǵ� ��ũ�� */
#define PIPE_TYPE_INIT	2000
#define PIPE_TYPE_MP	2001

/* ������ ��� �� ���Ǵ� ����ü */
#define PIPE_MSG_SIZE	512
typedef struct tag_pipe_msg
{
	int type;
	char userName[9];
	char msg[128];
	char dummy[PIPE_MSG_SIZE - sizeof(int) - 128 - 9];
} PIPE_MSG;

/* ������ �̸� ��ũ�� */
#define PIPE_MASTER	TEXT("\\\\.\\pipe\\master")
#define PIPE_ASGARD TEXT("\\\\.\\pipe\\asgard")
#define PIPE_MIDGARD TEXT("\\\\.\\pipe\\midgard")

/* ������ ���� ���� �ε��� */
enum class PIPE_SERVER
{
	ASGARD = 0,
	MIDGARD,
};

/* ������ ������ ��Ʈ ��ȣ */
enum class PIPE_SERVER_PORT
{
	ASGARD = 9010,
	MIDGARD,
};

/* ���� ���� ������ ������ ���� ����*/
#define PIPE_SERVER_COUNT 2

/* ������ ���� Ÿ��Ʋ */
static TCHAR lpTitles[][64] = {
	TEXT("�ƽ�������"), TEXT("�̵尡����")
};

/* ������ ���� ���� ��� */
static TCHAR cmdLines[][64] = {
	TEXT("SlaveServer.exe asgard"), TEXT("SlaveServer.exe midgard")
};

/* ������ �̸� */
static LPCTSTR lpszPipeNames[] = {
	PIPE_ASGARD, PIPE_MIDGARD,
};

/* 
	������ ���� ���� ����ü�Դϴ�.
	�� ����ü�� �̿��� ������ ������ ������ �� �ֽ��ϴ�.
	
*/
typedef struct tag_pipe_server_info
{
	STARTUPINFO				si;				// ���μ��� ������ �ʿ�
	PROCESS_INFORMATION		pi;				// ���μ��� ���� �� ������ �ʿ�
	HANDLE					hPipe;			// ������
	LPCTSTR					lpszPipeName;	// ������ ��� �̸�
	TCHAR					cmdLine[64];	// ���� ���� ��� 

} PIPE_SERVER_INFO;