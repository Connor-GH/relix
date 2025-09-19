/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include <libgen.h>
#include <string.h>

char *
basename(char *path)
{
	if (path == NULL || path[0] == '\0') {
		return ".";
	}

	// Remove the last '/' characters.
	// In this case, "//" is defined in this implementation
	// to return "/".
	for (size_t len = strlen(path); len >= 2 && path[len - 1] == '/'; len--) {
		path[len - 1] = '\0';
	}
	if (strcmp(path, "/") == 0) {
		return "/";
	}

	char *last_slash = strrchr(path, '/');
	if (last_slash == NULL) {
		return path;
	}
	return last_slash + 1;
}

char *
dirname(char *path)
{
	size_t len;
	if (path == NULL || path[0] == '\0') {
		return ".";
	}

	len = strlen(path) - 1;

	// Find first non-slash.
	// "/usr/lib/foo"
	//             ^
	for (; path[len] == '/'; len--) {
		if (len == 0) {
			return "/";
		}
	}

	// Find first slash.
	// "/usr/lib//foo"
	//           ^
	for (; path[len] != '/'; len--) {
		if (len == 0) {
			return ".";
		}
	}

	// Skip over slashes if needed.
	// "/usr/lib//foo"
	//         ^
	for (; path[len] == '/'; len--) {
		if (len == 0) {
			return "/";
		}
	}
	// Put a NUL where the slash was.
	// "/usr/lib\0/foo" == "/usr/lib"
	path[len + 1] = '\0';
	return path;
}
