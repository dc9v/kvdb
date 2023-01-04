#include "MP2Node.h"
static int debug = 0;
#define mylog(fmt,...)  if(debug) log->LOG(&(memberNode->addr),fmt,__VA_ARGS__);


string MyMessage::stripMyHeader(string message){
	int pos = message.find('@');
	return message.substr(pos+1);
}
MyMessage::MyMessage(string message):Message(MyMessage::stripMyHeader(message)){
	int  header = stoi(message.substr(0,message.find('@')));
	msgType = static_cast<MyMessageType>(header);
}
MyMessage::MyMessage(MyMessage::MyMessageType mt, string normalMsg):Message(normalMsg),msgType(mt){
}
MyMessage::MyMessage(MyMessage::MyMessageType mt, Message normalMsg):Message(normalMsg),msgType(mt){
}
string MyMessage::toString(){
	return to_string(msgType) + '@' + Message::toString();
}


MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
	this->inited = false;
}

MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}

void MP2Node::updateRing() {
	vector<Node> curMemList;
	bool change = false;
	curMemList = getMembershipList();

	sort(curMemList.begin(), curMemList.end());
	ring = curMemList;
	stabilizationProtocol();
}

vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

void MP2Node::clientCreate(string key, string value) {
	g_transID++;
	vector<Node> recipients = findNodes(key);
	assert(recipients.size()==NUM_KEY_REPLICAS);
	Address* sendaddr = &(this->memberNode->addr);
	for (int i=0;i<NUM_KEY_REPLICAS;++i){
		MyMessage createMsg(MyMessage::QUERY,Message(g_transID,this->memberNode->addr,MessageType::CREATE,key,value,static_cast<ReplicaType>(i)));
		unicastMessage(createMsg,recipients[i].nodeAddress);
	}
	transaction tr(g_transID,par->getcurrtime(),QUORUM_COUNT,MessageType::CREATE,key,value);
	translog.push_front(tr);
}

void MP2Node::clientRead(string key){
	g_transID++;
	MyMessage createMsg(MyMessage::QUERY,Message(g_transID,this->memberNode->addr,MessageType::READ,key));
	transaction tr(g_transID,par->getcurrtime(),QUORUM_COUNT,MessageType::READ,key,"");
	translog.push_front(tr);
	vector<Node> recipients = findNodes(key);
	multicastMessage(createMsg,recipients);
}

void MP2Node::clientUpdate(string key, string value){
	g_transID++;
	vector<Node> recipients = findNodes(key);
	assert(recipients.size()==NUM_KEY_REPLICAS);
	Address* sendaddr = &(this->memberNode->addr);
	for (int i=0;i<NUM_KEY_REPLICAS;++i){
		MyMessage createMsg(MyMessage::QUERY,Message(g_transID,this->memberNode->addr,MessageType::UPDATE,key,value,static_cast<ReplicaType>(i)));
		unicastMessage(createMsg,recipients[i].nodeAddress);
	}
	transaction tr(g_transID,par->getcurrtime(),QUORUM_COUNT,MessageType::UPDATE,key,value);
	translog.push_front(tr);
}

void MP2Node::clientDelete(string key){
	if (key=="invalidKey")
		mylog("Got and invalid key client %s","key");
	g_transID++;
	MyMessage createMsg(MyMessage::QUERY,Message(g_transID,this->memberNode->addr,MessageType::DELETE,key));
	transaction tr(g_transID,par->getcurrtime(),QUORUM_COUNT,MessageType::DELETE,key,"");
	translog.push_front(tr);
	vector<Node> recipients = findNodes(key);
	multicastMessage(createMsg,recipients);
}

bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	Entry e(value,par->getcurrtime(),replica);
	keymap.insert(pair<string,Entry>(key,e));
	return true;

}

string MP2Node::readKey(string key) {
	KeyMap::iterator it;
	if ((it=keymap.find(key))!=keymap.end())
		return it->second.convertToString();
	else return "";
}

bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	Entry e(value,par->getcurrtime(),replica);
	KeyMap::iterator it;
	if ((it=keymap.find(key))!=keymap.end()){
		keymap.insert(pair<string,Entry>(key,e));
		return true;
	}else return false;
}

bool MP2Node::deletekey(string key) {
	if(keymap.erase(key)) return true;
	else return false;
}

void MP2Node::checkMessages() {
	char * data;
	int size;


	while ( !memberNode->mp2q.empty() ) {
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);
		mylog("got the message :%s",data);

		MyMessage msg(message);
		dispatchMessages(msg);

	}

}

vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

void MP2Node::updateTransactionLog(){

	list<transaction>::iterator it=translog.begin();
	while(it!=translog.end())
		if((par->getcurrtime()-it->local_ts)>RESPONSE_WAIT_TIME) {
			MessageType mtype = it->trans_type;
			int transid = it->gtransID;
			mylog("Transaction %d timeout",transid);
			switch(mtype){
			case MessageType::CREATE: log->logCreateFail(&memberNode->addr,true,transid,it->key,it->latest_val.second);break;
			case MessageType::UPDATE: log->logUpdateFail(&memberNode->addr,true,transid,it->key,it->latest_val.second);break;
			case MessageType::READ: log->logReadFail(&memberNode->addr,true,transid,it->key);break;
			case MessageType::DELETE: log->logDeleteFail(&memberNode->addr,true,transid,it->key);break;
			}
			translog.erase(it++);
		}else it++;

}

