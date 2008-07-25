#ifndef PARSELINE_H_
#define PARSELINE_H_

#include <time.h>
#include "binarylog.h"

typedef struct
{
	struct tm ptime;
	int32_t ms;
} playr_time;

typedef struct
{
	time_t seconds;
	int ms;
} playr_seconds;

typedef struct
{
	playr_blog *log;
	playr_time *ptime;
        char *pname;
	char *logitem;
} log_elem;

int parse_file ( char *file, char *outfile, int *count, char *format );

/*@null@*/ log_elem *parseline ( char *string, char *format,
                                 playr_time * ptime, playr_types type, char* pname );

#endif				// PARSELINE_H_
