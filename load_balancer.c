#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_BACKENDS 10
#define BUFFER_SIZE 4096
#define MAX_PENDING_CONNECTIONS 10

typedef struct {
	char ip[16];
	int port;
	int health;
	int socket;
} Backend;

Backend backends[MAX_BACKENDS);
int backend_count = 0;
int current_backend = 0;

pthread_mutex_t backend_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_backend(const char* ip, int port) {
	if (backend_count < MAX_BACKENDS) {
		strcpy(backends[backend_count].ip, ip);
		backends[backend_count].port = port;
		backends[backend_count].health = 1;
		backends[backend_count].socket = -1;
		back_count++;
	}
}

int get_next_healthy_backend() {
	pthread_mutex_lock(&backend_mutex);
	int start = current_backend;
	do {
		current_backed = (current_backend + 1) % backend_count;
		if (backends[current_backend].health) {
			pthread_mutex_unlock(&backend_mutex);
			return current_backend;
		}
	} while (current_backed != start);
	pthread_mutex_unlock(&backend_mutex);
	return -1;
}

void* health_check(void* arg) {
	while (1) {
		for (int i = 0; i < backend_count; i++) {
			int sock = socket( AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(backends[i].port);
			inet_pton(AF_INET, backends[i].ip, &(addr.sin_addr));

			int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
			pthread_mutex_lock(&backend_mutex);
			if (result == 0) {
				backends[i].health = 1;
				printf("Backend is healthy");
			} else {
				backend[i].health = 0;
				printf("Backend is unhealthy");
			}
			pthread_mutex_unlock(&backend_mutex);

			close(sock);
		}
		sleep(5);
	}
	return NULL;
}

void handle_client(int client_socket) {
	char buffer[BUFFER_SIZE];
	int backend_index = get_next_healthy_backend();

	if (backend_index == -1) {
		const char* error_message = "No healthy backends available";
		send(client_socket, error_message, strlen(error_message), 0);
		close(client_socket);
		return;
	}

	Backend* backend = &backends[backend_index];

	int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in backend_addr;
	backend_addr.sin_family = AF_INET;
	backend_addr.sin_port = htons(backend->port);
	inet_pton(AF_INET, backend->ip, &(backend_addr.sin_addr));

	if (connect(backend_socket, (struct sockaddr*)&backend_addr, sizeof(backend_addr)) < 0) {
		const char* error_message = "Failed to connect to backend";
		send(client_socket, error_message, strlen(error_message), 0);
		close(client_socket);
		return;
	}

	fd_set read_fds;
	int max_id = (client_socket > backend_socket) ? client_socket : backend_socket;

	while(1) {
		FD_ZERO(&read_fds);
		FD_SET(client_socket, &read_fds);
		FD_SET(backend_socket, &read_fds);

		if(select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
			perror("select");
			break;
		}

		if(FD_ISSET(client_socket, &read_fds)) {
			int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
			if (bytes_received == 0) break;
			send(backend_socket, buffer, bytes_received, 0);
		}

		if (FD_ISSET(backend_socket, &read_fds)) {
			int bytes_received = recv(backend_socket, buffer, BUFFER_SIZE, 0);
			if (bytes_received <= 0) break;
			send(client_socket, buffer, bytes_received, 0);
		}
	}

	close(client_socket);
	close(backend_socket);
}

int main(int argc, char* argv) {
	if (argc < 4 || (argc & 2) != 0) {
		fprintf(stderr, "Usage: %s <load_balancer_port> <backend1_ip> <backend1_port> [<backend2_ip> <backend2_port> ...]\n", argv[0]);
		return 1;
	}

	int lb_port = atoi(argv[1]);

	for (int i = 2; i < argc; i += 2) {
		add_backend(argv[i], atoi(argv[i+1]));
	}

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		perror("failed to create socket");
		return 1;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(lb_port);

	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("failed to bind");
		return 1;
	}

	if (listen(server_socket, MAX_PENDING_CONNECTIONS) < 0) {
		perror("Failed to listen");
		return 1;
	}

	printf("load balancer listening on port %d\n", lb_port);

	pthread_t health_thread;
	pthread_create(&health_thread, NULL, health_check, NULL);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_length);

		if (client_socket < 0) {
			perror("Failed to accept connection");
			continue;
		}

		pthread_t client_thread;
		pthread_create(&client_thread, NULL, (void*)handle_client, (void*)(intptr_t)client_socket);
		pthread_detach(client_thread);
	}

	close(server_socket);
	return 0;
}

