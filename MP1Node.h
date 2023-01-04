
#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

#define TREMOVE 20
#define TFAIL 5


enum MsgTypes{
	JOINREQ,
	JOINREP,
	HEARTBEAT,
	DUMMYLASTMSGTYPE
};

typedef struct HeartbeatElement {
	int id;
	short port;
	long heartbeat;
} HeartbeatElement;

typedef struct MessageHdr {
	enum MsgTypes msgType;
	int n;
} MessageHdr;

class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	int sendMessage(enum MsgTypes msgType, Address *dstAddr);
	Address getJoinAddress();
	Address getAddress(int id, short port);
	void initMemberListTable(Member *memberNode);
	void addMember(int id, short port, long heartbeat);
	bool updateMember(int id, short port, long heartbeat);
	void printAddress(Address *addr);
	virtual ~MP1Node();
};

#endif