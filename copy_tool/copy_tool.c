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

#define MEGA (1024*1024)

#define ERROR(...) \
	do { \
		perror(__VA_ARGS__); \
		goto end; \
	} while (FALSE)

//
// Declarations
//

bool do_copy(int source_fd, int dest_fd, size_t length);
bool copy_chunk(int source_fd, int dest_fd, size_t offset, size_t size);
void copy_memory(void* dest, void* src, size_t size);

//
// Implementation
//

int main(int argc, char** argv)
{
	int exit_status = EXIT_FAILURE;
	int source_fd = -1;
	int dest_fd = -1;

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

	source_fd = open(source_path, O_RDONLY);
	if (source_fd == -1) {
		ERROR("Error, open source file failed");
	}

	// NOTE: mmaping the file with PROT_WRITE requires it to be opened as read-write, not write-only.
	dest_fd = open(dest_path, O_RDWR | O_CREAT, 0777);
	if (dest_fd == -1) {
		ERROR("Error, open destination file failed");
	}

	if (ftruncate(dest_fd, stat_buf.st_size) == -1) {
		ERROR("Error, truncating destination file failed");
	}

	if (!do_copy(source_fd, dest_fd, stat_buf.st_size)) {
		goto end;
	}

	exit_status = EXIT_SUCCESS;

end:
	if (dest_fd != -1) {
		close(dest_fd);
	}
	if (source_fd != -1) {
		close(source_fd);
	}

	return exit_status;
}

bool do_copy(int source_fd, int dest_fd, size_t length)
{
	bool success = FALSE;

	long page_size = sysconf(_SC_PAGE_SIZE);
	if (page_size == -1) {
		ERROR("Error querying for page size");
	}
	size_t max_chunk_size = page_size > MEGA ? page_size : MEGA;

	size_t offset = 0;
	while (length > 0)
	{
		size_t chunk_size = max_chunk_size < length ? max_chunk_size : length;
		if (!copy_chunk(source_fd, dest_fd, offset, chunk_size)) {
			goto end;
		}
		offset += chunk_size;
		length -= chunk_size;
	}

	success = TRUE;

end:
	return success;
}

bool copy_chunk(int source_fd, int dest_fd, size_t offset, size_t size)
{
	bool success = FALSE;
	void* source = MAP_FAILED;
	void* dest = MAP_FAILED;

	source = mmap(NULL, size, PROT_READ, MAP_SHARED, source_fd, offset);
	if (source == MAP_FAILED) {
		ERROR("Error, mmap source failed");
	}

	dest = mmap(NULL, size, PROT_WRITE, MAP_SHARED, dest_fd, offset);
	if (source == MAP_FAILED) {
		ERROR("Error, mmap dest failed");
	}

	copy_memory(dest, source, size);

	success = TRUE;

end:
	if (dest != MAP_FAILED) {
		if(munmap(dest, size) == -1) {
			perror("Error, unmapping destination file memory failed");
		}
	}
	if (source != MAP_FAILED) {
		if(munmap(source, size) == -1) {
			perror("Error, unmapping source file memory failed");
		}
	}
	return success;
}

//Note: the TA has forbidden using memcpy.
void copy_memory(void* dest, void* src, size_t size)
{
	//Note: the pointers should already be aligned to PAGE_SIZE (and therefore long aligned)
	long* src_ = (long*)src;
	long* dest_ = (long*)dest;
	size_t size_ = size - (size % sizeof(long));
	size_t long_chunks = size_ / sizeof(long);

	for (size_t i = 0; i < long_chunks; ++i)
	{
		dest_[i] = src_[i];
	}

	// copy remainder
	char* src__ = (char*)src;
	char* dest__ = (char*)dest;
	for (size_t i  = size_; i < size; ++i)
	{
		dest__[i] = src__[i];
	}
}
