#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>

#define errquit(m)	{ perror(m); exit(-1); }
#define NOT_FOUND_RESPONSE "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>\r\n"
#define NOT_IMPLEMENTED_RESPONSE "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>\r\n"
static int port_http = 80;
static int port_https = 443;
static const char *docroot = "/html";


int hex_to_dec(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    }
    return -1; // Invalid hex character
}

// URL-decode a string in-place
void urldecode(char *url) {
    char *pos = url;

    while (*url != '\0') {
        if (*url == '%') {
            if (url[1] && url[2]) {
                int hi = hex_to_dec(url[1]);
                int lo = hex_to_dec(url[2]);

                if (hi != -1 && lo != -1) {
                    *pos++ = (hi << 4) | lo;
                    url += 3;
                    continue;
                }
            }
        }

        *pos++ = *url++;
    }

    *pos = '\0';
}
const char *get_content_type(const char *file_path) {
    const char *ext = strrchr(file_path, '.');
    if (ext != NULL) {
        // Convert the extension to lowercase for case-insensitive comparison
        char ext_lowercase[10];
        size_t len = strlen(ext);
        for (size_t i = 0; i < len && i < sizeof(ext_lowercase) - 1; ++i) {
            ext_lowercase[i] = tolower(ext[i]);
            ext_lowercase[i + 1] = '\0';
        }

        // Compare file extension and return the corresponding Content-Type
        if (strcmp(ext_lowercase, ".html") == 0 || strcmp(ext_lowercase, ".htm") == 0) {
            return "text/html;charset=utf-8";
        } else if (strcmp(ext_lowercase, ".txt") == 0) {
            return "text/plain;charset=utf-8";
        } else if (strcmp(ext_lowercase, ".mp3") == 0) {
            return "audio/mpeg";
        } else if (strcmp(ext_lowercase, ".png") == 0) {
            return "image/png";
        }
        // Add more file extensions and corresponding Content-Types as needed
    }

    // Default to text/plain;charset=utf-8 if the extension is not recognized
    return "text/plain;charset=utf-8";
}

void handle_request(int client_socket) {
	char buffer[1024];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

    
    
    if (bytes_received < 0) {
        perror("recv");
        close(client_socket);
        return;
    }

    // Add null terminator to the received data
    buffer[bytes_received] = '\0';

    // Parse the HTTP request to get the requested file path
    char method[10];
    char path[256];
    sscanf(buffer, "%s %s", method, path);

    if (strcmp(method, "GET") != 0) {
        send(client_socket, NOT_IMPLEMENTED_RESPONSE, strlen(NOT_IMPLEMENTED_RESPONSE), 0);
        close(client_socket);
        return;
    }

	
	if (strcmp(path, "/") == 0 ||path[1]=='?') {
        strcpy(path, "/index.html");
    }

	char* isfile=strstr(path,".");
	char* position=strstr(path,"/?");
	if(position!=0){
		if(isfile!=0){
			*position='\0';
		}else{
			position++;
			*position='\0';
		}
		
	}

    // Construct the full file path
    urldecode(path);
	
	char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", docroot, path);

    // Open the requested file
    FILE *file = fopen(full_path, "rb");
    if (file == NULL) {
        perror("fopen");
        send(client_socket, NOT_FOUND_RESPONSE, strlen(NOT_FOUND_RESPONSE), 0);
        close(client_socket);
        return;
    }

    

    

    struct stat path_stat;
    if (stat(full_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
        if (path[strlen(path) - 1] != '/') {
            char response_header[1024];
            snprintf(response_header, sizeof(response_header), "HTTP/1.1 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", path);
            send(client_socket, response_header, strlen(response_header), 0);
            close(client_socket);
            return;
        } else {
			char ppp[500];
			snprintf(ppp, sizeof(ppp), "%s%s", full_path, "index.html");
			FILE *f = fopen(ppp, "rb");
			if (f == NULL) {
        
				char response_header[1024];
				snprintf(response_header, sizeof(response_header), 
					"HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\n<html><body><h1>403 Forbidden</h1></body></html>\n");
				send(client_socket, response_header, strlen(response_header), 0);
				close(client_socket);
				return;

			}
        }
        
    }

   

    // Send HTTP response header
    char response_header[1024];
    const char *content_type = get_content_type(full_path);

    fseek(file, 0, SEEK_END);
    long content_length = ftell(file);
    rewind(file);

    struct stat st;
    stat(full_path, &st);
    time_t last_modified_time = st.st_mtime;

    // Format Last-Modified time
    char last_modified_str[256];
    strftime(last_modified_str, sizeof(last_modified_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&last_modified_time));


    // Get the current date for the Date header
    time_t current_time = time(NULL);
    char date_str[256];
    strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&current_time));

    snprintf(response_header, sizeof(response_header), 
        "HTTP/1.1 200 OK\r\n"
         "Content-Type: %s\r\n"
         "Content-Length: %ld\r\n"
         "Accept-Ranges: bytes\r\n"
         "Server: demo\r\n" // Change YourServerName to your server name
         "\r\n",
         content_type, (long)content_length);
    send(client_socket, response_header, strlen(response_header), 0);

    // Send the file content
    char file_buffer[7020];
    ssize_t bytes_read;
    while ((bytes_read = fread( file_buffer,1, sizeof(file_buffer),file)) > 0) {
        
       send(client_socket, file_buffer, bytes_read, 0);
    }

    // Close the file and the client socket
    fclose(file);
    close(client_socket);
}

int main(int argc, char *argv[]) {
	int s;
	struct sockaddr_in sin;

	if(argc > 1) { port_http  = strtol(argv[1], NULL, 0); }
	if(argc > 2) { if((docroot = strdup(argv[2])) == NULL) errquit("strdup"); }
	if(argc > 3) { port_https = strtol(argv[3], NULL, 0); }

	if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) errquit("socket");

	do {
		int v = 1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
	} while(0);

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);
	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0) errquit("bind");
	if(listen(s, SOMAXCONN) < 0) errquit("listen");


	do {
		int c;
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);

		if((c = accept(s, (struct sockaddr*) &csin, &csinlen)) < 0) {
			perror("accept");
			continue;
		}
        


		handle_request(c);
	} while(1);

	return 0;
}