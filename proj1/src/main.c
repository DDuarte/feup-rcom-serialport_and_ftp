#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "physical.h"
#include "link.h"

int main(int argc, char* argv[])
{
    char* message_to_send = "Hello World!";

    char* message_to_receive = NULL;


    if (argc != 2)
        return -1;

    printf("%s\n", argv[1]);

    if (strcmp(argv[1], "read") == 0)
    {
        link_layer conn = ll_open("/dev/ttyS0", RECEIVER);
        size_t message_received_size = 0;
        int i = 0;

        if (conn.connection.fd == -1)
        {
            perror("Error opening connection");
            exit(-1);
        }

        printf("fd: %d\n", conn.connection.fd );

        printf("\n");
        do
        {
            printf("sleeping..."); fflush(stdout); sleep(4); printf("\t finish sleeping...\n");
            message_received_size = ll_read(&conn, &message_to_receive);
            if (message_received_size > 0)
            {
                for(i = 0; i < message_received_size; ++i)
                {
                    printf("%c", message_to_receive[i]);
                }
                printf("\n");
            }
            printf("message_received_size = %d\n\n", message_received_size);
        } while (message_received_size > 0);
        printf("\n");

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

        printf("\n");
        ll_write(&conn, "Trama de Informacao 1", strlen("Trama de Informacao 1") + 1);
        ll_write(&conn, "Trama de Informacao 2", strlen("Trama de Informacao 2") + 1);
//        ll_write(&conn, "Message 3", strlen("Message 3") + 1);
//        ll_write(&conn, "Message 4", strlen("Message 4") + 1);
//        ll_write(&conn, "Message 5", strlen("Message 5") + 1);
//        ll_write(&conn, "Message 6", strlen("Message 6") + 1);
//        ll_write(&conn, "Message 7", strlen("Message 7") + 1);
//        ll_write(&conn, "Message 8", strlen("Message 8") + 1);
//        ll_write(&conn, "Message 9", strlen("Message 9") + 1);
//        ll_write(&conn, "Message 10", strlen("Message 10") + 1);

        printf("\n");

        if (!ll_close(&conn))
        {
            perror("Error closing connection");
            exit(-1);
        }

        return 0;
    }
    else return -1;

}

