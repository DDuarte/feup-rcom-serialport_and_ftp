#ifndef PHYSICAL_H_
#define PHYSICAL_H_

#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

/*! \file physical.h
	\brief Physical layer interface.

	Lower level utility functions that allow the configuration of the serial port
	connection, as the transmission of generic information.
*/

/*! \struct phy_connection
 *	\brief  Physical connection structure.
 *
 *	Encapsulates information about the connection established between the sender
 *	and the receiver.
 */
typedef struct
{
    int fd;					/*!< serial connection file descriptor */
    struct termios term;	/*!< definitions used by the terminal I/O interface */
} phy_connection;

/*! \brief Opens the connection identified by term.
 *
 * \param term connection identifier.
 * \return phy_connection with a positive fd on success, negative otherwise.
 */
phy_connection phy_open(const char* term);

/*! \brief Writes a message with a size given as a parameter to the connection buffer.
 *
 * \param conn connection properties.
 * \param message information to be sent.
 * \param size message size in bytes.
 * \return nonnegative integer on sucess, negative otherwise.
 */
ssize_t phy_write(const phy_connection* conn, const char* message, size_t size);

/*! \brief Reads a message with a size given as parameter from the connection buffer.
 *
 *	\param conn connection properties.
 *	\param message buffer where the read information will be stored.
 *	\param max_size maximum message size.
 */
ssize_t phy_read(const phy_connection* conn, char* message, size_t max_size);

/*! \brief Closes the connection identified by a term.
 *
 *	\param conn connection properties.
 *	\return true on sucess, false otherwise.
 */
bool phy_close(phy_connection* conn);

#endif /* PHYSICAL_H_ */
