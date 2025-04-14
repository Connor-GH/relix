/*	$OpenBSD: setenv.c,v 1.20 2022/08/08 22:40:03 millert Exp $ */
/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

static char **lastenv; /* last value of environ */

/*
 * putenv --
 *	Add a name=value string directly to the environmental, replacing
 *	any current value.
 */
int
putenv(char *str)
{
	char **newenviron, *ptr;
	size_t cnt = 0;
	int offset = 0;

	// "Walk" the pointer all the way up to "="
	for (ptr = str; ptr != NULL && *ptr && *ptr != '='; ptr++)
		;

	if (ptr == str || *ptr != '=') {
		// '=' is the first character of string or is missing.
		errno = EINVAL;
		return -1;
	}

	if (__findenv(str, (int)(ptr - str), &offset) != NULL) {
		environ[offset++] = str;

		// The variable could be set multiple times.
		while (__findenv(str, (int)(ptr - str), &offset)) {
			for (newenviron = &environ[offset];; newenviron++)
				if (!(*newenviron = *(newenviron + 1)))
					break;
		}
		return (0);
	}

	// Create new slot for string.
	if (environ != NULL) {
		for (newenviron = environ; *newenviron != NULL; newenviron++)
			;
		cnt = newenviron - environ;
	}
	newenviron = realloc(lastenv, (cnt + 2) * sizeof(char *));

	if (newenviron == NULL)
		return -1;

	if (lastenv != environ && environ != NULL)
		memcpy(newenviron, environ, cnt * sizeof(char *));

	lastenv = environ = newenviron;
	environ[cnt] = str;
	environ[cnt + 1] = NULL;
	return 0;
}

/*
 * setenv --
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 */
int
setenv(const char *name, const char *value, int rewrite)
{
	char *newentry, **newenviron;
	const char *np;
	int value_length, offset = 0;

	if (name == NULL || *name == '\0') {
		errno = EINVAL;
		return -1;
	}
	for (np = name; *np && *np != '='; ++np)
		;
	if (*np) {
		errno = EINVAL;
		return -1; /* has `=' in name */
	}

	value_length = strlen(value);
	if ((newentry = __findenv(name, (int)(np - name), &offset)) != NULL) {
		int tmpoff = offset + 1;
		if (!rewrite)
			return 0;
#if 0 /* XXX - existing entry may not be writable */
		if (strlen(newentry) >= value_length) {	/* old larger; copy over */
			while ((*newentry++ = *value++))
				;
			return (0);
		}
#endif
		/* could be set multiple times */
		while (__findenv(name, (int)(np - name), &tmpoff)) {
			for (newenviron = &environ[tmpoff];; newenviron++)
				if (!(*newenviron = *(newenviron + 1)))
					break;
		}
	} else { /* create new slot */
		size_t cnt = 0;

		if (environ != NULL) {
			for (newenviron = environ; *newenviron != NULL; newenviron++)
				;
			cnt = newenviron - environ;
		}
		newenviron = realloc(lastenv, (cnt + 2) * sizeof(char *));

		if (!newenviron)
			return -1;

		if (lastenv != environ && environ != NULL)
			memcpy(newenviron, environ, cnt * sizeof(char *));

		lastenv = environ = newenviron;
		offset = cnt;
		environ[cnt + 1] = NULL;
	}
	if (!(environ[offset] = /* name + `=' + value */
				malloc((int)(np - name) + value_length + 2)))
		return -1;

	// The new environ entry is going to be placed in this slot.
	// Prepare it and then copy over the bytes.
	for (newentry = environ[offset]; (*newentry = *name++) && *newentry != '='; newentry++)
		;
	for (*newentry++ = '='; (*newentry++ = *value++);)
		;
	return 0;
}

/*
 * unsetenv(name) --
 *	Delete environmental variable "name".
 */
int
unsetenv(const char *name)
{
	char **newenviron;
	const char *np;
	int offset = 0;

	if (name == NULL || *name == '\0') {
		errno = EINVAL;
		return -1;
	}
	for (np = name; *np && *np != '='; ++np)
		;
	if (*np) {
		errno = EINVAL;
		return -1; /* has `=' in name */
	}

	/* could be set multiple times */
	while (__findenv(name, (int)(np - name), &offset)) {
		for (newenviron = &environ[offset];; newenviron++)
			if (!(*newenviron = *(newenviron + 1)))
				break;
	}
	return 0;
}
