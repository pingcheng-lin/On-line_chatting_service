#include <netinet/in.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip> 
#include <cmath> 
#include <cstdlib> 
#include <cstring> 
#include <ctime> 
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <algorithm>
#include <sstream>
#define QLEN 32 // maximum connection queue length
#define BUFSIZE 4096

using namespace std;

struct data{
    int fd;
    string name;
    thread t;
    string ip;
};

vector<struct data> client_data;
stringstream ss;
mutex mtx, mmtx, mmmtx;

void output(stringstream &ss) {
    lock_guard<mutex> guard(mmtx);
    cout << ss.str();
    fflush(stdout);
    ss.str("");
    ss.clear();
}
void close_fd() {
    
}
void relay(int fd, string my_name){
    char buf[BUFSIZE];
    int newfd;
    while(1) {
        ss << "=== "  << my_name << " waiting input===" << endl;
        output(ss);
        
        if(recv(fd, &buf, sizeof(buf), 0) < 0) { 
            perror("first recv");
            exit(1);
        }
        ss << my_name << " choose to " << buf << ".\n";
        output(ss);
        if(!strcmp(buf, "connect")) {
            for(vector<struct data>::iterator it = client_data.begin(); it != client_data.end(); it++) {
                if(send(it->fd, "connect", sizeof(buf), 0) < 0) { 
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
            ss << "sentence read:" << buf << endl;
            output(ss);
            vector<struct data>::iterator it;
            vector<string>::iterator it2;
            for(it = client_data.begin(); it != client_data.end(); it++) {
                
                if(find(client_to_sent.begin(), client_to_sent.end(), it->name) != client_to_sent.end()) {
                    if(send(it->fd, "chat", sizeof(buf), 0) < 0) { 
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
                    ss << "sentence read:" << buf << endl;
                    output(ss);
                }
            }
            //check stranger
            for(it2 = client_to_sent.begin(); it2 != client_to_sent.end(); it2++) {
                for(it = client_data.begin(); it != client_data.end(); it++) {
                    if(*it2 == it->name)
                        break;
                }
                if(*it2 == it->name)
                    continue;
                if(send(fd, "chat", sizeof(buf), 0) < 0) { 
                    perror("send");
                    exit(1);
                }
                if(send(fd, "stranger", sizeof(buf), 0) < 0) { 
                    perror("send");
                    exit(1);
                }
                if(send(fd, (*it2).c_str(), sizeof(buf), 0) < 0) {  //send name
                    perror("send");
                    exit(1);
                }
            }
        }
        else if(!strcmp(buf, "bye")) {
            ss << my_name<< " is in bye.\n";
            output(ss);
            vector<struct data>::iterator current_iter; 
            for(vector<struct data>::iterator it = client_data.begin(); it != client_data.end(); it++) {
                if(send(it->fd, "bye", sizeof(buf), 0) < 0) { 
                    perror("send bye");
                    exit(1);
                }
                ss << "Send to " << it->name << endl;
                output(ss);
                if(send(it->fd, my_name.c_str(), sizeof(buf), 0) < 0) { 
                    perror("send bye name");
                    exit(1);
                }
                if(my_name == it->name) {
                    ss << "who i detach: " << it->name << endl;
                    output(ss);
                    it->t.detach();
                }
            }
        }
    }
}
