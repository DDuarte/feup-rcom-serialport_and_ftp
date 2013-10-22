#ifndef LINK_H_
#define LINK_H_

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

/*! \file link.h
    \brief Link layer interface.

     Suite of methods that allow the communication between two adjacent nodes (sender and receiver).
*/

#define TRANSMITTER 0
#define RECEIVER 1

/*! \brief Opens the connectio identified in a mode given as a parameter (stat).
 *
 * \param term connection identifier.
 * \param stat connection status (TRANSMITTER/RECEIVER).
 * \return positive file descriptor, negative on error
 */
int ll_open(const char* term, int stat);

/*! \brief Writes a message with a size given as a parameter to the connection buffer.
 *
 * \param fd file descriptor.
 * \param message information to be sent.
 * \param size message size in bytes.
 * \return true on success, false otherwise.
 */
bool ll_write(int fd, const char* message, size_t size);

/*! \brief Reads a message with a size given as parameter from the connection buffer.
 *
 *	\param fd file descriptor.
 *	\param message buffer where the read information will be stored.
 *	\return positive integer on success, negative otherwise.
 */
ssize_t ll_read(int fd, char** message);

/*! \brief Closes the connection identified by conn.
 *
 *	\param fd file descriptor.
 *	\return true on sucess, false otherwise.
 */
bool ll_close(int fd);

#endif /* LINK_H_ */
