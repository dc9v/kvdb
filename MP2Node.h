
#ifndef MP2NODE_H_
#define MP2NODE_H_

#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"
#include <list>

#define NUM_KEY_REPLICAS 3
#define QUORUM_COUNT ((NUM_KEY_REPLICAS)/2+1)
#define RESPONSE_WAIT_TIME 20

class MyMessage:public Message{
public:
	enum MyMessageType{ REPUPDATE,QUERY };
	MyMessage(string message);
	MyMessage(MyMessageType mType,string normalMsg);
	MyMessage(MyMessageType mType,Message normalMsg);
	string toString();
	static string stripMyHeader(string message);
	MyMessageType msgType;
};

struct transaction{
public:
	int gtransID;
	int local_ts;
	int quorum_count;
	MessageType trans_type;
	string key;
	pair<int,string> latest_val;
	transaction(int tid, int lts, int qc, MessageType ttype,string k,string value):
			gtransID(tid),
			local_ts(lts),
			quorum_count(qc),
			trans_type(ttype),
			key(k),
			latest_val(0,value){
	}
};
typedef std::map<string,Entry> KeyMap;

class MP2Node {
private:
	vector<Node> hasMyReplicas;
	vector<Node> haveReplicasOf;
	vector<Node> ring;
	HashTable * ht;
	Member *memberNode;
	Params *par;
	EmulNet * emulNet;
	Log * log;

	list<transaction> translog;
	KeyMap keymap;

	bool inited ;

	void processKeyCreate(Message message);
	void processKeyUpdate(Message message);
	void processKeyDelete(Message message);
	void processKeyRead(Message message);

	void processReadReply(Message message);
	void processReply(Message message);

	void unicastMessage(MyMessage message,Address& toaddr);
	void multicastMessage(MyMessage message,vector<Node>& recp);

	void updateTransactionLog();

	void processReplicate(Node toNode, ReplicaType rType);
	void processReplicaUpdate(Message msg);

public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(string key);
	void findNeighbors();

	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	void checkMessages();

	void dispatchMessages(MyMessage message);

	vector<Node> findNodes(string key);

	bool createKeyValue(string key, string value, ReplicaType replica);
	string readKey(string key);
	bool updateKeyValue(string key, string value, ReplicaType replica);
	bool deletekey(string key);

	void stabilizationProtocol();

	~MP2Node();
};

#endif