#include "ftp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // For read, write, close
#include <ctype.h>      // For isdigit

int send_ftp_command(int sockfd, const char* command, const char* arg) {
    char cmd_buffer[512]; // Max command length with arg
    int len;

    if (arg && strlen(arg) > 0) {
        len = snprintf(cmd_buffer, sizeof(cmd_buffer), "%s %s\r\n", command, arg);
    } else {
        len = snprintf(cmd_buffer, sizeof(cmd_buffer), "%s\r\n", command);
    }
    if (len < 0 || len >= (int)sizeof(cmd_buffer)) {
        fprintf(stderr, "Error: FTP command too long.\n");
        return -1;
    }

    printf("C: %s", cmd_buffer); // Log client command
    if (write(sockfd, cmd_buffer, len) < 0) {
        perror("write ftp command");
        return -1;
    }
    return 0;
}

// Simplified read_ftp_response. A robust version needs to handle
// multi-line responses where intermediate lines are "<code>-<text>"
// and the final line is "<code> <text>".
// In ftp_utils.c
int read_ftp_response(int sockfd, char* response_buffer, size_t buffer_size, int* ftp_code) {
    ssize_t bytes_read_chunk;
    size_t total_bytes_read = 0;
    response_buffer[0] = '\0'; // Clear buffer

    char line_buffer[1024]; // Buffer for a single line from the server
    int is_final_line = 0;

    // printf("S: "); // Moved the "S: " to be inside the loop for each line if preferred
                      // or print once before the loop and then just the content.
                      // For now, printing char-by-char includes the prompt implicitly.

    while (total_bytes_read < buffer_size -1 && !is_final_line) {
        // Read character by character to build a line
        int line_idx = 0;
        char c;
        printf("S: "); // Print S: for each line attempt (can be noisy)
        while(line_idx < (int)sizeof(line_buffer) - 1) {
            bytes_read_chunk = read(sockfd, &c, 1);
            if (bytes_read_chunk <= 0) {
                if (bytes_read_chunk == 0 && line_idx > 0) { // EOF but had partial line
                    line_buffer[line_idx] = '\0';
                    printf("%s\n(Partial line before EOF)\n", line_buffer);
                    goto process_response_label;
                } else if (bytes_read_chunk == 0) {
                    fprintf(stderr, "\nServer closed connection prematurely.\n");
                } else {
                    perror("\nread char from ftp response");
                }
                return -1;
            }
            printf("%c", c); // Log character by character
            line_buffer[line_idx++] = c;
            if (c == '\n') {
                break; // End of line
            }
        }
        line_buffer[line_idx] = '\0'; // Null-terminate the read line

        // Append line_buffer to the main response_buffer
        if (total_bytes_read + strlen(line_buffer) < buffer_size) {
            strcat(response_buffer, line_buffer);
            total_bytes_read += strlen(line_buffer);
        } else {
            fprintf(stderr, "\nResponse buffer too small for full multi-line response.\n");
            // Attempt to parse what we have
            goto process_response_label;
        }

        // Check if this line is the final line of the response
        // A line starting with 3 digits followed by a SPACE is the final line.
        if (strlen(line_buffer) >= 4 &&
            isdigit(line_buffer[0]) &&
            isdigit(line_buffer[1]) &&
            isdigit(line_buffer[2]) &&
            line_buffer[3] == ' ') {
            is_final_line = 1;
            // The ftp_code will be parsed from the *start* of the whole response_buffer later
        }
        // If it's "NNN-" or not a code line starting with 3 digits, loop continues
    }
    // No need for extra \n if char-by-char printf was used and line ended with \n
    // printf("\n"); // End logging for this response block if not char-by-char

process_response_label: // Label for goto
    // Try to parse the code from the *first line* of the potentially multi-line response
    if (sscanf(response_buffer, "%d", ftp_code) != 1) {
        fprintf(stderr, "Could not parse FTP code from the beginning of response: %s\n", response_buffer);
        // As a fallback, try the last parsed line if different logic was used to find it
        // For this version, the first line should contain the primary code.
        return -1;
    }
    
    if (!is_final_line && total_bytes_read >= buffer_size -1) {
        fprintf(stderr, "Warning: Response buffer filled; response might be truncated or not fully parsed as final.\n");
    } else if (!is_final_line && total_bytes_read < buffer_size -1) {
        // This case might happen if read returned 0 (EOF) before a final line was seen.
        // The ftp_code from the first line is still attempted.
    }


    return 0;
}


int ftp_login(int control_sockfd, const char* user, const char* pass) {
    char response_buf[FTP_RESPONSE_BUF_SIZE];
    int ftp_code;

    // Server should send a 220 welcome first. This is usually handled after connect.
    // Assuming welcome message already processed.

    if (send_ftp_command(control_sockfd, "USER", user) < 0) return -1;
    if (read_ftp_response(control_sockfd, response_buf, sizeof(response_buf), &ftp_code) < 0) return -1;

    if (ftp_code == 331) { // Password required
        if (send_ftp_command(control_sockfd, "PASS", pass) < 0) return -1;
        if (read_ftp_response(control_sockfd, response_buf, sizeof(response_buf), &ftp_code) < 0) return -1;
        if (ftp_code != 230) {
            fprintf(stderr, "Login failed after PASS. Server response %d: %s\n", ftp_code, response_buf);
            return -1;
        }
    } else if (ftp_code != 230) { // 230 User logged in, proceed (e.g., for some anonymous setups)
        fprintf(stderr, "USER command failed. Server response %d: %s\n", ftp_code, response_buf);
        return -1;
    }
    printf("Logged in successfully.\n");
    return 0;
}

