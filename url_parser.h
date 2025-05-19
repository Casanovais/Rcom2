#ifndef URL_PARSER_H
#define URL_PARSER_H

// These are the defines the compiler was looking for
#define MAX_SCHEME_LEN 10
#define MAX_USER_LEN 100
#define MAX_PASS_LEN 100
#define MAX_HOST_LEN 256
#define MAX_PATH_LEN 1024

// Structure to hold parsed URL components
typedef struct {
    char scheme[MAX_SCHEME_LEN]; // "ftp"
    char user[MAX_USER_LEN];
    char pass[MAX_PASS_LEN];
    char host[MAX_HOST_LEN];
    int port;
    char path[MAX_PATH_LEN];
} ParsedUrl;

/**
 * Parses an FTP URL string into its components.
 * @param url_string The FTP URL to parse.
 * @param parsed_url Pointer to a ParsedUrl struct to store the results.
 * @return 0 on success, -1 on failure.
 */
int parse_ftp_url(const char* url_string, ParsedUrl* parsed_url);

#endif // URL_PARSER_H