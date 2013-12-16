#ifndef FTP_H
#define FTP_H

#include <stdlib.h>

/**
* @file ftp.h
* @brief Partial implementation of a FTP connection as described in RFC959
*
* Note: Only the protocol required to download a file from a FTP server was implemented.
*/

/// FTP connection
typedef struct FTP
{
    int control_socket_fd; ///< File descriptor of the control socket
    int data_socket_fd; ///< File descriptor of the data socket
} FTP;

/// Creates a connection to a FTP server (does not send any command, only reads what is sent back from the server initially)
int ftp_connect(FTP* FTP, const char* ip, int port);

/// Sends USER and PASS commands to the FTP server
int ftp_login(FTP* ftp, const char* user, const char* pass);

/// Sends CWD (current working directory) command to the FTP server
int ftp_cwd(FTP* ftp, const char* path);

/// Sends PASV (passive mode) to the FTP server and creates a new data connection
int ftp_pasv(FTP* ftp);

/// Sends RETR to the FTP server
int ftp_retr(FTP* ftp, const char* file);

/// Sends QUIT to the FTP server and closes socket connections
int ftp_disconnect(FTP* ftp);

/// Retrieves a file from the FTP server (requires passive mode) and stores file in the local current working directory
int ftp_download(FTP* ftp, const char* file_name);

/// Auxiliary function to send a command but without reading the response from the FTP server
int ftp_send_cmd_no_read(FTP* ftp, const char* cmd);

/// Auxiliary function to send a command to the FTP server
int ftp_send_cmd(FTP* ftp, const char* cmd);

/// Auxiliary function to read from the FTP server
int ftp_read(FTP* ftp, char* cmd, size_t size);

#endif
