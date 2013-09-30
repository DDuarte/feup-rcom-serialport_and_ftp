#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "physical.h"
#include "link.h"

int main(int argc, char* argv[])
{
    ssize_t i;

    if (argc != 2)
        return -1;

    printf("%s\n", argv[1]);

    if (strcmp(argv[1], "read") == 0)
    {
        char* message;
        link_layer conn = ll_open("/dev/ttyS0", RECEIVER);

        if (conn.connection.fd == -1)
        {
            perror("Error opening connection");
            exit(-1);
        }

        printf("fd: %d\n", conn.connection.fd );

        ssize_t sizeRead = ll_read(&conn, &message);

        if (sizeRead < 0)
            perror("Error reading");
        else
        {
            printf("Size: %d\n", (int)sizeRead);

            for (i = 0; i < sizeRead; ++i)
                printf("0x%X\n", message[i]);

        }

        if (!ll_close(&conn))
        {
            perror("Error closing connection");
            exit(-1);
        }

        return 0;
    }
    else if (strcmp(argv[1], "write") == 0)
    {
        link_layer conn = ll_open("/dev/ttyS4", TRANSMITTER);

        // phy_connection conn = phy_open("/dev/ttyS4");

        if (conn.connection.fd == -1)
        {
            perror("Error opening connection");
            exit(-1);
        }

        printf("fd: %d\n", conn.connection.fd );

        // ssize_t sizeRead = phy_read(&conn, readValues, 255);
        // ssize_t sizeWritten = phy_write(&conn, "Hello World!", strlen("Hello World!") + 1);

        ssize_t sizeWritten = ll_send_command(&conn, CNTRL_SET, 0);

        if (sizeWritten < 0)
            perror("Error writing");

        sleep(1);

        if (!ll_close(&conn))
        {
            perror("Error closing connection");
            exit(-1);
        }

        return 0;
    }
    else return -1;

}

