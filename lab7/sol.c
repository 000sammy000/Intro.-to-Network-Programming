#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>


#define SOCKET_PATH "/queen.sock"
#define GRID_SIZE 30

int canx[30];
int cany[30];
int canp[60];
int canm[60];
char grid[GRID_SIZE][GRID_SIZE];
int order[30]={0,29,1,28,2,27,3,26,4,25,5,24,6,23,7,22
            ,8,21,9,20,10,19,11,18,12,17,13,16,14,15};
//vector<int> points;
void setcan(int x,int y,int value){
    canx[x]=value;
    cany[y]=value;
    canp[x+y]=value;
    canm[x-y+30]=value;
}

void processResponse(const char *response) {
   
    for (int i = 0; i < 60; ++i) {
        if(i<30){    
            canx[i]=1;
            cany[i]=1;
        }
        canp[i]=1;
        canm[i]=1;
    }
    
    
    const char *okPosition = strstr(response, "OK: ");
    
    if (okPosition != NULL) {
        const char *dataStart = okPosition + strlen("OK: ");
        
       
        if (strlen(dataStart) >= GRID_SIZE * GRID_SIZE) {

            for (int i = 0; i < GRID_SIZE; ++i) {
                for (int j = 0; j < GRID_SIZE; ++j) {
                    grid[i][j] = dataStart[i * GRID_SIZE + j];
                    if(grid[i][j]=='Q'){
                        setcan(i,j,0);
                    }
                }
            }
        } else {
            printf("Error: Response data is not long enough.\n");
        }
    } else {
        printf("Error: Response does not contain 'OK:'.\n");
    }
    
}
int check_ok(int x,int y){
    int canput=1;
    
    if(cany[y]==0||canp[x+y]==0||canm[x-y+30]==0){
        
        canput=0;
    }

    
    return canput;
}

bool putQueen(int queenNum,int line){
    //printf("%d ",queenNum);
    if(queenNum>=30){
        return true;
    }
    bool res=false;
    for(int i=0;i<30;i++){
        //int x=order[i];
        if(canx[i]==1){
            for(int j=0;j<30;j++){
                int y=order[j];
                if(check_ok(i,y)==1){
                    //printf("ok\n");
                    grid[i][y]='Q';
                    setcan(i,y,0);
                    res=putQueen(queenNum+1,i+1)||res;
                    if(res)break;
                    grid[i][y]='.';
                    setcan(i,y,1);
                }
            }
            break;
        }
        
        if(res)break;
    }
    return res;
}

int main() {
    int sockfd;
    struct sockaddr_un server_addr;
    char buffer[1024]; 

    //printf("hello\n");
    
    // 創建套接字
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 設置伺服器地址
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // 連接到伺服器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char S[5]="S\n\0";
    if (send(sockfd, S, strlen(S), 0) == -1) {
        perror("Error sending message");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received == -1) {
        perror("Error receiving message");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 確保接收到的數據以 null 結尾，以便於字符串操作
    buffer[bytes_received] = '\0';

    // 處理伺服器的回應，這裡假設它是一個字符串
    //printf("Server response: %s\n", buffer);

    printf("before\n");
    processResponse(buffer);

    
    
    
    
    putQueen(3,0);
    
    printf("after\n");
    for(int i=0;i<30;i++){
        for(int j=0;j<30;j++){
            printf("%c ",grid[i][j]);
            if(grid[i][j]=='Q'){
                char queen[10];
                snprintf(queen, sizeof(queen), "M %d %d\n\0", i, j);

                if (send(sockfd, queen, strlen(queen), 0) == -1) {
                    perror("Error sending message");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }

                ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received == -1) {
                    perror("Error receiving message");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }

                buffer[bytes_received] = '\0';
            }

        }
        printf("\n");
    }

   
    

    char C[5]="C\n\0";
    

    if (send(sockfd, C, strlen(C), 0) == -1) {
        perror("Error sending message");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

    buffer[bytes_received] = '\0';
    printf("Server response: %s\n", buffer);

    char P[5]="P\n\0";
    send(sockfd, P, strlen(P), 0);
    bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_received] = '\0';
    //printf("Server response: \n%s\n", buffer);


// 關閉套接字
    close(sockfd);

    return 0;
}
