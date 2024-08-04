#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <time.h>
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 10495
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [user_id]\n", argv[0]);
        exit(1);
    }


    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    
    // Resolve the server hostname to an IP address using getaddrinfo
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    if (getaddrinfo(SERVER_IP, NULL, &hints, &res) != 0) {
        perror("Failed to resolve hostname");
        exit(1);
    }

    struct sockaddr_in *server_info = (struct sockaddr_in *)res->ai_addr;
    
    // Step 1: Create UDP socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Socket creation error");
        exit(1);
    }
    
    // Initialize server_address using the resolved IP address
    memcpy(&server_address, server_info, sizeof(*server_info));
    server_address.sin_port = htons(SERVER_PORT);
    
    // Step 2: Send the 'hello' command to obtain a challenge id
    
    char user_id[50];
    strcpy(user_id, argv[1]);

    char hello_command[BUFFER_SIZE];
    sprintf(hello_command, "hello %s", user_id);
    sendto(client_socket, hello_command, strlen(hello_command), 0, (struct sockaddr*)&server_address, sizeof(server_address));
    
    // Receive and parse challenge id
    recvfrom(client_socket, buffer, BUFFER_SIZE, 0, NULL, NULL);
    char* challenge_id = strstr(buffer, "_");
    if (challenge_id == NULL) {
        perror("Failed to receive challenge id");
        exit(1);
    }
    
    // Step 3: Send the 'chals' command using the obtained challenge id
    sprintf(hello_command, "chals %s%s" ,user_id,challenge_id);
    sendto(client_socket, hello_command, strlen(hello_command), 0, (struct sockaddr*)&server_address, sizeof(server_address));
    //printf("%s%s\n",user_id,challenge_id);
    // Step 4: Receive and process UDP packets from the server
    

    time_t last_packet_time = time(NULL);
    int cnt=0;
   

    
    char flag[1024];
    printf("Enter the flag to verify: ");
    scanf("%s", flag);
    char verify_flag[1024];
    sprintf(verify_flag, "verfy %s", flag);
    printf("%s", verify_flag);
    // Step 3: Send the flag to the server
    
    if (sendto(client_socket, verify_flag, strlen(verify_flag), 0, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("sendto");
        exit(1);
    } else {
        printf("Data sent successfully.\n");
    }
    // Step 4: Receive the server's response
    
    // Step 5: Check the response
    int temp=0;
    while (1) {
        
        temp++;
        memset(buffer, 0, sizeof(buffer));
        
        if (recvfrom(client_socket, buffer, BUFFER_SIZE, 0, NULL, NULL) == -1) {
            perror("recvfrom");
        } 
        printf("Received: %s\n", buffer);
        if(temp==100){
            sendto(client_socket, verify_flag, strlen(verify_flag), 0, (struct sockaddr*)&server_address, sizeof(server_address)) ;
        }
        if(buffer[0]=='B'){
            break;
        }
        if(buffer[0]=='G'){
            break;
        }
        if(buffer[0]=='N'){
            break;
        }
        
       
    }

    
    
    // Close the socket (this won't be reached in this example)
    close(client_socket);
    
    return 0;
}
      