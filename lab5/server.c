#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CONNECTIONS 5
#define BUFFER_SIZE 5000000

char ack[BUFFER_SIZE];

void handle_client(int client_socket) {
    char test[100]="Hello,client";
    send(client_socket, test, sizeof(test), 0);
    ssize_t bytes_sent = send(client_socket, ack, sizeof(ack), 0);
    if (bytes_sent < 0) {
        perror("Error sending acknowledgment");
    } else {
        printf("ACK sent to client.\n");
    }
    close(client_socket);
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    memset(ack, 'A', sizeof(ack));

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CONNECTIONS) == 0) {
        printf("Server listening on port %d...\n", PORT);
    } else {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    char buffer[100];
    while (1) {
        new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (new_socket < 0) {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        ssize_t bytes_received = recv(new_socket, buffer, sizeof(buffer), 0);
		if (bytes_received < 0) {
			perror("Error receiving data");
			//close(new_socket);
			continue;
		}


    	//printf("Received message: %s Size= %ld\n", buffer,sizeof(buffer));

        
        handle_client(new_socket);
    }


    //close(server_socket);
    return 0;
}