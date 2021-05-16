#include "header.hpp"
int main() {
    //Get the file descriptor
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server_fd < 0) {
        perror("socket"); 
        exit(1);
    }

    struct sockaddr_in srv; //used by bind()
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
    

    //Accept requests
    struct sockaddr_in client; //used by accept()
    int newfd; //returned by accept()
    int cli_len = sizeof(client); //used by accept()
    char buf[BUFSIZE];
    while(1) {
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
        lock_guard<mutex> guard(mtx);
        client_online.push_back({newfd, buf, move(temp), inet_ntoa(client.sin_addr), true});
    }
    for(vector<struct data>::iterator it = client_online.begin(); it != client_online.end(); it++) {
        if(it->t.joinable())
            it->t.join();
    }
    close(server_fd);
    return 0;
}