bool MP2Node::recvLoop() {
	if ( memberNode->bFailed ) {
		return false;
	}
	else {
		updateTransactionLog();
		return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
	}
}

int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
void MP2Node::dispatchMessages(MyMessage message){
	if(message.msgType==MyMessage::QUERY){
		switch(message.type){
		case MessageType::CREATE: processKeyCreate(message);break;
		case MessageType::UPDATE: processKeyUpdate(message);break;
		case MessageType::DELETE: processKeyDelete(message);break;
		case MessageType::READ: processKeyRead(message);break;

		case MessageType::READREPLY: processReadReply(message);break;
		case MessageType::REPLY: processReply(message);break;
		}
	}else if(message.msgType==MyMessage::REPUPDATE)
		processReplicaUpdate(message);
	else{
		mylog("Dropping corrupted packet %s",message.toString().c_str());
		return;
	}
}
void MP2Node::processReplicate(Node toNode, ReplicaType repType){
	KeyMap::iterator it;
	for(it=keymap.begin();it!=keymap.end();++it){
		if(it->second.replica==repType){
			MyMessage keyupdate(MyMessage::REPUPDATE,Message(-1,(memberNode->addr),MessageType::CREATE,it->first,it->second.value,ReplicaType::TERTIARY));
			unicastMessage(keyupdate,toNode.nodeAddress);
		}
	}
}
void MP2Node::processReplicaUpdate(Message message){
	createKeyValue(message.key,message.value,message.replica);
}
void MP2Node::processKeyCreate(Message message){
	MyMessage reply(MyMessage::QUERY,Message(message.transID,(this->memberNode->addr),MessageType::REPLY,false));
	if( createKeyValue(message.key,message.value,message.replica)){
		reply.success=true;
		mylog("create server node with replica %d for key %s",message.replica,message.key.c_str());
		log->logCreateSuccess(&memberNode->addr,false,message.transID,message.key,message.value);
	}else{
		reply.success=false;
		log->logCreateFail(&memberNode->addr,false,message.transID,message.key,message.value);
	}
	unicastMessage(reply,message.fromAddr);
}
void MP2Node::processKeyUpdate(Message message){
	MyMessage reply(MyMessage::QUERY,Message(message.transID,(this->memberNode->addr),MessageType::REPLY,false));
	if( updateKeyValue(message.key,message.value,message.replica)){
		reply.success=true;
		mylog("update server node with replica %d for key %s",message.replica,message.key.c_str());
		log->logUpdateSuccess(&memberNode->addr,false,message.transID,message.key,message.value);
	}else{
		reply.success=false;
		log->logUpdateFail(&memberNode->addr,false,message.transID,message.key,message.value);
	}
	unicastMessage(reply,message.fromAddr);
}
void MP2Node::processKeyDelete(Message message){
	MyMessage reply(MyMessage::QUERY,Message(message.transID,(this->memberNode->addr),MessageType::REPLY,false));
	if( deletekey(message.key)){
		reply.success=true;
		mylog("delete server node with replica %d for key %s",message.replica,message.key.c_str());
		log->logDeleteSuccess(&memberNode->addr,false,message.transID,message.key);
	}
	else{
		reply.success=false;
		log->logDeleteFail(&memberNode->addr,false,message.transID,message.key);
	}
	unicastMessage(reply,message.fromAddr);

}
void MP2Node::processKeyRead(Message message){
	string keyval = readKey(message.key);
	if(!keyval.empty()){
		mylog("read server node with replica %d for key %s",message.replica,message.key.c_str());
		log->logReadSuccess(&memberNode->addr,false,message.transID,message.key,keyval);
	}else{
		log->logReadFail(&memberNode->addr,false,message.transID,message.key);
	}
	MyMessage reply(MyMessage::QUERY,Message(message.transID,(this->memberNode->addr),keyval));
	unicastMessage(reply,message.fromAddr);
}

