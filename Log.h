
#ifndef _LOG_H_
#define _LOG_H_

#include "stdincludes.h"
#include "Params.h"
#include "Member.h"

#define MAXWRITES 1
#define MAGIC_NUMBER "CS425"
#define DBG_LOG "dbg.log"
#define STATS_LOG "stats.log"

class Log{
private:
	Params *par;
	bool firstTime;
public:
	Log(Params *p);
	Log(const Log &anotherLog);
	Log& operator = (const Log &anotherLog);
	virtual ~Log();
	void LOG(Address *, const char * str, ...);
	void logNodeAdd(Address *, Address *);
	void logNodeRemove(Address *, Address *);
	void logCreateSuccess(Address * address, bool isCoordinator, int transID, string key, string value);
	void logReadSuccess(Address * address, bool isCoordinator, int transID, string key, string value);
	void logUpdateSuccess(Address * address, bool isCoordinator, int transID, string key, string newValue);
	void logDeleteSuccess(Address * address, bool isCoordinator, int transID, string key);
	void logCreateFail(Address * address, bool isCoordinator, int transID, string key, string value);
	void logReadFail(Address * address, bool isCoordinator, int transID, string key);
	void logUpdateFail(Address * address, bool isCoordinator, int transID, string key, string newValue);
	void logDeleteFail(Address * address, bool isCoordinator, int transID, string key);
};

#endif
