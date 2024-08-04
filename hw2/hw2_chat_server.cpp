#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string>
#include <map>
#include<vector>
#include <algorithm>
#include <cstring>
#include <pthread.h>
#include<iostream>


#define BUFFER_SIZE 512
#define MAX_EVENTS  15000
#define MAX_CHAT_HISTORY 10

typedef struct user_t {
    char                    username[20];
    struct sockaddr_in      cliaddr;
    char    password[20];
    int state;
    char show_state[20];
    int room_number;
} user_t;

std::map<int, user_t*>   connections_map;
user_t userlist[1001];
int total_user;
typedef struct chat_room_t {
    int number;
    char creator[20];
    std::vector<std::string> chat_history;
    std::vector<int> member;
    int pin;
} chat_room_t;

std::map<int, chat_room_t> chat_rooms;

int handle_read(epoll_event* event, int nfds, int to_handle,int epfd);

int broadcast(epoll_event* event, int nfds, char *msg, int len);

void sig_handler(int signum){

}

int handle_register(int connfd, const char* username, const char* password) {
    // Check for missing or redundant parameters
    if (strlen(username) == 0 || strlen(password) == 0) {
        char usernameUsedMessage[] = "Username is already used.\n";
        send(connfd,usernameUsedMessage , sizeof(usernameUsedMessage), MSG_DONTWAIT);
        return -1;
    }

    // Check if the username already exists
    for (const auto& user : userlist) {
        if (strcmp(user.username, username) == 0) {
            char usernameUsedMessage[] = "Username is already used.\n";
            send(connfd,usernameUsedMessage , sizeof(usernameUsedMessage), MSG_DONTWAIT);
            return -1;
        }
    }

    // Register the user
    user_t user_info;
    strncpy(user_info.username, username, 20);
    
    user_info.cliaddr = connections_map[connfd]->cliaddr;
    strncpy(user_info.password, password, 20);
    user_info.state=0;
    strncpy(user_info.show_state,"offline",20);
    userlist[total_user]=user_info;
    total_user++;
    // Send a success message
    char successMSG[]="Register successfully.\n";
    send(connfd, successMSG,sizeof(successMSG) , MSG_DONTWAIT);

    return 0;
}

int handle_login(int connfd, const char* username, const char* password) {
    // Check for missing or redundant parameters
    if (strlen(username) == 0 || strlen(password) == 0) {
        char loginErrorMsg[] = "Usage: login <username> <password>\n";
        send(connfd, loginErrorMsg, sizeof(loginErrorMsg), MSG_DONTWAIT);
        return -1;
    }

    
    bool foundUser = false;
    for (auto& user : userlist) {
        if (strcmp(user.username, username) == 0) {
            foundUser = true;
            // Check if the password is correct
            if (strcmp(user.password, password) == 0) {
                // Check if the account is already logged in by another client
               
                if (user.state == 1 ) {
                    char alreadyLoggedInMsg[] = "Please logout first.\n";
                    send(connfd, alreadyLoggedInMsg, sizeof(alreadyLoggedInMsg), MSG_DONTWAIT);
                    return -1;
                }
                
                char welcomeMsg[50];
                user.state = 1;
                strcpy(user.show_state,"online");
                user.room_number=-1;
                connections_map[connfd]=&user;
                sprintf(welcomeMsg, "Welcome, %s.\n", username);
                send(connfd, welcomeMsg, strlen(welcomeMsg), MSG_DONTWAIT);
                return 0;
            } else {
                // Incorrect password
                char loginFailedMsg[] = "Login failed.\n";
                send(connfd, loginFailedMsg, sizeof(loginFailedMsg), MSG_DONTWAIT);
                return -1;
            }
        }
    }
    if (!foundUser) {
        char loginFailedMsg[] = "Login failed.\n";
        send(connfd, loginFailedMsg, sizeof(loginFailedMsg), MSG_DONTWAIT);
        return -1;
    }



    return 0;
}

