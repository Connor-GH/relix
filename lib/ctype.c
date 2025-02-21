#include <ctype.h>

int
isdigit(int c)
{
	return '0' <= c && c <= '9';
}

int
isxdigit(int c)
{
	return ('0' <= c && c <= '9') || ('a' <= tolower(c) && tolower(c) <= 'z');
}

int
isspace(int c)
{
	return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' ||
				 c == '\v';
}
int
isalpha(int c)
{
	return islower(c) || isupper(c);
}

int
isgraph(int c)
{
	return 33 <= c && c <= 126;
}

int
isprint(int c)
{
	return isgraph(c) || c == ' ';
}

int
ispunct(int c)
{
	return (33 <= c && c <= 47) ||
		(58 <= c && c <= 64) ||
		(91 <= c && c <= 96) ||
		(123 <= c && c <= 126);
}

int
isupper(int c)
{
	return ('A' <= c && c <= 'Z');
}

int
islower(int c)
{
	return ('a' <= c && c <= 'z');
}

int
toupper(int c)
{
	if ('a' <= c && c <= 'z')
		return c - 0x20;
	else if ('A' <= c && c <= 'Z')
		return c;
	return c;
}

int
tolower(int c)
{
	if ('A' <= c && c <= 'Z')
		return c + 0x20;
	else if ('a' <= c && c <= 'z')
		return c;
	return c;
}

int
isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

int
iscntrl(int c)
{
	return c <= 31 || c == 127;
}

int
isblank(int c)
{
	return c == ' ' || c == '\t';
}

int
isascii(int c)
{
	return c <= 127;
}
