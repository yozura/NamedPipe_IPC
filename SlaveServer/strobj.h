#pragma once
#pragma warning(disable:4996)

#include <string>
#include <stdlib.h>


// String 타입 정의. char *로 쓰는 것보다 친숙하게 사용 가능.
#define STRING char *
typedef STRING String;
typedef String string;
#define TEMPSZ 128

int length(String str);
String concat(String str1, String str2);
String substring(String target, int from, int to);
//LenStringArr* split(String target, String delim);
int strat(String target, String input);
int charat(String target, char input);
void destroystr(String str);
String replace(String target, String delim, String rep);
String ltrim(String target);
String rtrim(String target);
String trim(String target);
String touppercase(String target);
String tolowercase(String target);
int lastindexof(String target, String input);