
#include "HashTable.h"

HashTable::HashTable() {}

HashTable::~HashTable() {}

bool HashTable::create(string key, string value) {
	hashTable.emplace(key, value);
	return true;
}

string HashTable::read(string key) {
	map<string, string>::iterator search;

	search = hashTable.find(key);
	if ( search != hashTable.end() ) {
		return search->second;
	}
	else {
		return "";
	}
}

bool HashTable::update(string key, string newValue) {
	map<string, string>::iterator update;

	if (read(key).empty()) {
		return false;
	}
	hashTable.at(key) = newValue;
	return true;
}

bool HashTable::deleteKey(string key) {
	uint eraseCount = 0;

	if (read(key).empty()) {
		return false;
	}
	eraseCount = hashTable.erase(key);
	if ( eraseCount < 1 ) {
		return false;
	}
	return true;
}

bool HashTable::isEmpty() {
	return hashTable.empty();
}

unsigned long HashTable::currentSize() {
	return (unsigned  long)hashTable.size();
}

void HashTable::clear() {
	hashTable.clear();
}

unsigned long HashTable::count(string key) {
	return (unsigned long) hashTable.count(key);
}

