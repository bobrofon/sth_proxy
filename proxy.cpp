#include "proxy.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <string.h>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "proxy.h"
#include "connection.h"
#include "client.h"
#define BACKLOG 50

using namespace std;

Proxy::Proxy(int port) {
	int s, yes = 1;
	struct sockaddr_in service;
	memset(pollC, 0, sizeof(*pollC) * MAX_CONNECT);
	pollLen = 0;

	listen_socket = -1;
	do {
		if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			break;
		}
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
			close(s);
			break;
		}
		service.sin_family = AF_INET;
		service.sin_addr.s_addr = htonl(INADDR_ANY);
		service.sin_port = htons(port);
		if ((bind( s, (struct sockaddr *) &service, sizeof(service))) < 0) {
			close(s);
			break;
		}

		if ((listen(s, BACKLOG)) < 0) {
			close(s);
			break;
		}
		listen_socket = s;
		listenState = true;
		return;
	} while(0);
	listenState = false;
}

Proxy::~Proxy() {
	if (listen_socket >= 0)
		close(listen_socket);
	for (vector<Client*>::iterator it = clients.begin(); it != clients.end(); ) {
		Client* proxyClient = (*it);
		it = clients.erase(it);
		delete proxyClient;
	}
}

void Proxy::loop() {
	for(;;) {
		preparePoll();
		//std::cout<<"wait"<<std::endl;
		if(poll(pollC, pollLen, -1) <= 0) {
			std::cout<<"pool error"<<std::endl;
			continue;
		}
		//std::cout<<"listen socket : "<<listen_socket<<" ("<<pollC[0].fd<<") event: "<<pollC[0].events<<std::endl;
		if(pollC[0].revents & POLLIN) {
			//std::cout<<"new accept"<<std::endl;
			acceptClient();
		}
		for(int i = 1; i < pollLen; ++i) {
			if(pollC[i].revents != 0) {
				//std::cout<<"event on socket "<<pollC[i].fd<<std::endl;
				route[pollC[i].fd]->continueWork(pollC + i);
			}
		}
	}
}

void Proxy::preparePoll() {
	memset(pollC, 0, sizeof(*pollC) * MAX_CONNECT);
	route.clear();
	pollC[0].fd = listen_socket;
	pollC[0].events = POLLIN;
	pollLen = 1;
	size_t ret = 0;

	for(vector<Client *>::iterator it = clients.begin(); it != clients.end(); ) {
		Client *proxyClient = *it;

		if(!proxyClient->alive()) {
			if(proxyClient->getURL() == "http://img.lenta.ru/_img/motor/b-motor/logo.png") {
				int i = 0;
			}
			it = clients.erase(it);
			std::cout<<"delete client : "<<proxyClient->getURL()<<std::endl;
			delete proxyClient;
			continue;
		}
		if(pollLen + 2 < MAX_CONNECT) {
			ret = proxyClient->setEventRequest(pollC + pollLen);
			if(ret > 0) {
				route[pollC[pollLen].fd] = proxyClient;
			}
			if(ret > 1) {
				route[pollC[pollLen + 1].fd] = proxyClient;
			}
			pollLen += ret;
		}
		++it;
	}
	//std::cout<<"client count: "<<clients.size()<<std::endl;
}

void Proxy::acceptClient() {
	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);
	int client;

	if ((client = accept(listen_socket, (struct sockaddr*)&addr, &addrSize)) < 0 ) {
		cerr << "accept() failed" << endl;
		return;
	}
	Client *ptr = NULL;
	try {
		ptr = new Client(new Connection(client));
		clients.push_back(ptr);
		std::cout<<"new connection accepted"<<std::endl;
	} catch (const bad_alloc& ba) {
		if (ptr != NULL){
			std::cout<<"accept failed with bad alloc"<<std::endl;
			delete ptr;
		}
	}
}

bool Proxy::isListen() const {
	return listenState;
}
