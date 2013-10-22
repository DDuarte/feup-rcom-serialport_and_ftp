#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

#include "misc.h"
#include "link.h"

#define MAX_FRAME_SIZE 512
#define BAUDRATE B38400

#define LL_FLAG 0x7E
#define LL_ESC  0x7D
#define LL_CTRL 0x20
#define LL_CMD_SIZE 5 * sizeof(char)
#define LL_MSG_SIZE_PARTIAL 6 * sizeof(char)

#define GET_CTRL(msg) (msg)[2]

#define LL_IS_INFO_FRAME(ctrl) (!((ctrl) & 0x1))
#define LL_IS_COMMAND(ctrl) (!(LL_IS_INFO_FRAME(ctrl)))

#define IS_COMMAND_SET(ctrl)    ((ctrl) == CNTRL_SET)
#define IS_COMMAND_DISC(ctrl)   ((ctrl) == CNTRL_DISC)
#define IS_COMMAND_UA(ctrl)     ((ctrl) == CNTRL_UA)
#define IS_COMMAND_RR(ctrl)     (((ctrl) == CNTRL_RR)  || ((ctrl) == (0x20 | CNTRL_RR)))
#define IS_COMMAND_REJ(ctrl)    (((ctrl) == CNTRL_REJ) || ((ctrl) == (0x20 | CNTRL_REJ)))

#define BCC_ERROR -2

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


/*! Frame address field byte values */
typedef enum
{
    ADDR_R_T = 0x03, /*!< receiver to transmitter */
    ADDR_T_R = 0x01  /*!< transmitter to receiver */
} ll_address;

typedef enum
{
    FRAME_INFO,	/*!< information frame */
    FRAME_SUPER,
    FRAME_NOT_NUMERED
} ll_frame_type;

/*! Control field byte values */
typedef enum
{
    CNTRL_SET     =    0x3,
    CNTRL_DISC    =     0xB,
    CNTRL_UA     =    0x7,
    CNTRL_RR    =    0x5,
    CNTRL_REJ     =    0x1
} ll_cntrl;

/*! Connection state */
typedef enum
{
    ST_CONNECTING,
    ST_TRANSFERRING,
    ST_DISCONNECTING
} State;

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

/*! \struct link_layer
 *	\brief  Link layer connection structure.
 *
 *	Encapsulates information about the connection established between the sender
 *	and the receiver, as several connection parameters: sequence number, timeout period and the number of transmissions.
 */
typedef struct
{
    int fd;					/*!< serial connection file descriptor */
    struct termios term;	/*!< definitions used by the terminal I/O interface */
    int status;             /*!< RECEIVER/TRANSMITTER status */
    State state;            /*!< current connection state */

    unsigned int sequence_number; /*!< current sequence number of the message(s) to be sent/received */
    unsigned int timeout; /*!< protocol timeout period */
    unsigned int number_transmissions; /*!< number of message retransmissions */

    char frame[MAX_FRAME_SIZE]; /*!< frame buffer for the data packets */
} link_layer;

static link_layer _ll;

static size_t _ll_byte_stuff(char** message, size_t size);
static size_t _ll_byte_destuff(char** message, size_t size);

int phy_open(const char* term)
{ LOG
    int fd;
    struct termios oldtio;
    struct termios newtio;

    if ((fd = open(term, O_RDWR | O_NOCTTY )) < 0)
        return -1;

    if (tcgetattr(fd,&oldtio) != 0)
        return -1;

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN]  = 1;

    if (tcflush(fd, TCIFLUSH) != 0)
        return -1;

    if (tcsetattr(fd,TCSANOW,&newtio) != 0)
        return -1;

    _ll.fd = fd;
    _ll.term = oldtio;

    return fd;
}

bool phy_close(int fd)
{ LOG
    if (fd == -1) return false;
    return tcsetattr(fd, TCSANOW, &_ll.term) == 0;
}

char ll_calculate_bcc(const char* buffer, size_t size)
{ LOG
    char bcc = 0;
    size_t i;
    for (i = 0; i < size; ++i)
        bcc ^= buffer[i];

    return bcc;
}

char* compose_command(ll_address address, ll_cntrl cntrl, int n)
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

ssize_t ll_send_message(int fd, const char* message, size_t size)
{ LOG
    char* msg;
    size_t bytesWritten;

    assert(message);

    msg = compose_message(_ll.status == TRANSMITTER ? ADDR_T_R : ADDR_R_T,
            message, size, _ll.sequence_number);

    size += LL_MSG_SIZE_PARTIAL;

    size = _ll_byte_stuff(&msg, size);

    bytesWritten = write(fd, msg, size);

    if (bytesWritten != size)
        perror("Error sending message");

    free(msg);

    return bytesWritten == size;
}

bool ll_send_command(int fd, ll_cntrl command)
{ LOG
    size_t bytesWritten;
    size_t messageSize = LL_CMD_SIZE;

    char* cmd = compose_command(_ll.status == TRANSMITTER ? ADDR_T_R : ADDR_R_T,
            command, _ll.sequence_number);

    messageSize = _ll_byte_stuff(&cmd, messageSize);

    bytesWritten = write(fd, cmd, messageSize);

    if (bytesWritten != LL_CMD_SIZE)
        perror("Error sending command");

    free(cmd);

    return bytesWritten == LL_CMD_SIZE;
}

#define BUFFER_SIZE 255