void MP2Node::processReadReply(Message message){
	string value = message.value;
	if(value.empty()) return;
	string delim = ":";
	vector<string> tuple;
	int start = 0;
	int pos = 0;
	while((pos=value.find(delim,start))!=string::npos){
		string token = value.substr(start,pos-start);
		tuple.push_back(token);
		start = pos+1;
	}
	tuple.push_back(value.substr(start));
	assert(tuple.size()==3);
	string keyval = tuple[0];
	int timestamp = stoi(tuple[1]);
	ReplicaType repType = static_cast<ReplicaType>(stoi(tuple[2]));
	int transid = message.transID;
	list<transaction>::iterator it;
	for (it = translog.begin(); it!=translog.end();++it)
		if(it->gtransID==transid)
			break;
		else mylog("unmatched transaction transid %d",it->gtransID);

	if(it==translog.end()){
		mylog("dropping reply for transid: %d",transid);
	}else if(--(it->quorum_count)==0){
		mylog("Received reply for op %d for transid: %d,%d replies remaining",it->trans_type,it->gtransID,it->quorum_count);
		log->logReadSuccess(&memberNode->addr,true,message.transID,it->key,it->latest_val.second);
		translog.erase(it);
	}else{
		mylog("Received reply for op %d for transid: %d,%d replies remaining",it->trans_type,it->gtransID,it->quorum_count);
		if(timestamp>=it->latest_val.first){
			it->latest_val = pair<int,string>(timestamp,keyval);
			mylog("Changing latest val for transid :%d and key: %s",it->gtransID,it->key.c_str());
		}
	}

}
void MP2Node::processReply(Message message){
	int transid = message.transID;
	list<transaction>::iterator it;
	for (it = translog.begin(); it!=translog.end();++it)
		if(it->gtransID==transid)
			break;
	if(it==translog.end()) {
		mylog("dropping reply for transid: %d",transid);
	}else if(!message.success){
	}else if(--(it->quorum_count)==0){
		mylog("Received reply for op %d for transid: %d,%d replies remaining",it->trans_type,it->gtransID,it->quorum_count);
		switch(it->trans_type){
		case MessageType::CREATE: log->logCreateSuccess(&memberNode->addr,true,message.transID,it->key,it->latest_val.second);break;
		case MessageType::UPDATE: log->logUpdateSuccess(&memberNode->addr,true,message.transID,it->key,it->latest_val.second);break;
		case MessageType::DELETE: log->logDeleteSuccess(&memberNode->addr,true,message.transID,it->key);break;
		}
		translog.erase(it);
	}else{
		mylog("Received reply for op %d for transid: %d,%d replies remaining",it->trans_type,it->gtransID,it->quorum_count);
	}
}


void MP2Node::multicastMessage(MyMessage message,vector<Node>& recipients){

	Address* sendaddr = &(this->memberNode->addr);
	string strrep = message.toString();
	char * msgstr = (char*)strrep.c_str();
	size_t msglen = strlen(msgstr);
	mylog("Sending the client request : %s of length %d",msgstr,msglen);
	for (size_t i=0;i<recipients.size();++i)
		this->emulNet->ENsend(sendaddr,&(recipients[i].nodeAddress),msgstr,msglen);
}
void MP2Node::unicastMessage(MyMessage message,Address& toaddr){
	Address* sendaddr = &(this->memberNode->addr);
	string strrep = message.toString();
	char * msgstr = (char*)strrep.c_str();
	size_t msglen = strlen(msgstr);
	mylog("Sending the client request : %s  of length %d",msgstr,msglen);
	this->emulNet->ENsend(sendaddr,&toaddr,msgstr,msglen);
}
void MP2Node::stabilizationProtocol() {
	int i=0;
	for (i=0;i<ring.size();++i){
		if((ring[i].nodeAddress==this->memberNode->addr))
			break;
	}

	int p_2 = ((i-2)<0?i-2+ring.size():i-2)%ring.size();
	int p_1 = ((i-1)<0?i-1+ring.size():i-2)%ring.size() ;
	int n_1 = (i+1)%ring.size();
	int n_2 = (i+2)%ring.size();
	if(!inited){
		mylog("Created the member tables initially at ring pos %s",ring[i].nodeAddress.getAddress().c_str());
		haveReplicasOf.push_back(ring[p_2]);
		haveReplicasOf.push_back(ring[p_1]);
		hasMyReplicas.push_back(ring[n_1]);
		hasMyReplicas.push_back(ring[n_2]);
		inited = true;
	}else{
		mylog("Created the neighbours, position values are %s,%s,<%s>,%s,%s",
				ring[p_2].nodeAddress.getAddress().c_str(),
				ring[p_1].nodeAddress.getAddress().c_str(),
				ring[i].nodeAddress.getAddress().c_str(),
				ring[n_1].nodeAddress.getAddress().c_str(),
				ring[n_2].nodeAddress.getAddress().c_str());
		Node n1 = ring[n_1];
		Node n2 = ring[n_2];
		Node n3 = ring[p_1];
		if(!(n1.nodeAddress==hasMyReplicas[0].nodeAddress)){
			processReplicate(n2,ReplicaType::PRIMARY);
		}else if(!(n2.nodeAddress==hasMyReplicas[1].nodeAddress)){
			processReplicate(n2,ReplicaType::PRIMARY);
		}else if(!(n3.nodeAddress==haveReplicasOf[0].nodeAddress)){
			processReplicate(n2,ReplicaType::SECONDARY);
		}else{
		}
		haveReplicasOf.clear();
		hasMyReplicas.clear();
		haveReplicasOf.push_back(ring[p_2]);
		haveReplicasOf.push_back(ring[p_1]);
		hasMyReplicas.push_back(ring[n_1]);
		hasMyReplicas.push_back(ring[n_2]);
	}

}