
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAX_ROWS 100
#define MAX_COLS 100

char **maze;
char currentPath[MAX_ROWS * MAX_COLS];
int pathLength;
int maze_rows, maze_cols;

char* findPath(int x,int y ) {
    //printf("%d %d %c\n",x,y,maze[x][y]);
    if (maze[x][y]=='E') {
        printf("find\n");
        currentPath[pathLength]='\n';
        return currentPath;
    }
    
    if (maze[x][y] == '#' || maze[x][y] == 'V') {
        return NULL; 
    }

    maze[x][y] = 'V'; 

        
    char *path = NULL;
    
    
    currentPath[pathLength]='W';  
    pathLength++;
    
    if ((path = findPath(x-1, y)) != NULL) return path;
    currentPath[--pathLength] = '\0'; // 回溯
    //pathLength--;

    currentPath[pathLength]='S';
    pathLength++;
    if ((path = findPath(x+1, y)) != NULL) return path;
    currentPath[--pathLength] = '\0'; // 回溯
    //pathLength--;

    currentPath[pathLength]='A';
    pathLength++;
    if ((path = findPath(x, y-1)) != NULL) return path;
    currentPath[--pathLength] = '\0'; // 回溯
    //pathLength--;

    currentPath[pathLength]='D';
    pathLength++;
    if ((path = findPath(x, y+1))!= NULL) return path;
    currentPath[--pathLength] = '\0'; // 回溯
    //pathLength--;


    // 如果无法找到有效路径，回溯
    maze[x][y] = ' ';
    currentPath[pathLength] = '\0';

    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;


    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(10302);  // 请更改为您要连接的服务器端口
    server_addr.sin_addr.s_addr = inet_addr("140.113.213.213");  // 请替换为您要连接的服务器IP地址

    // 尝试连接到服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");
    
    char *receivedData = NULL;
    size_t receivedDataSize = 0;
    char buffer[1024];
    ssize_t n;

       
    int rows=0; //動態變化 
    int maze_size_received = 0; // 指示是否已接收迷宫大小信息
     

    

    while ((n = read(sockfd, buffer, sizeof(buffer))) > 0) {
        buffer[n] = '\0';
        size_t currentDataSize = receivedDataSize + n;
        receivedData = (char *)realloc(receivedData, currentDataSize + 1); // +1 給 null 終止符
        if (receivedData == NULL) {
            fprintf(stderr, "內存分配失敗\n");
            exit(1);
        }
        memcpy(receivedData + receivedDataSize, buffer, n);
        receivedDataSize = currentDataSize;
        receivedData[receivedDataSize] = '\0'; // 添加 null 終止符
        printf("Received: %s", buffer);

        if(strstr(buffer, "Enter your move(s)>")){
            break;
        }
        
       
    }
    printf("out\n");
   
    char *start = strstr(receivedData, "Size of the maze = ");
    if (start) {
        start += strlen("Size of the maze = ");
    
        if (sscanf(start, "%d x %d", &maze_cols, &maze_rows) == 2) {
            printf("Maze Size: %d x %d\n", maze_cols, maze_rows);
            maze_size_received=1;
            maze = (char **)malloc(maze_rows * sizeof(char *));
            for (int i = 0; i < maze_rows; i++) {
                maze[i] = (char *)malloc(maze_cols + 1);
            }
            char *map_start = strstr(receivedData, "##");
            if (map_start) {
                printf("maze rows=%d\n",maze_rows);
                for (int i = 0; i < maze_rows; i++) {
                    printf("%d\n",i);
                    strncpy(maze[i], map_start, maze_cols);
                    
                    maze[i][maze_cols] = '\0';
                    map_start = strchr(map_start, '\n'); // 移动到下一行
                    if (map_start) {
                        map_start++;
                    }
                    
                }
            }
        } else {
            printf("Unable to parse maze size.\n");
        }
    }

    

    int player_row, player_col, exit_row, exit_col;
    
    if (maze_size_received) {
        for (int i = 0; i < maze_rows; i++) {
            printf("row%d :%s\n", i,maze[i]);
            for (int j = 0; j < maze_cols; j++) {
                if (maze[i][j] == '*') {
                    player_row = i;
                    player_col = j;
                } else if (maze[i][j] == 'E') {
                    exit_row = i;
                    exit_col = j;
                }
            }

        }
    }

    free(receivedData);

    // 打印 '*' 和 'E' 的坐标
    printf("Player's position: row %d, col %d\n", player_row, player_col);
    printf("Exit's position: row %d, col %d\n", exit_row, exit_col);

    pathLength=0;
    //char *currentPath = (char *)malloc((maze_rows * maze_cols) + 1);
    currentPath[0] = '\0';
    findPath(player_row, player_col);

    if(currentPath){
        printf("%s\n",currentPath);
    }else{
        printf("沒找到\n");
    }
    //path[pathLength++]='\n';
    currentPath[++pathLength]='\0';
    
    //printf("%s %d\n",path,strlen(pathLength));
    sendto(sockfd, currentPath, strlen(currentPath), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("sent\n");
    while ((n = read(sockfd, buffer, sizeof(buffer))) > 0) {
        buffer[n] = '\0';  // 添加字符串结束符
        printf("%s", buffer);

    }
        


    if (maze_size_received) {
        for (int i = 0; i < rows; i++) {
            free(maze[i]);
        }
        free(maze);
    }

    close(sockfd);

    return 0;
}
