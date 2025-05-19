#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <stddef.h>     // For size_t
#include <sys/socket.h> // For INET_ADDRSTRLEN on some systems

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16 // For IPv4 "ddd.ddd.ddd.ddd\0"
#endif

/**
 * Resolves a hostname to an IPv4 address string.
 * @param hostname The hostname to resolve.
 * @param ip_address_str Buffer to store the resulting IP address string.
 * @param ip_str_len Size of the ip_address_str buffer.
 * @return 0 on success, -1 on failure.
 */
int resolve_hostname(const char* hostname, char* ip_address_str, size_t ip_str_len);

/**
 * Creates a TCP socket.
 * @return Socket file descriptor on success, -1 on failure.
 */
int create_tcp_socket();

/**
 * Connects a TCP socket to a server.
 * @param sockfd The socket file descriptor.
 * @param ip_address The server's IP address string.
 * @param port The server's port number.
 * @return 0 on success, -1 on failure.
 */
int connect_to_server(int sockfd, const char* ip_address, int port);

#endif // SOCKET_UTILS_H