
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
int visit[110][250];
int pathLength;
int maze_rows, maze_cols;

char* findPath(int x,int y ,int end) {
    if(x<0||y<0||x>110||y>250){
        return NULL;
    }
    //printf("%d %d %c\n",x,y,maze[x][y]);
    if (end==1&&maze[x][y]=='E') {
        printf("find\n");
        currentPath[pathLength]='\n';
        return currentPath;
    }

    if (end==0&&maze[x][y]=='?') {
        printf("find\n");
        currentPath[--pathLength]='\n';
        return currentPath;
    }

    if (end==1&&maze[x][y]=='?') {
        return NULL;
    }
    
    if (maze[x][y] == '#' || visit[x][y] == 1) {
        return NULL;
    }

    //maze[x][y] = 'V'; 
    visit[x][y]=1;

    
    char *path = NULL;
    
    
    currentPath[pathLength]='W';  
    pathLength++;
    if ((path = findPath(x-1, y,end)) != NULL) return path;
    currentPath[--pathLength] = '\0'; // 回溯
    //pathLength--;

    currentPath[pathLength]='S';
    pathLength++;
    if ((path = findPath(x+1, y,end)) != NULL) return path;
    currentPath[--pathLength] = '\0'; // 回溯
    //pathLength--;

    currentPath[pathLength]='A';
    pathLength++;
    if ((path = findPath(x, y-1,end)) != NULL) return path;
    currentPath[--pathLength] = '\0'; // 回溯
    //pathLength--;

    currentPath[pathLength]='D';
    pathLength++;
    if ((path = findPath(x, y+1,end))!= NULL) return path;
    currentPath[--pathLength] = '\0'; // 回溯
    //pathLength--;


    // 如果无法找到有效路径，回溯
    //maze[x][y] = ' ';
    //printf("%d %d都沒找到\n",x,y);
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
    server_addr.sin_port = htons(10304);  // 请更改为您要连接的服务器端口
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
    char buffer[2000];
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
                a=(temp[1]-'0')*(-1);
                down=1;
            }else{
                a=(temp[0]-'0')*10+temp[1]-'0';
            }
            
            printf("a=%d\n",a);

            for (int i = 0; i < maze_rows; i++) {
                maze[i] = (char *)malloc(250 + 1);
            }
            for(int i=0;i<maze_rows;i++){
                for(int j=0;j<230;j++){
                    maze[i][j]='?';
                }
            }
            
            int r=a,c=100;
            int k=1,d=10;
            int end=0;
            while (1){
                if(n>1){
                    buffer[n]='\0';
                    printf("buffer=%s\n",buffer);
                }else{
                    break;
                }
                char *map_start;
                if(k==1){
                   map_start = strstr(buffer, "Note2: View port area = 11 x 7"); 
                   map_start+=strlen("Note2: View port area = 11 x 7");
                   map_start+= strlen("\n\n0000000");
                   
                   k--;
                }else{
                    //char temp[vcol+5];
                    strncpy(temp, &buffer[4], 4+vcol);
                    temp[vcol+4]='\0';
                    printf("%s\n",temp);
                    
                    if(temp[0]==' '){
                        a=temp[1]-'0';
                    }else if(temp[0]=='-'){
                        a=(temp[1]-'0')*(-1);
                    }else{
                        a=(temp[0]-'0')*10+temp[1]-'0';
                    }
                    
                    printf("a=%d\n",a);

                    map_start = strstr(buffer, ": ");
                    map_start+=strlen(": ");
                }
                
               
                r=a;
                printf("%d %d\n",r,c);
                int player_row, player_col, exit_row, exit_col;
                for(int i=0;i<vrow;i++){
                    if(r+i<0||r+i>100){
                        printf("這行不讀\n");
                    }else{
                        for(int j=0;j<vcol;j++){
                            strncpy(&maze[r+i][c+j], map_start,1);
                            if(maze[r+i][c+j]=='*'){
                                player_row = r+i;
                                player_col = c+j;
                            }else if (maze[r+i][c+j] == 'E') {
                                exit_row = r+i;
                                exit_col = c+j;
                                end=1;
                            }
                            map_start++;
                        }
                    }
                    map_start = strchr(map_start, '\n'); // 移动到下一行
                    if (map_start) {
                        map_start+=8;
                    }
                }
                if(end){
                    printf("End's position: row %d, col %d\n", exit_row, exit_col);
                }
                
                printf("Player's position: row %d, col %d\n", player_row, player_col);
                /*if(exit_row==player_row&&exit_col==player_col){
                    while(n = read(sockfd, buffer, sizeof(buffer))>0){
                        buffer[n]='\0';
                        printf("%s",buffer);
                    }
                    //break;
                }*/
                int fin=0;
                if(strstr(buffer, "Enter your move(s)>")==NULL){
                    n = read(sockfd, buffer, sizeof(buffer));
                    buffer[n]='\0';
                    printf("buffer=%s",buffer);
                    //printf("buffer1=%c\n",buffer[1]);
                    if(buffer[1]=='B'){
                        fin=1;
                        sendto(sockfd, currentPath, strlen(currentPath), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                        while(1){
                            n = read(sockfd, buffer, sizeof(buffer));
                            //printf("n=%d",n);
                            if(n>0){
                                buffer[n]='\0';
                                printf("%s",buffer);
                            }
                            sendto(sockfd, currentPath, strlen(currentPath), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                            
                        }

                    }
                }
               
                //currentPath[0] = '\0';
                
                for(int i=0;i<110;i++){
                    for(int j=0;j<250;j++){
                        visit[i][j]=0;
                    }
                }
                pathLength=0;
                //printf("where ");
                
                char*path=findPath(player_row, player_col,end);
                //printf("not here\n");
                

                if(path){
                    printf("find?");
                    currentPath[++pathLength] = '\0';
                    printf("%s\n",currentPath);
                    sendto(sockfd, currentPath, strlen(currentPath), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    n= read(sockfd, buffer, sizeof(buffer));
                    buffer[n]='\0';
                    if(end==1){
                        printf("good\n");
                        printf("%s",buffer);
                    }
                }else{
                    printf("沒找到\n");
                    if(end==1){
                        for(int i=0;i<110;i++){
                            for(int j=0;j<250;j++){
                                visit[i][j]=0;
                            }
                        }
                        pathLength=0;
                        findPath(player_row, player_col,0);
                        currentPath[++pathLength] = '\0';
                        printf("%s\n",currentPath);
                        sendto(sockfd, currentPath, strlen(currentPath), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                        n = read(sockfd, buffer, sizeof(buffer));
                        buffer[n]='\0';
                        printf("%s",buffer);

                    }
                }
                //if(end==1)break;
                if(fin==1){
                    while(n = read(sockfd, buffer, sizeof(buffer))>0){
                            //printf("%d\n",n);
                            buffer[n]='\0';
                            printf("%s",buffer);
                            //sendto(sockfd, currentPath, strlen(currentPath), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                            //sendto(sockfd, currentPath, strlen(currentPath), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    }
                    break;
                }
                
                maze[player_row][player_col]='.';
                for(int i=0;i<strlen(currentPath);i++){
                    if(currentPath[i]=='A'){
                        c--;
                    }else if(currentPath[i]=='D'){
                        c++;
                    }
                }
                
            }
                
                
            
        } else {
            printf("Unable to parse maze size.\n");
        }
    }
   
    /*for(int i=0;i<101;i++){
        printf("row %d",i);
        for(int j=80;j<140;j++){
            printf("%c",maze[i][j]);
        }
        printf("\n");
    }*/
    
    
    /*
    

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
    printf("sent\n");*/
    printf("out");
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
