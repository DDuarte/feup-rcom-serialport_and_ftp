#ifndef URL_H
#define URL_H

/* ftp://[<user>:<password>@]<host>[:port]/<url-path> */

typedef char url_string[256];

typedef struct URL
{
    url_string user;
    url_string password;
    url_string host;
    int port;
    int num_parts;
    url_string* parts;
    url_string ip;
} URL;

void url_init(URL* url);
void url_destroy(URL* url);
int url_from_string(URL* url, const char* str);
int url_host_to_ip(URL* url);

#endif
