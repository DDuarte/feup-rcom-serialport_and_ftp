#include "link.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "misc.h"

typedef enum
{
    INFORMATION, COMMAND, ERROR
} message_type_t;

typedef enum
{
    IO_ERROR, BCC1_ERROR, BCC2_ERROR
} error_code_t;

typedef enum
{
    SET, UA, RR, REJ, DISC
} command_t;

typedef struct
{
    message_type_t type;
    ll_address adress;

    union
    {
        unsigned char r;
        unsigned char s;
    };

    union
    {
        struct
        {
            char* message;
            size_t message_size;
        } information;

        struct
        {
            command_t code;
        } command;

        struct
        {
            error_code_t code;
        } error;
    };
} message_t;

static size_t _ll_byte_stuff(char** message, size_t size);
static size_t _ll_byte_destuff(char** message, size_t size);

char ll_calculate_bcc(const char* buffer, size_t size)
{ LOG
    char bcc = 0;
    size_t i;
    for (i = 0; i < size; ++i)
        bcc ^= buffer[i];

    return bcc;
}

char * compose_command(ll_address address, ll_cntrl cntrl, int n)
{ LOG
    char * command = malloc(LL_CMD_SIZE);
    command[0] = LL_FLAG;
    command[1] = address;

    switch (cntrl)
    {
        case CNTRL_RR:
        case CNTRL_REJ:
            command[2] = cntrl | (n << 5);
            break;
        default:
            command[2] = cntrl;
            break;
    }

    command[3] = ll_calculate_bcc(&command[1], 2);
    command[4] = LL_FLAG;

    return command;
}

char* compose_message(ll_address address, const char* msg, size_t size, int ns)
{ LOG
    char* message = malloc(LL_MSG_SIZE_PARTIAL + size);

    message[0] = LL_FLAG;
    message[1] = address;
    message[2] = ns << 1;
    message[3] = ll_calculate_bcc(&message[1], 2);

    memcpy(&message[4], msg, size);

    message[4 + size] = ll_calculate_bcc(msg, size);
    message[5 + size] = LL_FLAG;

    return message;
}

ssize_t ll_send_message(link_layer* conn, const char* message, size_t size)
{ LOG
    char* msg;
    size_t bytesWritten;

    assert(conn && message);

    msg = compose_message(conn->stat == TRANSMITTER ? ADDR_T_R : ADDR_R_T,
            message, size, conn->sequence_number);

    size += LL_MSG_SIZE_PARTIAL;

    size = _ll_byte_stuff(&msg, size);

    bytesWritten = phy_write(&conn->connection, msg, size);

    if (bytesWritten != size)
        perror("Error sending message");

    free(msg);

    return bytesWritten == size;
}

bool ll_send_command(link_layer* conn, ll_cntrl command)
{ LOG
    size_t bytesWritten;
    size_t messageSize = LL_CMD_SIZE;

    assert(conn);

    char* cmd = compose_command(conn->stat == TRANSMITTER ? ADDR_T_R : ADDR_R_T,
            command, conn->sequence_number);

    messageSize = _ll_byte_stuff(&cmd, messageSize);

    bytesWritten = phy_write(&conn->connection, cmd, messageSize);

    if (bytesWritten != LL_CMD_SIZE)
        perror("Error sending command");

    free(cmd);

    return bytesWritten == LL_CMD_SIZE;
}

#define BUFFER_SIZE 255

