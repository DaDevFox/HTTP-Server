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

char *response_good = "HTTP/1.1 200 OK\r\n\r\n";


int main()
{
	// Disable output buffering
	setbuf(stdout, NULL);

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(1,1), &wsaData) == SOCKET_ERROR) {
        printf ("Error initialising WSA.\n");
        return -1;
    }
#endif

    // You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage

	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;

    // AF_INET specifies address families usable on the internet and SOCK_STREAM specifies a two-way TCP connection via a byte stream
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
#if defined(_WIN32) || defined(_WIN64)
        printf("Socket creation failed: %d/%s...\n", WSAGetLastError(), strerror(WSAGetLastError()));
#else
		printf("Socket creation failed: %s...\n", strerror(errno));
#endif
		return 1;
	}

	// Since the tester restarts your program quite often, setting REUSE_PORT
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
	{
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(4221),    // converts from host-represented 4221 (short) in its byte order to " network-represented
		.sin_addr = {htonl(INADDR_ANY)},
	};

	if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
	{
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0)
	{
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);

	int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
	printf("Client connected\n");

    char data_buffer[256];
    if(read(client_fd, data_buffer, 256) < 1)
        printf("No data read from client\n");
    else
        printf("Data read from client: %s\n", data_buffer);

    if(send(client_fd, response_good, (int)strlen(response_good), 0) == -1){
        printf("Error responding to client: %s\n", strerror(errno));
    }

    close(client_fd);
    close(server_fd);
	return 0;
}
