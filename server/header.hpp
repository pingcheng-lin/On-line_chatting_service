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
#include <ctime>
#include <map>

#define QLEN 32 // maximum connection queue length
#define BUFSIZE 4096

using namespace std;

struct data{
    int fd;
    string name;
    thread t;
    string ip;
};

struct offline_data{
    string name;
    string ip;
    vector<string> sender;
    map<string, vector<string>> sender_message;
    map<string, vector<string>> sender_time;
};

vector<struct data> client_online;
vector<struct offline_data> client_offline;
mutex mtx, mmtx, mmmtx;

stringstream ss;
void output() {
    lock_guard<mutex> lck(mmtx);
    cout << ss.str();
    fflush(stdout);
    ss.str("");
    ss.clear();
}

int server_fd;
void close_fd(int param) {
    ss << "\nShutdown server.\n";
    output();
    close(server_fd);
    exit(0);
}

void relay(int fd, string my_name, bool is_backer){
    char buf[BUFSIZE];
    int newfd;
    while(1) {
        ss << "\n===waiting input===\n";
        output();
        //bzero(buf, sizeof(buf));
        if(recv(fd, &buf, sizeof(buf), 0) < 0) { 
            perror("first recv");
            exit(1);
        }
        
        //connect section
        if(!strcmp(buf, "connect")) { //send someone online to other users
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
                    ss << "<User " << it->name << " is on-line, IP address: " << it->ip << ".>\n";
                    output();
                }
            }

            if(is_backer) { //send history message to user who come back online
                if(send(fd, "=start_send_history=\0", sizeof(buf), 0) < 0) { //send history message
                    perror("send");
                    exit(1);
                }
                vector<struct offline_data>::iterator off_it;
                for(off_it = client_offline.begin(); off_it != client_offline.end(); off_it++)//get my iterator
                    if(off_it->name == my_name)
                        break;
                while(!off_it->sender.empty()) { //loop all sender
                    string current_sender = off_it->sender.back();
                    while(!off_it->sender_message[current_sender].empty()) { //loop all message sent by sender
                        string message = off_it->sender_message[current_sender].back();
                        off_it->sender_message[current_sender].pop_back();
                        string time = off_it->sender_time[current_sender].back();
                        off_it->sender_time[current_sender].pop_back();
                        string history_message = "<User " + current_sender + " has sent " + my_name + " a message \"" + message + "\" at " + time + ".>\n";
                        ss << history_message;
                        output();
                        if(send(fd, history_message.c_str(), sizeof(buf), 0) < 0) { //send history message
                            perror("send");
                            exit(1);
                        }
                    }
                    off_it->sender.pop_back();
                }
                if(send(fd, "=terminate_history_message=\0", sizeof(buf), 0) < 0) { //send terminate history message
                    perror("send");
                    exit(1);
                }
                client_offline.erase(off_it);
            }
            else
                if(send(fd, "=no_send_history=\0", sizeof(buf), 0) < 0) { //no need to send history message
                    perror("send");
                    exit(1);
                }
        }
        //chat section
        else if(!strcmp(buf, "chat")) {
            vector<string> client_to_sent;
            while(1) { //read who receive data and what data is it
                if(recv(fd, &buf, sizeof(buf), 0) < 0) { //read user's name
                    perror("recv");
                    exit(1);
                }
                if(!strcmp(buf, "=terminate="))
                    break;
                client_to_sent.push_back((string)buf);
            }
            if(read(fd, &buf, sizeof(buf)) < 0) { //read message
                perror("read");
                exit(1);
            }
            string message = buf;
            ss << my_name << ": " << message << endl;
            output();


            vector<struct offline_data>::iterator off_it; //add data to offline user when someone send message to
            for(off_it = client_offline.begin(); off_it != client_offline.end(); off_it++)
                if(find(client_to_sent.begin(), client_to_sent.end(), off_it->name) != client_to_sent.end()) { //search if target is in offline vector
                    off_it->sender.push_back(my_name);
                    off_it->sender_message[my_name].push_back(message);
                    time_t now = time(0);
                    tm *ltm = localtime(&now);
                    string temp_time = to_string(ltm->tm_hour) + ":" + to_string(ltm->tm_min) + " " + to_string(1900+ltm->tm_year) + "/" + to_string(1+ltm->tm_mon) + "/" + to_string(ltm->tm_mday);
                    off_it->sender_time[my_name].push_back(temp_time);
                    ss << my_name << " => " << off_it->name << "(inactive)" << endl;
                    output();
                }
            
            vector<struct data>::iterator on_it;
            vector<string>::iterator name_it;
            //check who to send and send message
            for(on_it = client_online.begin(); on_it != client_online.end(); on_it++) {
                if(find(client_to_sent.begin(), client_to_sent.end(), on_it->name) != client_to_sent.end()) {  //search if target is in online vector
                    if(send(on_it->fd, "chat\0", sizeof(buf), 0) < 0) { 
                        perror("send");
                        exit(1);
                    }
                    if(send(on_it->fd, my_name.c_str(), sizeof(buf), 0) < 0) {  //send name
                        perror("send");
                        exit(1);
                    }
                    if(send(on_it->fd, buf, sizeof(buf), 0) < 0) {  //send sentence
                        perror("send");
                        exit(1);
                    }
                    ss << my_name << " => " << on_it->name << "(active)" << endl;
                    output();
                }
            }
            
            //check stranger who not exists in the vector
            for(name_it = client_to_sent.begin(); name_it != client_to_sent.end(); name_it++) {
                string temp_name = *name_it;
                for(on_it = client_online.begin(); on_it != client_online.end(); on_it++) { //check online
                    if(temp_name == on_it->name)
                        break;
                }
                if(on_it != client_online.end() && temp_name == on_it->name)
                    continue;
                for(off_it = client_offline.begin(); off_it != client_offline.end(); off_it++) { //check offline
                    if(temp_name == off_it->name)
                        break;
                }
                if(off_it != client_offline.end() && temp_name == off_it->name) { //when stranger is actually user offline
                    if(send(fd, "chat\0", sizeof(buf), 0) < 0) { 
                        perror("send");
                        exit(1);
                    }
                    if(send(fd, "offline\0", sizeof(buf), 0) < 0) { 
                        perror("send");
                        exit(1);
                    }
                    if(send(fd, temp_name.c_str(), sizeof(buf), 0) < 0) {  //send name
                        perror("send");
                        exit(1);
                    }
                    ss << "<User " << temp_name << " is off-line. The message will be passed when he comes back.>\n"; 
                    output();
                    continue;
                }
                if(send(fd, "chat\0", sizeof(buf), 0) < 0) { //really a stranger
                    perror("send");
                    exit(1);
                }
                if(send(fd, "stranger\0", sizeof(buf), 0) < 0) { 
                    perror("send");
                    exit(1);
                }
                if(send(fd, temp_name.c_str(), sizeof(buf), 0) < 0) {  //send name
                    perror("send");
                    exit(1);
                }
                ss << "<User " << temp_name << " does not exist.>\n";
                output();
            }
        }
        //bye section
        else if(!strcmp(buf, "bye")) {
            vector<struct data>::iterator current_iter;
            vector<struct data>::iterator it; //send bye to other clients
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
            if(my_name == current_iter->name) { //move client who say bye from online to offline
                ss << current_iter->name << " become inactive.\n"
                   << "\n===waiting input===\n";
                output();
                current_iter->t.detach();
                client_offline.push_back({current_iter->name, current_iter->ip, {}, {{}}, {{}} });
                client_online.erase(current_iter);
                mmmtx.unlock();
                break;
            }
        }
    }
}