message_t ll_read_message(link_layer* conn)
{ LOG
    message_t result;

    size_t size = 0;
    ssize_t readRet;
    char c;

    assert(conn);

    DEBUG_LINE();

    do
    {
        readRet = phy_read(&conn->connection, &c, 1);
    } while (readRet == 1 && c != LL_FLAG);

    DEBUG_LINE();

    if (readRet <= 0)
    {
        result.type = ERROR;
        result.error.code = IO_ERROR;
        return result;
    }

    DEBUG_LINE();

    char* message = malloc(BUFFER_SIZE * sizeof(char));
    message[0] = LL_FLAG;
    size = 1;

    DEBUG_LINE();

    do
    {
        DEBUG_LINE();
        readRet = phy_read(&conn->connection, &c, 1);
        message[size++] = c;

        DEBUG_LINE();

        if (readRet == 1 && c != LL_FLAG && size % BUFFER_SIZE == 0)
        {
            int mult = size / BUFFER_SIZE + 1;
            message = (char*) realloc(message, mult * BUFFER_SIZE);
        }
    } while (readRet == 1 && c != LL_FLAG);

    DEBUG_LINE();

    if (readRet == 0)
    {
        result.type = ERROR;
        result.error.code = IO_ERROR;
        free(message);
        return result;
    }

    DEBUG_LINE();

    size = _ll_byte_destuff(&message, size);

    DEBUG_LINE();

    char bcc1 = ll_calculate_bcc(&message[1], 2);

    if (bcc1 != message[3])
    {
        result.type = ERROR;
        result.error.code = BCC1_ERROR;
        free(message);
        return result;
    }

    DEBUG_LINE();

    if (LL_IS_INFO_FRAME(GET_CTRL(message)))
    {
        size_t msg_size = size - LL_MSG_SIZE_PARTIAL;
        char bcc2 = ll_calculate_bcc(&message[4], msg_size);

        result.adress = message[1];
        result.s = (message[2] >> 1) & 0x1;

        if (bcc2 != message[4 + msg_size])
        {
            result.type = ERROR;
            result.error.code = BCC2_ERROR;
        }
        else
        {
            result.type = INFORMATION;
            result.information.message_size = msg_size;
            result.information.message = malloc(
                    result.information.message_size * sizeof(char));
            memcpy(result.information.message, &message[4],
                    result.information.message_size);
        }
    }
    else
    {
        result.type = COMMAND;
        result.adress = message[1];

        if (IS_COMMAND_SET(GET_CTRL(message)))
            result.command.code = SET;
        else if (IS_COMMAND_UA(GET_CTRL(message)))
            result.command.code = UA;
        else if (IS_COMMAND_DISC(GET_CTRL(message)))
            result.command.code = DISC;
        else if (IS_COMMAND_RR(GET_CTRL(message)))
        {
            result.command.code = RR;
            result.r = (GET_CTRL(message)>> 5) & 0x1;
        }
        else if (IS_COMMAND_REJ(GET_CTRL(message)))
        {
            result.command.code = REJ;
            result.r = (GET_CTRL(message) >> 5) & 0x1;
        }
    }

    DEBUG_LINE();

    free(message);
    return result;
}

static size_t _ll_byte_stuff(char** message, size_t size)
{ LOG
    size_t i;
    size_t newSize = size;

    for (i = 1; i < size - 1; ++i)
        if ((*message)[i] == LL_FLAG || (*message)[i] == LL_ESC)
            ++newSize;

    *message = (char*) realloc(*message, newSize);

    for (i = 1; i < size - 1; ++i)
    {
        if ((*message)[i] == LL_FLAG || (*message)[i] == LL_ESC)
        {
            memmove(*message + i + 1, *message + i, size - i);
            size++;
            (*message)[i] = LL_ESC;
            (*message)[i + 1] ^= LL_CTRL;
        }
    }

    return newSize;
}

static size_t _ll_byte_destuff(char** message, size_t size)
{ LOG
    size_t i;

    for (i = 1; i < size - 1; ++i)
    {
        if ((*message)[i] == LL_ESC)
        {
            memmove(*message + i, *message + i + 1, size - i - 1);
            size--;
            (*message)[i] ^= LL_CTRL;
        }
    }

    *message = (char*) realloc(*message, size);

    return size;
}

#define TIMEOUT_TIME 3
#define RESEND_TRIES 4

