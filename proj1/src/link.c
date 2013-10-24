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
    FRAME_INFO,    /*!< information frame */
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
 *    \brief  Link layer connection structure.
 *
 *    Encapsulates information about the connection established between the sender
 *    and the receiver, as several connection parameters: sequence number, timeout period and the number of transmissions.
 */
typedef struct
{
    int fd;                    /*!< serial connection file descriptor */
    struct termios term;    /*!< definitions used by the terminal I/O interface */
    int status;             /*!< RECEIVER/TRANSMITTER status */
    State state;            /*!< current connection state */

    unsigned int sequence_number; /*!< current sequence number of the message(s) to be sent/received */
    unsigned int timeout; /*!< protocol timeout period */
    unsigned int number_transmissions; /*!< number of message retransmissions */
} link_layer;

static link_layer _ll;

static size_t _ll_byte_stuff(char** message, size_t size);
static size_t _ll_byte_destuff(char** message, size_t size);

typedef struct
{
    int baudrate; /// baudrate (before conversion to termios.h's defines)
    int max_info_frame_size; /// maximum size for I frames
    int retries; /// number of retries
    int timeout; /// timeout in seconds
    int rand_seed; /// seed for srand()
    int bcc1_prob_error; /// probability of bcc1 having errors (per packet) [0-100]
    int bcc2_prob_error; /// probability of bcc2 having errors (per packet) [0-100]
    int print_stats; /// to or print or not to print, that is the question
} config_t;

static config_t conf = // default values
{
    .baudrate = 38400,
    .max_info_frame_size = 512,
    .retries = 4,
    .timeout = 3,
    .rand_seed = 0,
    .bcc1_prob_error = 0,
    .bcc2_prob_error = 0,
    .print_stats = 0
};

void conf_set_baudrate(int baudrate) { LOG conf.baudrate = baudrate; }
void conf_set_max_info_frame_size(int max_info_frame_size) { LOG conf.max_info_frame_size = max_info_frame_size; }
void conf_set_retries(int retries) { LOG conf.retries = retries; }
void conf_set_timeout(int timeout) { LOG conf.timeout = timeout; }
void conf_set_rand_seed(int rand_seed) { LOG conf.rand_seed = rand_seed; }
void conf_bcc1_prob_error(int bcc1_prob_error) { LOG conf.bcc1_prob_error = bcc1_prob_error; }
void conf_bcc2_prob_error(int bcc2_prob_error) { LOG conf.bcc2_prob_error = bcc2_prob_error; }
void conf_set_print_stats(int print_stats) { LOG conf.print_stats = print_stats; }

typedef struct
{
    int num_rej;
    int num_rr;
    int num_info_frames;
    int num_timeouts;
    int num_errors_bcc1;
    int num_errors_bcc2;
    int num_ua;
    int num_set;
    int num_disc_sent;
    int num_disc_received;
} connection_stat_t;

static connection_stat_t stats_sender = // default values
{
    .num_rej = 0,
    .num_rr = 0,
    .num_info_frames = 0,
    .num_timeouts = 0,
    .num_errors_bcc1 = 0,
    .num_errors_bcc2 = 0,
    .num_ua = 0,
    .num_set = 0,
    .num_disc_sent = 0,
    .num_disc_received = 0
};

static connection_stat_t stats_receiver = // default values
{
    .num_rej = 0,
    .num_rr = 0,
    .num_info_frames = 0,
    // .num_timeouts = 0,
    .num_errors_bcc1 = 0,
    .num_errors_bcc2 = 0,
    .num_ua = 0,
    .num_set = 0,
    .num_disc_sent = 0,
    .num_disc_received = 0
};

