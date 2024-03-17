#include <stdio.h>
#include <stdlib.h>
#if defined(_WIN32) || defined(_WIN64)
# include <winsock2.h>
#else
# include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#endif
#include <string.h>
#include <errno.h>
#include <unistd.h>

enum HTTP_METHOD{
    GET,
    HEAD,
    POST,
    PUT
};

const char *startline_200 = "HTTP/1.1 200 OK";
const char *startline_404 = "HTTP/1.1 404 Not Found";
const char *response_good = "HTTP/1.1 200 OK\r\n\r\n";
const char *response_404 = "HTTP/1.1 404 Not Found\r\n\r\n";

int server_init();
int server_listen_socket(int *server_fd);
void server_run(const int server_fd);
void respond(const int client_fd, char *startline, char *headers[], int header_count, char *body);
void send_response(const int client_fd, char *response_body, int response_len);

void HTTP_GET(int client_fd, char message[], int max_message_length);
void HTTP_process(int client_fd, char message[], int max_message_length);

// Returns 0 on error
int server_init(){
    // Disable output buffering
    setbuf(stdout, NULL);

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1,1), &wsaData) == SOCKET_ERROR) {
        printf ("Error initialising WSA.\n");
        return 0;
    }
#endif

    return 1;
}

int server_listen_socket(int *server_fd){
    // AF_INET specifies address families usable on the internet and SOCK_STREAM specifies a two-way TCP connection via a byte stream
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_fd == -1)
    {
#if defined(_WIN32) || defined(_WIN64)
        printf("Socket creation failed: %d/%s...\n", WSAGetLastError(), strerror(WSAGetLastError()));
#else
        printf("Socket creation failed: %s...\n", strerror(errno));
#endif
        return 0;
    }

    // Since the tester restarts your program quite often, setting REUSE_PORT
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        printf("SO_REUSEADDR failed: %s \n", strerror(errno));
        return 0;
    }

    struct sockaddr_in serv_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(4221),    // converts from host-represented 4221 (short) in its byte order to " network-represented
            .sin_addr = {htonl(INADDR_ANY)},
    };

    if (bind(*server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        printf("Bind failed: %s \n", strerror(errno));
        return 0;
    }

    int connection_backlog = 5;
    if (listen(*server_fd, connection_backlog) != 0)
    {
        printf("Listen failed: %s \n", strerror(errno));
        return 0;
    }

    return 1;
}

void server_run(const int server_fd){
//    while(1) {
        printf("Waiting for a client to connect...\n");
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        printf("Client connected\n");

        char data_buffer[256];
#if defined(_WIN32) || defined(_WIN64)
        if(recv(client_fd, data_buffer, 256, 0) == SOCKET_ERROR)
#else
        if(recv(client_fd, data_buffer, 256, 0) == SO_ERROR)
#endif
            printf("No data read from client\n\n");
        else
            printf("Data read from client: %s\n", data_buffer);

        HTTP_process(client_fd, data_buffer, 256);

        close(client_fd);
//    }
}

// OLDD
//void respond(const int client_fd, char *startline, char *headers[], int header_count, char *body){
//    int bytes_to_allocate = strlen(startline) + 2; // 2 for each <cr><lf> or CRLF or \r\n
//    for(int i = 0; i < header_count; i++)
//        bytes_to_allocate += strlen(headers[i]) + 2;
//    bytes_to_allocate += 2 + strlen(body) + 1; // 1 for null terminator at end
//
//    char *message = malloc(bytes_to_allocate);
//
//    int i = 0;
//    while(i < strlen(startline)) {
//        message[i] = startline[i];
//        i++;
//    }
//    message[i] = '\r';
//    message[++i] = '\n';
//
//    printf("\n%d\n", i);
//
//    for(int header = 0; header < header_count; header++)
//    {
//        printf("\n%d, %d\n", i, strlen(headers[header]));
//        int start = i;
//        while(i < start + strlen(headers[header])) {
//            message[i] = headers[header][i - start];
//            i++;
//        }
//        message[i] = '\r';
//        message[++i] = '\n';
//    }
//
//    printf(message);
//
//    // format message; use strlen to cull out \0's
////    snprintf(message, strlen(startline), "%.*s\r\n", strlen(startline), startline);
////    for(int i = 0; i < header_count; i++)
////        snprintf(message, bytes_to_allocate,"%.*s\r\n", strlen(headers[i]),  headers[i]);
////
////    snprintf(message, bytes_to_allocate, "\r\n%.*s", strlen(body), body);
//
//    printf("response message:\r\n%.*s", bytes_to_allocate - 1, message);
//    send_response(client_fd, message, bytes_to_allocate);
//    free(message);
//}

