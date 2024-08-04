
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
    server_addr.sin_port = htons(10303);  // 请更改为您要连接的服务器端口
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
    

    int vcol,vrow;
    char *start = strstr(receivedData, "Size of the maze = ");
    if (start) {
        start += strlen("Size of the maze = ");
    
        if (sscanf(start, "%d x %d", &maze_cols, &maze_rows) == 2) {
            printf("Maze Size: %d x %d\n", maze_cols, maze_rows);
            
            maze = (char **)malloc(maze_rows * sizeof(char *));
            start = strstr(receivedData, "View port area = ");
            start += strlen("View port area = ");
            sscanf(start, "%d x %d", &vcol, &vrow);
            printf("View Size: %d x %d\n", vcol, vrow);
            start += strlen("%d x %d\n\n  ");
            char temp[vcol+5];
            strncpy(temp, start, 4+vcol);
            temp[vcol+4]='\0';
            printf("%s\n",temp);
            int a,down=0;
            if(temp[0]==' '){
                a=temp[1]-'0';
            }else if(temp[0]=='-'){
                a=(temp[1]-'0');
                down=1;
            }else{
                a=(temp[0]-'0')*10+temp[1]-'0';
            }
            char up[105];
            printf("a=%d\n",a);
            if(down==0){
                for(int i=0;i<a;i++){
                    up[i]='I';
                }
            }else{
                for(int i=0;i<a;i++){
                    up[i]='K';
                }
            }
            
            up[a]='\n';
            up[a+1]='\0';
            sendto(sockfd, up, strlen(up), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            
            while((n = read(sockfd, buffer, sizeof(buffer))) > 0){ 
                buffer[n] = '\0';  // 添加字符串结束符
                printf("%s", buffer);
                start = strstr(buffer, "0: ");

                if(strstr(buffer, "Enter your move(s)>")){
                    break;
                }
            }

            //左移
            char moveLeft[7]="JJJJ\n\0";
            //moveLeft[0]='J';
            //moveLeft[1]='\n';
            //moveLeft[2]='\0';
            //printf("left=%s\n",moveLeft);

            sendto(sockfd, moveLeft, strlen(moveLeft), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            while((n = read(sockfd, buffer, sizeof(buffer))) > 0){  
                buffer[n] = '\0';  // 添加字符串结束符
                //printf("%s", buffer);
                start = strstr(buffer, ": ");
                char lef[11];
                if(start){
                    start+=strlen(": ");
                    strncpy(lef, start, 10);
                    lef[10]='\0';
                }else{
                    printf("no start");
                }
                if(lef[0]==' '){
                    n = read(sockfd, buffer, sizeof(buffer));
                    char moveRight[3]="L\n\0";
                    sendto(sockfd, moveRight, strlen(moveRight), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    while((n = read(sockfd, buffer, sizeof(buffer))) > 0){
                        buffer[n] = '\0';  // 添加字符串结束符
                        printf("%s", buffer);
                        start = strstr(buffer, ": ");
                        char ri[11];
                        if(start){
                            start+=strlen(": ");
                            strncpy(ri, start, 10);
                            ri[10]='\0';
                        }
                        if(ri[0]=='#'){
                            break;
                        }
                        n = read(sockfd, buffer, sizeof(buffer));
                        sendto(sockfd, moveRight, strlen(moveRight), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    }
                    break;
                }
                n = read(sockfd, buffer, sizeof(buffer));
                sendto(sockfd, moveLeft, strlen(moveLeft), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

            }


            for (int i = 0; i < maze_rows; i++) {
                maze[i] = (char *)malloc(maze_cols + 1);
            }
            
            int r=0,c=0;
            
            while (1){
                //printf("buffer=%s\n",buffer);
                /*if(strstr(buffer, "Move view port:")){
                    printf("等一下");
                    continue;
                }*/
                char *map_start = strstr(buffer, ": ");
                map_start+=strlen(": ");
                for(int i=0;i<vrow&&r+i<maze_rows;i++){
                    for(int j=0;j<vcol&&c+j<maze_cols;j++){
                        strncpy(&maze[r+i][c+j], map_start,1);
                        map_start++;
                    }
                    map_start = strchr(map_start, '\n'); // 移动到下一行
                    if (map_start) {
                        map_start+=8;
                    }
                }
                buffer[0]='\0';
                while(strstr(buffer, "Enter your move(s)>")==NULL){
                    n = read(sockfd, buffer, sizeof(buffer));
                    //printf("buffer=%s",buffer);
                }
                
                char goLeft[200];
                char goRight[100];
                
                for(int i=0;i<vcol*(maze_cols/vcol);i++){
                    goLeft[i]='J';
                }
                
                for(int i=0;i<vrow;i++){
                    goLeft[vcol*(maze_cols/vcol)+i]='K';
                }
                goLeft[vcol*(maze_cols/vcol)+vrow]='\n';
                goLeft[vcol*(maze_cols/vcol)+vrow+1]='\0';
                for(int i=0;i<vcol;i++){
                    goRight[i]='L';
                }
                goRight[vcol]='\n';
                goRight[vcol+1]='\0';
                
                

                if(c+vcol>maze_cols&&r+vrow>maze_rows){
                    break;
                }else if(c+vcol>maze_cols){
                    c=0;
                    r=r+vrow;
                    sendto(sockfd,goLeft, strlen(goLeft), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    //printf("send:%s\n",goLeft);
                    //sendto(sockfd,goDown, strlen(goDown), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    n = read(sockfd, buffer, sizeof(buffer));
                }else{
                    c+=vcol;
                    sendto(sockfd,goRight, strlen(goRight), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    //printf("send:%s\n",goRight);
                    n = read(sockfd, buffer, sizeof(buffer));
                }
            }
                
                
            
        } else {
            printf("Unable to parse maze size.\n");
        }
    }

    

    int player_row, player_col, exit_row, exit_col;
    
    
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
    

    free(receivedData);

    // 打印 '*' 和 'E' 的坐标
    printf("Player's position: row %d, col %d\n", player_row, player_col);
    printf("Exit's position: row %d, col %d\n", exit_row, exit_col);

    pathLength=0;
    currentPath[0] = '\0';
    findPath(player_row, player_col);

    if(currentPath){
        printf("%s\n",currentPath);
    }else{
        printf("沒找到\n");
    }
    //path[pathLength++]='\n'; 
    currentPath[++pathLength]='\0';
    
    sendto(sockfd, currentPath, strlen(currentPath), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("sent\n");
    while ((n = read(sockfd, buffer, sizeof(buffer))) > 0) {
        buffer[n] = '\0';  // 添加字符串结束符
        printf("%s", buffer);

    }
        

    for (int i = 0; i < rows; i++) {
        free(maze[i]);
    }
    free(maze);
    

    close(sockfd);

    return 0;
}
