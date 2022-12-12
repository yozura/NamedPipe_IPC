/*
	�� ��� ������ 
	(������ ���� <-> �����̺� ����) 
	Ȥ��
	(�����̺� ���� <-> Ŭ���̾�Ʈ)
	�� ����Ǵ� ��� ��Ŷ�� ���Ͽ� �ۼ��մϴ�.

*/

#pragma once

/* ��Ŷ ���� �� Ÿ�� ������ ���� ��Ŷ�� ���ϼ� �ο� */
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

/* ��� �޽��� ��Ŷ */
typedef struct tag_msg_header
{
	int type;
	int length;	
	char userName[NAMESIZE];
	char path[PATHSIZE];
	char dummy[HEADSIZE - (NAMESIZE + PATHSIZE + 2) - (2 * sizeof(int))];
} MSG_HEADER;