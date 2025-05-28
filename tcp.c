#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define BUFFER_SIZE 1024

typedef struct {
	uint32_t seq;
	uint32_t ack;
	uint16_t window;
	uint8_t flags;
} TCPHeader;

typedef struct {
	int socket;
	struct sockaddr_in addr;
	TCPHeader header;
} TCPConnection;

TCPConnection* tcp_connect(const char* ip, int port) {
	TCPConnection* conn = (TCPConnection*)malloc(sizeof(TCPConnection));
	conn->socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);

	if (conn->socket < 0) {
		perror("Socket creation failed");
		free(conn);
		return NULL;
	}

	memset(&conn->addr, 0, sizeof(conn->addr));
	conn->addr.sin_family = AF_INET;
	conn->addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &(conn->addr.sin_addr));

	conn->header.seq = rand();
	conn->header.ack = 0;
	conn->header.window = htons(65535);
	conn->header.flags = 0x02;

	sendto(conn->socket, &conn->header, sizeof(TCPHeader), 0, (struct sockadrr*)&conn->addr, sizeof(conn->addr));

	char buffer[BUFFER_SIZE];
	recv(conn->socket, buffer, BUFFER_SIZE, 0);
	
	TCPHeader* received_header = (TCPHeader*)buffer;
	conn->header.ack = ntohl(received_header->seq) + 1;
	conn->header.seq++;
	conn->header.flags = 0x10;

	sednto(conn->socket, &conn->header, sizeof(TCPHeader), 0, (struct sockaddr*)&conn->addr, sizeof(conn->addr));

	return conn;

}

void tcp_send(TCPConnection* conn, const char* data, int length) {
	char buffer[BUFFER_SIZE];
	memcpy(buffer, &conn->header, sizeof(TCPHeader));
	memcpy(buffer + sizeof(TCPHeader), data, length);

	sendto(conn->socket, buffer, sizeof(TCPHeader) + length, 0, (struct sockaddr*)&conn->addr, sizeof(conn->addr));

	conn->header.seq += length;
}

int tcp_receive(TCPConnection* conn, char* buffer, int length) {
	int received = recv(conn->socket, buffer, length, 0);
	if (received > 0) {
		TCPHeader* header = (TCPHeader*)buffer;
		conn->header.ack = ntohl(header->seq) + received - sizeof(TCPHeader);
		conn->header.flags = 0x10;
		sendto(conn->socket, &conn->header, sizeof(TCPHeader), 0, (struct sockaddr*)&conn->addr, sizeof(conn->addr));
	}
	return received - sizeof(TCPHeader);
}

void tcp_close(TCPConnection* conn) {
	conn->header.flags = 0x01;
	sendto(conn->socket, &conn->header, sizeof(TCPHeader), 0, (struct sockaddr*)&conn->addr, sizeof(conn->addr));

	char buffer[BUFFER_SIZE];
	free(conn);
}
