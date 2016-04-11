#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

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
	} while (0)

//
// Globals
//

const char* file_path;
int fd = -1;
bool fifo_file_exists = FALSE;

//
// Function Declarations
//

bool register_signal_handlers();
void kill_signal_handler(int signum);
void pipe_signal_handler(int signum);
bool create_fifo();
bool open_fifo();
void delete_fifo_and_close();
void close_fifo();
bool write_file(int fd);

//
// Implementation
//

int main(int argc, char** argv)
{
	int exit_status = EXIT_FAILURE;

	if (argc != 2) {
		printf("Usage: ./writer_pipe <path>\n");
		goto end;
	}
	file_path = argv[1];

	if (!register_signal_handlers()) {
		goto end;
	}

	if (!create_fifo()) {
		goto end;
	}

	if (!open_fifo()) {
		goto end;
	}

	if (!write_file(fd)) {
		goto end;
	}

	exit_status = EXIT_SUCCESS;

end:
	delete_fifo_and_close();

	return exit_status;
}

bool register_signal_handlers()
{
	bool success = FALSE;

	struct sigaction kill_action;
	kill_action.sa_handler = kill_signal_handler;
	if (sigemptyset(&kill_action.sa_mask) == -1) {
		ERROR("Error calling sigemptyset");
	}
	kill_action.sa_flags = 0;

	struct sigaction pipe_action;
	pipe_action.sa_handler = pipe_signal_handler;
	if (sigemptyset(&pipe_action.sa_mask) == -1) {
		ERROR("Error calling sigemptyset");
	}
	if (sigaddset(&pipe_action.sa_mask, SIGINT) == -1) {
		ERROR("Error calling sigaddset");
	}
	if (sigaddset(&pipe_action.sa_mask, SIGTERM) == -1) {
		ERROR("Error calling sigaddset");
	}
	pipe_action.sa_flags = 0;

	if (sigaction(SIGINT, &kill_action, NULL) == -1) {
		ERROR("Error, sigaction failed");
	}
	if (sigaction(SIGTERM, &kill_action, NULL) == -1) {
		ERROR("Error, sigaction failed");
	}
	if (sigaction(SIGPIPE, &pipe_action, NULL) == -1) {
		ERROR("Error, sigaction failed");
	}

	success = TRUE;

end:
	return success;
}

void kill_signal_handler(int signum)
{
	delete_fifo_and_close();
	exit(EXIT_SUCCESS);
}

void pipe_signal_handler(int signum)
{
	close_fifo();
}

bool create_fifo()
{
	bool success = FALSE;

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

	fifo_file_exists = TRUE;
	success = TRUE;

end:
	return success;
}

bool open_fifo()
{
	bool success = FALSE;

	assert(fd == -1);
	fd = open(file_path, O_WRONLY);
	if (fd == -1) {
		ERROR("Error, open pipe failed");
	}

	// Make sure the file opened is fifo
	// (the original fifo might have been deleted
	// and a regular file created between stat and open)
	struct stat stat_buf;
	if (fstat(fd, &stat_buf) == -1) {
		perror("Error, stat failed");
		delete_fifo_and_close();
		goto end;
	}
	if (!S_ISFIFO(stat_buf.st_mode)) {
		perror("Error, opened file is not a pipe");
		delete_fifo_and_close();
		goto end;
	}

	success = TRUE;

end:
	return success;
}

void delete_fifo_and_close()
{
	if (fifo_file_exists) {
		if (unlink(file_path) == -1) {
			perror("Error, unlink failed");
		}
		fifo_file_exists = FALSE;
	}
	close_fifo();
}

void close_fifo()
{
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
}

bool write_file(int fd)
{
	bool success = FALSE;

	static char buffer[1024*1024];
	while (fgets(buffer, sizeof(buffer), stdin) != NULL)
	{
		int len = strlen(buffer);
		int bytes_written = write(fd, buffer, len);
		if (bytes_written == -1) {
			int reason = errno;
			perror("Error, write failed");
			if (reason == EPIPE) {
				if (!open_fifo()) {
					ERROR("Error, open pipe failed");
				}
				continue;
			} else {
				goto end;
			}
		} else if (bytes_written != len) {
			fprintf(stderr, "Error, write partially failed (%d / %d)", bytes_written, len);
			goto end;
		}
	}

	success = TRUE;

end:
	return success;
}
