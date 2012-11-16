#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>
#include <sys/types.h>

#define BUFSIZE (64*1024)

class Connection {
private:
	int sock;
	size_t written;
	size_t avaliable;
	char buf[BUFSIZE];

public:
	Connection(int sock);
	~Connection();
	ssize_t recvData();
	ssize_t sendData(const char *data, size_t len);
	size_t getAvaliable() const;
	size_t getWritten() const;
	bool isOpen() const;
	char *getBuffer();
	void resetWritten();
	void resetAvaliable();
	int getSocket();
	void forceClose();
	void closeSocket();
	void catBuf();
};

#endif // CONNECTION_H