int handle_logout(int connfd) {
    // Check if the client has logged in
    auto userIter = connections_map.find(connfd);
    if (userIter != connections_map.end()) {
        
        if(userIter->second->state == 0){
            char notLoggedInMsg[] = "Please login first.\n";
            send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
            return -1;
        }
        // Successful logout
        char logoutMsg[50];
        sprintf(logoutMsg, "Bye, %s.\n", userIter->second->username);
        send(connfd, logoutMsg, strlen(logoutMsg), MSG_DONTWAIT);

        

        // Update user state to indicate logout
        
        strcpy(userIter->second->show_state,"offline");
        userIter->second->state=0;
        userIter->second->room_number=-1;
        

        return 0;
    }else {
        // Client has not logged in
        char notLoggedInMsg[] = "Please login first.\n";
        send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
        return -1;
    }
}
int handle_exit(int connfd,int epfd) {
    // Check if the client has logged out
    auto userIter = connections_map.find(connfd);
    if (userIter != connections_map.end()) {
        

        if(userIter->second->state==1){
            userIter->second->state=0;
            strcpy(userIter->second->show_state,"offline");
            char logoutMsg[50];
            sprintf(logoutMsg, "Bye, %s.\n", userIter->second->username);
            send(connfd, logoutMsg, strlen(logoutMsg), MSG_DONTWAIT);
        }
        

        // Remove user from the map
        connections_map.erase(connfd);

        // Close the connection
        
    }
    
    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
    close(connfd);

    return 0; 
}

int handle_whoami(int connfd) {
    // Check if the client has logged in
    auto userIter = connections_map.find(connfd);
    if (userIter != connections_map.end()) {
        if (userIter->second->state == 0) {
            char notLoggedInMsg[] = "Please login first.\n";
            send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
            return -1;
        }

        // Show the username
        char whoamiMsg[50];
        sprintf(whoamiMsg, "%s\n", userIter->second->username);
        send(connfd, whoamiMsg, strlen(whoamiMsg), MSG_DONTWAIT);

        return 0;
    } else {
        // Client has not logged in
        char notLoggedInMsg[] = "Please login first.\n";
        send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
        return -1;
    }
}

int handle_set_status(int connfd, const char* status) {
    // Check if the client has logged in
    auto userIter = connections_map.find(connfd);
    if (userIter != connections_map.end()) {
        if (userIter->second->state == 0) {
            char notLoggedInMsg[] = "Please login first.\n";
            send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
            return -1;
        }

        
        // Check if the status is valid
        if (strncmp(status, "online",6) == 0 || strncmp(status, "offline",7) == 0 || strncmp(status, "busy",4) == 0) {
            
            strcpy(userIter->second->show_state, status);
            userIter->second->show_state[strlen(status)-1]='\0';
            
            // Show success message
            char setStatusSuccessMsg[50];
            sprintf(setStatusSuccessMsg, "%s %s\n",userIter->second->username, userIter->second->show_state);
            send(connfd, setStatusSuccessMsg, strlen(setStatusSuccessMsg), MSG_DONTWAIT);

            // Trigger list-user for broadcasting updated user list

            return 0;
        } else {
            // Invalid status
            char setStatusFailedMsg[] = "set-status failed\n";
            send(connfd, setStatusFailedMsg, sizeof(setStatusFailedMsg), MSG_DONTWAIT);
            return -1;
        }
    } else {
        // Client has not logged in
        char notLoggedInMsg[] = "Please login first.\n";
        send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
        return -1;
    }
}
int handle_list_user(int connfd) {
    // Check if the client has logged in
    auto userIter = connections_map.find(connfd);
    if (userIter != connections_map.end()) {
        if (userIter->second->state == 0) {
            char notLoggedInMsg[] = "Please login first.\n";
            send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
            return -1;
        }

        // Sort users alphabetically
        std::vector<std::string> userStatusList;
        for (int i=0;i<total_user;i++) {
            std::string userStatus = userlist[i].username + std::string(" ") + userlist[i].show_state;
            userStatusList.push_back(userStatus);
        }
        std::sort(userStatusList.begin(), userStatusList.end());

        // Display the list to the user
        for (const auto& userStatus : userStatusList) {
            send(connfd, userStatus.c_str(), userStatus.length(), MSG_DONTWAIT);
            send(connfd, "\n", 1, MSG_DONTWAIT);
        }

        return 0;
    } else {
        char notLoggedInMsg[] = "Please login first.\n";
        send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
        return -1;
    }
}

void broadcast_room(int room_number, char *msg, int len) {
    for (const auto &user : connections_map) {
        if (user.second->state == 1 && user.second->room_number == room_number) {
            send(user.first, msg, len, MSG_DONTWAIT);
        }
    }
}