int get_proper_baudrate(int baudrate)
{ LOG
    switch (baudrate)
    {
        case 0:     return B0;
        case 50:    return B50;
        case 75:    return B75;
        case 110:   return B110;
        case 134:   return B134;
        case 150:   return B150;
        case 200:   return B200;
        case 300:   return B300;
        case 600:   return B600;
        case 1200:  return B1200;
        case 1800:  return B1800;
        case 2400:  return B2400;
        case 4800:  return B4800;
        case 9600:  return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        default: return -1;
    }
}

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
    newtio.c_cflag = get_proper_baudrate(conf.baudrate) | CS8 | CLOCAL | CREAD;
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

    char* message = malloc(conf.max_info_frame_size);
    message[0] = LL_FLAG;
    size = 1;

    do
    {
        readRet = read(fd, &c, 1);
        message[size++] = c;

        if (readRet == 1 && c != LL_FLAG && size % conf.max_info_frame_size == 0)
        {
            int mult = size / conf.max_info_frame_size + 1;
            message = (char*) realloc(message, mult * conf.max_info_frame_size);
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

    if (_ll.status == RECEIVER && conf.bcc1_prob_error > 0)
    {
        int prob = (rand() % 100) + 1;
        if (prob <= conf.bcc1_prob_error) bcc1 *= -1;
    }

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

        if (_ll.status == RECEIVER && conf.bcc2_prob_error > 0)
        {
            int prob = (rand() % 100) + 1;
            if (prob <= conf.bcc2_prob_error) bcc2 *= -1;
        }

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

bool signaled = false;
time_t alarm_subscribed;

void alarm_handler(int sig)
{ LOG
    if (sig != SIGALRM)
        return;
    time_t time_passed = time(NULL) - alarm_subscribed;
    ERRORF("Timeout!!!! -> %lld", (long long) time_passed);
    stats_sender.num_timeouts++;
    signaled = true;
    alarm(conf.timeout);
}

void subscribe_alarm()
{ LOG
    struct sigaction sig;
    sig.sa_handler = alarm_handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sigaction(SIGALRM, &sig, NULL);
    signaled = false;
    alarm(conf.timeout);
    alarm_subscribed = time(NULL);
}

void unsubscribe_alarm()
{ LOG
    struct sigaction sig;
    sig.sa_handler = NULL;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sigaction(SIGALRM, &sig, NULL);
    alarm(0);
}

int ll_open(const char* term, int status)
{ LOG
    srand(conf.rand_seed);
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

                if (times_sent >= conf.retries)
                {
                    unsubscribe_alarm();
                    ERROR("Couldn't establish connection.");
                    return fd;
                }

                ll_send_command(fd, CNTRL_SET);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                print_message("SET sent\n");
                stats_sender.num_set++;
            }

            message = ll_read_message(fd);

            if (message.type == COMMAND && message.command.code == UA)
            {
                _ll.state = ST_TRANSFERRING;
                print_message("UA received\n");
                stats_sender.num_ua++;
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
                print_message("SET received\n");
                stats_receiver.num_set++;
                if (!ll_send_command(fd, CNTRL_UA))
                {
                    perror("Error sending UA.");
                    return fd;
                }

                print_message("UA sent\n");
                stats_receiver.num_ua++;

                print_message("Connection established.\n");

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

            if (times_sent >= conf.retries)
            {
                unsubscribe_alarm();
                ERROR("Couldn't send message.");
                return false;
            }

            ll_send_message(fd, message_to_send, message_size);

            ++times_sent;

            if (times_sent == 1)
                subscribe_alarm();

            print_message("\nmessage sent\n");
            stats_sender.num_info_frames++;
        }

        message = ll_read_message(fd);

        if (message.type == COMMAND && message.command.code == RR)
        {
            if (_ll.sequence_number != message.r)
            {
                print_message("RR received\n");
                stats_sender.num_rr++;
                _ll.sequence_number = message.r;
            }
            unsubscribe_alarm();
            break;
        }
        else if (message.type == COMMAND && message.command.code == REJ)
        {
            print_message("REJ received\n");
            stats_sender.num_rej++;
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
                        print_message("REJ sent\n");
                        stats_receiver.num_rej++;
                        stats_receiver.num_errors_bcc2++;
                        break;
                    case IO_ERROR:
                        perror("Error reading message");
                        return -1;
                    case BCC1_ERROR:
                        ERROR("Received BCC1_ERROR command in ll_read.");
                        stats_receiver.num_errors_bcc1++;
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
                        print_message("DISC received\n");
                        stats_receiver.num_disc_received++;

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
                    free(message.information.message);

                    print_message("message received\n");
                    stats_receiver.num_info_frames++;
                    _ll.sequence_number = !message.s;
                    ll_send_command(fd, CNTRL_RR);
                    stats_receiver.num_rr++;
                    print_message("RR sent\n");
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

                if (times_sent >= conf.retries)
                {
                    unsubscribe_alarm();
                    ERROR("Couldn't send disconnect.");
                    return false;
                }

                ll_send_command(fd, CNTRL_DISC);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                print_message("DISC sent\n");
                stats_sender.num_disc_sent++;
            }

            message = ll_read_message(fd);

            if (message.type == COMMAND && message.command.code == DISC)
            {
                print_message("DISC received\n");
                stats_sender.num_disc_received++;
                _ll.state = ST_DISCONNECTING;
            }
            else
            {
                print_message("(DISC re-sent)\n");
            }
        }

        unsubscribe_alarm();

        if (!ll_send_command(fd, CNTRL_UA))
        {
            perror("Error sending UA.");
            return false;
        }
        print_message("UA sent\n");
        stats_sender.num_ua++;
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
                print_message("DISC received\n");
                stats_receiver.num_disc_received++;
                _ll.state = ST_DISCONNECTING;
            }
        }

        while (!ua_received)
        {
            if (times_sent == 0 || signaled)
            {
                signaled = false;

                if (times_sent >= conf.retries)
                {
                    unsubscribe_alarm();
                    ERROR("Couldn't send disconnect.");
                    return false;
                }

                ll_send_command(fd, CNTRL_DISC);

                ++times_sent;

                if (times_sent == 1)
                    subscribe_alarm();

                print_message("DISC sent\n");
                stats_receiver.num_disc_sent++;
            }

            message = ll_read_message(fd);

            if (message.type == COMMAND && message.command.code == UA)
            {
                print_message("UA received\n");
                stats_receiver.num_ua++;
                print_message("Connection terminated.\n");
                ua_received = true;
            }
        }
    }

    unsubscribe_alarm();

    if (conf.print_stats)
    {
        if (_ll.status == TRANSMITTER)
        {
            printf("CSI (Connection Statistical Information) - sender\n");
            printf("  REJ \t\t - %d\n", stats_sender.num_rej);
            printf("  RR \t\t - %d\n",  stats_sender.num_rr);
            printf("  UA \t\t - %d\n",  stats_sender.num_ua);
            printf("  SET \t\t - %d\n", stats_sender.num_set);
            printf("  DISC (send) \t - %d\n", stats_sender.num_disc_sent);
            printf("  DISC (recv) \t - %d\n", stats_sender.num_disc_received);
            printf("  timeouts \t - %d\n",    stats_sender.num_timeouts);
            printf("  bcc1 errors \t - %d\n", stats_sender.num_errors_bcc1);
            printf("  bcc2 errors \t - %d\n", stats_sender.num_errors_bcc2);
            printf("  info frames \t - %d\n", stats_sender.num_info_frames);
        }
        else
        {
            printf("CSI (Connection Statistical Information) - receiver\n");
            printf("  REJ \t\t - %d\n", stats_receiver.num_rej);
            printf("  RR  \t\t - %d\n", stats_receiver.num_rr);
            printf("  UA  \t\t - %d\n", stats_receiver.num_ua);
            printf("  SET \t\t - %d\n", stats_receiver.num_set);
            printf("  DISC (send) \t - %d\n", stats_receiver.num_disc_sent);
            printf("  DISC (recv) \t - %d\n", stats_receiver.num_disc_received);
            printf("  bcc1 errors \t - %d\n", stats_receiver.num_errors_bcc1);
            printf("  bcc2 errors \t - %d\n", stats_receiver.num_errors_bcc2);
            printf("  info frames \t - %d\n", stats_receiver.num_info_frames);
        }
    }

    return phy_close(fd);
}
