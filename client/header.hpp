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

//read input from terminal. Ex: chat, bye, help
int input(vector<string> &name, string &sentence) {
    string command;
    while(1) {
        cin >> command;

        if(command == "chat") {
            while(1) {
                string temp;
                cin >> ws; //eat white
                if(cin.peek() == '<') 
                    break;
                cin >> temp;
                name.push_back(temp);
            }
            cin.ignore(1, '<');
            cin >> ws;
            getline(cin, sentence);
            return 1;
        }

        else if(command == "bye")
            return 2;

        else if(command == "help") {
            cout << "===\nHelp: You can input 'chat' [users] >[words], 'bye', or 'help'.\n"
                 << "Waiting...\n===\n";
            fflush(stdout);
        }

        else {
            cout << "===\nWrong input: You can input 'chat' [users] >[words], 'bye', or 'help'.\n"
                 << "Waiting...\n===\n";
            fflush(stdout);
        }
    }
}

//to send data to server
void my_send(int client_fd) {
    while(1) {
        char buf[BUFSIZE];
        vector<string> name; //names to send string from this user
        string sentence; //string which is send(chat) by this user
        int flag = input(name, sentence);
        
        //send chat section
        if(flag == 1) {
            if(send(client_fd, "chat\0", sizeof(buf), 0) < 0) { //send what server do next
                perror("send");
                exit(1);
            }
            //send name
            vector<string>::iterator it;
            for(it = name.begin(); it != name.end(); it++) //send all name
                if(send(client_fd, (*it).c_str(), sizeof(buf), 0) < 0) {
                    perror("send");
                    exit(1);
                }
            if(send(client_fd, "=terminate=\0", sizeof(buf), 0) < 0) { //before this is names after is string (for chat)
                    perror("send");
                    exit(1);
                }
            if(send(client_fd, sentence.c_str(), sizeof(buf), 0) < 0) { //send string to chat
                perror("send");
                exit(1);
            }
        }

        //send bye section
        else if(flag == 2) {
            if(send(client_fd, "bye\0", sizeof(buf), 0) < 0) { //send what server do next
                perror("send");
                exit(1);
            }
            t_s.detach();
            break;
        }
    }
}

//to receive data from server
void my_recv(int client_fd, string my_name) {
    while(1) {
        char buf[BUFSIZE];
        if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv command
            perror("recv");
            exit(1);
        }

        //receive connect section
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

        //receive chat section
        else if(!strcmp(buf, "chat")) {
            if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv name of user who send the senetence
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

        //receive bye section
        else if(!strcmp(buf, "bye")) {
            if(recv(client_fd, &buf, sizeof(buf), 0) < 0) { //recv leaver
                perror("recv");
                exit(1);
            }
            if(my_name != (string)buf) { //if I am not the leaver
                cout << "<User " + (string)buf + " is off-line.>\n";
                fflush(stdout);
            }
            else { // if I am the leaver
                cout << "Bye bye.\n";
                fflush(stdout);
                t_r.detach();
                break;
            }
        }
    }
}