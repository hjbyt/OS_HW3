#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

//#define ERROR(...) error(EXIT_FAILURE, errno, __VA_ARGS__)
#define ERROR(...) \
	do { \
		perror(__VA_ARGS__); \
		goto end; \
	} while (0)

int main(int argc, char** argv)
{
	int exit_status = EXIT_FAILURE;


	exit_status = EXIT_SUCCESS;

//end:

	return exit_status;
}
