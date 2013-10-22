#ifndef LINK_H_
#define LINK_H_

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

/*! \file link.h
    \brief Link layer interface.

     Suite of methods that allow the communication between two adjacent nodes (sender and receiver).
*/

#define TRANSMITTER 0 /// Connection status for transmitter
#define RECEIVER 1    /// Connection status for receiver

/*! \brief Opens the connection identified in a mode given as a parameter (stat).
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

/*! \brief Reads a message a link layer message.
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

void conf_set_baudrate(int baudrate);
void conf_set_max_info_frame_size(int max_info_frame_size);
void conf_set_retries(int retries);
void conf_set_timeout(int timeout);
void conf_set_rand_seed(int rand_seed);
void conf_bcc1_prob_error(int bcc1_prob_error);
void conf_bcc2_prob_error(int bcc2_prob_error);

#endif /* LINK_H_ */
