/*
	이 헤더 파일은 
	(마스터 서버 <-> 슬레이브 서버) 
	혹은
	(슬레이브 서버 <-> 클라이언트)
	에 적용되는 모든 패킷에 대하여 작성합니다.

*/

#pragma once

/* 패킷 전송 시 타입 구분을 위해 패킷에 유일성 부여 */
#define TYPE_END		1000
#define TYPE_CHAT		1001
#define TYPE_TXT		1002
#define TYPE_IMG		1003
#define TYPE_MP         1004
#define TYPE_FILEPATH   1005
#define TYPE_TXT_REQ	1006
#define TYPE_IMG_REQ	1007
#define TYPE_NUM		1008
	
#define NAMESIZE	9
#define PATHSIZE	128
#define HEADSIZE	256

/* 헤더 메시지 패킷 */
typedef struct tag_msg_header
{
	int type;
	int length;	
	char userName[NAMESIZE];
	char path[PATHSIZE];
	char dummy[HEADSIZE - (NAMESIZE + PATHSIZE + 2) - (2 * sizeof(int))];
} MSG_HEADER;