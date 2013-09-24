#ifndef PHYSICAL_H_
#define PHYSICAL_H_

#include <stdio.h>
#include <stdbool.h>
#include <termios.h>

typedef struct
{
	int fd;
	struct termios term;
} phy_connection;

phy_connection phy_open(const char* term);
ssize_t phy_write(const phy_connection* conn, const char* message, size_t size);
ssize_t phy_read(const phy_connection* conn, char* message, size_t max_size);
bool phy_close(phy_connection* conn);


#endif /* PHYSICAL_H_ */
