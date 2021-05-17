#include <netinet/in.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <sstream>
#include <csignal>

#define QLEN 32 // maximum connection queue length
#define BUFSIZE 4096

using namespace std;

struct data{
    int fd;
    string name;
    thread t;
    string ip;
    bool active;
};

vector<struct data> client_online;
vector<struct data> client_offline;
stringstream ss;
mutex mtx, mmtx, mmmtx;

void output(stringstream &ss) {
    lock_guard<mutex> lck(mmtx);
    cout << ss.str();
    fflush(stdout);
    ss.str("");
    ss.clear();
}

int server_fd;
void close_fd(int param) {
    cout << "\nShutdown server.\n";
    close(server_fd);
    exit(0);//cant work?
}

void relay(int fd, string my_name){
    char buf[BUFSIZE];
    int newfd;
    while(1) {
        ss << "===waiting input===" << endl;
        output(ss);
        //memset(buf, 0, 512*sizeof(buf[0]));
        if(recv(fd, &buf, sizeof(buf), 0) < 0) { 
            perror("first recv");
            exit(1);
        }
        //connect section
        if(!strcmp(buf, "connect")) {
            for(vector<struct data>::iterator it = client_online.begin(); it != client_online.end(); it++) {
                if(send(it->fd, "connect\0", sizeof(buf), 0) < 0) { 
                    perror("send connect");
                    exit(1);
                }
                if(send(it->fd, my_name.c_str(), sizeof(buf), 0) < 0) { //send user's name
                    perror("send name");
                    exit(1);
                }
                if(send(it->fd, it->ip.c_str(), sizeof(buf), 0) < 0) { //send user's IP address
                    perror("send address");
                    exit(1);
                }
                if(it->name == my_name) {
                    ss << "<User " + it->name + " is on-line, IP address: " + it->ip + ".>\n";
                    output(ss);
                    continue;
                }
            }
        }
        //chat section
        else if(!strcmp(buf, "chat")) {
            vector<string> client_to_sent;
            while(1) {
                if(recv(fd, &buf, sizeof(buf), 0) < 0) { //read user's name
                    perror("recv");
                    exit(1);
                }
                if(!strcmp(buf, "=terminate="))
                    break;
                client_to_sent.push_back((string)buf);
            }
            if(read(fd, &buf, sizeof(buf)) < 0) { //read sentence
                perror("read");
                exit(1);
            }
            ss << my_name << ": " << buf << endl;
            output(ss);
            vector<struct data>::iterator it;
            vector<string>::iterator it2;
            for(it = client_online.begin(); it != client_online.end(); it++) {
                
                if(find(client_to_sent.begin(), client_to_sent.end(), it->name) != client_to_sent.end()) {
                    if(send(it->fd, "chat\0", sizeof(buf), 0) < 0) { 
                        perror("send");
                        exit(1);
                    }
                    if(send(it->fd, my_name.c_str(), sizeof(buf), 0) < 0) {  //send name
                        perror("send");
                        exit(1);
                    }
                    if(send(it->fd, buf, sizeof(buf), 0) < 0) {  //send sentence
                        perror("send");
                        exit(1);
                    }
                    ss << my_name << " => " << it->name << endl;
                    output(ss);
                }
            }
            //check stranger
            for(it2 = client_to_sent.begin(); it2 != client_to_sent.end(); it2++) {
                for(it = client_online.begin(); it != client_online.end(); it++) {
                    if(*it2 == it->name)
                        break;
                }
                if(*it2 == it->name)
                    continue;
                if(send(fd, "chat\0", sizeof(buf), 0) < 0) { 
                    perror("send");
                    exit(1);
                }
                if(send(fd, "stranger\0", sizeof(buf), 0) < 0) { 
                    perror("send");
                    exit(1);
                }
                if(send(fd, (*it2).c_str(), sizeof(buf), 0) < 0) {  //send name
                    perror("send");
                    exit(1);
                }
                ss << "<User " << (string)(*it2) << " does not exist.>\n";
                output(ss);
            }
        }
        //bye section
        else if(!strcmp(buf, "bye")) {
            vector<struct data>::iterator current_iter;
            vector<struct data>::iterator it;
            for(it = client_online.begin(); it != client_online.end(); it++) {
                if(send(it->fd, "bye\0", sizeof(buf), 0) < 0) {
                    perror("send bye");
                    exit(1);
                }
                if(send(it->fd, my_name.c_str(), sizeof(buf), 0) < 0) { //send who is leaving
                    perror("send bye name");
                    exit(1);
                }
                if(my_name == it->name)
                    current_iter = it;
            }
            mmmtx.lock();
            if(my_name == current_iter->name) {
                ss << current_iter->name << " become inactive.\n"
                   << "===waiting input===\n";
                output(ss);
                current_iter->t.detach();//why
                close(current_iter->fd);
                client_online.erase(current_iter);
                mmmtx.unlock();
                break;
            }
        }
    }
}
