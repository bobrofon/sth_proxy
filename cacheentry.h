#ifndef CACHEENTRY_H
#define CACHEENTRY_H

#include <string>

class CacheEntry {
private:
	bool finished;
	std::string data;
public:
	CacheEntry();
	const char *getData() const;
	void appendData(const char *buf, size_t length);
	size_t getLength() const;
	void setFinished();
	bool isFinished() const;
};

#endif // CACHEENTRY_H
