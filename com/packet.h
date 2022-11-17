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
#define TYPE_TEXT	1002
#define TYPE_IMG	1003

/* ��� �޽��� ��Ŷ */
typedef struct tag_msg_header
{
	int type;
	int length;
	char path[255];
} MSG_HEADER;

/* �ٵ� �޽��� ��Ŷ */
typedef struct tag_msg_body
{
	char* contents;
} MSG_BODY;