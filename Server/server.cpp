#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>

#define BUFFER_SIZE 64
#define SERVER_PORT 5823
#define FAIL_CODE -1
#define EPOLL_SIZE 50

using namespace std;

enum packet_type
{
	PT_QUIT = -1,
	PT_CONNECT = 0,
	PT_MORSE = 1
};

enum morse_type
{
	MT_EMPTY = 0,
	MT_SHORT = 1,
	MT_LONG = 2
};

enum lang_type
{
	LANG_ENGLISH = 0,
	LANG_KOREAN = 1
};

struct packet_data
{
	packet_type type;
	string name;
	morse_type morse;
	lang_type lang;
	
	string to_string()
	{
		stringstream buffer;
		
		buffer << type << endl;
		buffer << name << endl;
		buffer << morse << endl;
		buffer << lang;
		
		return buffer.str();
	}
	
	packet_data(string packet)
	{
		stringstream stream(packet);
		string buffer;
		vector<string> lines;
		while(getline(stream, buffer, '\n'))
			lines.push_back(buffer);
			
		type = static_cast<packet_type>(atoi(lines[0].c_str()));
		name = lines[1];
		morse = static_cast<morse_type>(atoi(lines[2].c_str()));
		lang = static_cast<lang_type>(atoi(lines[3].c_str()));
	}
	
	packet_data(packet_type _type, string _name, morse_type _morse, lang_type _lang) : 
		type(_type), name(_name), morse(_morse), lang(_lang) {}
};

void error_handling(string message);
void print_log(string message);

void send_packet_all(int epfd, int clnt_sock, char * data, int len);
void send_packet(int epfd, int socket, char * data, int len);

void drop_user(int epfd, int socket);

vector<int> user;

int main() {
	
	int serv_sock;
	
	// Create socket
	serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);	
	if (serv_sock == FAIL_CODE)
		error_handling("socket error");
	print_log("Success create socket");

	// Initialize server
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERVER_PORT);

	int sockOpt = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(sockOpt));
	
	// Bind server
	if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == FAIL_CODE)
		error_handling("bind error");
	print_log("Success bind server with socket discriptor " + to_string(serv_sock));
	
	// Listen server
	if (listen(serv_sock, SOMAXCONN) == FAIL_CODE)
		error_handling("listen error");
	
	int epfd = epoll_create(EPOLL_SIZE);
	epoll_event * ep_events = (epoll_event*)malloc(sizeof(epoll_event)*EPOLL_SIZE);
	epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);
	
	print_log("Start server with port " + to_string(SERVER_PORT));	
	
	packet_data packet_morse_long(PT_MORSE, "Server", MT_LONG, LANG_ENGLISH);
	packet_data packet_morse_short(PT_MORSE, "Server", MT_SHORT, LANG_ENGLISH);
	packet_data packet_morse_empty(PT_MORSE, "Server", MT_EMPTY, LANG_ENGLISH);
	
	bool running = true;
	
	while (running)
	{		
		// Detect socket event
		int event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if(event_cnt == -1)
		{
			print_log("Error epoll_wait()");
			break;
		}	
			
		for (int i = 0; i < event_cnt; i++)
		{	
			int fd = ep_events[i].data.fd;
			
			// Connect user
			if (fd == serv_sock)
			{
				int client = accept(serv_sock, nullptr, nullptr);
				event.events = EPOLLIN;
				event.data.fd = client;
				epoll_ctl(epfd, EPOLL_CTL_ADD, client, &event);
				user.push_back(client);
				print_log("Connect user with descriptor " + to_string(client));
			}
			else // Receive message
			{
				char buf[BUFFER_SIZE];
				memset(&buf, 0, BUFFER_SIZE);
				
				int bufLength = recv(ep_events[i].data.fd, buf, BUFFER_SIZE, 0);
				if (bufLength <= 0)
				{
					// Drop client - wrong packet
					drop_user(epfd, fd);
					print_log("Closed client " + to_string(fd) + ", Because wrong packet");
				}
				else
				{
					try
					{
						print_log("Receive packet from " + to_string(fd));
					
						string conv(buf);
						packet_data data(conv.substr(0, bufLength));
						
						switch(data.type)
						{
							case PT_CONNECT:
							break;
							case PT_MORSE:
								send_packet_all(epfd, -1, buf, bufLength);
							break;
							case PT_QUIT:
							break;
						}
					}
					catch(int i)
					{
						print_log("Receive error from " + to_string(fd));
					}
				}
			}
		}
	}
	
	close(serv_sock);
	close(epfd);
	
	print_log("server close");
	
	return 0;
}

void error_handling(string message)
{
	cout << message << endl;
	exit(1);
}

void print_log(string message)
{
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[64];
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
	string timestr(buffer);
	
	cout << "[" << buffer << "] " << message << endl;
}

void send_packet_all(int epfd, int clnt_sock, char * data, int len)
{
	for (unsigned i = 0; i < user.size(); i++)
		if (user[i] != clnt_sock)
			send_packet(epfd, user[i], data, len);
}

void send_packet(int epfd, int socket, char * data, int len)
{
	if (write(socket, data, len) == FAIL_CODE)
	{
		// Drop client - socket error
		drop_user(epfd, socket);
		print_log("Closed client " + to_string(socket) + ", Because socket error");
	}
	else
		print_log("Send packet to " + to_string(socket));
}

void drop_user(int epfd, int fd)
{
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	for(unsigned i = 0; i < user.size(); i++)
	{
		if (user[i] == fd)
		{
			user.erase(user.begin() + i);
			break;
		}
	}
}
