#ifndef LINK_H_
#define LINK_H_

#include <stdbool.h>

#include "physical.h"

#define MAX_FRAME_SIZE 1337

#define LL_FLAG 0x7E
#define LL_ESC  0x7D
#define LL_CTRL 0x20
#define LL_CMD_SIZE 5 * sizeof(char)
#define LL_MSG_SIZE_PARTIAL 6 * sizeof(char)

#define LL_IS_INFO_FRAME(ctrl) (!((ctrl) & 0x1))

typedef enum
{
    ADDR_R_T = 0x03, // receiver to transmitter
    ADDR_T_R = 0x01  // transmitter to receiver
} ll_address;

typedef enum
{
    FRAME_INFO,
    FRAME_SUPER,
    FRAME_NOT_NUMERED
} ll_frame_type;


typedef enum
{
    TRANSMITTER,
    RECEIVER
} ll_status;

typedef enum
{
    CNTRL_SET     =    0x3,
    CNTRL_DISC    =     0xB,
    CNTRL_UA     =    0x7,
    CNTRL_RR    =    0x5,
    CNTRL_REJ     =    0x1
} ll_cntrl;

typedef struct
{
    phy_connection connection;
    ll_status stat;

    unsigned int sequence_number;
    unsigned int timeout;
    unsigned int number_transmissions;

    char frame[MAX_FRAME_SIZE];
} link_layer;

link_layer ll_open(const char* term, ll_status stat);
ssize_t ll_write(link_layer* conn, const char* message, size_t size);
char* compose_command(ll_address address, ll_cntrl cntrl, int nr);
char* compose_message(ll_address address, const char* msg, size_t size, int ns);
char ll_calculate_bcc(const char* buffer, size_t size);
ssize_t ll_read(link_layer* conn, char** message);
bool ll_close(link_layer* conn);

#endif /* LINK_H_ */
