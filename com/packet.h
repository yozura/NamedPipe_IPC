/*
	이 헤더 파일은 
	(마스터 서버 <-> 슬레이브 서버) 
	혹은
	(슬레이브 서버 <-> 클라이언트)
	에 적용되는 모든 패킷에 대하여 작성합니다.

*/

#pragma once

/* 패킷 전송 시 타입 구분을 위해 패킷에 유일성 부여 */
#define TYPE_CHAT	1001
#define TYPE_TEXT	1002
#define TYPE_IMG	1003

/* 헤더 메시지 패킷 */
typedef struct tag_msg_header
{
	int type;
	int length;
	char path[255];
} MSG_HEADER;

/* 바디 메시지 패킷 */
typedef struct tag_msg_body
{
	char* contents;
} MSG_BODY;