//one thread
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 5000000
#define DATA_SIZE 5000000

char buffer[BUFFER_SIZE] ;

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
	struct timeval send_time, receive_time;
    

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
        return 1;
    }


	char little_buffer[100]="Hello,server";
    // TODO: Send/receive data as needed
    gettimeofday(&send_time, NULL);
    send(client_socket, little_buffer, sizeof(little_buffer), 0);
    recv(client_socket, buffer, sizeof(buffer), 0);
    
    gettimeofday(&receive_time, NULL);

    long delay = ((receive_time.tv_sec - send_time.tv_sec) * 1000000L+(receive_time.tv_usec - send_time.tv_usec))/2;

        
        if (delay < 0) {
        // Adjust for potential borrow from seconds
            delay+= 1000000L;
        }
        double delay_ms = (double)delay / 1000.0;

	
    int total=0,cnt=0,total_amount=0,lcnt=0;
    double total_time=0;
    while(total<DATA_SIZE){
        cnt++;
        gettimeofday(&send_time, NULL);
        // Receive the timestamp from the server
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
           
            break;
        }
        
        // Get current time again
        gettimeofday(&receive_time, NULL);

        long latency = ((receive_time.tv_sec - send_time.tv_sec) * 1000000L+(receive_time.tv_usec - send_time.tv_usec));

        
        if (latency < 0) {
        // Adjust for potential borrow from seconds
            latency += 1000000L;
        }
        double latency_ms = (double)latency / 1000.0;
        // Calculate bandwidth
        double bandwidth =  (double)(bytes_received*8)/ latency ;

        //printf("# RESULTS: delay = %.2f ms, bandwidth = %.2f Mbps\n", latency_ms, bandwidth);
        
        if(latency_ms<15&&bandwidth<1500){
            lcnt++;
            total_time+=latency_ms;
            total_amount+=bytes_received;

        }
        total+=bytes_received;

        //printf("total=%d",total);

    }
    


    printf("# RESULTS: delay = %.2f ms, bandwidth = %.2f Mbps\n", delay_ms, (double)total_amount*8/(total_time*1000));
    

    

    close(client_socket);
    return 0;
}