bool signaled = false;
time_t alarm_subscribed;

void alarm_handler(int sig)
{ LOG
    if (sig != SIGALRM)
        return;
    time_t time_passed = time(NULL) - alarm_subscribed;
    printf("Timeout!!!! -> %lld\n", (long long) time_passed);
    signaled = true;
    alarm(TIMEOUT_TIME);
}

void subscribe_alarm()
{ LOG
    struct sigaction sig;
    sig.sa_handler = alarm_handler;
    sigaction(SIGALRM, &sig, NULL);
    signaled = false;
    alarm(TIMEOUT_TIME);
    alarm_subscribed = time(NULL);
}

void unsubscribe_alarm()
{ LOG
    struct sigaction sig;
    sig.sa_handler = NULL;
    sigaction(SIGALRM, &sig, NULL);
    alarm(0);
}

link_layer ll_open(const char* term, ll_status stat)
{ LOG
    link_layer ll;

    ll.connection = phy_open(term);

    if (ll.connection.fd < 0)
        return ll;

    if (stat == TRANSMITTER)
    {
        message_t message;
        int times_sent = 0;
        ll.state = ST_CONNECTING;

        while (ll.state == ST_CONNECTING)
        {
            if (times_sent == 0 || signaled)
            {
                signaled = false;

                if (times_sent == RESEND_TRIES)
                {
                    unsubscribe_alarm();
                    ERROR("Couldn't establish connection.");
                    return ll;
                }

                ll_send_command(&ll, CNTRL_SET);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                DEBUG("SET sent");
            }

            message = ll_read_message(&ll);

            if (message.type == COMMAND && message.command.code == UA)
            {
                ll.state = ST_TRANSFERRING;
                DEBUG("UA Received");
            }
        }

        unsubscribe_alarm();
    }
    else
    {
        message_t message;
        ll.state = ST_CONNECTING;

        while (ll.state == ST_CONNECTING)
        {
            message = ll_read_message(&ll);

            if (message.type == COMMAND && message.command.code == SET)
            {
                DEBUG("SET received");
                if (!ll_send_command(&ll, CNTRL_UA))
                {
                    perror("Error sending UA.");
                    return ll;
                }

                DEBUG("UA Sent");

                DEBUG("Connection established");

                ll.state = ST_TRANSFERRING;
            }
        }
    }

    ll.stat = stat;
    ll.number_transmissions = 0;
    ll.sequence_number = 0;
    ll.timeout = 0;

    return ll;
}

bool ll_write(link_layer* conn, const char* message_to_send,
        size_t message_size)
{ LOG
    message_t message;
    int times_sent = 0;

    while (conn->state == ST_TRANSFERRING)
    {
        if (times_sent == 0 || signaled)
        {
            signaled = false;

            if (times_sent == RESEND_TRIES)
            {
                unsubscribe_alarm();
                ERROR("Couldn't send message.");
                return false;
            }

            ll_send_message(conn, message_to_send, message_size);

            ++times_sent;

            if (times_sent == 1)
                subscribe_alarm();

            DEBUG("Message sent");
            DEBUG_LINE();
        }

        DEBUG_LINE();
        message = ll_read_message(conn);
        DEBUG_LINE();

        if (message.type == COMMAND && message.command.code == RR)
        {
            if (conn->sequence_number != message.r)
            {
                DEBUG("RR Received");
                conn->sequence_number = message.r;
                printf("message r = %d\n", message.r);
            }
            unsubscribe_alarm();
            break;
        }
        else if (message.type == COMMAND && message.command.code == REJ)
        {
            DEBUG("REJ Received");
            unsubscribe_alarm();
            times_sent = 0;
        }
    }

    unsubscribe_alarm();
    DEBUG("Exiting ll_write.");
    return true;
}

