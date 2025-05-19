#include "socket_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>      // For gethostbyname, herror

int resolve_hostname(const char* hostname, char* ip_address_str, size_t ip_str_len) {
    struct hostent *h;
    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname");
        return -1;
    }

    if (h->h_addrtype == AF_INET) { // Check if it's an IPv4 address
        if (inet_ntop(AF_INET, h->h_addr_list[0], ip_address_str, ip_str_len) == NULL) {
            perror("inet_ntop");
            return -1;
        }
        return 0;
    } else {
        fprintf(stderr, "Error: Hostname resolved to non-IPv4 address type: %d\n", h->h_addrtype);
        return -1;
    }
}

int create_tcp_socket() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation");
        return -1;
    }
    return sockfd;
}

int connect_to_server(int sockfd, const char* ip_address, int port) {
    struct sockaddr_in server_addr;

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        perror("inet_pton for server IP");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect to server");
        return -1;
    }
    return 0;
}