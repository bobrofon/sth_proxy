#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iostream>
#include "cache.h"
#include "client.h"

static const std::string ERROR_MESSAGE_502 = "HTTP/1.0 502 Bad Gateway\r\n \
		\r\n<html><head><title>502 Bad Gateway</title></head> \
		<body><h2>502 Bad Gateway</h2><h3>Host not found or connection failed.</h3></body></html>\r\n";
static const std::string ERROR_MESSAGE_505 = "HTTP/1.0 505 HTTP Version not supported\r\n \
		\r\n<html><head><title>505 HTTP Version not supported</title></head> \
		<body><h2>505 HTTP Version not supported</h2><h3>Proxy server does not support the HTTP protocol version used in the request.</h3></body></html>\r\n";
static const std::string ERROR_MESSAGE_405 = "HTTP/1.0 405 Method Not Allowed\r\n \
		\r\n<html><head><title>405 Method Not Allowed</title></head> \
		<body><h2>405 Method Not Allowed</h2><h3>Proxy server does not support the HTTP method used in the request.</h3></body></html>\r\n";
static const std::string ERROR_MESSAGE_414 = "HTTP/1.0 414 Request-URL Too Long\r\n \
		\r\n<html><head><title>414 Request-URL Too Long</title></head> \
		<body><h2>414 Request-URL Too Long</h2><h3>The URL provided was too long for the server to process.</h3></body></html>\r\n";
static const std::string ERROR_MESSAGE_500 = "HTTP/1.0 500 Internal Server Error\r\n \
		\r\n<html><head><title>500 Internal Server Error</title></head> \
		<body><h2>500 Internal Server Error</h2></body></html>\r\n";
static const std::string SUPPORT_VERSION = "HTTP/1.0";

Client::Client(Connection *conn) : client(conn), remote(NULL) {
	cacheState = CACHE_NO_SET;
	headStart = true;
	if(client == NULL || !client->isOpen()) {
		errorState = INTERNAL_ERROR;
		return;
	}
	errorState = OK;
}

bool Client::alive() {
	if(client == NULL) {
		return false;
	}
	if(client->isOpen() && remote != NULL && remote->isOpen()) {
		return true;
	}
	if(client->isOpen() && remote != NULL && remote->getAvaliable() > 0) {
		return true;
	}
	if(client->isOpen() && cacheState == CACHE_READ && Cache::getCache()->get(url) != NULL) {
		return true;
	}
	if(client->isOpen() && errorState != OK) {
		return true;
	}
	if(client->isOpen() && url.empty() && cacheState == CACHE_NO_SET) {
		return true;
	}
	if(client->isOpen()) {
		return false;
	}
	if(remote != NULL && remote->isOpen() && cacheState == CACHE_WRITE) {
		return true;
	}
	return false;
}

Client::~Client() {
	delete remote;
	delete client;
}

ErrorState Client::getErrorState() const {
	return errorState;
}

void Client::continueWork(pollfd *ufds) {
	if(!alive()) {
		return;
	}
	if(client != NULL && client->isOpen() && client->getSocket() == ufds->fd && ufds->revents != 0) {
		continueClient(ufds);
		return;
	}
	if(remote != NULL && remote->isOpen() && remote->getSocket() == ufds->fd && ufds->revents != 0) {
		continueRemote(ufds);
	}
}

void Client::continueClient(pollfd * ufds) {
	short revent = ufds->revents;
	CacheEntry *cache = Cache::getCache()->get(url);

	if( revent & (POLLIN | POLLOUT) == 0) {
		client->closeSocket();
		return ;
	}
	if(revent & POLLOUT) {
		if(errorState != OK) {
			sendError();
			client->closeSocket();
			return;
		} else if(cacheState == CACHE_READ && cache != NULL) {
			client->sendData(cache->getData(), cache->getLength());
			if(cache->getLength() == client->getWritten() && cache->isFinished()) {
				client->closeSocket();
			}
		} else if(remote != NULL && remote->getAvaliable() > 0) {
			client->sendData(remote->getBuffer(), remote->getAvaliable());
			client->resetWritten();
			remote->resetAvaliable();
		}
	}
	if(revent & POLLIN) {
		client->recvData();
		if(cacheState == CACHE_NO_SET && remote == NULL && url.empty() && errorState == OK) {
			connectToRemote(client->getBuffer());
			if(remote != NULL) {
				client->catBuf();
			}
		}
	}
}

void Client::continueRemote(pollfd * ufds) {
	short revent = ufds->revents;

	if( revent & (POLLIN | POLLOUT) == 0) {
		remote->closeSocket();
		return ;
	}
	if(revent & POLLOUT) {
		remote->sendData(client->getBuffer(), client->getAvaliable());
		remote->resetWritten();
		client->resetAvaliable();
	}
	if(revent & POLLIN) {
		remote->recvData();
		if(cacheState == CACHE_NO_SET && headStart) {
			headStart = false;
			std::stringstream ss;
			remote->getBuffer()[7] = '0';
			ss<<remote->getBuffer();
			std::string protocol;
			int code = 0;
			ss>>protocol>>code;
			if(protocol.compare("HTTP/1.0") == 0 && code == 200) {
				cacheState = CACHE_WRITE;
				//std::cout<<remote->getBuffer()<<std::endl;
			}
		}
		if(cacheState == CACHE_WRITE) {
			Cache::getCache()->put(url, remote->getBuffer(), remote->getAvaliable());
			if(!remote->isOpen()) {
				Cache::getCache()->get(url)->setFinished();
			}
		}
	}
}

