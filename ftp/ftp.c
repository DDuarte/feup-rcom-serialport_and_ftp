#include "ftp.h"
#include "url.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

static int connect_socket(const char* ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if ((sockfd = socket(server_addr.sin_family, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return -2;
    }

    return sockfd;
}

int ftp_connect(FTP* ftp, const char* ip, int port) {
    int sockfd = connect_socket(ip, port);
    if (sockfd < 0) {
        perror("connect_socket");
        return sockfd;
    }

    ftp->control_socket_fd = sockfd;
    ftp->data_socket_fd = 0;

    usleep(500000); // sleep half a second

    char str[1024] = "";
    int error = ftp_read(ftp, str, sizeof(str));
    if (error)
    {
        perror("ftp_read");
        return error;
    }

    return 0;
}

int ftp_login(FTP* ftp, const char* user, const char* pass) {
    char buffer[1024] = "";
    sprintf(buffer, "USER %s\r\n", user);
    int error = ftp_send_cmd(ftp, buffer);
    if (error)
    {
        perror("ftp_send_cmd (USER)");
        return error;
    }

    sprintf(buffer, "PASS %s\r\n", pass);
    error = ftp_send_cmd(ftp, buffer);
    if (error)
    {
        perror("ftp_send_cmd (PASS)");
        return error;
    }

    return 0;
}

int ftp_cwd(FTP* ftp, const char* path) {
    char buffer[1024] = "";
    sprintf(buffer, "CWD %s\r\n", path);
    int error = ftp_send_cmd(ftp, buffer);
    if (error)
    {
        perror("ftp_send_cmd (CWD)");
        return error;
    }

    return 0;
}

int ftp_pasv(FTP* ftp) {
    char buffer[1024] = "PASV\r\n";
    int error = ftp_send_cmd_no_read(ftp, buffer);
    if (error)
    {
        perror("ftp_send_cmd (CWD)");
        return error;
    }

    error = ftp_read(ftp, buffer, sizeof(buffer));
    if (error) {
        perror("ftp_read");
        return error;
    }

    int ip1, ip2, ip3, ip4, port1, port2;
    error = sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
    if (error < 0) {
        perror("sscanf");
        return error;
    }

    char ip[16] = "";
    error = sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    if (error < 0) {
        perror("sprintf");
        return error;
    }

    int port = port1 * 256 + port2;

    int sockfd = connect_socket(ip, port);
    if (sockfd < 0) {
        perror("connect_socket");
        return 0;
    }

    ftp->data_socket_fd = sockfd;

    usleep(500000); // sleep half a second

    return 0;
}

int ftp_download(FTP* ftp, const char* file_name) {

    FILE* file = fopen(file_name, "w");
    if (!file) {
        perror("fopen");
        return -1;
    }

    char receive_buffer[256];
    int len;
    while ((len = read(ftp->data_socket_fd, receive_buffer, sizeof(receive_buffer)))) {
        if (len < 0) {
            perror("read");
            return len;
        }

        int error = fwrite(receive_buffer, len, 1, file);
        if (error < 0) {
            perror("fwrite");
            return error;
        }
    }

    close(ftp->data_socket_fd);
    ftp->data_socket_fd = 0;

    fclose(file);

    return 0;
}

int ftp_retr(FTP* ftp, const char* file) {
    char buffer[1024] = "";
    sprintf(buffer, "RETR %s\r\n", file);
    int error = ftp_send_cmd(ftp, buffer);
    if (error)
    {
        perror("ftp_send_cmd (RETR)");
        return error;
    }

    return 0;
}

int ftp_disconnect(FTP* ftp) {
    char buffer[1024] = "QUIT\r\n";
    int error = ftp_send_cmd(ftp, buffer);
    if (error)
    {
        perror("ftp_send_cmd (QUIT)");
        return error;
    }

    if (ftp->control_socket_fd) {
        close(ftp->control_socket_fd);
        ftp->control_socket_fd = 0;
    }

    if (ftp->data_socket_fd) {
        close(ftp->data_socket_fd);
        ftp->data_socket_fd = 0;
    }

    return 0;
}

int ftp_send_cmd(FTP *ftp, const char *cmd) {
    int error = ftp_send_cmd_no_read(ftp, cmd);
    if (error)
    {
        perror("ftp_send_cmd_no_read");
        return error;
    }

    usleep(100000);

    char str[1024] = "";
    error = ftp_read(ftp, str, sizeof(str));
    if (error)
    {
        perror("ftp_read");
        return error;
    }

    return 0;
}

int ftp_send_cmd_no_read(FTP* ftp, const char* cmd) {
    ssize_t bytes_written = write(ftp->control_socket_fd, cmd, strlen(cmd));
    if (!bytes_written)
    {
        perror("write");
        return -1;
    }

    printf("%.*s\n", (int)bytes_written, cmd);

    return 0;
}

int ftp_read(FTP* ftp, char* cmd, size_t size) {
    ssize_t bytes_read = read(ftp->control_socket_fd, cmd, size);
    if (!bytes_read)
    {
        perror("read");
        return -1;
    }

    printf("%.*s\n", (int)bytes_read, cmd);
    return 0;
}
