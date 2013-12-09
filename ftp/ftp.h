#ifndef FTP_H
#define FTP_H

#include <stdlib.h>

typedef struct FTP
{
    int control_socket_fd;
    int data_socket_fd;
} FTP;

int ftp_connect(FTP* FTP, const char* ip, int port);
int ftp_login(FTP* ftp, const char* user, const char* pass);
int ftp_cwd(FTP* ftp, const char* path);
int ftp_pasv(FTP* ftp);
int ftp_retr(FTP* ftp, const char* file);
int ftp_disconnect(FTP* ftp);

int ftp_download(FTP* ftp, const char* file_name);

int ftp_send_cmd_no_read(FTP* ftp, const char* cmd);
int ftp_send_cmd(FTP* ftp, const char* cmd);
int ftp_read(FTP* ftp, char* cmd, size_t size);

#endif
