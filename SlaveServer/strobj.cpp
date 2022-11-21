#include "strobj.h"

int length(String str) {
	return (int)strlen(str);
}

String concat(String str1, String str2) {
	int str1len = length(str1);
	int str2len = length(str2);
	char* result = (char*)calloc(str1len + str2len + 1, sizeof(char));
	if (NULL == result)
		return NULL;

	sprintf(result, "%s%s", str1, str2);
	return result;
}

String substring(String target, int from, int to) {
	int sz = to - from;
	if (sz < 0) return NULL;
	else if (sz == 0) return String();
	String result = (char*)calloc(sz + 1, sizeof(char));
	if (NULL == result)
		return NULL;

	strncpy(result, target + from, sz);
	result[sz] = '\0';
	return result;
}

int strat(String target, String input) {
	int result = 0;
	char* comp = strstr(target, input);
	if (comp == NULL) return -1;
	while (true) {
		if (target == comp) break;
		target++;
		result++;
	}
	return result;
}

int charat(String target, char input) {
	int result = 0;
	char* comp = strchr(target, input);
	if (comp == NULL) return -1;
	while (true) {
		if (target == comp) break;
		target++;
		result++;
	}
	return result;
}

String replace(String target, String delim, String rep) {
	int idx, delimlen;
	String result = String("");
	while (true) {
		idx = strat(target, delim);
		if (idx == -1) {
			result = concat(result, target);
			break;
		}
		delimlen = length(delim);
		String prestr = substring(target, 0, idx);
		target = substring(target, idx + delimlen, length(target) + 1);
		result = concat(result, prestr);
		result = concat(result, rep);
	}
	return result;
}

String ltrim(String target) {
	String result = target;
	while (result[0] == ' ' || result[0] == '\t')
		result = substring(result, 1, length(result));
	return result;
}

String rtrim(String target) {
	String result = target;
	int len = length(target);
	while (result[len - 1] == ' ' || result[len - 1] == '\t') {
		result = substring(result, 0, len - 1);
		len = length(result);
	}
	return result;
}

String trim(String target) {
	String result = target;
	result = ltrim(result);
	result = rtrim(result);
	return result;
}

void destroystr(String str) {
	free(str);
}

String touppercase(String target) {
	int len = length(target);
	char* result = (char*)calloc(len + 1, sizeof(char));
	if (NULL == result)
		return NULL;

	int i = 0;
	while (target[i] != '\0') {
		result[i] = toupper(target[i]);
		i++;
	}
	result[len] = '\0';
	return result;
}

String tolowercase(String target) {
	int len = length(target);
	char* result = (char*)calloc(len + 1, sizeof(char));
	if (NULL == result)
		return NULL;

	int i = 0;
	while (target[i] != '\0') {
		result[i] = tolower(target[i]);
		i++;
	}
	result[len] = '\0';
	return result;
}

int lastindexof(String target, String input) {
	int res = strat(target, input);
	int previdx = res;
	int idx = previdx;
	while (idx != -1) {
		target = substring(target, idx + 1, length(target) + 1);
		idx = strat(target, input);
		previdx = idx + 1;
		res += previdx;
	}
	return res;
}