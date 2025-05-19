#include "url_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For atoi

int parse_ftp_url(const char* url_string, ParsedUrl* parsed_url) {
    // Initialize with defaults
    strncpy(parsed_url->user, "anonymous", MAX_USER_LEN -1);
    parsed_url->user[MAX_USER_LEN-1] = '\0';
    strncpy(parsed_url->pass, "guest@example.com", MAX_PASS_LEN -1); // Common practice
    parsed_url->pass[MAX_PASS_LEN-1] = '\0';
    parsed_url->port = 21; // Default FTP port
    parsed_url->path[0] = '\0'; // Initialize path as empty
    parsed_url->host[0] = '\0';
    strncpy(parsed_url->scheme, "ftp", MAX_SCHEME_LEN-1);
    parsed_url->scheme[MAX_SCHEME_LEN-1] = '\0';


    const char* current_pos = url_string;

    // 1. Check and skip scheme "ftp://"
    if (strncmp(current_pos, "ftp://", 6) == 0) {
        current_pos += 6;
    } else {
        fprintf(stderr, "Error: URL must start with ftp://\n");
        return -1;
    }

    // 2. Look for user:pass@
    char* at_symbol = strchr(current_pos, '@');
    const char* host_start = current_pos;

    if (at_symbol) {
        // Temp buffer for the "user:pass" part before the '@'
        char user_pass_part[MAX_USER_LEN + MAX_PASS_LEN + 2]; // +1 for ':', +1 for '\0'
        size_t user_pass_part_len = at_symbol - current_pos;

        if (user_pass_part_len >= sizeof(user_pass_part)) {
            fprintf(stderr, "Error: User/password string part too long.\n");
            return -1;
        }
        strncpy(user_pass_part, current_pos, user_pass_part_len);
        user_pass_part[user_pass_part_len] = '\0';

        host_start = at_symbol + 1; // Start of host is after '@'

        char* colon_symbol = strchr(user_pass_part, ':');
        if (colon_symbol) { // Found user:pass
            size_t user_len = colon_symbol - user_pass_part;
            if (user_len >= MAX_USER_LEN) {
                 fprintf(stderr, "Error: Username too long.\n"); return -1;
            }
            strncpy(parsed_url->user, user_pass_part, user_len);
            parsed_url->user[user_len] = '\0';

            // The rest is password
            if (strlen(colon_symbol + 1) >= MAX_PASS_LEN) {
                fprintf(stderr, "Error: Password too long.\n"); return -1;
            }
            strncpy(parsed_url->pass, colon_symbol + 1, MAX_PASS_LEN -1);
            parsed_url->pass[MAX_PASS_LEN -1] = '\0';

        } else { // Only username provided
            if (strlen(user_pass_part) >= MAX_USER_LEN) {
                 fprintf(stderr, "Error: Username too long.\n"); return -1;
            }
            strncpy(parsed_url->user, user_pass_part, MAX_USER_LEN -1);
            parsed_url->user[MAX_USER_LEN -1] = '\0';
            // Keep default password
        }
    }

    // 3. Parse host, optional port, and path
    // host_start now points to the beginning of the host part
    char* path_slash = strchr(host_start, '/');
    char host_port_part[MAX_HOST_LEN + 7]; // temp buffer for "host" or "host:port" part

    if (path_slash) { // Path is present
        size_t host_port_part_len = path_slash - host_start;
        if (host_port_part_len >= sizeof(host_port_part)) {
            fprintf(stderr, "Error: Host/port string part too long.\n"); return -1;
        }
        strncpy(host_port_part, host_start, host_port_part_len);
        host_port_part[host_port_part_len] = '\0';

        if (strlen(path_slash + 1) >= MAX_PATH_LEN) { // +1 to skip the slash itself
            fprintf(stderr, "Error: Path too long.\n"); return -1;
        }
        strncpy(parsed_url->path, path_slash + 1, MAX_PATH_LEN -1);
        parsed_url->path[MAX_PATH_LEN -1] = '\0';
        if (strlen(parsed_url->path) == 0) { // e.g. ftp://host/
             strncpy(parsed_url->path, ".", MAX_PATH_LEN -1); // Treat as current dir
             parsed_url->path[MAX_PATH_LEN -1] = '\0';
        }
    } else { // No path component after host (e.g., ftp://host or ftp://host:port)
        if (strlen(host_start) >= sizeof(host_port_part)) {
            fprintf(stderr, "Error: Host/port string part too long.\n"); return -1;
        }
        strcpy(host_port_part, host_start);
        // Default path or error? For a download, a path is usually needed.
        // Let's default to "." for listing the current directory if path is not specified.
        strncpy(parsed_url->path, ".", MAX_PATH_LEN -1);
        parsed_url->path[MAX_PATH_LEN -1] = '\0';
    }


    // Now, from host_port_part, extract host and optional port
    char* colon_in_host = strrchr(host_port_part, ':');
    if (colon_in_host) { // Port is specified
        size_t host_len = colon_in_host - host_port_part;
        if (host_len >= MAX_HOST_LEN) {
             fprintf(stderr, "Error: Hostname too long.\n"); return -1;
        }
        strncpy(parsed_url->host, host_port_part, host_len);
        parsed_url->host[host_len] = '\0';

        parsed_url->port = atoi(colon_in_host + 1);
        if (parsed_url->port <= 0 || parsed_url->port > 65535) {
            fprintf(stderr, "Error: Invalid port number '%s'\n", colon_in_host + 1);
            return -1;
        }
    } else { // No port specified, use default
        if (strlen(host_port_part) >= MAX_HOST_LEN) {
             fprintf(stderr, "Error: Hostname too long.\n"); return -1;
        }
        strncpy(parsed_url->host, host_port_part, MAX_HOST_LEN -1);
        parsed_url->host[MAX_HOST_LEN -1] = '\0';
        // Port remains default 21
    }

    if (strlen(parsed_url->host) == 0) {
        fprintf(stderr, "Error: Hostname cannot be empty.\n");
        return -1;
    }

    return 0;
}