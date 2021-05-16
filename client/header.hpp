#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <thread>

#define QLEN 32 // maximum connection queue length
#define BUFSIZE 4096

using namespace std;

thread t_s;
thread t_r;

int input(vector<string> &name, string &sentence) {
    string command;
    while(1) {
        cin >> command;
        if(command == "chat") {
            while(1) {
                string temp;
                cin >> ws; //eat withe
                if(cin.peek() == '"')
                    break;
                cin >> temp;
                name.push_back(temp);
            }
            getline(cin, sentence);
            fflush(stdout);
            return 1;
        }
        else if(command == "bye")
            return 2;
        else if(command == "help") {
            cout << "===\nHelp: You can 'chat' [users] \"[words]\", or 'bye'\n"
                 << "Waiting...\n===\n";
            fflush(stdout);
        }
        else {
            cout << "===\nWrong input: You can 'chat' [users] \"[words]\", or 'bye'\n"
                 << "Waiting...\n===\n";
            fflush(stdout);
        }
    }
}
void my_send(int client_fd) {
    while(1) {
        char buf[BUFSIZE];
        vector<string> name;
        string sentence;
        int flag = input(name, sentence);
        if(flag == 1) {
            //send flag
            if(send(client_fd, "chat\0", sizeof(buf), 0) < 0) { 
                perror("send");
                exit(1);
            }
            //send name
            vector<string>::iterator it;
            for(it = name.begin(); it != name.end(); it++)
                if(send(client_fd, (*it).c_str(), sizeof(buf), 0) < 0) { 
                    perror("send");
                    exit(1);
                }
            if(send(client_fd, "=terminate=\0", sizeof(buf), 0) < 0) { 
                    perror("send");
                    exit(1);
                }
            if(send(client_fd, sentence.c_str(), sizeof(buf), 0) < 0) { 
                perror("send");
                exit(1);
            }
        }
        else if(flag == 2) {
            if(send(client_fd, "bye\0", sizeof(buf), 0) < 0) { 
                perror("send");
                exit(1);
            }
            t_s.detach();
            break;
        }
    }
}
void my_recv(int client_fd, string my_name) {
    while(1) {
        char buf[BUFSIZE];
        if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv command
            perror("recv");
            exit(1);
        }
            
        if(!strcmp(buf, "connect")) {
            if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv user's name who pop on-line
                perror("recv");
                exit(1);
            }
            string temp_name = buf;
            if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv user's IP address
                perror("recv");
                exit(1);
            }
            string addr = buf;
            if(my_name == (string)temp_name)
                continue;
            else
                cout << "<User " + temp_name + " is on-line, IP address: " + addr + ".>\n";
            fflush(stdout);
        }
        else if(!strcmp(buf, "chat")) {
            if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv name
                perror("recv");
                exit(1);
            }
            string temp_name = buf;
            if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv sentence
                perror("recv");
                exit(1);
            }
            if(temp_name == "stranger")
                cout << "<User " << (string)buf << " does not exist.>\n";
            else
                cout << temp_name << ": " << (string)buf << endl;
            fflush(stdout);
        } 
        else if(!strcmp(buf, "bye")) {
            if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv leaver
                perror("recv");
                exit(1);
            }
            if(my_name != (string)buf) {
                cout << "<User " + (string)buf + " is off-line.>\n";
                fflush(stdout);
            }
            else {
                cout << "Bye bye.\n";
                fflush(stdout);
                t_r.detach();
                break;
            }
        }
    }
}