#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void
validate_data(const char *original, const char *received, size_t size)
{
	if (memcmp(original, received, size) != 0) {
		fprintf(stderr, "Data validation failed!\n");
		fprintf(stderr, "Got '%s', expected '%s'\n", received, original);

		exit(EXIT_FAILURE);
	}
}

void
run_tests(void)
{
	int pipefd[2];
	pid_t p;

	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	// Set to 1 because malloc(0) is undefined.
	for (size_t test_size = 1; test_size <= PIPE_BUF; test_size *= 2) {
		char *data_to_send = malloc(test_size);
		char *received_data = malloc(test_size);
		if (data_to_send == NULL || received_data == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		// Some varying data to help later validation.
		for (size_t i = 0; i < test_size; i++) {
			data_to_send[i] = 'A' + (i % 26);
		}

		p = fork();
		if (p == -1) {
			perror("fork");
			free(data_to_send);
			free(received_data);
			exit(EXIT_FAILURE);
		}
		// Child
		if (p == 0) {
			(void)write(pipefd[1], data_to_send, test_size);

			free(data_to_send);

			exit(EXIT_SUCCESS);

			// Parent
		} else {
			ssize_t bytes_read = read(pipefd[0], received_data, test_size);
			if (bytes_read < 0) {
				perror("read");
				free(data_to_send);
				free(received_data);
				exit(EXIT_FAILURE);
			}

			// Wait for child process to finish
			wait(NULL);

			// Validate the received data
			validate_data(data_to_send, received_data, test_size);
			printf("Test passed for size: %zu bytes\n", test_size);

			free(data_to_send);
			free(received_data);
		}
	}
	// Write end
	close(pipefd[1]);
	// Read end
	close(pipefd[0]);
}

int
main(void)
{
	run_tests();
	return 0;
}
