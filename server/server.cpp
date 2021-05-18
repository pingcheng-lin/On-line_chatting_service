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
        ss << "Listening...\n";
        output();
    }
    
    signal(SIGINT, close_fd); // signal by ctrl+c

    //Accept requests
    struct sockaddr_in client;
    int newfd;
    int cli_len = sizeof(client);
    char buf[BUFSIZE];
    string my_name;
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
        my_name = buf;

        ss << "Create new socket: " << newfd << " ,name： " << buf << endl;
        output();

        //prevent duplicate user
        bool is_duplicate = false;
        for(vector<struct data>::iterator it = client_online.begin(); it != client_online.end(); it++)
            if(it->name == my_name && it->ip == inet_ntoa(client.sin_addr)) {
                is_duplicate = true;
                ss << "duplicate " << my_name << " => deny login.\n";
                output();
                if(send(newfd, "duplicate\0", sizeof(buf), 0) < 0) { //send user name
                    perror("send duplicate");
                    exit(1);
                }
                //close(newfd);
                break;
            }
        if(is_duplicate) //there is at least one duplicate
            continue;

        //handle come back user
        bool is_backer = false;
        for(vector<struct offline_data>::iterator it = client_offline.begin(); it != client_offline.end(); it++)
            if(it->name == my_name && it->ip == inet_ntoa(client.sin_addr)) {
                is_backer = true;
                if(send(newfd, "back\0", sizeof(buf), 0) < 0) { //send user name
                    perror("send");
                    exit(1);
                }
            }
        if(!is_backer && send(newfd, "normal\0", sizeof(buf), 0) < 0) {
            perror("send");
            exit(1);
        }

        thread temp(relay, newfd, my_name, is_backer);
        lock_guard<mutex> lck(mtx);

        //old or new user store data
        client_online.push_back({newfd, my_name, move(temp), inet_ntoa(client.sin_addr)});
    }
    return 0;
}