void Client::sendError() {
	switch(errorState) {
		case CONNECT_ERROR:
			client->sendData(ERROR_MESSAGE_502.c_str(), ERROR_MESSAGE_502.length());
			break;
		case UNSUPPORTED_VERSION:
			client->sendData(ERROR_MESSAGE_505.c_str(), ERROR_MESSAGE_505.length());
			break;
		case LONG_URL:
			client->sendData(ERROR_MESSAGE_414.c_str(), ERROR_MESSAGE_414.length());
			break;
		case INTERNAL_ERROR:
			client->sendData(ERROR_MESSAGE_500.c_str(), ERROR_MESSAGE_500.length());
			break;
		case UNSUPPORTED_METHOD:
			client->sendData(ERROR_MESSAGE_405.c_str(), ERROR_MESSAGE_405.length());
			break;
	}
}

size_t Client::setEventRequest(pollfd *ufds) {
	if(!alive()) {
		return 0;
	}

	size_t cn = 0;
	size_t rn = 0;

	if(client != NULL && client->isOpen()) {
		ufds[0].fd = client->getSocket();
		cn = 1;
		if(remote == NULL && client->getAvaliable() < BUFSIZE) {
			ufds[0].fd = client->getSocket();
			ufds[0].events |= POLLIN;
			cn = 1;
		}
		if((cacheState == CACHE_READ && Cache::getCache()->get(url)->getLength() > client->getWritten())
				|| (remote != NULL && remote->getAvaliable() > 0)
				|| errorState != OK) {
			ufds[0].fd = client->getSocket();
			ufds[0].events |= POLLOUT;
			cn = 1;
		}
	}
	if(remote != NULL && remote->isOpen()) {
		ufds[cn].fd = remote->getSocket();
		rn = 1;
		if(remote->getAvaliable() < BUFSIZE) {
			ufds[cn].fd = remote->getSocket();
			ufds[cn].events |= POLLIN;
			rn = 1;
		}
		if(client != NULL && client->getAvaliable() > 0) {
			ufds[cn].fd = remote->getSocket();
			ufds[cn].events |= POLLOUT;
			rn = 1;
		}
	}

	return cn + rn;
}

void Client::processRequest(const std::string &headStr, std::string &method, std::string &protocol, std::string &version, std::string &host, int &port) {
	size_t res = headStr.find("\r\n");
	if(res == std::string::npos) {
		errorState = LONG_URL;
		cacheState = CACHE_NO_SET;
		return ;
	}
	port = 80;
	std::stringstream ss;
	ss << headStr.substr(0, res);
	ss >> method >> url >> version;
	res = url.find("://");
	if(res != std::string::npos) {
		protocol = url.substr(0, res);
	}
	res += 3;
	size_t pslash = url.find("/", res);
	size_t pport = url.find(':', res);
	if (pport != std::string::npos && pport < pslash) {
		host = url.substr(res, pport - res);
		pport += 1;
		std::string tmp = url.substr(pport, pslash - pport);
		port = atoi(tmp.c_str());
	} else if (res > 2) {
		host = url.substr(res, pslash - res);
	}
}

void Client::connectToRemote(const std::string &headStr) {
	std::string method;
	std::string protocol;
	std::string version;
	std::string host;
	int port;
	processRequest(headStr, method, protocol, version, host, port);
	std::cout<<"try to connect :"<<url<<std::endl;
	if(errorState != OK) {
		return ;
	}
	if(protocol.compare("http") != 0) {
		errorState = INTERNAL_ERROR;
		cacheState = CACHE_NO_SET;
		return;
	}
	if(method.compare("GET") != 0 && method.compare("HEAD") != 0) {
		errorState = UNSUPPORTED_METHOD;
		cacheState = CACHE_NO_SET;
		return;
	}
	if(version.compare(SUPPORT_VERSION) != 0)  {
		errorState = UNSUPPORTED_VERSION;
		cacheState = CACHE_NO_SET;
		return;
	}

	if(Cache::getCache()->get(url) != NULL) {
		cacheState = CACHE_READ;
		std::cout<<"route to cache"<<std::endl;
		return ;
	}

	std::cout<<"Connect to host: "<<host<<std::endl;
	int s = -1;
	sockaddr_in host_addr;
	do {
		if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			break;
		}
		host_addr.sin_family = AF_INET;
		if ((host_addr.sin_addr.s_addr = inet_addr(host.c_str())) == INADDR_NONE)
		{
			struct hostent *hp;
			std::cout<<"ghbn"<<std::endl;
			if ((hp = gethostbyname(host.c_str())) == NULL) {
				std::cout<<"ghbn error"<<std::endl;
				close(s);
				s = -1;
				break;
			}
			std::cout<<"ghbn end"<<std::endl;
			host_addr.sin_family = hp->h_addrtype;
			memcpy(&(host_addr.sin_addr), hp->h_addr, hp->h_length);
		} else {
			host_addr.sin_family = AF_INET;
		}
		host_addr.sin_port = htons(port);
		if (connect(s, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
			close(s);
			break;
		}
	} while(0);

	if(s < 0) {
		errorState = CONNECT_ERROR;
		cacheState = CACHE_NO_SET;
		return ;
	}
	remote = new Connection(s);
	errorState = OK;
	cacheState = CACHE_NO_SET;
	std::cout<<"remote connected "<<url<<" on port "<<port<<" (socket "<<s<<" )"<<std::endl;
}

Connection * Client::getClient() {
	return client;
}

Connection * Client::getRemote() {
	return remote;
}

std::string Client::getURL() const {
	return url;
}

void Client::remoteError() {
	if(remote == NULL) {
		return ;
	}
	remote->forceClose();
	if(cacheState == CACHE_WRITE) {
		//Cache::getCache()->remove(url);
	}
	if(client != NULL) {
		client->forceClose();
	}
}


void Client::clientError() {
	if(client == NULL) {
		return ;
	}
	client->forceClose();
}
