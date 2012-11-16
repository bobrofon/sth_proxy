#include "cacheentry.h"

CacheEntry::CacheEntry() : finished(false) {

}

const char *CacheEntry::getData() const {
	return data.c_str();
}

void CacheEntry::appendData(const char *buf, size_t length) {
	data.append(buf, length);
}

size_t CacheEntry::getLength() const {
	return data.length();
}

void CacheEntry::setFinished() {
	finished = true;
}

bool CacheEntry::isFinished() const {
	return finished;
}
