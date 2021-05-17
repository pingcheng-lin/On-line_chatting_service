#include "header.hpp"
int main() {
    //Get the file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server_fd < 0) {
        perror("socket"); 
        exit(1);
    }

    struct sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_port = htons(1234);
    //Connect the file descriptor with the IP address and port
    if(bind(server_fd, (struct sockaddr*) &srv, sizeof(srv)) < 0) {
        perror("bind"); 
        exit(1);
    }
    
    //Wait for requests from clients
    if(listen(server_fd, QLEN) < 0) {
        perror("listen");
        exit(1);
    }
    else {
        cout << "Listening...\n";
    }
    
    signal(SIGINT, close_fd); // signal by ctrl+c

    //Accept requests
    struct sockaddr_in client;
    int newfd;
    int cli_len = sizeof(client);
    char buf[BUFSIZE];
    while(1) { //keep accept until crtl+c
        newfd = accept(server_fd, (struct sockaddr*) &client, (socklen_t*)&cli_len);
        if(newfd < 0) {
            perror("accept");
            exit(1);
        }
        if(recv(newfd, &buf, sizeof(buf), 0) < 0) { //recv name
            perror("recv");
            exit(1);
        }
        cout << "Create new socket: " << newfd << " name： " << buf << endl;
        fflush(stdout);
        thread temp(relay, newfd, buf);
        lock_guard<mutex> lck(mtx);
        //offline先尋一次同重複名子 IP避免重複
        client_online.push_back({newfd, buf, move(temp), inet_ntoa(client.sin_addr), true});
    }
    return 0;
}