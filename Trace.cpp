
#include "Trace.h"

int Trace::traceFileCreate() {

    int rc = SUCCESS;

    logF = fopen(LOG_FILE_LOCATION, "w");
    if ( NULL == logF )
    {
        printf("\nUnable to open log file in write mode\n");
        rc = FAILURE;
    }

    return rc;
}

int Trace::printToTrace(char *keyMsg, char *valueMsg) {

    int rc = SUCCESS;

    fprintf(logF, "%s : %s\n", keyMsg, valueMsg);
    fflush(logF);

    return rc;
}

int Trace::traceFileClose() {

    int rc = SUCCESS;

    fclose(logF);

    return rc;
}

int Trace::funcEntry(char *funcName) {

    int rc = SUCCESS;

    fprintf(logF, "ENTRY - %s\n", funcName);
    fflush(logF);

    return rc;
}

int Trace::funcExit(char *funcName, int f_rc) {

    int rc = SUCCESS;

    fprintf(logF, "EXIT - %s with rc = %d\n", funcName, f_rc);
    fflush(logF);

    return rc;
}
