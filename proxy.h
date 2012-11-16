#ifndef PROXY_H
#define PROXY_H
#include <vector>
#include <string>
#include <map>
#include <sys/types.h>
#include <poll.h>
#include "client.h"
#include "cache.h"

#define MAX_CONNECT 256

class Proxy {
	public:
		Proxy(int);
		~Proxy();
		void loop();
		bool isListen() const;

	private:
		int listen_socket;
		std::vector<Client*> clients;
		bool listenState;
		pollfd pollC[MAX_CONNECT];
		size_t pollLen;
		std::map<int, Client*> route;

		void preparePoll();
		void acceptClient();
};

#endif // PROXY_H
