#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <poll.h>
#include "connection.h"

enum CacheState {CACHE_NO_SET, CACHE_READ, CACHE_WRITE};
enum ErrorState {OK, CONNECT_ERROR, UNSUPPORTED_VERSION, LONG_URL, INTERNAL_ERROR, UNSUPPORTED_METHOD};

class Client
{
private:
	Connection *client;
	Connection *remote;
	std::string url;
	CacheState cacheState;
	ErrorState errorState;
	int contentLen;
	int recvedLen;
	int headerLen;
	bool headStart;

	int processHeader(char *header);
	int processHeaderCL(char *header, size_t len);
	void connectToRemote(const std::string &headStr);
	void processRequest(const std::string &headStr, std::string &method, std::string &protocol, std::string &version, std::string &url, int &port);
public:
	Client(Connection *conn);
	~Client();
	ErrorState getErrorState() const;
	std::string getURL() const;
	void continueWork(pollfd * ufds);
	void continueClient(pollfd * ufds);
	void continueRemote(pollfd * ufds);
	size_t setEventRequest(pollfd *ufds);
	Connection * getClient();
	Connection * getRemote();
	bool alive();
	void sendError();
	void remoteError();
	void clientError();
};

#endif // CLIENT_H
