#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
    server_addr.sin_port = htons(10301);  // 请更改为您要连接的服务器端口
    server_addr.sin_addr.s_addr = inet_addr("140.113.213.213");  // 请替换为您要连接的服务器IP地址

    // 尝试连接到服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");
    
    char buffer[1024];
    ssize_t n;

    char **maze;    
    int rows=0; //動態變化 
    int maze_size_received = 0; // 指示是否已接收迷宫大小信息
    int maze_rows=100, maze_cols; 

    

    while ((n = read(sockfd, buffer, sizeof(buffer))) > 0) {
        buffer[n] = '\0';  // 添加字符串结束符
        printf("Received: %s", buffer);

        if (!maze_size_received){
            char *start = strstr(buffer, "Size of the maze = ");
            if (start) {
                start += strlen("Size of the maze = ");
            
                if (sscanf(start, "%d x %d", &maze_cols, &maze_rows) == 2) {
                    printf("Maze Size: %d x %d\n", maze_cols, maze_rows);
                    maze_size_received=1;
                    maze = (char **)malloc(maze_rows * sizeof(char *));
                    for (int i = 0; i < maze_rows; i++) {
                        maze[i] = (char *)malloc(maze_cols + 1);
                    }
                    char *map_start = strstr(buffer, "#");
                    if (map_start) {
                        printf("maze rows=%d\n",maze_rows);
                        for (int i = 0; i < maze_rows; i++) {
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

        }
        if(maze_size_received){
            break;
        }
       
    }
    printf("out\n");
    
    if (maze_size_received) {
        for (int i = 0; i < maze_rows; i++) {
            printf("row%d :%s\n", i,maze[i]);
        }
    }

    
    int player_row, player_col, exit_row, exit_col;

    // 在已经收到迷宫并存储在maze数组后，查找 '*' 和 'E' 的坐标
    for (int i = 0; i < maze_rows; i++) {
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

    

    // 打印 '*' 和 'E' 的坐标
    printf("Player's position: row %d, col %d\n", player_row, player_col);
    printf("Exit's position: row %d, col %d\n", exit_row, exit_col);

    int x=exit_col-player_col;
    int y=exit_row-player_row;



    char command[1024];
    int idx = 0;  

    if (x > 0) {
        for (int i = 0; i < x; i++) {
            command[idx++] = 'D';  
        }
    } else if (x < 0) {
        for (int i = 0; i > x; i--) {
            command[idx++] = 'A';  
        }
    }

    if (y > 0) {
        for (int i = 0; i < y; i++) {
            command[idx++] = 'S';  
        }
    } else if (y < 0) {
        for (int i = 0; i > y; i--) {
            command[idx++] = 'W';  
        }
    }

    command[idx++]='\n';
    command[idx]='\0';
    
    printf("%s %d\n",command,strlen(command));
    sendto(sockfd, command, strlen(command), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("sent\n");
    while ((n = read(sockfd, buffer, sizeof(buffer))) > 0) {
        buffer[n] = '\0';  // 添加字符串结束符
        printf("%s", buffer);

    }
    
    
    


    // 打印迷宫
    

    


    if (maze_size_received) {
        for (int i = 0; i < rows; i++) {
            free(maze[i]);
        }
        free(maze);
    }

    close(sockfd);

    return 0;
}