ssize_t ll_read(link_layer* conn, char** message_received)
{ LOG
    message_t message;
    bool done = false;

    while (!done)
    {
        message = ll_read_message(conn);

        switch (message.type)
        {
            case ERROR:
            {
                switch (message.error.code)
                {
                    case BCC2_ERROR:
                        conn->sequence_number = message.s;
                        ll_send_command(conn, CNTRL_REJ);
                        break;
                    case IO_ERROR:
                        perror("Error reading message");
                        return -1;
                    case BCC1_ERROR:
                        ERROR("Received BCC1_ERROR command in ll_read.");
                        break;
                }

                break;
            }
            case COMMAND:
            {
                switch (message.command.code)
                {
                    case SET:
                    {
                        ERROR("Received SET command: state is not connecting.");
                        break;
                    }
                    case UA:
                    {
                        ERROR("Received UA command: state is not disconnecting.");
                        break;
                    }
                    case DISC:
                    {
                        DEBUG("DISC received");

                        conn->state = ST_DISCONNECTING;

                        done = true;
                        break;
                    }
                    case RR:
                        ERROR("Received RR command in ll_read.");
                        break;
                    case REJ:
                        ERROR("Received REJ command in ll_read.");
                        break;
                }
                break;
            }
            case INFORMATION:
            {
                if (message.s == conn->sequence_number)
                {
                    *message_received = malloc(message.information.message_size);

                    memcpy(*message_received, message.information.message, message.information.message_size);

                    DEBUG("Message Received");
                    printf("Message s = %d, size = %d\n", message.s, message.information.message_size);
                    conn->sequence_number = !message.s;
                    ll_send_command(conn, CNTRL_RR);
                    DEBUG("RR Sent");
                    done = true;
                }
                break;
            }
        }
    }

    unsubscribe_alarm();

    return message.information.message_size;
}

bool ll_close(link_layer* conn)
{ LOG
    assert(conn);

    if (conn->stat == TRANSMITTER)
    {
        message_t message;
        int times_sent = 0;

        while (conn->state == ST_TRANSFERRING)
        {
            if (times_sent == 0 || signaled)
            {
                signaled = false;

                if (times_sent == RESEND_TRIES)
                {
                    unsubscribe_alarm();
                    ERROR("Couldn't send disconnect.");
                    return false;
                }

                ll_send_command(conn, CNTRL_DISC);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                DEBUG("DISC sent");
            }

            DEBUG_LINE();
            message = ll_read_message(conn);
            DEBUG_LINE();

            if (message.type == COMMAND && message.command.code == DISC)
            {
                DEBUG("DISC Received");
                conn->state = ST_DISCONNECTING;
            }
            else
            {
                DEBUG("Resending DISC");
                printf("message.type = %d\n", message.type);
                printf("error code = %d\n", message.error.code);
            }
        }

        unsubscribe_alarm();

        if (!ll_send_command(conn, CNTRL_UA))
        {
            perror("Error sending UA.");
            return false;
        }
        DEBUG("UA Sent");
    }
    else
    {
        message_t message;
        bool ua_received = false;
        int times_sent = 0;

        while (conn->state == ST_TRANSFERRING)
        {
            message = ll_read_message(conn);

            if (message.type == COMMAND && message.command.code == DISC)
            {
                DEBUG("DISC received");

                conn->state = ST_DISCONNECTING;
            }
        }

        while (!ua_received)
        {
            if (times_sent == 0 || signaled)
            {
                signaled = false;

                if (times_sent == RESEND_TRIES)
                {
                    unsubscribe_alarm();
                    ERROR("Couldn't send disconnect.");
                    return false;
                }

                ll_send_command(conn, CNTRL_DISC);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                DEBUG("DISC sent");
            }

            message = ll_read_message(conn);

            if (message.type == COMMAND && message.command.code == UA)
            {
                DEBUG("UA received");
                DEBUG("Connection terminated.");
                ua_received = true;
            }
        }
    }

    unsubscribe_alarm();

    return phy_close(&conn->connection);
}
