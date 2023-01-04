#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "stdincludes.h"
#include "Member.h"
#include "common.h"

class Message{
public:
	MessageType type;
	ReplicaType replica;
	string key;
	string value;
	Address fromAddr;
	int transID;
	bool success;
	string delimiter;
	Message(string message);
	Message(const Message& anotherMessage);
	Message(int _transID, Address _fromAddr, MessageType _type, string _key, string _value);
	Message(int _transID, Address _fromAddr, MessageType _type, string _key, string _value, ReplicaType _replica);
	Message(int _transID, Address _fromAddr, MessageType _type, string _key);
	Message(int _transID, Address _fromAddr, MessageType _type, bool _success);
	Message(int _transID, Address _fromAddr, string _value);
	Message& operator = (const Message& anotherMessage);
	string toString();
};

#endif