void send_chat_history(int connfd, int room_number) {
    auto roomIter = chat_rooms.find(room_number);
    if (roomIter != chat_rooms.end()) {
        // Send the latest 10 records of chat history
        int start_index = std::max(0, static_cast<int>(roomIter->second.chat_history.size()) - MAX_CHAT_HISTORY);
        for (int i = start_index; i < roomIter->second.chat_history.size(); ++i) {
            send(connfd, roomIter->second.chat_history[i].c_str(), roomIter->second.chat_history[i].length(), MSG_DONTWAIT);
        }
    }
}

int handle_enter_chat_room(int connfd, int room_number) {
    // Check if the client has logged in
    auto userIter = connections_map.find(connfd);
    if (userIter == connections_map.end() || userIter->second->state == 0) {
        char notLoggedInMsg[] = "Please login first.\n";
        send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
        return -1;
    }

    // Check for missing or redundant parameters
    if (room_number <= 0 || room_number > 100) {
        char enterRoomUsage[50];
        sprintf(enterRoomUsage, "Number %d is not valid.\n", room_number);
        send(connfd, enterRoomUsage, strlen(enterRoomUsage), MSG_DONTWAIT);
        return -1;
    }

    auto roomIter = chat_rooms.find(room_number);
    
    
    if (roomIter == chat_rooms.end()) {
        userIter->second->room_number=room_number;
        chat_room_t new_room;
        new_room.number = room_number;
        strncpy(new_room.creator, userIter->second->username, 20);
        new_room.member.push_back(connfd);
        new_room.pin=-1;
        chat_rooms[room_number] = new_room;

        // Notify the client
        char roomCreatedMsg[128];
        sprintf(roomCreatedMsg, "Welcome to the public chat room.\nRoom number: %d\nOwner: %s\n", room_number,
                userIter->second->username);
        send(connfd, roomCreatedMsg, strlen(roomCreatedMsg), MSG_DONTWAIT);
    } else {
        // Notify the client
        char roomExistsMsg[128];
        sprintf(roomExistsMsg, "Welcome to the public chat room.\nRoom number: %d\nOwner: %s\n", room_number,
                roomIter->second.creator);
        send(connfd, roomExistsMsg, strlen(roomExistsMsg), MSG_DONTWAIT);
        
        roomIter->second.member.push_back(connfd);
        // Notify all clients in the chat room
        char roomEnterMsg[128];
        sprintf(roomEnterMsg, "%s had enter the chat room.\n", connections_map[connfd]->username);
        broadcast_room(room_number, roomEnterMsg, strlen(roomEnterMsg));
        connections_map[connfd]->room_number=room_number;
        
        // Send chat history to the new client
        send_chat_history(connfd, room_number);
    }

    return 0;
}

void list_chat_rooms(int connfd) {
    for (const auto& room : chat_rooms) {
        char roomInfo[128];
        sprintf(roomInfo, "%s %d\n", room.second.creator, room.second.number);
        send(connfd, roomInfo, strlen(roomInfo), MSG_DONTWAIT);
    }
}

int handle_close_chat_room(int connfd, int room_number) {
    // Check if the client has logged in
    auto userIter = connections_map.find(connfd);
    if (userIter == connections_map.end() || userIter->second->state == 0) {
        char notLoggedInMsg[] = "Please login first.\n";
        send(connfd, notLoggedInMsg, sizeof(notLoggedInMsg), MSG_DONTWAIT);
        return -1;
    }

    // Check if the user is the owner of the chat room
    auto roomIter = chat_rooms.find(room_number);
    if (roomIter == chat_rooms.end()) {
        char roomNotExistMsg[50];
        sprintf(roomNotExistMsg, "Chat room %d does not exist.\n", room_number);
        send(connfd, roomNotExistMsg, strlen(roomNotExistMsg), MSG_DONTWAIT);
        return -1;
    }

    if (strcmp(userIter->second->username, roomIter->second.creator) != 0) {
        char notOwnerMsg[50];
        sprintf(notOwnerMsg, "Only the owner can close this chat room.\n");
        send(connfd, notOwnerMsg, strlen(notOwnerMsg), MSG_DONTWAIT);
        return -1;
    }

    // Close the chat room
    char roomClosedMsg[50];
    sprintf(roomClosedMsg, "Chat room %d was closed.\n", room_number);
    send(connfd, roomClosedMsg, strlen(roomClosedMsg), MSG_DONTWAIT);


    // Notify all clients in the chat room
    char roomClosedBroadcastMsg[70];
    sprintf(roomClosedBroadcastMsg, "Chat room %d was closed.\n", room_number);
    broadcast_room(room_number, roomClosedBroadcastMsg, strlen(roomClosedBroadcastMsg));
    char pa[3]="% ";
    broadcast_room(room_number, pa, strlen(pa));


    for (auto& user : connections_map) {
        if (user.second->room_number == room_number) {
            user.second->room_number = -1;
        }
    }

    chat_rooms.erase(roomIter);
    return 0;
}

