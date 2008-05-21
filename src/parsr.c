/*
*  C Implementation: parsr
*
* Description: 
*
*
* Author: Frederick F. Kautz IV <fkautz@myyearbook.com>, (C) 2008
*
* Copyright: See COPYING file that comes with this distribution
*
*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "match.h"
#include "parseline.h"

static void print_help(void);

int main(int argc, char **argv)
{
    int i = 0;
    size_t length = 0;
    char *prefixstring = 0;
    char *logfile = 0;
    char *outputfile = 0;
    int count = 0;

    printf("%d\n", argc);
    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
	    print_help();
	    return -1;
	}
	if (strcmp(argv[i], "-p") == 0) {
	    if (i + 1 == argc) {
		printf("no config file specified after -p\n");
		return -1;
	    }
	    prefixstring = argv[i + 1];
	    i++;
	    continue;
	}
	if (strcmp(argv[i], "-o") == 0) {
	    if (outputfile != 0) {
		printf("multiple output files specified");
		return -1;
	    }
	    if (i + 1 == argc) {
		printf("no output file specified after --output");
		return -1;
	    }
	    outputfile = argv[i + 1];
	    i++;
	    continue;
	}
	if (logfile == 0) {
	    logfile = argv[i];
	    continue;
	} else {
	    printf("multiple log files not supported\n");
	}

    }

    // DEFAULT VALUES
    if (prefixstring == 0) {
        length = strlen("%m [%p]: [%s %c-%l] %S");
	prefixstring = (char *) malloc(sizeof(char) * length+1);
	if (prefixstring == 0) {
	    printf("%s:%d: Could not allocate memory",__FILE__,__LINE__);
	    return -1;
	}
	strcpy(prefixstring, "%m [%p]: [%s %c-%l] %S");
    }
    if (outputfile == 0) {
	outputfile = (char *) malloc(sizeof(char) * 9);
	if (outputfile == 0) {
	    printf("%s:%d: Could not allocate memory",__FILE__,__LINE__);
	    return -1;
	}
	strcpy(outputfile, "file.out");
    }

    if (logfile == 0) {
	printf("logfile not specified\n");
	return -1;
    }
    printf("config file: %s\n", prefixstring);
    printf("output file: %s\n", outputfile);
    printf("log file: %s\n", logfile);
    parse_file(logfile, outputfile, &count, prefixstring);
      
    free(prefixstring);
    return 0;
}


static void print_help(void)
{
    printf("Usage: parsr [OPTION] [FILE]\n"
	   "Converts a PostgreSQL log FILE to a binary format for Playr\n"
	   "\n"
	   "  -h, --help                display this help and exit\n"
	   "  -p \"FORMAT\"               specify a prefix format to use, default is \n"
	   "  -o [FILE]                 specify what the output file should be\n"
	   "                            \"%%m [%%p]: [%%s %%c-%%l] LOG:  duration: %%D ms  statement: %%S\"\n"
	   "\n"
	   "Examples:\n"
	   "  parsr pgsql.log                     Converts pgsql.log to binary format using default prefix\n"
	   "  parsr -c \"%%m [%%p]\" pgsql.log  Converts pgsql.log to binary format\n");

}
