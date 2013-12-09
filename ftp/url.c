#include "url.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string.h>
#include <pcre.h>
#include <stdio.h>
#include <string.h>

#define OVECTOR_SIZE 30

void url_init(URL* url)
{
    memset(url->user, 0, sizeof(url_string));
    memset(url->password, 0, sizeof(url_string));
    memset(url->host, 0, sizeof(url_string));
    url->port = 0;
    url->num_parts = 0;
    url->parts = NULL;
}

void url_destroy(URL* url)
{
    free(url->parts);
}

int url_from_string(URL* url, const char* str)
{
    const char* regex = "ftp://(([^:]+)(:([^@]+))?@)?([A-Za-z\\.]+)(:([0-9]+))?/(.*)";

    const char* error;
    int erroroffset;
    pcre* re = pcre_compile(regex, 0, &error, &erroroffset, NULL);
    if (!re)
        return -1;

    int ovector[OVECTOR_SIZE];
    int rc = pcre_exec(re, NULL, str, strlen(str), 0, 0, ovector, OVECTOR_SIZE);
    if (rc < 0)
    {
        switch (rc)
        {
            case PCRE_ERROR_NOMATCH:
                fprintf(stderr, "No match found in text\n");
                return -2;
            default:
                fprintf(stderr, "Match error %d\n", rc);
                return -3;
        }
    }

    if (ovector[5] - ovector[4] != 0)
        strncpy(url->user, str + ovector[4], ovector[5] - ovector[4]);

    if (ovector[9] - ovector[8] != 0)
        strncpy(url->password, str + ovector[8], ovector[9] - ovector[8]);

    if (ovector[11] - ovector[10] != 0)
        strncpy(url->host, str + ovector[10], ovector[11] - ovector[10]);

    if (ovector[15] - ovector[14] != 0)
    {
        url_string portstr;
        memset(portstr, 0, sizeof(url_string));
        strncpy(portstr, str + ovector[14], ovector[15] - ovector[14]);
        url->port = atoi(portstr);
    }

    if (ovector[17] - ovector[16] != 0)
    {
        url_string url_path;
        memset(url_path, 0, sizeof(url_string));
        strncpy(url_path, str + ovector[16], ovector[17] - ovector[16]);

        char* c = url_path;
        url->num_parts = 1;
        while (*c)
        {
            if (*c == '/')
                url->num_parts++;
            c++;
        }

        const char* delim = "/";
        char* save_ptr;
        char* pch = strtok_r(url_path, delim, &save_ptr);
        url->parts = malloc(url->num_parts * sizeof(url_string));
        int i = 0;
        while (pch != NULL)
        {
            memset(url->parts[i], 0, sizeof(url_string));
            strcpy(url->parts[i], pch);
            i++;

            pch = strtok_r(NULL, delim, &save_ptr);
        }
    }

    return 0;
}

int url_host_to_ip(URL* url)
{
    struct addrinfo* result;
    struct addrinfo* res;
    int error;

    error = getaddrinfo(url->host, NULL, NULL, &result);
    if (error != 0)
    {
        if (error == EAI_SYSTEM)
            perror("getaddrinfo");
        else
            fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    for (res = result; res != NULL; res = res->ai_next)
    {
        struct in_addr* addr;
        if (res->ai_family == AF_INET)
        {
            struct sockaddr_in* ipv = (struct sockaddr_in*)res->ai_addr;
            addr = &(ipv->sin_addr);
            if (inet_ntop(res->ai_family, addr, url->ip, sizeof(url_string)) != NULL)
            {
                freeaddrinfo(result);
                return 0;
            }
        }
        /* else // ipv6 support not required atm
        {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)res->ai_addr;
            addr = (struct in_addr*)&(ipv6->sin6_addr);
        } */
    }

    freeaddrinfo(result);
    return -2;
}