void respond(const int client_fd, char *startline, char *headers[], int header_count, char *body){
    int bytes_to_allocate = strlen(startline) + 2; // 2 for each <cr><lf> or CRLF or \r\n
    for(int i = 0; i < header_count; i++)
        bytes_to_allocate += strlen(headers[i]) + 2;
    bytes_to_allocate += 2 + strlen(body) + 1; // 1 for null terminator at end

    char *message = malloc(bytes_to_allocate);

//    for(int i = 0; i < strlen(startline); i++)
//        message[i] = startline[i];
//

    // format message; use strlen to cull out \0's
    snprintf(message, strlen(startline), "%.*s\r\n", strlen(startline), startline);
    for(int i = 0; i < header_count; i++)
        snprintf(message, bytes_to_allocate,"%.*s\r\n", strlen(headers[i]),  headers[i]);

    snprintf(message, bytes_to_allocate, "\r\n%.*s", strlen(body), body);

    printf("response message:\r\n%.*s", bytes_to_allocate - 1, message);
    send_response(client_fd, message, bytes_to_allocate);
    free(message);
}

void send_response(const int client_fd, char *response_body, int response_len)
{
    if (send(client_fd, response_body, response_len, 0) <0) {
        printf("Error responding to client: %s\n", strerror(errno));
    }
}

void HTTP_GET(int client_fd, char *message, int max_message_length) {
    // START LINE
    printf("HTTP GET; ");

    // set path
    char *path = message; // now after the http mode specifier since prior method consumed
    int path_len = 0;
    while(message[path_len++] != ' '){}
    printf("for path %.*s ", path_len - 1, message); // - 1 offset for known trailing space

    // set http version
    message += path_len; // don't offset so we cull the space
    char *http_version = message; // now after the http mode specifier since prior method consumed
    int http_version_len = 8;
//    printf("with HTTP version %s", message);

    // consume until newline
    while(strncmp(message++, "\r\n", 2)){}
    message++; // pushes it past the \n and onto next line

    // TODO: process headers


    if(strncmp(path, "/", path_len - 1) == 0)
        send_response(client_fd, response_good, strlen(response_good));
    else
    {
        path++; // to cull the first '/'
        int operation_terminator = 0;
        while(path[operation_terminator] != '/') operation_terminator++;

        if(strncmp(path, "echo", operation_terminator) == 0)
        {
            char *headers[2] = {
                    "Content-Type: text/plain",
                    "Content-Length: 3"
            };

            path += operation_terminator + 1; // cull out / again; rest of path is actual message

            // trim end-of-string other things
            int body_size = 0;
            while(path[body_size] != ' ') body_size++;
            char *body = malloc(body_size);
            sprintf_s(body, body_size, "%.*s", body_size, path);
            printf("%d, %.*s\n", body_size,body_size, path);
            printf("%d, %s\n", body_size, body);

            printf("responding 200 with echo %.*s\n", body_size, body);
            respond(client_fd, startline_200, headers, 2, body);
            free(body);
        }



        send_response(client_fd, response_404, strlen(response_404));
    }
}

// returns -1 on fail; consumes first word if consume param is nonzero
enum HTTP_METHOD HTTP_get_method(char **message, int *max_message_length, int consume){
    int output = -1;

    // get first space in message (should be after request)
    int i = 0;
    while((*message)[i++] != ' '){}

    if(strncmp(*message, "GET", i - 1) == 0)
        output = GET;
    if(strncmp(*message, "HEAD", i - 1) == 0)
        output = HEAD;
    if(strncmp(*message, "POST", i - 1) == 0)
        output = POST;
    if(strncmp(*message, "PUT", i - 1) == 0)
        output = PUT;

    if(consume) {
        *message += i;
        *max_message_length -= i;
    }
    return output;
}


void HTTP_process(int client_fd, char message[], int max_message_length){
    enum HTTP_METHOD method = HTTP_get_method(&message, &max_message_length, 1);
    if(method == GET)
        HTTP_GET(client_fd, message, max_message_length);
}

int main()
{
    if(!server_init())
        return 1;

	int server_fd;
    if(!server_listen_socket(&server_fd))
        return 1;

    server_run(server_fd);

    close(server_fd);
	return 0;
}