int handle_chat_message(int connfd, const char* message) {
    auto userIter = connections_map.find(connfd);
    if (userIter != connections_map.end() ) {
        if(userIter->second->state == 1 && userIter->second->room_number != -1){
            int room_number = userIter->second->room_number;
            char formattedMessage[512];
            
            snprintf(formattedMessage, sizeof(formattedMessage), "[%s]: %s", userIter->second->username, message);
            broadcast_room(room_number, formattedMessage, strlen(formattedMessage));

            // Store the message in the chat history
            auto roomIter = chat_rooms.find(room_number);
            if (roomIter != chat_rooms.end()) {
                roomIter->second.chat_history.push_back(formattedMessage);
            }


            return 0;

        }else {
            char notInRoomMsg[50];
            sprintf(notInRoomMsg,"User %s is not in room. %d %d\n", userIter->second->username, userIter->second->state,userIter->second->room_number);
            send(connfd, notInRoomMsg, strlen(notInRoomMsg), MSG_DONTWAIT);
            return -1;
        }
        
    } else{
        char notInRoomMsg[50];
        sprintf(notInRoomMsg,"Didn't find connection\n");
        send(connfd, notInRoomMsg, strlen(notInRoomMsg), MSG_DONTWAIT);
        return -1;

    }
}



int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <Port>\n", argv[0]);
        return -1;
    }

    int                     listenfd, connfd;
    int                     epfd;
    int                     online_users = 0;
    struct sockaddr_in      servaddr;
    struct sockaddr_in      cliaddr;
    struct epoll_event      ev;
    struct epoll_event      *events = (struct epoll_event*) malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    
    struct sigaction        sigac   = {sig_handler};
    sigaction(SIGPIPE, &sigac, NULL);
    
    int reuse = 1;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(atoi(argv[1]));

    bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
    listen(listenfd, MAX_EVENTS);

    epfd = epoll_create(MAX_EVENTS);
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    
    total_user=0;
    while (1) {
       
        int nfds = epoll_wait(epfd, events, 20, -1);
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listenfd) {    // Newly connection
                int clilen = sizeof(cliaddr);
                connfd = accept(listenfd, (struct sockaddr*) &cliaddr, (socklen_t*) &clilen);
                if (connfd < 0) {
                    printf("Error accepting connection");
                    return -1;
                }

                char buff[0x90] = {0};
                inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff));
		        //printf("client connected from %s:%d\n", buff, cliaddr.sin_port);

                // Add the new connection to the epoll
                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);


                // generate user information and add to map
                user_t user_info;
                char name[50];
                sprintf(name, "temp%d", rand() % 100000);
                //strncpy(user_info.username, name, 20);
                user_info.cliaddr = cliaddr;
                user_info.state=0;
                user_info.room_number=-1;
                connections_map[connfd] = &user_info;

                
                char bnnr[1024];
                sprintf(bnnr, "*********************************\n** Welcome to the Chat server. **\n*********************************\n% ");
                send(connfd, bnnr, strlen(bnnr), MSG_DONTWAIT);

                if(connections_map[connfd]->room_number==-1){
                    send(connfd, "% ", 2, MSG_DONTWAIT);   
                }
                
            }
            else if (events[i].events & EPOLLIN) {  // Readable
                connfd = events[i].data.fd;
                if (connfd < 0) {
                    continue;
                }
                char buff[512] = {0};
                if (handle_read(events, nfds, connfd,epfd) < 0) {
                    
                    // client disconnected
                    char buff[0x90] = {0};
                    
                    //printf("client disconnected\n");

                    online_users--;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
                    close(connfd);
                }else if(connections_map[connfd]->room_number==-1){
                    send(connfd, "% ", 2, MSG_DONTWAIT);   
                }          
            }
            
        }
    }

    free(events);

    return 0;
}


