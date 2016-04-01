#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

//
// Definitions
//

#define FALSE 0
#define TRUE 1
typedef int bool;


#define ERROR(...) \
	do { \
		perror(__VA_ARGS__); \
		goto end; \
	} while (FALSE)

//
// Implementation
//

int main(int argc, char** argv)
{
	int exit_status = EXIT_FAILURE;
	int source_fd = -1;
	int dest_fd = -1;
	char* source = MAP_FAILED;
	char* dest = MAP_FAILED;
	size_t length = 0;

	if (argc != 3) {
		printf("Usage: ./copy_tool <source_path> <dest_path>\n");
		goto end;
	}
	const char* source_path = argv[1];
	const char* dest_path = argv[2];

	struct stat stat_buf;
	if (stat(source_path, &stat_buf) == -1) {
		if (errno == ENOENT) {
			printf("Error, source file does not exist\n");
			goto end;
		} else {
			ERROR("Error, stat source failed");
		}
	}
	length = stat_buf.st_size;

	source_fd = open(source_path, O_RDONLY);
	if (source_fd == -1) {
		ERROR("Error, open source file failed");
	}

	// NOTE: mmaping the file with PROT_WRITE requires it to be opened as read-write, not write-only.
	dest_fd = open(dest_path, O_RDWR | O_CREAT, 0777);
	if (dest_fd == -1) {
		ERROR("Error, open destination file failed");
	}

	if (ftruncate(dest_fd, length) == -1) {
		ERROR("Error, truncating destination file failed");
	}

	//TODO: should we map the whole file, even if it's very large?
	source = (char*)mmap(NULL, length, PROT_READ, MAP_SHARED, source_fd, 0);
	if (source == MAP_FAILED) {
		ERROR("Error, mmap source failed");
	}

	dest = (char*)mmap(NULL, length, PROT_WRITE, MAP_SHARED, dest_fd, 0);
	if (source == MAP_FAILED) {
		ERROR("Error, mmap dest failed");
	}

	memcpy(dest, source, length);

	exit_status = EXIT_SUCCESS;

end:
	if (dest != MAP_FAILED) {
		if(munmap(dest, length) == -1) {
			perror("Error, unmapping destination file memory failed");
		}
	}
	if (source != MAP_FAILED) {
		if(munmap(source, length) == -1) {
			perror("Error, unmapping source file memory failed");
		}
	}
	if (dest_fd != -1) {
		close(dest_fd);
	}
	if (source_fd != -1) {
		close(source_fd);
	}

	return exit_status;
}
