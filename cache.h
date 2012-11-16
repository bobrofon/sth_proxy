#ifndef CACHE_H
#define CACHE_H

#include <map>
#include <string>
#include "cacheentry.h"

class Cache
{
private:
	std::map<std::string, CacheEntry *> cache;
	static Cache *singleObject;
	Cache();
	~Cache();
public:
	static Cache *getCache();
	CacheEntry *get(const std::string &url);
	void put(const std::string &url, const char *data, size_t len);
	void remove(const std::string &url);
};


#endif // CACHE_H
