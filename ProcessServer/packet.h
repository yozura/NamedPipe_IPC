#pragma once

#define TYPE_CHAT	1001
#define TYPE_TEXT	1002
#define TYPE_IMG	1003

typedef struct tag_msg_header
{
	int type;
	int length;
	char path[255];
};

typedef struct tag_msg_body
{
	char* contents;
};