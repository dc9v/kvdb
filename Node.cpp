
#include "Node.h"

Node::Node() {}

Node::Node(Address address) {
	this->nodeAddress = address;
	computeHashCode();
}

Node::~Node() {}

void Node::computeHashCode() {
	nodeHashCode = hashFunc(nodeAddress.addr)%RING_SIZE;
}

Node::Node(const Node& another) {
	this->nodeAddress = another.nodeAddress;
	this->nodeHashCode = another.nodeHashCode;
}

Node& Node::operator=(const Node& another) {
	this->nodeAddress = another.nodeAddress;
	this->nodeHashCode = another.nodeHashCode;
	return *this;
}

bool Node::operator < (const Node& another) const {
	return this->nodeHashCode < another.nodeHashCode;
}

size_t Node::getHashCode() {
	return nodeHashCode;
}

Address * Node::getAddress() {
	return &nodeAddress;
}

void Node::setHashCode(size_t hashCode) {
	this->nodeHashCode = hashCode;
}

void Node::setAddress(Address address) {
	this->nodeAddress = address;
}
