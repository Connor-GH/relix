/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1991 Keith Muller.
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#if 0
#ifndef lint
static char sccsid[] = "@(#)egetopt.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * egetopt:	get option letter from argument vector (an extended
 *		version of getopt).
 *
 * Non standard additions to the ostr specs are:
 * 1) '?': immediate value following arg is optional (no white space
 *    between the arg and the value)
 * 2) '#': +/- followed by a number (with an optional sign but
 *    no white space between the arg and the number). The - may be
 *    combined with other options, but the + cannot.
 */
#if 0
extern int opterr;
int	opterr = 1;		/* if error message should be printed */
int	optind = 1;		/* index into parent argv vector */
int	optopt;		/* character checked for validity */
char	*optarg;		/* argument associated with option */
#endif
#define	BADCH	(int)'?'

static char	emsg[] = "";

int
getopt(int nargc, char *const *nargv, const char *ostr)
{
	static char *place = emsg;	/* option letter processing */
	char *oli;			/* option letter list index */
	static int delim;		/* which option delimiter */
	char *p;
	static char savec = '\0';

	if (savec != '\0') {
		*place = savec;
		savec = '\0';
	}

	if (!*place) {
		/*
		 * update scanning pointer
		 */
		if ((optind >= nargc) ||
		    ((*(place = nargv[optind]) != '-') && (*place != '+'))) {
			place = emsg;
			return (-1);
		}

		delim = (int)*place;
		if (place[1] && *++place == '-' && !place[1]) {
			/*
			 * found "--"
			 */
			++optind;
			place = emsg;
			return (-1);
		}
	}

	/*
	 * check option letter
	 */
	if ((optopt = (int)*place++) == (int)':' || (optopt == (int)'?') ||
	    !(oli = strchr(ostr, optopt))) {
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means -1 when by itself.
		 */
		if ((optopt == (int)'-') && !*place)
			return (-1);
		if (strchr(ostr, '#') && (isdigit(optopt) ||
		    (((optopt == (int)'-') || (optopt == (int)'+')) &&
		      isdigit(*place)))) {
			/*
			 * # option: +/- with a number is ok
			 */
			for (p = place; *p != '\0'; ++p) {
				if (!isdigit(*p))
					break;
			}
			optarg = place-1;

			if (*p == '\0') {
				place = emsg;
				++optind;
			} else {
				place = p;
				savec = *p;
				*place = '\0';
			}
			return (delim);
		}

		if (!*place)
			++optind;
		if (opterr) {
			if (!(p = strrchr(*nargv, '/')))
				p = *nargv;
			else
				++p;
			(void)fprintf(stderr, "%s: illegal option -- %c\n",
			    p, optopt);
		}
		return (BADCH);
	}
	if (delim == (int)'+') {
		/*
		 * '+' is only allowed with numbers
		 */
		if (!*place)
			++optind;
		if (opterr) {
			if (!(p = strrchr(*nargv, '/')))
				p = *nargv;
			else
				++p;
			(void)fprintf(stderr,
				"%s: illegal '+' delimiter with option -- %c\n",
				p, optopt);
		}
		return (BADCH);
	}
	++oli;
	if ((*oli != ':') && (*oli != '?')) {
		/*
		 * don't need argument
		 */
		optarg = NULL;
		if (!*place)
			++optind;
		return (optopt);
	}

	if (*place) {
		/*
		 * no white space
		 */
		optarg = place;
	} else if (*oli == '?') {
		/*
		 * no arg, but NOT required
		 */
		optarg = NULL;
	} else if (nargc <= ++optind) {
		/*
		 * no arg, but IS required
		 */
		place = emsg;
		if (opterr) {
			if (!(p = strrchr(*nargv, '/')))
				p = *nargv;
			else
				++p;
			(void)fprintf(stderr,
			    "%s: option requires an argument -- %c\n", p,
			    optopt);
		}
		return (BADCH);
	} else {
		/*
		 * arg has white space
		 */
		optarg = nargv[optind];
	}
	place = emsg;
	++optind;
	return (optopt);
}
