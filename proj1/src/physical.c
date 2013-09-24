#include "physical.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define BAUDRATE B38400

phy_connection phy_open(const char* term)
{
	phy_connection conn;
	int fd;
	struct termios oldtio;
	struct termios newtio;

	conn.fd = -1;

	if ((fd = open(term, O_RDWR | O_NOCTTY )) < 0)
		return conn;

	if (tcgetattr(fd,&oldtio) != 0)
		return conn;

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN]  = 1;

    if (tcflush(fd, TCIFLUSH) != 0)
    	return conn;

	if (tcsetattr(fd,TCSANOW,&newtio) != 0)
		return conn;

	conn.fd = fd;
	conn.term = oldtio;

	return conn;
}

ssize_t phy_write(const phy_connection* conn, const char* message, size_t size)
{
	assert(conn && message);

	return write(conn->fd, message, size);
}

ssize_t phy_read(const phy_connection* conn, char** message, size_t max_size)
{
	assert(conn && message);

	return read(conn->fd, *message, max_size);
}

bool phy_close(phy_connection* conn)
{
	if (!conn || conn->fd == -1) return false;

	if (tcsetattr(conn->fd, TCSANOW, &conn->term) != 0)
		return false;

	conn->fd = -1;

	return true;
}
