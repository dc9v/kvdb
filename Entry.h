
#include "stdincludes.h"
#include "Message.h"

class Entry{
public:
	string value;
	int timestamp;
	ReplicaType replica;
	string delimiter;

	Entry(string entry);
	Entry(string _value, int _timestamp, ReplicaType _replica);
	string convertToString();
};