message_t ll_read_message(int fd)
{ LOG
    message_t result;

    size_t size = 0;
    ssize_t readRet;
    char c;

    do
    {
        readRet = read(fd, &c, 1);
    } while (readRet == 1 && c != LL_FLAG);

    if (readRet <= 0)
    {
        result.type = ERROR;
        result.error.code = IO_ERROR;
        return result;
    }

    char* message = malloc(BUFFER_SIZE * sizeof(char));
    message[0] = LL_FLAG;
    size = 1;

    do
    {
        readRet = read(fd, &c, 1);
        message[size++] = c;

        if (readRet == 1 && c != LL_FLAG && size % BUFFER_SIZE == 0)
        {
            int mult = size / BUFFER_SIZE + 1;
            message = (char*) realloc(message, mult * BUFFER_SIZE);
        }
    } while (readRet == 1 && c != LL_FLAG);

    if (readRet == 0)
    {
        result.type = ERROR;
        result.error.code = IO_ERROR;
        free(message);
        return result;
    }

    size = _ll_byte_destuff(&message, size);

    char bcc1 = ll_calculate_bcc(&message[1], 2);

    if (bcc1 != message[3])
    {
        result.type = ERROR;
        result.error.code = BCC1_ERROR;
        free(message);
        return result;
    }

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
            result.information.message = malloc(result.information.message_size);
            memcpy(result.information.message, &message[4], result.information.message_size);
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

int ll_open(const char* term, int status)
{ LOG
    int fd = phy_open(term);

    if (fd < 0)
        return fd;

    if (status == TRANSMITTER)
    {
        message_t message;
        int times_sent = 0;
        _ll.state = ST_CONNECTING;

        while (_ll.state == ST_CONNECTING)
        {
            if (times_sent == 0 || signaled)
            {
                signaled = false;

                if (times_sent == RESEND_TRIES)
                {
                    unsubscribe_alarm();
                    ERROR("Couldn't establish connection.");
                    return fd;
                }

                ll_send_command(fd, CNTRL_SET);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                DEBUG("SET sent");
            }

            message = ll_read_message(fd);

            if (message.type == COMMAND && message.command.code == UA)
            {
                _ll.state = ST_TRANSFERRING;
                DEBUG("UA Received");
            }
        }

        unsubscribe_alarm();
    }
    else
    {
        message_t message;
        _ll.state = ST_CONNECTING;

        while (_ll.state == ST_CONNECTING)
        {
            message = ll_read_message(fd);

            if (message.type == COMMAND && message.command.code == SET)
            {
                DEBUG("SET received");
                if (!ll_send_command(fd, CNTRL_UA))
                {
                    perror("Error sending UA.");
                    return fd;
                }

                DEBUG("UA Sent");

                DEBUG("Connection established");

                _ll.state = ST_TRANSFERRING;
            }
        }
    }

    _ll.status = status;
    _ll.number_transmissions = 0;
    _ll.sequence_number = 0;
    _ll.timeout = 0;

    return fd;
}

bool ll_write(int fd, const char* message_to_send, size_t message_size)
{ LOG
    message_t message;
    int times_sent = 0;

    while (_ll.state == ST_TRANSFERRING)
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

            ll_send_message(fd, message_to_send, message_size);

            ++times_sent;

            if (times_sent == 1)
                subscribe_alarm();

            DEBUG("Message sent");
        }

        message = ll_read_message(fd);

        if (message.type == COMMAND && message.command.code == RR)
        {
            if (_ll.sequence_number != message.r)
            {
                DEBUG("RR Received");
                _ll.sequence_number = message.r;
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

ssize_t ll_read(int fd, char** message_received)
{ LOG
    message_t message;
    bool done = false;

    while (!done)
    {
        message = ll_read_message(fd);

        switch (message.type)
        {
            case ERROR:
            {
                switch (message.error.code)
                {
                    case BCC2_ERROR:
                        _ll.sequence_number = message.s;
                        ll_send_command(fd, CNTRL_REJ);
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

                        _ll.state = ST_DISCONNECTING;

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
                if (message.s == _ll.sequence_number)
                {
                    *message_received = malloc(message.information.message_size);

                    memcpy(*message_received, message.information.message, message.information.message_size);

                    DEBUG("Message Received");
                    _ll.sequence_number = !message.s;
                    ll_send_command(fd, CNTRL_RR);
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

bool ll_close(int fd)
{ LOG
    if (_ll.status == TRANSMITTER)
    {
        message_t message;
        int times_sent = 0;

        while (_ll.state == ST_TRANSFERRING)
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

                ll_send_command(fd, CNTRL_DISC);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                DEBUG("DISC sent");
            }

            message = ll_read_message(fd);

            if (message.type == COMMAND && message.command.code == DISC)
            {
                DEBUG("DISC Received");
                _ll.state = ST_DISCONNECTING;
            }
            else
            {
                DEBUG("Resending DISC");
            }
        }

        unsubscribe_alarm();

        if (!ll_send_command(fd, CNTRL_UA))
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

        while (_ll.state == ST_TRANSFERRING)
        {
            message = ll_read_message(fd);

            if (message.type == COMMAND && message.command.code == DISC)
            {
                DEBUG("DISC received");
                _ll.state = ST_DISCONNECTING;
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

                ll_send_command(fd, CNTRL_DISC);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                DEBUG("DISC sent");
            }

            message = ll_read_message(fd);

            if (message.type == COMMAND && message.command.code == UA)
            {
                DEBUG("UA received");
                DEBUG("Connection terminated.");
                ua_received = true;
            }
        }
    }

    unsubscribe_alarm();

    return phy_close(fd);
}
