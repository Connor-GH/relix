#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(void)
{
	FILE *fp = fopen("/foo", "w");
	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}

	fprintf(fp, "foo bar");
	int closed = fclose(fp);
	if (closed == -1) {
		perror("fclose");
		exit(2);
	}
	FILE *fp2 = fopen("/foo", "r");
	if (fp2 == NULL) {
		perror("fopen");
		exit(3);
	}
	char *buf = malloc(strlen("foo bar") + 1);
	if (buf == NULL) {
		perror("malloc");
		exit(1);
	}
	char *s = fgets(buf, strlen("foo bar") + 1, fp2);
	if (s == NULL) {
		perror("fgets");
		free(buf);
		fclose(fp2);
		exit(4);
	}
	if (strcmp(buf, "foo bar") != 0) {
		fprintf(stderr, "strcmp failed!\n");
		free(buf);
		fclose(fp2);
		exit(5);
	}
	free(buf);
	int closed2 = fclose(fp2);
	if (closed2 == -1) {
		perror("fclose");
		exit(6);
	}
	printf("TEST PASSED\n");
	return 0;
}
