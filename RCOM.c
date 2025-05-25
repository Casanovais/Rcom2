// myftpclient.c (Exemplo Conceitual)

#include <stdio.h>      // Para printf, fopen, fclose, etc.
#include <stdlib.h>     // Para exit, atoi
#include <string.h>     // Para strcmp, strcpy, strlen, strtok, memset
#include <sys/socket.h> // Para socket, connect, send, recv, close
#include <netinet/in.h> // Para struct sockaddr_in, htons, inet_addr
#include <arpa/inet.h>  // Para inet_addr (embora netinet/in.h possa ser suficiente)
#include <netdb.h>      // Para gethostbyname
#include <unistd.h>     // Para close (pode estar em sys/socket.h também)
#include <errno.h>      // Para perror (diagnóstico de erros)

#define BUFFER_SIZE 4096 // Tamanho do buffer para ler respostas e dados

// Função para enviar um comando e opcionalmente receber uma resposta
// Retorna o código de resposta do servidor FTP ou -1 em erro de socket
int send_ftp_command(int sockfd, const char *command, char *response_buffer, size_t buffer_len) {
    printf("CLIENT > %s", command); // Mostra o comando enviado
    if (send(sockfd, command, strlen(command), 0) < 0) {
        perror("Erro ao enviar comando");
        return -1;
    }

    if (response_buffer != NULL) {
        memset(response_buffer, 0, buffer_len); // Limpa o buffer de resposta
        int bytes_received = recv(sockfd, response_buffer, buffer_len - 1, 0);
        if (bytes_received < 0) {
            perror("Erro ao receber resposta");
            return -1;
        }
        if (bytes_received == 0) {
            printf("Servidor fechou a conexão inesperadamente.\n");
            return -1;
        }
        response_buffer[bytes_received] = '\0'; // Adiciona terminador nulo
        printf("SERVER < %s", response_buffer);   // Mostra a resposta recebida

        // Extrai o código de resposta (os primeiros 3 caracteres)
        char response_code_str[4];
        strncpy(response_code_str, response_buffer, 3);
        response_code_str[3] = '\0';
        return atoi(response_code_str); // Converte para inteiro
    }
    return 0; // Se não esperamos resposta (raro, mas possível)
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s ftp://[user:pass@]host/path/to/file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *url = argv[1];
    char response_buffer[BUFFER_SIZE];
    int control_sockfd, data_sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    // --- 1. Parsear a URL ---
    // (Simplificado - uma implementação real seria mais robusta)
    char *user = "anonymous";
    char *pass = "guest@example.com"; // Senha típica para anônimo
    char host[256];
    char path[1024];
    char filename[256];

    // Exemplo de parsing muito básico
    // ftp://host/path -> sscanf(url, "ftp://%[^/]/%s", host, path);
    // ftp://user:pass@host/path -> sscanf(url, "ftp://%[^:]:%[^@]", user, pass); e depois pegar host e path
    // Para este exemplo, vamos assumir que já temos:
    // sscanf(url, "ftp://%[^/]/%s", host, path); // Simplificação extrema
    if (sscanf(url, "ftp://%255[^/]/%1023s", host, path) != 2) {
         // Tentar com user:pass
        if (sscanf(url, "ftp://%63[^:]:%63[^@]/%255[^/]/%1023s", user, pass, host, path) != 4) {
             fprintf(stderr, "URL mal formatada.\n");
             exit(EXIT_FAILURE);
        }
    }

    // Obter o nome do arquivo do path
    char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        strcpy(filename, last_slash + 1);
    } else {
        strcpy(filename, path); // Se não houver barras, o path é o nome do arquivo
    }
    printf("Host: %s, Path: %s, User: %s, Filename: %s\n", host, path, user, filename);


    // --- 2. Resolver o Nome do Host para Endereço IP ---
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERRO, host não encontrado: %s\n", host);
        exit(EXIT_FAILURE);
    }

    // --- 3. Criar Socket de Controle e Conectar ---
    control_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (control_sockfd < 0) {
        perror("ERRO ao abrir socket de controle");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Limpa a estrutura
    server_addr.sin_family = AF_INET;             // Família de endereços (IPv4)
    server_addr.sin_port = htons(21);             // Porta FTP (21), convertida para ordem de bytes da rede
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length); // Copia o IP do host resolvido

    if (connect(control_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERRO ao conectar ao socket de controle");
        close(control_sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Conectado ao servidor %s na porta 21.\n", host);

    // Receber mensagem de boas-vindas do servidor (pode ser multi-linha)
    // Uma implementação robusta leria até que o código 220 final fosse recebido
    int welcome_code = send_ftp_command(control_sockfd, "", response_buffer, BUFFER_SIZE);
    if (welcome_code != 220 && !(welcome_code >= 200 && welcome_code < 300 && strstr(response_buffer, "220 ") != NULL) ) { // Alguns servidores enviam várias linhas 220- antes do 220 final
        fprintf(stderr, "Servidor não respondeu com 220 Welcome. Resposta: %d\n", welcome_code);
        close(control_sockfd);
        exit(EXIT_FAILURE);
    }


    // --- 4. Login ---
    char command[BUFFER_SIZE];
    sprintf(command, "USER %s\r\n", user);
    int response_code = send_ftp_command(control_sockfd, command, response_buffer, BUFFER_SIZE);
    if (response_code != 331 && response_code != 230) { // 230 se não precisar de senha
        fprintf(stderr, "Comando USER falhou. Resposta: %d\n", response_code);
        close(control_sockfd);
        exit(EXIT_FAILURE);
    }

    if (response_code == 331) { // Se precisar de senha
        sprintf(command, "PASS %s\r\n", pass);
        response_code = send_ftp_command(control_sockfd, command, response_buffer, BUFFER_SIZE);
        if (response_code != 230) {
            fprintf(stderr, "Comando PASS falhou. Resposta: %d\n", response_code);
            close(control_sockfd);
            exit(EXIT_FAILURE);
        }
    }
    printf("Login bem-sucedido.\n");

    // --- 5. Entrar em Modo Passivo (PASV) ---
    response_code = send_ftp_command(control_sockfd, "PASV\r\n", response_buffer, BUFFER_SIZE);
    if (response_code != 227) {
        fprintf(stderr, "Comando PASV falhou. Resposta: %d\n", response_code);
        close(control_sockfd);
        exit(EXIT_FAILURE);
    }

    // Parsear o IP e a Porta da resposta PASV: "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)."
    int h1, h2, h3, h4, p1, p2;
    char *pasv_data_start = strchr(response_buffer, '(');
    if (pasv_data_start == NULL) {
        fprintf(stderr, "Resposta PASV mal formatada.\n");
        close(control_sockfd);
        exit(EXIT_FAILURE);
    }
    sscanf(pasv_data_start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    unsigned short data_port = (p1 * 256) + p2;
    char data_ip[16];
    sprintf(data_ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    printf("Modo passivo: IP %s Porta %d\n", data_ip, data_port);


    // --- 6. Criar Socket de Dados e Conectar ---
    data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sockfd < 0) {
        perror("ERRO ao abrir socket de dados");
        close(control_sockfd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in data_server_addr;
    memset(&data_server_addr, 0, sizeof(data_server_addr));
    data_server_addr.sin_family = AF_INET;
    data_server_addr.sin_port = htons(data_port);
    // data_server_addr.sin_addr.s_addr = inet_addr(data_ip); // Converte string IP para formato de rede
    if (inet_pton(AF_INET, data_ip, &data_server_addr.sin_addr) <= 0) { // Alternativa mais moderna e segura
        perror("inet_pton erro para IP de dados");
        close(control_sockfd);
        close(data_sockfd);
        exit(EXIT_FAILURE);
    }


    if (connect(data_sockfd, (struct sockaddr *)&data_server_addr, sizeof(data_server_addr)) < 0) {
        perror("ERRO ao conectar ao socket de dados");
        close(control_sockfd);
        close(data_sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Conectado ao servidor de dados.\n");


    // --- 7. Solicitar o Arquivo (RETR) ---
    // (Ainda no socket de CONTROLE)
    sprintf(command, "RETR %s\r\n", path);
    response_code = send_ftp_command(control_sockfd, command, response_buffer, BUFFER_SIZE);
    if (response_code != 150 && response_code != 125) { // 150 (File status okay; about to open data connection), 125 (Data connection already open; transfer starting)
        fprintf(stderr, "Comando RETR falhou. Resposta: %d\n", response_code);
        close(control_sockfd);
        close(data_sockfd);
        exit(EXIT_FAILURE);
    }


    // --- 8. Receber o Arquivo (no socket de DADOS) ---
    FILE *output_file = fopen(filename, "wb"); // "wb" para escrita binária
    if (output_file == NULL) {
        perror("Erro ao abrir arquivo de saída");
        close(control_sockfd);
        close(data_sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Recebendo arquivo '%s'...\n", filename);

    char file_buffer[BUFFER_SIZE];
    int bytes_read_data;
    long total_bytes_received = 0;
    while ((bytes_read_data = recv(data_sockfd, file_buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(file_buffer, 1, bytes_read_data, output_file);
        total_bytes_received += bytes_read_data;
        printf("\rRecebidos: %ld bytes", total_bytes_received);
        fflush(stdout);
    }
    printf("\n");

    if (bytes_read_data < 0) {
        perror("Erro ao receber dados do arquivo");
    }
    fclose(output_file);
    printf("Arquivo recebido e salvo como '%s' (%ld bytes).\n", filename, total_bytes_received);


    // --- 9. Fechar Conexão de Dados e Verificar Status ---
    close(data_sockfd); // Importante fechar o socket de dados!

    // Ler a resposta final da transferência no socket de CONTROLE
    // O servidor envia "226 Transfer complete" após fechar a conexão de dados
    // send_ftp_command já tem um recv embutido, mas precisamos garantir que lemos a resposta do RETR
    memset(response_buffer, 0, BUFFER_SIZE);
    int bytes_received_final_status = recv(control_sockfd, response_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received_final_status > 0) {
        response_buffer[bytes_received_final_status] = '\0';
        printf("SERVER < %s", response_buffer);
        char final_code_str[4];
        strncpy(final_code_str, response_buffer, 3);
        final_code_str[3] = '\0';
        response_code = atoi(final_code_str);
        if (response_code != 226 && response_code != 250) { // 226 (Closing data connection), 250 (Requested file action okay, completed)
            fprintf(stderr, "Transferência pode não ter sido concluída com sucesso. Resposta: %d\n", response_code);
        } else {
            printf("Transferência concluída com sucesso pelo servidor.\n");
        }
    } else if (bytes_received_final_status == 0) {
        printf("Conexão de controle fechada pelo servidor após transferência de dados.\n");
    } else {
        perror("Erro ao receber status final da transferência");
    }


    // --- 10. Sair (QUIT) ---
    // (No socket de CONTROLE)
    response_code = send_ftp_command(control_sockfd, "QUIT\r\n", response_buffer, BUFFER_SIZE);
    if (response_code != 221) { // 221 Service closing control connection
        fprintf(stderr, "Comando QUIT pode ter falhado. Resposta: %d\n", response_code);
    } else {
        printf("Desconectado do servidor.\n");
    }


    // --- 11. Fechar Socket de Controle ---
    close(control_sockfd);

    return EXIT_SUCCESS;
}