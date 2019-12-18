#include "iohelper.h"
#include <unistd.h>
#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)

size_t read_to_cv(int fd, cv_t* cv, size_t startidx, size_t max_read)
{
	int bytes_read = 0, total_read = startidx;
	static const unsigned int read_size = 512;
	do
	{
		cv_resize(cv, min((total_read + read_size), max_read));
		bytes_read = read(fd, &cv->data[total_read], min(max_read, read_size));

		if(bytes_read <= 0)
		{
			cv->length = total_read;
			return bytes_read;
		}
		total_read += bytes_read;

	}
	while(total_read < max_read && bytes_read > 0
		&& bytes_read == read_size);

/*
	if(total_read)
	{
		cv_resize(cv, total_read);
	}
*/
	cv->length = total_read;
	return total_read;
}

int write_from_cv_raw(int fd, cv_t* cv)
{
	int written = 0;
	int total_written = 0;
	int bufsize = cv->length;
	do
	{
		written = write(fd,
				&cv->data[total_written], cv->length - total_written);

		if(written < 0)
			return written;
		total_written += written;
	}
	while(total_written < bufsize);
	return total_written;
}

int write_full_raw(int fd, char* msg, size_t len)
{
	int written = 0;
	int total_written = 0;

	do
	{
		written = write(fd, &msg[total_written], len - total_written);

		if(written < 0)
			return written;
		total_written += written;
	}

	while(total_written < len);
	return total_written;
}
