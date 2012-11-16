#ifndef CACHEENTRY_H
#define CACHEENTRY_H

#include <string>

class CacheEntry {
private:
	bool finished;
    int readerCount;
	std::string data;
public:
	CacheEntry();
	const char *getData() const;
	void appendData(const char *buf, size_t length);
	size_t getLength() const;
	void setFinished();
	bool isFinished() const;
    void readStart();
    void readFinish();
    bool isReaded();
};

#endif // CACHEENTRY_H
