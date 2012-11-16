#include "cacheentry.h"

CacheEntry::CacheEntry() : finished(false), readerCount(0) {

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

void CacheEntry::readStart() {
    ++readerCount;
}

void CacheEntry::readFinish() {
    if(readerCount > 0) {
        --readerCount;
    }
}

bool CacheEntry::isReaded() {
    return readerCount > 0;
}
