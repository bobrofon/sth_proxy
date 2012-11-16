#include <iostream>
#include <cstdlib>
#include <signal.h>
#include "proxy.h"

static Proxy *proxy = NULL;

void releaseProxy() {
	delete proxy;
	proxy = NULL;
	exit(0);
}

void signalHandler(int sig) {
	releaseProxy();
}

using namespace std;

int main() {
    sigset(SIGINT, signalHandler);
    sighold(SIGPIPE);
	proxy = new Proxy(5555);
	if (proxy) {
		if (proxy->isListen())
			proxy->loop();
		else
			cout << "Can't start proxy" << endl;
		releaseProxy();
	} else {
		cout << "Can't create proxy" << endl;
	}
	return 0;
}
