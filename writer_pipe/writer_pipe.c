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
	int fd = -1;

	if (argc != 2) {
		printf("Usage: ./writer_pipe <pipe path>");
		goto end;
	}

	const char* pipe_path = argv[1];

	struct stat stat_buf;
	if (stat(pipe_path, &stat_buf) == -1) {
		if (errno == ENOENT) {
			if (mkfifo(pipe_path, 0777) == -1) {
				ERROR("Error, mkfifo failed");
			}
		} else {
			ERROR("Error, stat on pipe file failed");
		}
	} else {
		if (!S_ISFIFO(stat_buf.st_mode)) {
			if (unlink(pipe_path) != 0) {
				ERROR("Error, unlink failed");
			}
			if (mkfifo(pipe_path, 0777) == -1) {
				ERROR("Error, mkfifo failed");
			}
		}
	}

	fd = open(pipe_path, O_WRONLY);
	if (fd == -1) {
		ERROR("Error, open pipe failed");
	}

	//TODO: should this really be done?
	if (fstat(fd, &stat_buf) == -1) {
		ERROR("Error, stat on pipe file failed");
	}
	if (!S_ISFIFO(stat_buf.st_mode)) {
		ERROR("Error, opened file is not a pipe");
	}

	//TODO: is this line size ok?
	char line[1024];
	while (fgets(line, sizeof(line), stdin) != NULL)
	{
		int len = strlen(line);
		if (write(fd, line, len) != len) {
			ERROR("Error, write failed");
		}
	}

	exit_status = EXIT_SUCCESS;

end:
	if (fd != -1) {
		unlink(pipe_path);
		close(fd);
	}

	return exit_status;
}
