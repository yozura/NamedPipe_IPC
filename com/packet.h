/*
	�� ��� ������ 
	(������ ���� <-> �����̺� ����) 
	Ȥ��
	(�����̺� ���� <-> Ŭ���̾�Ʈ)
	�� ����Ǵ� ��� ��Ŷ�� ���Ͽ� �ۼ��մϴ�.

*/

#pragma once

/* ��Ŷ ���� �� Ÿ�� ������ ���� ��Ŷ�� ���ϼ� �ο� */
#define TYPE_CHAT	1001
#define TYPE_TXT	1002
#define TYPE_IMG	1003

#define NAMESIZE	8
#define PATHSIZE	127
#define HEADSIZE	256

#define BODYSIZE	65535

/* ��� �޽��� ��Ŷ */
typedef struct tag_msg_header
{
	int type;
	int length;
	char name[9];
	char path[128];
	char dummy[HEADSIZE - (NAMESIZE + PATHSIZE + 2) - (2 * sizeof(int))];
} MSG_HEADER;

/* �ٵ� �޽��� ��Ŷ */
typedef struct tag_msg_body
{
	char contents[BODYSIZE];
} MSG_BODY;