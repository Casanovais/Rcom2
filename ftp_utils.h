#ifndef FTP_UTILS_H
#define FTP_UTILS_H

#include <stdio.h>  // For FILE*
#include <stddef.h> // For size_t

#define FTP_RESPONSE_BUF_SIZE 4096
#define FTP_FILE_BUF_SIZE 4096

/**
 * Sends an FTP command to the server.
 * @param sockfd The control connection socket.
 * @param command The FTP command (e.g., "USER", "PASV").
 * @param arg The argument for the command (can be NULL).
 * @return 0 on success, -1 on failure.
 */
int send_ftp_command(int sockfd, const char* command, const char* arg);

/**
 * Reads a response from the FTP server and extracts the status code.
 * NOTE: This version is simplified for multi-line responses.
 * @param sockfd The control connection socket.
 * @param response_buffer Buffer to store the full server response.
 * @param buffer_size Size of the response_buffer.
 * @param ftp_code Pointer to store the extracted 3-digit FTP status code.
 * @return 0 on success, -1 on failure.
 */
int read_ftp_response(int sockfd, char* response_buffer, size_t buffer_size, int* ftp_code);

/**
 * Logs into the FTP server.
 * @param control_sockfd The control connection socket.
 * @param user Username.
 * @param pass Password.
 * @return 0 on success, -1 on failure.
 */
int ftp_login(int control_sockfd, const char* user, const char* pass);

/**
 * Enters passive mode for data transfer.
 * @param control_sockfd The control connection socket.
 * @param data_ip_str Buffer to store the data connection IP address.
 * @param data_ip_len Length of the data_ip_str buffer.
 * @param data_port Pointer to store the data connection port.
 * @return 0 on success, -1 on failure.
 */
int ftp_enter_passive_mode(int control_sockfd, char* data_ip_str, size_t data_ip_len, int* data_port);


/**
 * Sets the transfer type to binary (Image).
 * @param control_sockfd The control connection socket.
 * @return 0 on success, -1 on failure.
 */
int ftp_set_type_image(int control_sockfd);

/**
 * Retrieves a file from the FTP server.
 * @param control_sockfd The control connection socket.
 * @param data_sockfd The data connection socket.
 * @param remote_path The path of the file on the server.
 * @param local_filename The name to save the file as locally.
 * @return 0 on success, -1 on failure.
 */
int ftp_retrieve_file(int control_sockfd, int data_sockfd, const char* remote_path, const char* local_filename);

/**
 * Sends the QUIT command and closes the control connection.
 * @param control_sockfd The control connection socket.
 * @return 0 on success, -1 on failure.
 */
int ftp_quit(int control_sockfd);

#endif // FTP_UTILS_H