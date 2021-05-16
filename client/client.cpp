#include "header.hpp"
int main() {
    int client_fd;
    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(client_fd < 0) {
        perror("socket"); 
        exit(1);
    }

    cout << "===\nWelcome\n"
         << "===\nFirst, you need to 'connect' [an IP address] [a port] [name].\n"
         << "Waiting...\n===\n";

    //connect
    char buf[BUFSIZE];
    struct sockaddr_in client;
    string command, ip_address, my_name;
    int port;
    while(1) {
        cout << "Enter your input: ";
        cin >> command;
        if(command == "connect") {
            cin >> ip_address;
            client.sin_addr.s_addr = inet_addr(ip_address.c_str()); //connect: connect to IP address
            cin >> port;
            client.sin_port = htons(port); //connect: socket 'server_fd' to port
            cin >> my_name;
            client.sin_family = AF_INET;
            if(connect(client_fd, (struct sockaddr*) &client, sizeof(client)) < 0) {
                cout << "Unable to connect.\n"
                     << "Please connect again.\n"
                     << "===\n";
                continue;
            }
            if(send(client_fd, my_name.c_str(), sizeof(my_name.c_str()), 0) < 0) { 
                perror("send");
                exit(1);
            }
            if(send(client_fd, "connect", sizeof(my_name.c_str()), 0) < 0) { 
                perror("send");
                exit(1);
            }
            break;
        }
        else if(command == "bye") {
            cout << "Bye bye.\n";
            return 0;
        }
        else if(command == "help") {
            cout << "===\nInvalid Input.\n"
                 << "You need to 'connect' [an IP address] [a port] [name].\n"
                 << "Or leave by 'bye'.\n"
                 << "Waiting...\n===\n";
        }
        else {
            cout << "===\nYou need to 'connect' [an IP address] [a port] [name] first.\n"
                 << "Or leave by 'bye'.\n"
                 << "Waiting...\n===\n";
        }
    }

    cout << "Successfully connect!!!\n"
         << "===\nYou can 'chat' [users] \"[words]\" or 'bye'\n"
         << "Waiting...\n===\n";

    thread t_a(my_send, client_fd);
    thread t_b(my_recv, client_fd, my_name);

    t_s = move(t_a);
	t_r = move(t_b);

	if(t_s.joinable())
		t_s.join();
	if(t_r.joinable())
		t_r.join();
			
	return 0;
}

