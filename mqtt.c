#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_TOPICS 100
#define MAX_TOPIC_LENGTH 128
#define MAX_MESSAGE_LENGTH 256

typedef struct{
	TCPConnection* con;
	char client_id[24];
} MQTTClient;

typedef struct {
	char topic[MAX_TOPIC_LENGTH];
	char message[MAX_MESSAGE_LENGTH];
} MQTTMessage;

MQTTClient clients[MAX_CLIENTS];
int client_count = 0;

char topics[MAX_TOPICS][MAX_TOPIC_LENGTH];
int topic_count = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t topics_mutex = PTHREAD_MUTEX_INITIALIZER;

void mqtt_publish(const char* topic, const char* message) {
	pthread_mutex_lock(&topics_mutex);
	for (int i = 0; i < topic_count; i++) {
		if (strcmp(topics[i], topic) == 0) {
			pthread_mutex_lock(&clients_mutex);
			for (int j = 0; j < client_count; j++) {
				char publish_message[MAX_MESSAGE_LENGTH + MAX_TOPIC_LENGTH + 10];
				snprintf(publish_message, sizeof(publish_message), "PUBLISH %s %s", topic, message);
				tcp_send(clients[j].conn, publish_message, strlen(publish_message));
			}
			pthread_mutex_unlock(&clients_mutex);
			pthread_mutex_unlock(&topics_mutex);
			return;
		}
	}

	if (topic_count < MAX_TOPICS) {
		strncpy(topics[topic_count], topic, MAX_TOPIC_LENGTH);
		topic_count++;
	}
	pthread_mutex_unlock(&topics_mutex);
}

void mqtt_subscribe(MQTTTClient* client, const char* topic) {
	pthread_mutex_lock(&topics_mutex);
	for (int i = 0; i < topic_count; i++) {
		if (strcmp(topics[i], topic) == 0) {
			pthread_mutex_unlock(&topics_mutex);
			return;
		}
	}

	if (topic_count < MAX_TOPIC) {
		strncpy(topics[topic_count], topic, MAX_TOPIC_LENGTH);
		topic_count++
	}
	pthread_mutex_unlock(&topics_mutex);
}

void* handle_client(void* arg) {
	MQTTClient* client = (MQTTClient*)arg;
	char buffer[BUFFER_SIZE];

	while (1) {
		int received = tcp_receive(client->conn, buffer, BUFFER_SIZE);
		if (received == 0) {
			break;
		}

		buffer[received] = '\0';

		if (strncmp(buffer, "CONNECT", 7) == 0) {
			sscanf(buffer, "CONNECT %s", client->client_id);
			tcp_send(client->conn, "CONNACK", 7);
		} else if (strncmp(buffer, "PUBLISH", 7) == 0) {
			char topic[MAX_TOPIC_LENGTH];
			char message[MAX_MESSAGE_LENGTH];
			sscanf(buffer, "PUBLISH %s %s", topic, message);
			mqtt_publish(topic, message);
		} else if (strncmp(buffer, "SUBSCRIBE", 9) == 0) {
			char topic[MAX_TOPIC_LENGTH];
			sscanf(buffer, "SUBSCRIBE %s", topic);
			mqtt_subscribe(client, topic);
			tcp_send(client->conn, "SUBACK", 6);
		} else if (strncmp(buffer, "DISCONNECT", 10) == 0) {
			break;
		}
	}

	pthread_mutex_lock(&client_mutex);
	for (int i = 0; i < client_count; i++) {
		if (&client[i] == client) {
			for (int j = i; j < client_count - 1; j+=) {
				client[j] = clients[j + 1];
			}
			client_count--;
			break;
		}
	}
	pthread_mutex_unlock(&clients_mutex);

	tcp_close(client->conn);
	return NULL;
}

void run_mqtt_broker(const char* ip, int port) {
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		perror("socket creation failed");
		exit(1);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind failed");
		exit(1);
	}

	if (listen(server_socket, 5) < 0) {
		perror("listen_failed");
		exit(1);
	}

	printf("MQTT Broker listening on %s:%d\n", ip, port);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_socket = accept(server_socket, (struct sockaddr*)&clientaddr, &client_addr_length);

		if (client_socket < 0) {
			perror("Accept failed");
			continue;
		}

		if (client_count >= MAX_CLIENTS) {
			close(client_socket);
			continue;
		}

		TCPConnection* conn = (TCPConnection*)malloc(sizeof(TCPConnection));
		conn->socket = client_socket;
		conn->addr = client_addr;

		pthread_mutex_lock(&clients_mutex);
		clients[client_count].conn = conn;
		client_count++;
		pthread_mutex_unlock(&client_mutex);

		pthread_t thread;
		pthread(&thread, NULL, handle_client, &clients[client_count - 1]);
		pthread_detach(thread);
	}

	close(server_socket);
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
		return 1;
	}

	const char* ip = argv[1];
	int port = atoi(argv[2]);

	run_mqtt_broker(ip, port);
	
	return 0;
}

