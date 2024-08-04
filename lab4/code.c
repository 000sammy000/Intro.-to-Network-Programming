#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>


int needsEncoding(char c) {
    // 可以自定義需要編碼的字符
    // 通常需要編碼的字符包括非字母數字字符、空格等
    char specialChars[] = "!*'();:@&=+$,/?#[]";
    int i;
    for (i = 0; specialChars[i]; i++) {
        if (c == specialChars[i]) {
            return 1;
        }
    }
    return 0;
}

// 將字串進行URL編碼
char *urlEncode(const char *str) {
    char *encodedStr = (char *)malloc(strlen(str) * 3 + 1); // 分配足夠的內存
    if (!encodedStr) {
        return NULL;
    }
    int i, j = 0;
    for (i = 0; str[i]; i++) {
        if (needsEncoding(str[i])) {
            // 如果字符需要編碼，將其轉換為URL編碼形式
            sprintf(encodedStr + j, "%%%02X", (unsigned char)str[i]);
            j += 3;
        } else {
            // 否則，直接複製字符
            encodedStr[j] = str[i];
            j++;
        }
    }
    encodedStr[j] = '\0'; // 字串結尾
    return encodedStr;
}
void connecting(int sockfd,int port,char* addr){
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);  // 请更改为您要连接的服务器端口
    server_addr.sin_addr.s_addr = inet_addr(addr);  // 请替换为您要连接的服务器IP地址

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");

}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    int port=10001;  //part3=10314 part4=10001
    char *addr="172.21.0.4"; //part3=140.113.213.213 part4=172.21.0.4
    

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    connecting(sockfd,port,addr);
    
    char request[1000];
    snprintf(request, sizeof(request), "GET /otp?name=110550142 HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n",addr);
  

    // 发送HTTP请求
    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("Failed to send the HTTP request");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 接收和打印服务器响应
    char response[2000];
    ssize_t n;
    char otp[1000]="";

    while (1) {
        n = recv(sockfd, response, sizeof(response), 0);

        if (n == -1) {
            perror("Failed to receive data");
            break;
        } else if (n == 0) {
            // 连接已关闭
            break;
        }

        response[n] = '\0';
        printf("Received data from the server:\n%s\n", response);

        // 将接收到的数据追加到 otp 中
        

        // 检查是否包含特定字符串
        if (strstr(response, "110550142") != NULL) {
            char*start=(strstr(response, "110550142"));
            strcat(otp, start);
            break;
        }
    }
    printf("%s",otp);
    

    FILE *file = fopen("output.txt", "w");

    // 检查文件是否成功打开
    if (file != NULL) {
        // 要写入文件的字符串

        // 使用 fprintf 将字符串写入文件
        fprintf(file, "%s", otp);

        // 关闭文件
        fclose(file);

        printf("File 'output.txt' has been created and the text has been written to it.\n");
    } else {
        perror("Failed to open the file for writing");
    }

    close(sockfd);

    int sockfd3;
    sockfd3 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd3 == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    connecting(sockfd3,port,addr);

    char *encodedStr = urlEncode(otp);
    printf("Encoded String: %s\n", encodedStr);

    snprintf(request, sizeof(request), "GET /verify?otp=%s HTTP/1.1\r\nHost: 140.113.213.213\r\nConnection: keep-alive\r\n\r\n",encodedStr);

    if (send(sockfd3, request, strlen(request), 0) == -1) {
        perror("Failed to send the HTTP request");
        close(sockfd3);
        exit(EXIT_FAILURE);
    }

    n = read(sockfd3, response, sizeof(response));
    response[n]='\0';
    printf("Response from the server:\n%s\n", response);
    n = read(sockfd3, response, sizeof(response));
    response[n]='\0';
    printf("Response from the server:\n%s\n", response);

    close(sockfd3);

    int sockfd2;
    sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd2 == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    connecting(sockfd2,port,addr);

    

   
    int lngth=strlen("--myboundary\r\n")
    +strlen("Content-Type: application/octet-stream\r\n")
    +strlen("Content-Disposition: form-data; name=\"file\"; filename=\"myfile.ext\"\r\n")
    +strlen("Content-Transfer-Encoding: 8bit\r\n");
   
    printf("%d %d\n",lngth,strlen(otp));
    lngth+=strlen("\r\n--myboundary--\r\n");
    
    
    snprintf(request, sizeof(request), "POST /upload HTTP/1.1\r\n"
                                        "Host: %s\r\n"
                                        "Content-Length: %d\r\n"
                                        "Content-Type: multipart/form-data; boundary=myboundary\r\n"
                                        "Connection: close\r\n\r\n"
                                        "--myboundary\r\n"
                                        "Content-Type: application/octet-stream\r\n"
                                        "Content-Disposition: form-data; name=\"file\"; filename=\"myfile.ext\"\r\n"
                                        "Content-Transfer-Encoding: 8bit\r\n"
                                        "\r\n%s"
                                        "\r\n--myboundary--\r\n",addr,lngth+strlen(otp),otp);
    
    
    
    printf("%s",request);
    // 发送HTTP POST请求
    if (send(sockfd2, request, strlen(request), 0) == -1) {
        perror("Failed to send the HTTP request");
        close(sockfd2);
        exit(EXIT_FAILURE);
    }

   

    n = read(sockfd2, response, sizeof(response));
    response[n]='\0';
    printf("Response from the server:\n%s\n", response);
    n = read(sockfd2, response, sizeof(response));
    response[n]='\0';
    printf("Response from the server:\n%s\n", response);
    
    

    close(sockfd2);
    

    return 0;
}

