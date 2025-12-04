/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include <stdlib.h>
#include <unistd.h>

// TODO: we should call _init() in __libc_start_main().

void __fini_stdio(void);
static void
cleanup(void)
{
	__fini_stdio();
}

void __init_stdio(void);

static void
startup(void)
{
	__init_stdio();
}

char **environ;

// Standard says that main can't have a prototype (N3220 5.1.2.3.2),
// so we funnel main's pointer through this parameter in __libc_start_main.
// Of course, if an application specifies main(void), the arguments get
// added to the registers but never used, which isn't a huge problem.
// I don't see a way of doing this purely in C.
void
__libc_start_main(int argc, char **argv, char **envp,
                  int (*mainfunc)(int argc, char **argv, char **envp))
{
	environ = envp;

	startup();
	atexit(cleanup);
	/* Possible parameters:
	 * - int argc, char **argv, char **envp
	 * - int argc, char **argv
	 * - void
	 */
	exit(mainfunc(argc, argv, envp));
}
