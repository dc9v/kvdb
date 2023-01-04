
#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include "stdincludes.h"
#include "common.h"
#include "Entry.h"

class HashTable {
public:
	map<string, string> hashTable;
	HashTable();
	bool create(string key, string value);
	string read(string key);
	bool update(string key, string newValue);
	bool deleteKey(string key);
	bool isEmpty();
	unsigned long currentSize();
	void clear();
	unsigned long count(string key);
	virtual ~HashTable();
};

#endif
