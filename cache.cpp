#include "cache.h"


Cache *Cache::singleObject = NULL;

Cache::Cache() {

}

Cache::~Cache() {
	for(std::map<std::string, CacheEntry *>::iterator it = cache.begin(); it != cache.end(); ++it) {
		delete it->second;
	}
}

Cache *Cache::getCache() {
	if(singleObject == NULL) {
		singleObject = new Cache();
	}
	return singleObject;
}

CacheEntry *Cache::get(const std::string &url) {
	if(!url.empty() && cache.find(url) != cache.end()) {
		return cache[url];
	}
	return NULL;
}

void Cache::put(const std::string &url, const char *data, size_t len) {
	CacheEntry *p = NULL;

	if(url.empty()) {
		return ;
	}
	if(cache.find(url) != cache.end()) {
		p = cache[url];
	} else {
		p = new CacheEntry();
		cache[url] = p;
	}
	p->appendData(data, len);
}

void Cache::remove(const std::string &url) {
	if(url.empty()) {
		return ;
	}
	std::map<std::string, CacheEntry *>::iterator it = cache.find(url);
	if(it != cache.end()) {
		delete it->second;
		cache.erase(it);
	}
}
