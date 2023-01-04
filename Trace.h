
#ifndef TRACE_H_
#define TRACE_H_

#include "stdincludes.h"

#define LOG_FILE_LOCATION "machine.log"

class Trace {
public:
	FILE *logF;
	int traceFileCreate();
	int printToTrace(
             char *keyMessage,
             char *valueMessage
             );
	int traceFileClose();
	int funcEntry(
              char *valueMessage
			  );
	int funcExit(
             char *valueMessage,
             int f_rc = SUCCESS
             );
};

#endif
