#pragma once
#include "memory.h"
#include <string.h>

#define DEFAULT_STRING_SIZE 256

class String {
public:
	char* cstr;
	u32 curLen; // excludes the '\0'
	u32 maxLen; // includes the '\0'

	String(Memory& mem) :
		String(mem, DEFAULT_STRING_SIZE)
	{}

	String(Memory& mem, u32 maxLen) :
		curLen(0),
		maxLen(maxLen)
	{
		this->cstr = (char*) Alloc(mem, maxLen);
	}
	
	operator char*() {return this->cstr;}

	String& operator+=(const char* rhs) {
		for(u32 i = 0; rhs[i] != '\0'; i++) {
			if(this->curLen == this->maxLen - 1) {
				printf("ERROR: attempting to assign String to something larger than its maxLen\n");
				this->cstr[this->curLen] = '\0';
				return *this;
			}
			this->cstr[this->curLen] = rhs[i];
			this->curLen++;
		}
		this->cstr[this->curLen] = '\0';
		return *this;
	}
	String& operator=(const char* rhs) {
		this->curLen = 0;
		return operator+=(rhs);
	}
};

void InitString(Memory& mem, String* str, u32 maxLen = DEFAULT_STRING_SIZE) {
	str->curLen = 0;
	str->maxLen = maxLen;
	str->cstr = (char*) Alloc(mem, maxLen);
}

String* InitString(Memory& mem, u32 maxLen = DEFAULT_STRING_SIZE) {
	String* str = (String*) Alloc(mem, sizeof(String));
	InitString(mem, str, maxLen);
	return str;
}

bool operator==(const String& lhs, const char* rhs) {return strcmp(lhs.cstr, rhs) == 0;}
bool operator!=(const String& lhs, const char* rhs) {return !operator==(lhs,rhs);}
bool operator< (const String& lhs, const char* rhs) {return strcmp(lhs.cstr, rhs) < 0;}
bool operator> (const String& lhs, const char* rhs) {return strcmp(lhs.cstr, rhs) > 0;}
bool operator<=(const String& lhs, const char* rhs) {return !operator> (lhs,rhs);}
bool operator>=(const String& lhs, const char* rhs) {return !operator< (lhs,rhs);}

bool isWhitespace(unsigned char c) {
	return (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f');
}

void RemoveWhitespace(String& str) {
	u32 newIndex = 0;
	for(u32 i = 0; i < str.curLen; i++) {
		if(!isWhitespace(str[i])) {
			str[newIndex] = str[i];
			newIndex++;
		}
	}
	str[newIndex] = '\0';
	str.curLen = newIndex;
}

u32 GetFirstIndexOf(String& str, char c) {
	for(u32 i = 0; i < str.curLen; i++) {
		if(str[i] == c) return i;
	}
	return -1;
}

String* Substring(Memory& mem, String& str, u32 startInclusive, u32 endExclusive, u32 newStrLen = 0) {
	if(startInclusive < 0 || startInclusive >= str.curLen) {
		printf("ERROR: passed invalid arguments in String.Substring()\n");
		return NULL;
	}
	if(endExclusive <= 0 || endExclusive > str.curLen) {
		printf("ERROR: passed invalid arguments in String.Substring()\n");
		return NULL;
	}
	u32 newLen = newStrLen;
	if(newLen == 0) newLen = endExclusive - startInclusive + 1;
	String& str_copy = *InitString(mem, newLen);
	str_copy.curLen = endExclusive - startInclusive;
	for(u32 i = 0; i < str_copy.curLen; i++) {
		str_copy[i] = str[i + startInclusive];
	}
	str_copy[str_copy.curLen] = '\0';
	return &str_copy;
}

s64 StringToLong(String& str) {
	s64 ret = 0;
	s8 sign = 1;
	for(u32 i = 0; i < str.curLen; i++) {
		char c = str[i];
		if(i == 0 && c == '-') {
			sign = -1;
			continue;
		}
		ret = ret * 10 + (c - '0');
	}
	return sign == 1 ? ret : -ret;
}

r64 StringToDouble(String& str) {
	r64 ret = 0;
	s8 sign = 1;
	u32 decimal_index = (u32)-1;
	for(u32 i = 0; i < str.curLen; i++) {
		char c = str[i];
		if(i == 0 && c == '-') {
			sign = -1;
			continue;
		}
		if(c == '.') {
			decimal_index = i;
			continue;
		}
		if(decimal_index == (u32)-1) {
			ret = ret * 10 + (c - '0');
		}
		else {
			double divisor = (i - decimal_index) * 10;
			ret += (c - '0') / divisor;
		}
	}
	return sign == 1 ? ret : -ret;
}

// simple unit test
// int main() {
// 	Memory mem;
// 	InitMemory(mem, 10 KB);
// 	String str(mem);
// 	str = "test";
// 	str += " test2";
// 	str += "\ntest3";
// 	printf("%s\n", (char*)str);
// 	for(int i = 0; i < str.curLen; i++) {
// 		printf("%c", str[i]);
// 	}
// 	printf("\n");
// 	if(str == "test test2\ntest3") {
// 		printf("comparison works\n");
// 	} else {
// 		printf("comparison doesnt work\n");
// 	}
//	DeinitMemory(mem);
// 	return 0;
// }
