#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include "connection.h"

Connection::Connection(int sock) : sock(sock), written(0), avaliable(0) {

}

Connection::~Connection() {
	closeSocket();
}

void Connection::closeSocket() {
	if(isOpen()) {
		shutdown(sock, SHUT_RDWR);
		close(sock);
		sock = -1;
	}
}

ssize_t Connection::recvData() {
	if(!isOpen()) {
		return -1;
	}
	memset(buf, 0, BUFSIZE);
	ssize_t result = recv(sock, buf + avaliable, BUFSIZE - avaliable, 0);

	if(result > 0) {
		avaliable += result;
	} else {
		closeSocket();
	}

	return result;
}

ssize_t Connection::sendData(const char *data, size_t len) {
	if(!isOpen()) {
		return -1;
	}
	std::cerr<<"buf"<<std::endl<<data<<std::endl<<"end"<<std::endl;
	ssize_t result = send(sock, data + written, len - written, 0);

	if(result > 0) {
		written += result;
	} else {
		closeSocket();
	}

	return result;
}

size_t Connection::getAvaliable() const {
	return avaliable;
}

size_t Connection::getWritten() const {
	return written;
}

bool Connection::isOpen() const {
	return sock < 0 ? false : true;
}

char *Connection::getBuffer(){
	return buf;
}

void Connection::resetWritten() {
	written = 0;
}
void Connection::resetAvaliable() {
	avaliable = 0;
}

int Connection::getSocket() {
	return sock;
}
void Connection::forceClose() {
	sock = -1;
}

void Connection::catBuf() {
	char *p = strstr(buf, "\r\n");
	if(p != NULL) {
		p[2] = '\r';
		p[3] = '\n';
		p[4] = 0;
		avaliable = (p+4) - buf;
	}
}
void Connection::setAvaliable(size_t len) {
	avaliable = len;
	buf[len] = 0;
}
