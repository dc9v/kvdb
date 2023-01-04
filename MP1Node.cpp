
#include "MP1Node.h"


MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

MP1Node::~MP1Node() {}

int MP1Node::recvLoop() {
	if ( memberNode->bFailed ) {
		return false;
	}
	else {
		return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
	}
}

int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

void MP1Node::nodeStart(char *servaddrstr, short servport) {
	Address joinaddr;
	joinaddr = getJoinAddress();

	if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
		log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
		exit(1);
	}

	if( !introduceSelfToGroup(&joinaddr) ) {
		finishUpThisNode();
#ifdef DEBUGLOG
		log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
		exit(1);
	}

	return;
}

int MP1Node::initThisNode(Address *joinaddr) {

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = TREMOVE;
	initMemberListTable(memberNode);

	return 0;
}

int MP1Node::introduceSelfToGroup(Address *joinaddr) {

#ifdef DEBUGLOG
	static char s[1024];
#endif

	if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
#ifdef DEBUGLOG
		log->LOG(&memberNode->addr, "Starting up group...");
#endif
		memberNode->inGroup = true;
	}
	else {
		sendMessage(JOINREQ, joinaddr);
#ifdef DEBUGLOG
		sprintf(s, "Trying to join...");
		log->LOG(&memberNode->addr, s);
#endif
	}

	return 1;

}

int MP1Node::finishUpThisNode(){
	return 0;
}

void MP1Node::nodeLoop() {
	if (memberNode->bFailed) {
		return;
	}

	checkMessages();

	if( !memberNode->inGroup ) {
		return;
	}

	nodeLoopOps();

	return;
}

void MP1Node::checkMessages() {
	void *ptr;
	int size;

	while ( !memberNode->mp1q.empty() ) {
		ptr = memberNode->mp1q.front().elt;
		size = memberNode->mp1q.front().size;
		memberNode->mp1q.pop();
		recvCallBack((void *)memberNode, (char *)ptr, size);
	}
	return;
}

bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	MessageHdr *msg = (MessageHdr *)data;
	HeartbeatElement *hbe = (HeartbeatElement *)(msg + 1);
	Address *srcAddr = (Address *)hbe;
	for (int i = 0; i < msg->n; i++) {
		addMember(hbe->id, hbe->port, hbe->heartbeat);
		hbe++;
	}

	if (msg->msgType == JOINREQ) {
		sendMessage(JOINREP, srcAddr);
	}
	if (msg->msgType == JOINREP) {
		memberNode->inGroup = true;
	}
	return true;
}

void MP1Node::nodeLoopOps() {
	memberNode->heartbeat++;

	Address dstAddr;
	std::random_shuffle ( memberNode->memberList.begin(), memberNode->memberList.end() );
	for (int i = 0; i < 3; ++i) {
		dstAddr = getAddress(memberNode->memberList[i].id, memberNode->memberList[i].port);
		sendMessage(HEARTBEAT, &dstAddr);
	}

	for (vector<MemberListEntry>::iterator m = memberNode->memberList.begin(); m != memberNode->memberList.end(); ) {
		if (par->getcurrtime() - m->timestamp > memberNode->timeOutCounter ) {
			dstAddr = getAddress(m->id, m->port);
			log->logNodeRemove(&(memberNode->addr), &dstAddr);

			m = memberNode->memberList.erase(m);
		} else {
			++m;
		}
	}
	return;
}

int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

int MP1Node::sendMessage(enum MsgTypes msgType, Address *dstAddr) {

	MessageHdr *msg;
	HeartbeatElement *hbe;
	int n = 1;
	if (msgType != JOINREQ) {
		n += memberNode->memberList.size();
	}

	size_t msgsize = sizeof(MessageHdr) + n * sizeof(HeartbeatElement);
	msg = (MessageHdr *) malloc(msgsize);
	msg->msgType = msgType;
	msg->n = n;
	hbe = (HeartbeatElement *)(msg + 1);
	hbe->id = *(int *)(&memberNode->addr.addr);
	hbe->port = *(short *)(&memberNode->addr.addr[4]);
	hbe->heartbeat = memberNode->heartbeat;
	if (msgType != JOINREQ) {
		for (vector<MemberListEntry>::iterator m = memberNode->memberList.begin(); m != memberNode->memberList.end(); ++m) {
			hbe++;
			if (par->getcurrtime() - m->timestamp <= memberNode->pingCounter ) {
				hbe->id = m->id;
				hbe->port = m->port;
				hbe->heartbeat = m->heartbeat;
			} else {
				hbe->id = *(int *)(&memberNode->addr.addr);
				hbe->port = *(short *)(&memberNode->addr.addr[4]);
				hbe->heartbeat = memberNode->heartbeat;
			}
		}
	}

	emulNet->ENsend(&memberNode->addr, dstAddr, (char *)msg, msgsize);
	free(msg);
	return 0;
}

Address MP1Node::getJoinAddress() {
	Address joinaddr;

	memset(&joinaddr, 0, sizeof(Address));
	*(int *)(&joinaddr.addr) = 1;
	*(short *)(&joinaddr.addr[4]) = 0;

	return joinaddr;
}

Address MP1Node::getAddress(int id, short port) {
	Address a;

	memset(&a, 0, sizeof(Address));
	*(int *)(&a.addr) = id;
	*(short *)(&a.addr[4]) = port;

	return a;
}

void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

void MP1Node::addMember(int id, short port, long heartbeat) {

	if (!updateMember(id, port, heartbeat)) {
		MemberListEntry *newMember = new MemberListEntry(id, port, heartbeat, par->getcurrtime());
		memberNode->memberList.insert(memberNode->memberList.begin(), *newMember);

		Address *srcAddr = (Address *)malloc(sizeof(Address));
		*(int *)(&srcAddr->addr[0]) = id;
		*(short *)(&srcAddr->addr[4]) = port;
		log->logNodeAdd(&(memberNode->addr), srcAddr);
	}

}

bool MP1Node::updateMember(int id, short port, long heartbeat) {
	for (vector<MemberListEntry>::iterator m = memberNode->memberList.begin(); m != memberNode->memberList.end(); ++m) {
		if (m->id == id && m->port == port) {
			if (m->heartbeat < heartbeat) {
				m->heartbeat = heartbeat;
				m->timestamp = par->getcurrtime();
			}
			return true;
		}
	}
	return false;
}

void MP1Node::printAddress(Address *addr)
{
	printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
			addr->addr[3], *(short*)&addr->addr[4]) ;
}