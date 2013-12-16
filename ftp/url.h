#ifndef URL_H
#define URL_H

/**
* @file url.h
* @brief Representation of an URL as described in RFC1738
*
* ftp://[<user>:<password>@]<host>[:port]/<url-path>
*/

typedef char url_string[256];

/// Structure to store the multiple parts of an URL
typedef struct URL
{
    url_string user; ///< Username, empty if not specified (requires call to url_from_string)
    url_string password; ///< Password, empty if not specified (requires call to url_from_string)
    url_string host; ///< Hostname (requires call to url_from_string)
    int port; ///< Port, 0 if not specified (requires call to url_from_string)
    int num_parts; ///< Number of parts in the path of the URL (requires call to url_from_string)
    url_string* parts; ///< Array with num_parts of parts in the path of the URL (requires call to url_from_string)
    url_string ip; ///< IP Address (requires call to url_host_to_ip after url_from_string)
} URL;

/// Initialization of an empty URL
void url_init(URL* url);

/// Deallocate resources of an URL
void url_destroy(URL* url);

/// Parse a string with the url to fill in the URL structure
int url_from_string(URL* url, const char* str);

/// Converts the hostname (url->host) to its IPv4 address (url->ip)
int url_host_to_ip(URL* url);

#endif