int handle_read(epoll_event* event, int nfds, int to_handle,int epfd) {
    char buff[512]   = {0};
    int nread;
    nread = read(to_handle, buff, 512);
    if (nread == 0) {   // client has disconnected.
        return -1;
    }


    auto userIter = connections_map.find(to_handle);
    if (userIter == connections_map.end()) {
        return -1; // Connection not found
    }
    if (userIter->second->room_number != -1){
        int room_number=userIter->second->room_number;
        if(buff[0]=='/'){
            if(strncmp(buff,"/exit-chat-room",15)==0){
                char left[50];
                sprintf(left,"%s had left the chat room.\n",userIter->second->username);
                userIter->second->room_number=-1;
                broadcast_room(room_number,left,strlen(left));
               
            }
            else if (strncmp(buff, "/pin",4) == 0) {
                // Handle the pin message
                char* space = strchr(buff, ' ');
                if (space != NULL) {
                    *space = '\0';
                    char* msg = buff+5;
                    if(chat_rooms[room_number].pin!=-1){
                        chat_rooms[room_number].chat_history.erase(chat_rooms[room_number].chat_history.begin()+chat_rooms[room_number].pin);
                    }
                    std::vector<std::string> filterWords = {"==","Superpie", "hello", "Starburst Stream", "Domain Expansion"};
                    for(auto &word:filterWords){
                        char* filter = strcasestr(msg,word.c_str());
                        if(filter!=NULL){
                            int position = filter - msg;
                            std::fill_n(msg+position, word.length(), '*');
                        }
                    }
                    chat_rooms[room_number].pin=chat_rooms[room_number].chat_history.size();
                    char pinMsg[200];
                    snprintf(pinMsg, sizeof(pinMsg), "Pin -> [%s]: %s", userIter->second->username, msg);
                    broadcast_room(room_number, pinMsg, strlen(pinMsg));
                    chat_rooms[room_number].chat_history.push_back(pinMsg);
                    
                } else {
                    char setStatusUsage[] = "Usage: /pin <message>\n";
                    send(to_handle, setStatusUsage, sizeof(setStatusUsage), MSG_DONTWAIT);
                }
                 
            }
            else if (strcmp(buff, "/delete-pin\n") == 0) {
                // Handle delete pin message
                if (chat_rooms[room_number].pin==-1) {
                    char noPinMsg[50];
                    snprintf(noPinMsg, sizeof(noPinMsg), "No pin message in chat room %d\n", room_number);
                    send(to_handle, noPinMsg, strlen(noPinMsg), MSG_DONTWAIT);
                } else {
                    chat_rooms[room_number].chat_history.erase(chat_rooms[room_number].chat_history.begin()+chat_rooms[room_number].pin);
                    chat_rooms[room_number].pin=-1;
                    
                }
            }
            else if (strcmp(buff, "/list-user\n") == 0) {
                
                std::vector<std::string> userStatusList;
                for (int i=0;i<total_user;i++) {
                    if(userlist[i].state==1&&userlist[i].room_number==room_number){
                        std::string userStatus = userlist[i].username + std::string(" ") + userlist[i].show_state;
                        userStatusList.push_back(userStatus);
                    }
                }
                std::sort(userStatusList.begin(), userStatusList.end());

                // Display the list to the user
                for (const auto& userStatus : userStatusList) {
                    send(to_handle, userStatus.c_str(), userStatus.length(), MSG_DONTWAIT);
                    send(to_handle, "\n", 1, MSG_DONTWAIT);
                }
                
            }
            else{
                char error[]="Error: Unknown command\n";
                send(to_handle, error,sizeof(error) , MSG_DONTWAIT);

            }
        }
        else{
            std::vector<std::string> filterWords = {"==","Superpie", "hello", "Starburst Stream", "Domain Expansion"};
            for(auto &word:filterWords){
                char* filter = strcasestr(buff,word.c_str());
                if(filter!=NULL){
                    int position = filter - buff;
                    std::fill_n(buff+position, word.length(), '*');
                }

            }
            handle_chat_message(to_handle,buff);
        }
    }
    
    else if (strncmp(buff, "register", 8) == 0) {
        char* space = strchr(buff+9, ' ');
        if (space != NULL) {
            *space = '\0';
            const char* username = buff+9;
            const char* password = space + 1;
            handle_register(to_handle, username, password);
        } else {
            char usage[]="Usage: register <username> <password>\n";
            send(to_handle, usage,sizeof(usage) , MSG_DONTWAIT);
        }
    }
    else if (strncmp(buff, "login", 5) == 0) {
        char* space = strchr(buff+6, ' ');
        if (space != NULL) {
            *space = '\0';
            const char* username = buff+6;
            const char* password = space + 1;
            handle_login(to_handle, username, password);
        } else {
            char loginUsage[] = "Usage: login <username> <password>\n";
            send(to_handle, loginUsage, sizeof(loginUsage), MSG_DONTWAIT);
        }
    }  
    else if (strncmp(buff, "logout", 6) == 0) {
        if(strlen(buff)>7){
            char logoutUsage[] = "Usage: logout\n";
            send(to_handle, logoutUsage, sizeof(logoutUsage), MSG_DONTWAIT);
        }else{
            handle_logout(to_handle);
        }
    }
    else if (strncmp(buff, "exit", 4) == 0) {
        if (strlen(buff) > 5) {
            char exitUsage[] = "Usage: exit\n";
            send(to_handle, exitUsage, sizeof(exitUsage), MSG_DONTWAIT);
        } else {
            handle_exit(to_handle,epfd);
            return -1;  // Terminate the connection after handling exit
        }
    }
    else if (strncmp(buff, "whoami", 6) == 0) {
        if (strlen(buff) > 7) {
            char whoamiUsage[] = "Usage: whoami\n";
            send(to_handle, whoamiUsage, sizeof(whoamiUsage), MSG_DONTWAIT);
        } else {
            handle_whoami(to_handle);
        }
    }
    else if (strncmp(buff, "set-status", 10) == 0) {
        char* space = strchr(buff, ' ');
        if (space != NULL) {
            *space = '\0';
            const char* status = buff+11;
            handle_set_status(to_handle, status);
        } else {
            char setStatusUsage[] = "Usage: set-status <status>\n";
            send(to_handle, setStatusUsage, sizeof(setStatusUsage), MSG_DONTWAIT);
        }
    }
    else if (strncmp(buff, "list-user", 9) == 0) {
        if (strlen(buff) > 10) {
            char listUserUsage[] = "Usage: list-user\n";
            send(to_handle, listUserUsage, sizeof(listUserUsage), MSG_DONTWAIT);
        } else {
            handle_list_user(to_handle);
        }
    }
    else if (strncmp(buff, "enter-chat-room", 15) == 0) {
        int room_number;
        if (sscanf(buff, "enter-chat-room %d", &room_number) == 1) {
            handle_enter_chat_room(to_handle, room_number);
        } else {
            char enterRoomUsage[] = "Usage: enter-chat-room <number>\n";
            send(to_handle, enterRoomUsage, sizeof(enterRoomUsage), MSG_DONTWAIT);
        }
    }
    else if (strncmp(buff, "list-chat-room", 14) == 0) {
        if (strlen(buff) > 15) {
            char listChatRoomUsage[] = "Usage: list-chat-room\n";
            send(to_handle, listChatRoomUsage, sizeof(listChatRoomUsage), MSG_DONTWAIT);
        } else {
            // Handle the list-chat-room command
            list_chat_rooms(to_handle);
        }
    }
    else if (strncmp(buff, "close-chat-room", 15) == 0) {
        int room_number;
        if (sscanf(buff, "close-chat-room %d", &room_number) == 1) {
            handle_close_chat_room(to_handle, room_number);
        } else {
            char closeRoomUsage[] = "Usage: close-chat-room <number>\n";
            send(to_handle, closeRoomUsage, sizeof(closeRoomUsage), MSG_DONTWAIT);
        }
    }
    else {
        char unknownCommandMsg[] = "Error: Unknown command\n";
        send(to_handle, unknownCommandMsg, sizeof(unknownCommandMsg), MSG_DONTWAIT);
    }
    
   
    return 0;
}

int broadcast(epoll_event* event, int nfds, char *msg, int len) {
    std::map<int, user_t*>::iterator it;
    for (it = connections_map.begin(); it != connections_map.end(); it++) {
        send(it->first, msg, len, MSG_DONTWAIT);
    }
    return 0;
}