#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pcap.h>


#include <time.h>
#define SERVER_IP "127.0.0.1"
//#define SERVER_IP "inp.zoolab.org"
#define SERVER_PORT 10495
#define BUFFER_SIZE 1024

int begin, end;
int packet_lengths[500]; 

void packet_handler(const struct pcap_pkthdr *pkthdr, const unsigned char *packet) {
    int seq = 0;
    for (int i = 0; i < pkthdr->len; i++) {
        if (packet[i] == '/') {
            seq = 100 * (packet[i+3] - '0') + 10 * (packet[i+4] - '0') + (packet[i+5] - '0');
        }
        
        if (packet[i] == ':') {
            if (packet[i+1] == 'B') {
                printf("begin=%d\n", seq);
                begin = seq;
            } else if (packet[i+1] == 'E') {
                printf("end=%d\n", seq);
                end = seq;
            }
        }
    }
 
    packet_lengths[seq] = pkthdr->len;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [user_id]\n", argv[0]);
        exit(1);
    }

   

    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    if (getaddrinfo(SERVER_IP, NULL, &hints, &res) != 0) {
        perror("Failed to resolve hostname");
        exit(1);
    }

    struct sockaddr_in *server_info = (struct sockaddr_in *)res->ai_addr;

    /*char pcap_cmd[1024];
    sprintf(pcap_cmd, "sudo tcpdump -ni any -Xxnv udp and port 10495 -w ooo.pcap &");  //*/

    
    system("sudo tcpdump -ni any -w ooo.pcap -Xxnv udp and port 10495  &");
    
   
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
    printf("%s%s\n",user_id,challenge_id);

    
  


    // Step 4: Receive and process UDP packets from the server
    int sequence_number = 0;

    time_t last_packet_time = time(NULL);
    int cnt=0;
    while (cnt<15) {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(client_socket, buffer, BUFFER_SIZE, 0, NULL, NULL);
        cnt++;
       
    }
    
    //printf("pcap已儲存\n");
    
    // Close the socket (this won't be reached in this example)
    




    char *pcap_file = "ooo.pcap";
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;

    const int minimum_file_size = 1;
    while (1) {
        FILE *file = fopen(pcap_file, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fclose(file);

            if (file_size >= minimum_file_size) {
               
                //printf("%s 文件有足夠數據\n", pcap_file);
                break;
            }
        }

       
        sleep(1);  
    }

    
    handle = pcap_open_offline(pcap_file, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Error opening pcap file: %s\n", errbuf);
        return 1;
    }

    const unsigned char *packet;
    struct pcap_pkthdr header;

   
    for (int i = 0; i < 500; i++) {
        packet_lengths[i] = -1;
    }

   
    while ((packet = pcap_next(handle, &header)) != NULL) {
        packet_handler(&header, packet);
    }

    char flag[1024];
    for (int i = begin+1; i < end; i++) {
        if (packet_lengths[i] != -1) {
            //printf("%c", packet_lengths[i]-48);
            flag[i-begin-1]=packet_lengths[i] -48;
        }
    }
    printf("\n");
    flag[end-begin-1]='\0';
    printf("Result: %s\n", flag);
    char verify_flag[1024];
    sprintf(verify_flag, "verfy %s", flag);

    int temp=0;
    while (1) {
        
        temp++;
        memset(buffer, 0, sizeof(buffer));
        
        if (recvfrom(client_socket, buffer, BUFFER_SIZE, 0, NULL, NULL) == -1) {
            perror("recvfrom");
        } 
        
        if(temp==100){
            sendto(client_socket, verify_flag, strlen(verify_flag), 0, (struct sockaddr*)&server_address, sizeof(server_address)) ;
        }
        if(buffer[0]=='B'){
            printf("%s\n", buffer);
            break;
        }
        if(buffer[0]=='G'){
            printf("%s\n", buffer);
            break;
        }
        if(buffer[0]=='N'){
            printf("%s\n", buffer);
            break;
        }
        
       
    }
    pcap_close(handle);

    close(client_socket);

    
  
    
    return 0;
}
