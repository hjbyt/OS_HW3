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
	} while (FALSE)

//
// Globals
//

struct sigaction int_sig_handler;
struct sigaction term_sig_handler;

//
// Function Declarations
//

int open_fifo(const char* file_path);
bool ignore_kill_signals();
bool restore_signal_handlers();
bool read_file(int fd);

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

		if (!ignore_kill_signals()) {
			goto end;
		}

		if (!read_file(fd)) {
			goto end;
		}

		if (!restore_signal_handlers()) {
			goto end;
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
				//Note: no point checking return value of sleep.
				sleep(1);
				continue;
			} else {
				ERROR("Error, stat failed");
			}
		} else {
			if (S_ISFIFO(stat_buf.st_mode)) {
				// Try open fifo file
				fd = open(file_path, O_RDONLY);
				if (fd == -1) {
					// If it was deleted (between stat and open)
					if (errno == ENOENT) {
						// Retry
						continue;
					} else {
						ERROR("Error opening input file");
					}
				}
				break;
			} else {
				printf("Error, input file is not a pipe.\n");
				goto end;
			}
		}
	}

	// Make sure the file opened is fifo
	// (the original fifo might have been deleted
	// and a regular file created between stat and open)
	if (fstat(fd, &stat_buf) == -1) {
		perror("Error, stat failed");
		close(fd);
		fd = -1;
		goto end;
	}
	if (!S_ISFIFO(stat_buf.st_mode)) {
		perror("Error, opened file is not a pipe");
		close(fd);
		fd = -1;
		goto end;
	}

end:
	return fd;
}

bool ignore_kill_signals()
{
	bool success = FALSE;

	struct sigaction kill_action;
	kill_action.sa_handler = SIG_IGN;
	if (sigemptyset(&kill_action.sa_mask) == -1) {
		ERROR("Error calling sigemptyset");
	}
	kill_action.sa_flags = 0;

	if (sigaction(SIGINT, &kill_action, &int_sig_handler) == -1) {
		ERROR("Error, sigaction failed");
	}
	if (sigaction(SIGTERM, &kill_action, &term_sig_handler) == -1) {
		ERROR("Error, sigaction failed");
	}

	success = TRUE;

end:
	return success;
}

bool restore_signal_handlers()
{
	bool success = FALSE;

	if (sigaction(SIGINT, &int_sig_handler, NULL) == -1) {
		ERROR("Error, sigaction failed");
	}
	if (sigaction(SIGTERM, &term_sig_handler, NULL) == -1) {
		ERROR("Error, sigaction failed");
	}

	success = TRUE;

end:
	return success;
}

bool read_file(int fd)
{
	bool success = FALSE;

	static char buffer[1024*1024];
	while (TRUE)
	{
		int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
		if (bytes_read == -1) {
			ERROR("Error, read failed");
		}
		if (bytes_read == 0) {
			//EOF reached
			break;
		} else {
			assert(bytes_read > 0);
			// NOTE: it would make more sense to use fwrite/write here,
			// but the exercise explicitly asked to use printf.
			buffer[bytes_read] = '\0';
			printf("%s", buffer);
		}
	}
	//Note: normally when stdout is directed to a console, it's automatically flushed after every line.
	// but when it's directed to a file, it won't necessarily get flushed.
	// So if stdout isn't flushed and a SIGINT is received later, the data might not actually get written.
	// Therefore i call flush here to make sure the data will actually get written.
	//Note: I suppress errors here because the exercise description didn't really
	// ask to flush the writes.
	//Note: fsync(STDOUT_FILENO) could be used here instead.
	fflush(stdout);

	success = TRUE;

end:
	return success;
}
