#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "url.h"
#include "ftp.h"

void print_usage(char* prog);

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    URL url;
    url_init(&url);
    int error = url_from_string(&url, argv[1]);
    if (error)
    {
        fprintf(stderr, "Could not sucessfully parse FTP url (error code: %d)\n", error);
        url_destroy(&url);
        return EXIT_FAILURE;
    }

    error = url_host_to_ip(&url);
    if (error)
    {
        fprintf(stderr, "Could not get an IPv4 IP address from hostname %s (error code: %d)\n", url.host, error);
        return EXIT_FAILURE;
    }

    FTP ftp;
    error = ftp_connect(&ftp, url.ip, url.port ? url.port : 21);
    if (error)
    {
        fprintf(stderr, "Could not connect to ftp at IP %s, port %d\n", url.ip, url.port ? url.port : 21);
        return EXIT_FAILURE;
    }

    error = ftp_login(&ftp, strlen(url.user) ? url.user : "anonymous", strlen(url.password) ? url.password : "anony@mo.us");

    char path[1024] = "";
    for (int i = 0; i < url.num_parts - 1; ++i) {
        strcat(path, url.parts[i]);
        strcat(path, "/");
    }

    if (path[0] != 0) {
        error = ftp_cwd(&ftp, path);
        if (error) {
            perror("ftp_cwd");
            return error;
        }
    }

    error = ftp_pasv(&ftp);
    if (error) {
        perror("ftp_pasv");
        return error;
    }

    const char* file_name = url.num_parts ? url.parts[url.num_parts - 1] : "";

    error = ftp_retr(&ftp, file_name);
    if (error) {
        perror("ftp_retr");
        return error;
    }

    error = ftp_download(&ftp, file_name);
    if (error) {
        perror("ftp_download");
        return error;
    }

    error = ftp_disconnect(&ftp);
    if (error) {
        perror("ftp_disconnect");
        return error;
    }

    url_destroy(&url);

    return EXIT_SUCCESS;
}

void print_usage(char* prog)
{
    printf("Usage: %s ftp://[<user>:<password>@]<host>[:port]/<url-path>\n", prog);
}
