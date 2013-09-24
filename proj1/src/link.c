#include "link.h"

#include <assert.h>
#include <stdio.h>

link_layer ll_open(const char* term, ll_status stat)
{
	link_layer ll;

	return ll;
}

ssize_t ll_write(link_layer* conn, const char* message, size_t size)
{
	assert(conn && message);

	return -1;
}

ssize_t ll_read(link_layer* conn, char** message)
{
	assert(conn);

	return -1;

}

bool ll_close(link_layer* conn)
{
	assert(conn);

	return false;
}
