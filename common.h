#ifndef COMMON_H_
#define COMMON_H_

static int g_transID = 0;

enum MessageType {CREATE, READ, UPDATE, DELETE, REPLY, READREPLY};
enum ReplicaType {PRIMARY, SECONDARY, TERTIARY};

#endif