int ftp_set_type_image(int control_sockfd) {
    char response_buf[FTP_RESPONSE_BUF_SIZE];
    int ftp_code;
    if (send_ftp_command(control_sockfd, "TYPE", "I") < 0) return -1;
    if (read_ftp_response(control_sockfd, response_buf, sizeof(response_buf), &ftp_code) < 0) return -1;
    if (ftp_code != 200) {
        fprintf(stderr, "TYPE I command failed. Server response %d: %s\n", ftp_code, response_buf);
        return -1; // Or just a warning
    }
    printf("Transfer type set to Binary (Image).\n");
    return 0;
}

int ftp_enter_passive_mode(int control_sockfd, char* data_ip_str, size_t data_ip_len, int* data_port) {
    char response_buf[FTP_RESPONSE_BUF_SIZE];
    int ftp_code;

    if (send_ftp_command(control_sockfd, "PASV", NULL) < 0) return -1;
    if (read_ftp_response(control_sockfd, response_buf, sizeof(response_buf), &ftp_code) < 0) return -1;

    if (ftp_code != 227) { // 227 Entering Passive Mode
        fprintf(stderr, "PASV command failed. Server response %d: %s\n", ftp_code, response_buf);
        return -1;
    }

    // Parse "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)."
    int h1, h2, h3, h4, p1, p2;
    char *ptr_open_paren = strchr(response_buf, '(');
    if (!ptr_open_paren || sscanf(ptr_open_paren, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        fprintf(stderr, "Could not parse PASV response IP/Port: %s\n", response_buf);
        return -1;
    }
    snprintf(data_ip_str, data_ip_len, "%d.%d.%d.%d", h1, h2, h3, h4);
    *data_port = (p1 << 8) + p2; // p1*256 + p2
    printf("Passive mode: Data connection to %s:%d\n", data_ip_str, *data_port);
    return 0;
}

int ftp_retrieve_file(int control_sockfd, int data_sockfd, const char* remote_path, const char* local_filename) {
    char response_buf[FTP_RESPONSE_BUF_SIZE];
    int ftp_code;

    if (send_ftp_command(control_sockfd, "RETR", remote_path) < 0) return -1;
    if (read_ftp_response(control_sockfd, response_buf, sizeof(response_buf), &ftp_code) < 0) return -1;

    // 150: File status okay; about to open data connection.
    // 125: Data connection already open; transfer starting.
    if (ftp_code != 150 && ftp_code != 125) {
        fprintf(stderr, "RETR command failed. Server response %d: %s\n", ftp_code, response_buf);
        return -1;
    }
    printf("Server ready to send file. Code: %d\n", ftp_code);

    FILE* local_file = fopen(local_filename, "wb");
    if (!local_file) {
        perror("fopen local file for writing");
        return -1;
    }
    printf("Downloading '%s' to '%s'...\n", remote_path, local_filename);

    char file_buffer[FTP_FILE_BUF_SIZE];
    ssize_t bytes_received;
    long total_bytes_downloaded = 0;

    while ((bytes_received = read(data_sockfd, file_buffer, sizeof(file_buffer))) > 0) {
        if (fwrite(file_buffer, 1, bytes_received, local_file) != (size_t)bytes_received) {
            perror("fwrite to local file");
            fclose(local_file);
            return -1; // Indicate write error
        }
        total_bytes_downloaded += bytes_received;
    }
    fclose(local_file);

    if (bytes_received < 0) {
        perror("read from data socket");
        // File might be partially downloaded. Server might not send 226.
        return -1; // Indicate read error
    }
    printf("Downloaded %ld bytes.\n", total_bytes_downloaded);

    // After data transfer, data_sockfd is usually closed by the server, then the client.
    // Then, client expects a 226 Transfer complete on control_sockfd.
    if (read_ftp_response(control_sockfd, response_buf, sizeof(response_buf), &ftp_code) < 0) {
        fprintf(stderr, "Error reading final response after RETR.\n");
        return -1; // Or treat as warning if file seems complete
    }
    if (ftp_code != 226 && ftp_code != 250) { // 226 Transfer complete, 250 Requested file action okay
        fprintf(stderr, "File transfer may not have completed successfully on server. Response %d: %s\n", ftp_code, response_buf);
        return -1; // Indicate potential server-side issue
    }
    printf("Transfer confirmed by server (Code %d).\n", ftp_code);
    return 0;
}

int ftp_quit(int control_sockfd) {
    char response_buf[FTP_RESPONSE_BUF_SIZE];
    int ftp_code;
    if (send_ftp_command(control_sockfd, "QUIT", NULL) < 0) {
        // Still try to close the socket
    }
    // Try to read response, but don't fail hard if it doesn't come
    if (read_ftp_response(control_sockfd, response_buf, sizeof(response_buf), &ftp_code) == 0) {
        if (ftp_code != 221) { // 221 Service closing control connection
            printf("QUIT command acknowledged with code %d: %s\n", ftp_code, response_buf);
        } else {
            printf("QUIT successful (Code %d).\n", ftp_code);
        }
    } else {
        printf("No clear response to QUIT, or error reading. Closing connection.\n");
    }
    return 0; // Always returns 0 for quit, as we'll close socket anyway
}