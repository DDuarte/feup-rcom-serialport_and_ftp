#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "physical.h"
#include "link.h"

#define TIMEOUT_TIME 3

typedef enum
{
    ST_CONNECTING,
    ST_TRANSFERRING,
    ST_DISCONNECTING

} State;

void alarm_handler(int sig)
{
    if (sig != SIGALRM) return;
    printf("Timeout!!!!\n");
    alarm(TIMEOUT_TIME);
}

void subcribe_alarm()
{
    struct sigaction sig;
    sig.sa_handler = alarm_handler;
    sigaction(SIGALRM, &sig, NULL);
    alarm(TIMEOUT_TIME);
}

void unsubcribe_alarm()
{
    struct sigaction sig;
    sig.sa_handler = NULL;
    sigaction(SIGALRM, &sig, NULL);
    alarm(0);
}

bool receiver_read_cycle(link_layer* conn)
{
    char* message = NULL;
    ssize_t sizeRead;
    bool done = false;
    State state = ST_CONNECTING;

    while (!done)
    {
        sizeRead = ll_read(conn, &message);

        if (sizeRead < 0 && state == ST_DISCONNECTING)
        {
            if (!ll_send_command(conn, CNTRL_DISC))
            {
                perror("Error sending DISC.");
                return false;
            }
            printf("DISC resent!\n");
            continue;
        }
        else if (sizeRead < 0)
        {
            perror("Error reading message");
            return false;
        }

        if (LL_IS_COMMAND(GET_CTRL(message)) && IS_COMMAND_SET(GET_CTRL(message)))
        {
            if (state == ST_CONNECTING)
            {
                printf("SET received\n");
                if (!ll_send_command(conn, CNTRL_UA))
                {
                    perror("Error sending UA.");
                    return false;
                }
                printf("Connection established.\n");
                state = ST_TRANSFERRING;
            }
            else
            {
                fprintf(stderr, "Received SET command: state is not connecting.\n");
            }
        }
        else if (LL_IS_COMMAND(GET_CTRL(message)) && IS_COMMAND_UA(GET_CTRL(message)))
        {
            if (state == ST_DISCONNECTING)
            {
                printf("UA Received\n");
                printf("Connection terminated.\n");
                unsubcribe_alarm();
                done = true;
            }
            else
            {
                fprintf(stderr, "Received UA command: state is not disconnecting.\n");
            }
        }
        else if (LL_IS_COMMAND(GET_CTRL(message)) && IS_COMMAND_DISC(GET_CTRL(message)))
        {
            if (state == ST_TRANSFERRING)
            {
                printf("DISC received\n");
                if (!ll_send_command(conn, CNTRL_DISC))
                {
                    perror("Error sending DISC.");
                    return false;
                }

                subcribe_alarm();
                state = ST_DISCONNECTING;

            }
            else
            {
                fprintf(stderr, "Received DISC command: state is not transferring.\n");
            }
        }
        else if (state == ST_TRANSFERRING)
        {

        }
        else
        {
            fprintf(stderr, "Received message: state is not transferring.\n");
        }
    }

    return true;
}

int main(int argc, char* argv[])
{
    ssize_t i;
    if (argc != 2)
        return -1;

    printf("%s\n", argv[1]);

    if (strcmp(argv[1], "read") == 0)
    {
        link_layer conn = ll_open("/dev/ttyS0", RECEIVER);

        if (conn.connection.fd == -1)
        {
            perror("Error opening connection");
            exit(-1);
        }

        printf("fd: %d\n", conn.connection.fd );

        receiver_read_cycle(&conn);

        if (!ll_close(&conn))
        {
            perror("Error closing connection");
            exit(-1);
        }

        return 0;
    }
    else if (strcmp(argv[1], "write") == 0)
    {
        char* message;
        ssize_t sizeRead;

        link_layer conn = ll_open("/dev/ttyS4", TRANSMITTER);

        // phy_connection conn = phy_open("/dev/ttyS4");

        if (conn.connection.fd == -1)
        {
            perror("Error opening connection");
            exit(-1);
        }

        printf("fd: %d\n", conn.connection.fd );

        ssize_t sizeWritten = ll_send_command(&conn, CNTRL_SET); // Send Set Command

        sizeRead = ll_read(&conn, &message);

        for (i = 0; i < sizeRead; ++i)
            printf("0x%X, ", message[i]);
        printf("\n");

        sleep(1);

        ll_send_command(&conn, CNTRL_DISC); // Send Set Command

        sizeRead = ll_read(&conn, &message);

        for (i = 0; i < sizeRead; ++i)
            printf("0x%X, ", message[i]);
        printf("\n");

        sleep(10);

        ll_send_command(&conn, CNTRL_UA);

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

