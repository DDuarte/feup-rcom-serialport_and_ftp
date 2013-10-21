#ifndef LINK_H_
#define LINK_H_

#include <stdbool.h>
#include "physical.h"

/*! \file link.h
	\brief Link layer interface.

	 Suite of methods that allow the communication between two adjacent nodes (sender and receiver).
*/

#define MAX_FRAME_SIZE 1337

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
    TRANSMITTER,
    RECEIVER
} ll_status;

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


/*! \struct link_layer
 *	\brief  Link layer connection structure.
 *
 *	Encapsulates information about the connection established between the sender
 *	and the receiver, as several connection parameters: sequence number, timeout period and the number of transmissions.
 */
typedef struct
{
    phy_connection connection; /*!< connection data (file descriptor and terminal configuration) */
    ll_status stat; /*!< RECEIVER/SENDER status */
    State state; /*!< current connection state */

    unsigned int sequence_number; /*!< current sequence number of the message(s) to be sent/received */
    unsigned int timeout; /*!< protocol timeout period */
    unsigned int number_transmissions; /*!< number of message retransmissions */

    char frame[MAX_FRAME_SIZE]; /*!< frame buffer for the data packets */
} link_layer;

/*! \brief Opens the connectio identified in a mode given as a parameter (stat).
 *
 * \param term connection identifier.
 * \param stat connection status (RECEIVER/SENDER).
 * \return link_layer structure with a positive fd on success, negative otherwise.
 */
link_layer ll_open(const char* term, ll_status stat);

/*! \brief Writes a message with a size given as a parameter to the connection buffer.
 *
 * \param conn connection properties.
 * \param message information to be sent.
 * \param size message size in bytes.
 * \return positive integer on success, negative otherwise.
 */
ssize_t ll_write(link_layer* conn, const char* message, size_t size);

/*! \brief Reads a message with a size given as parameter from the connection buffer.
 *
 *	\param conn connection properties.
 *	\param message buffer where the read information will be stored.
 *	\return positive integer on success, negative otherwise.
 */
ssize_t ll_read(link_layer* conn, char** message);

/*! \brief Closes the connection identified by conn.
 *
 *	\param conn connection properties.
 *	\return true on sucess, false otherwise.
 */
bool ll_close(link_layer* conn);

#endif /* LINK_H_ */
