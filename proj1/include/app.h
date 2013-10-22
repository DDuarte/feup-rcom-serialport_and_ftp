#ifndef APP_H_
#define APP_H_

/*! \file app.h
    \brief Application layer interface.

     Definition of methods used to send a receive files.
*/

/*! \brief Sends a file with filename `file_name` through the terminal `term`
 *
 * \param term serial port terminal.
 * \param file_name file name of the file to be sent, must exist in current working directory.
 * \return 0 on success, negative integer otherwise
 */
int app_send_file(const char* term, const char* file_name);

/*! \brief Receives a file from the terminal `term`
 *
 *  File content is written to standard output.
 *
 * \param term serial port terminal.
 * \return 0 on success, negative integer otherwise
 */
int app_receive_file(const char* term);

#endif // APP_H_
