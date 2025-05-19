#include "url_parser.h"
#include "socket_utils.h"
#include "ftp_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For close()

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[user:pass@]host[:port]/path/to/file\n", argv[0]);
        return 1;
    }

    ParsedUrl url_components;
    char ip_addr_str[INET_ADDRSTRLEN];
    int control_sockfd = -1, data_sockfd = -1;
    char response_buf[FTP_RESPONSE_BUF_SIZE];
    int ftp_code;
    char data_ip_str[INET_ADDRSTRLEN];
    int data_port;

    // 1. Parse URL
    if (parse_ftp_url(argv[1], &url_components) < 0) {
        return 1;
    }
    printf("Parsed URL:\n  Host: %s\n  Port: %d\n  User: %s\n  Path: %s\n",
           url_components.host, url_components.port, url_components.user, url_components.path);
    // Note: Password is in url_components.pass, not printed for security.

    // 2. Resolve hostname
    if (resolve_hostname(url_components.host, ip_addr_str, sizeof(ip_addr_str)) < 0) {
        return 1;
    }
    printf("Resolved IP Address: %s\n", ip_addr_str);

    // 3. Create and connect control socket
    control_sockfd = create_tcp_socket();
    if (control_sockfd < 0) {
        return 1;
    }
    if (connect_to_server(control_sockfd, ip_addr_str, url_components.port) < 0) {
        close(control_sockfd);
        return 1;
    }
    printf("Control connection established to %s:%d.\n", ip_addr_str, url_components.port);

    // 4. Read initial welcome message(s) from server
    if (read_ftp_response(control_sockfd, response_buf, sizeof(response_buf), &ftp_code) < 0) {
        fprintf(stderr, "Failed to read welcome message.\n");
        close(control_sockfd);
        return 1;
    }
    if (ftp_code != 220) {
        fprintf(stderr, "Server did not send 220 welcome. Got %d: %s\n", ftp_code, response_buf);
        close(control_sockfd);
        return 1;
    }
    printf("FTP Server Welcome OK (Code %d).\n", ftp_code);

    // 5. Login
    if (ftp_login(control_sockfd, url_components.user, url_components.pass) < 0) {
        close(control_sockfd);
        return 1;
    }

    // 6. Set transfer type to Image (Binary)
    if (ftp_set_type_image(control_sockfd) < 0) {
        // This might be a warning for some servers, but generally required for files
        fprintf(stderr, "Warning: Could not set TYPE I. File transfer might be corrupted.\n");
    }

    // 7. Enter Passive Mode
    if (ftp_enter_passive_mode(control_sockfd, data_ip_str, sizeof(data_ip_str), &data_port) < 0) {
        close(control_sockfd);
        return 1;
    }

    // 8. Create and connect data socket
    data_sockfd = create_tcp_socket();
    if (data_sockfd < 0) {
        close(control_sockfd);
        return 1;
    }
    if (connect_to_server(data_sockfd, data_ip_str, data_port) < 0) {
        close(control_sockfd);
        close(data_sockfd);
        return 1;
    }
    printf("Data connection established to %s:%d.\n", data_ip_str, data_port);

    // 9. Retrieve the file
    char* local_filename = strrchr(url_components.path, '/');
    if (local_filename) {
        local_filename++; // Point after the slash
    } else {
        local_filename = url_components.path; // Use the whole path if no slash
    }
    if (strlen(local_filename) == 0) { // If path was just "/" or empty after slash
        local_filename = "downloaded_file"; // Default filename
    }

    int retrieve_status = ftp_retrieve_file(control_sockfd, data_sockfd, url_components.path, local_filename);
    // retrieve_status will be 0 on success, -1 on failure.

    close(data_sockfd); // Data socket should be closed after transfer
    printf("Data socket closed.\n");


    // 10. Quit
    ftp_quit(control_sockfd); // Send QUIT, attempt to read reply
    close(control_sockfd);    // Ensure control socket is closed
    printf("Control socket closed.\n");

    if (retrieve_status == 0) {
        printf("File '%s' downloaded successfully as '%s'.\n", url_components.path, local_filename);
        return 0;
    } else {
        printf("File download failed for '%s'.\n", url_components.path);
        // Optionally, delete the (potentially partial) local_filename here
        // remove(local_filename);
        return 1;
    }
}