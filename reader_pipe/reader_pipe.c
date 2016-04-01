#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

//
//
//

#define TRUE 1
#define FALSE 0
typedef int bool;

#define ERROR(...) \
	do { \
		perror(__VA_ARGS__); \
		goto end; \
	} while (FALSE)

//
// Function Declarations
//

int open_fifo(const char* file_path);

//
// Implementation
//

int main(int argc, char** argv)
{
	int exit_status = EXIT_FAILURE;
	int fd = -1;

	if (argc != 2) {
		printf("Usage: ./reader_pipe <path>\n");
		goto end;
	}

	const char* file_path = argv[1];

	while (TRUE)
	{
		fd = open_fifo(file_path);
		if (fd == -1) {
			goto end;
		}

		char buffer[1024];
		while (TRUE)
		{
			int bytes_read = read(fd, buffer, sizeof(buffer));
			if (bytes_read == -1) {
				ERROR("Error, read failed");
			}
			if (bytes_read == 0) {
				//EOF reached
				break;
			} else {
				assert(bytes_read > 0);
				int bytes_written = fwrite(buffer, 1, bytes_read, stdout);
				if (bytes_written != bytes_read) {
					fprintf(stderr, "Error, writing to stdout failed (%d / %d)\n", bytes_written, bytes_read);
					goto end;
				}
			}
		}
	}

	exit_status = EXIT_SUCCESS;

end:
	if (fd != -1) {
		close(fd);
	}

	return exit_status;
}

int open_fifo(const char* file_path)
{
	int fd = -1;

	// Poll until fifo file exists
	struct stat stat_buf;
	while (TRUE)
	{
		if (stat(file_path, &stat_buf) == -1) {
			if (errno == ENOENT) {
				sleep(1);
				continue;
			} else {
				ERROR("Error, stat failed");
			}
		} else {
			if (S_ISFIFO(stat_buf.st_mode)) {
				break;
			} else {
				printf("Error, input file is not a pipe.\n");
				goto end;
			}
		}
	}

	fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		ERROR("Error opening input file");
	}

	if (fstat(fd, &stat_buf) == -1) {
		close(fd);
		fd = -1;
		perror("Error, stat failed");
		goto end;
	}
	if (!S_ISFIFO(stat_buf.st_mode)) {
		close(fd);
		fd = -1;
		perror("Error, opened file is not a pipe");
		goto end;
	}

end:
	return fd;
}
