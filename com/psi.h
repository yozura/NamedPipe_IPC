/* 
	이 헤더 파일은 파이프 서버 정보 구조체와 관련되어 있습니다.
	수정 시 코드에 대한 주석을 간략하게 남겨주세요.

*/
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* 파이프 통신 시 사용되는 매크로 */
#define PIPE_TYPE_INIT	2000
#define PIPE_TYPE_MP	2001

/* 파이프 통신 시 사용되는 구조체 */
#define PIPE_MSG_SIZE	512
typedef struct tag_pipe_msg
{
	int type;
	char userName[9];
	char msg[128];
	char dummy[PIPE_MSG_SIZE - sizeof(int) - 128 - 9];
} PIPE_MSG;

/* 파이프 이름 매크로 */
#define PIPE_MASTER	TEXT("\\\\.\\pipe\\master")
#define PIPE_ASGARD TEXT("\\\\.\\pipe\\asgard")
#define PIPE_MIDGARD TEXT("\\\\.\\pipe\\midgard")

/* 파이프 서버 접근 인덱스 */
enum class PIPE_SERVER
{
	ASGARD = 0,
	MIDGARD,
};

/* 파이프 서버별 포트 번호 */
enum class PIPE_SERVER_PORT
{
	ASGARD = 9010,
	MIDGARD,
};

/* 현재 가동 가능한 파이프 서버 개수*/
#define PIPE_SERVER_COUNT 2

/* 파이프 서버 타이틀 */
static TCHAR lpTitles[][64] = {
	TEXT("아스가르드"), TEXT("미드가르드")
};

/* 파이프 서버 실행 경로 */
static TCHAR cmdLines[][64] = {
	TEXT("SlaveServer.exe asgard"), TEXT("SlaveServer.exe midgard")
};

/* 파이프 이름 */
static LPCTSTR lpszPipeNames[] = {
	PIPE_ASGARD, PIPE_MIDGARD,
};

/* 
	파이프 서버 정보 구조체입니다.
	이 구조체를 이용해 파이프 서버를 관리할 수 있습니다.
	
*/
typedef struct tag_pipe_server_info
{
	STARTUPINFO				si;				// 프로세스 생성시 필요
	PROCESS_INFORMATION		pi;				// 프로세스 생성 및 관리에 필요
	HANDLE					hPipe;			// 파이프
	LPCTSTR					lpszPipeName;	// 파이프 경로 이름
	TCHAR					cmdLine[64];	// 서버 실행 경로 

} PIPE_SERVER_INFO;