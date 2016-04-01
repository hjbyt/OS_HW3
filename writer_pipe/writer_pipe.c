#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

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
		printf("Usage: ./writer_pipe <path>\n");
		goto end;
	}

	const char* file_path = argv[1];

	struct stat stat_buf;
	if (stat(file_path, &stat_buf) == -1) {
		if (errno == ENOENT) {
			if (mkfifo(file_path, 0777) == -1) {
				ERROR("Error, mkfifo failed");
			}
		} else {
			ERROR("Error, stat failed");
		}
	} else {
		if (!S_ISFIFO(stat_buf.st_mode)) {
			if (unlink(file_path) != 0) {
				ERROR("Error, unlink failed");
			}
			if (mkfifo(file_path, 0777) == -1) {
				ERROR("Error, mkfifo failed");
			}
		}
	}

	fd = open(file_path, O_WRONLY);
	if (fd == -1) {
		ERROR("Error, open pipe failed");
	}

	//TODO: should this really be done?
	if (fstat(fd, &stat_buf) == -1) {
		ERROR("Error, stat failed");
	}
	if (!S_ISFIFO(stat_buf.st_mode)) {
		ERROR("Error, opened file is not a pipe");
	}

	//TODO: is this line size ok?
	char line[1024];
	while (fgets(line, sizeof(line), stdin) != NULL)
	{
		int len = strlen(line);
		//TODO: should we fail if written bytes != len ?
		if (write(fd, line, len) == -1) {
			ERROR("Error, write failed");
		}
	}

	exit_status = EXIT_SUCCESS;

end:
	if (fd != -1) {
		unlink(file_path);
		close(fd);
	}

	return exit_status;
}
