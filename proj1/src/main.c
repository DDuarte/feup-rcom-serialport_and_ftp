#include <stdio.h>
#include <stdlib.h>

#include "physical.h"

int main()
{
	phy_connection conn = phy_open("/dev/ttyS0");
	char readValues[255];

	if (conn.fd == -1)
	{
		perror("Error opening connection");
		exit(-1);
	}

	printf("fd: %d\n", conn.fd);

	ssize_t sizeRead = phy_read(&conn, readValues, 255);

	if (sizeRead < 0)
		perror("Error reading");
	else
		printf("Value: %s\nSize: %d\n", readValues, sizeRead);

	if (!phy_close(&conn))
	{
		perror("Error closing connection");
		exit(-1);
	}

    return 0;
}
