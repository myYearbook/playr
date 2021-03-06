/*
*  C Implementation: parseline
*
* Description:
*
*
* Author: Frederick F. Kautz IV <fkautz@myyearbook.com>, (C) 2008
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#define BUFSIZE 10000
#define DEFAULT_START_SIZE 10000

#ifndef DEBUG
//#define DEBUG
#endif				// DEBUG


#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "match.h"
#include "parseline.h"
#include "binarylog.h"

static pid_t parse_p(char *string, size_t * count);
/*@null@*/ static char *parse_c(char *string, size_t * count);
/*@null@*/ static char *parse_l(char *string, size_t * count);
/*@null@*/ static int parse_m(char *string, size_t * count,
			      playr_time * ptime, int ms);
/*@null@*/ static playr_seconds *parse_D(char *string, size_t * count);
/*@null@*/ static char* parse_N(char *string, size_t * count);

int parse_file(char *file, char *fileout, int *count, char *format)
{
    int i = 0;
    char lastquote = 0;
    int qflag = 0;
    int linecount = 0;

    FILE *fin = 0;
    FILE *fout = 0;
    char *sfout = 0;

    char *line = 0;
    char *retline = 0;
    size_t linesize = LINE_MAX;
    size_t read = 0;

    log_elem *log;
    playr_time ptime;
    time_t utime;
    time_t earliest = 0;

    char *line_format = 0;
    int line_format_length = 100;

    int process_detail = 0;
    char *detail_name = 0;
    
    detail_name = (char *) malloc(sizeof(char));

    line_format = (char *) malloc(sizeof(char) * line_format_length);

    line = (char *) malloc(sizeof(char) * linesize);
    if (line == 0) {
	printf("%s:%d: Could not allocate memory for line", __FILE__,
	       __LINE__);
    }

    fin = fopen(file, "r");
    if (fin == 0) {
	free(line);
	return -1;
    }

    read = 0;
    while (1) {
	if (linesize - (read + 1) < LINE_MAX) {
	    line = realloc(line, sizeof(char) * (linesize + LINE_MAX));
	    if (line == 0) {
		printf("%s:%d: Could not resize line[%d] to line[%d]\n",
		       __FILE__, __LINE__, linesize, linesize + LINE_MAX);
		continue;
	    }
	    linesize = linesize + LINE_MAX;
	}
	retline = fgets(line + read, LINE_MAX, fin);
	if (retline == 0)
	    break;
	read = strlen(line);
	if (line[read - 1] != '\n') {
	    continue;
	}
	// test for end of sql statement
	for (i = 0; i < read; i++) {
	    if (line[i] == '\'') {
		if (line == 0) {
		    qflag++;
		    continue;
		}
		if (line[i - 1] != '\'') {
		    qflag++;
		}
	    }
	}

	if (qflag % 2 == 1) {
	    continue;
	}

	line[read - 1] = '\0';

	if (linecount % 1000 == 0)
	    printf("line: %d\n", linecount);
	linecount++;

	strcpy(line_format, format);
	int line_format_length = strlen(format) - 2;
	strcpy((line_format + line_format_length),
	       "LOG:  duration: %D ms  statement: %S");
	log = parseline(line, line_format, &ptime, PLAYR_NORMAL_STATEMENT, 0);
	if(log != 0)
	  process_detail = 0;


	if (log == 0) {
	    // parse
	    strcpy(line_format, format);
	    int line_format_length = strlen(format) - 2;
	    strcpy((line_format + line_format_length),
		   "LOG:  duration: %D ms  parse <unnamed>: %S");
	    log =
		parseline(line, line_format, &ptime,
			  PLAYR_PREPARED_STATEMENT_PARSE,0);
	    if(log != 0) {
	      process_detail = 1;
	      detail_name = (char *)realloc(detail_name, sizeof(char)*2);
	      log->log->plength = 1;
	      strcpy(detail_name, " ");
	    }
	}

	if (log == 0) {
	    // detail

	    /* Refactor so that if the last detail was an execute, skip.
	     * 
	     * If a detail occured again without a preceeding parse <unnamed>
	     * or bind <nammed>, skip.
	     * 
	     * Notice we skip bind <unnamed> 
	     * This keeps multiple DETAILS from showing up and we avoid more
	     * unnecessary processing
	     */

	  if(process_detail == 1) {
	    strcpy(line_format, format);
	    int line_format_length = strlen(format) - 2;
	    strcpy((line_format + line_format_length),
		   "DETAIL:  parameters: %S");
	    log =
		parseline(line, line_format, &ptime,
			  PLAYR_PREPARED_STATEMENT_DETAIL,detail_name);
	  }
	  if(log != 0)
	    process_detail = 0;
	}

	if (log == 0) {
	    // execute
	    strcpy(line_format, format);
	    int line_format_length = strlen(format) - 2;
	    strcpy((line_format + line_format_length),
		   "LOG:  duration: %D ms  execute <unnamed>: %S");
	    log =
		parseline(line, line_format, &ptime,
			  PLAYR_PREPARED_STATEMENT_EXECUTE,0);
	    if(log != 0)
	      process_detail = 0;

	}

	if (log == 0) {
	    // named bind
	    strcpy(line_format, format);
	    int line_format_length = strlen(format) - 2;
	    strcpy((line_format + line_format_length),
		   "LOG:  duration: %D ms  bind %N: %S");
	    log =
		parseline(line, line_format, &ptime,
			  PLAYR_NAMED_STATEMENT_BIND,detail_name);
	    if(log != 0) {
	      printf("(bind) detail name: %s (%d)\n", log->pname, log->log->plength);
	    }
	    if(log != 0) {
		    process_detail = 1;
		    detail_name = (char *)realloc(detail_name, sizeof(char)*strlen(log->pname));
		    strcpy(detail_name, log->pname);
	    }
	}

	if (log == 0) {
	    // named execute
	    strcpy(line_format, format);
	    int line_format_length = strlen(format) - 2;
	    strcpy((line_format + line_format_length),
		   "LOG:  duration: %D ms  execute %N: %S");
	    log =
		parseline(line, line_format, &ptime,
			  PLAYR_NAMED_STATEMENT_EXECUTE,detail_name);
	    if(log != 0)
	      printf("(execute) detail name: %s\n", log->pname);
	  if(log != 0)
	    process_detail = 0;
	  
	}


	if (log == 0) {
	  // continue on to the next line since there was no match
	  if (!(*line != 0 && (strstr(line, "bind <unnamed>") != 0 || strstr(line, "DETAIL") != 0) || strstr(line, "DEBUG:")!=0)) {
#ifdef DEBUG // TODO remove this for non-alpha version
	    printf("Could not parse: %s\n\n", line);
#endif // DEBUG
	  }
	  // set read = 0 to continue on to next line
	  read = 0;
	  continue;
	}
#ifdef DEBUG
	printf("pid: %d type: %d plength: %d length: %d logitem: %s\n",
	       log->log->pid, log->log->type, log->log->plength, log->log->length);
#endif				// DEBUG

	sfout = (char *) malloc(sizeof(char) * (strlen(fileout) + 32));
	sprintf(sfout, "%s.%d", fileout, log->log->pid);

	utime = mktime(&(ptime.ptime));
	
	
	if(log->log->type == PLAYR_PREPARED_STATEMENT_DETAIL) {
		log->log->plength = sizeof(detail_name);
	}

	fout = fopen(sfout, "a");
	fwrite(log->log, sizeof(playr_blog), 1, fout);
	fwrite(&utime, sizeof(time_t), 1, fout);
	fwrite(&(ptime.ms), sizeof(int), 1, fout);
	if(log->log->plength > 0) {
		/*
		printf("type: %d\n", log->log->type);
		printf("log->log->plength: %d '%s' '%s'\n", log->log->plength, log->logitem, detail_name);
		*/
		fwrite(detail_name, sizeof(char), log->log->plength, fout);
	}
	fwrite(log->logitem, sizeof(char), log->log->length, fout);
	fclose(fout);
	if (utime < earliest || earliest == 0) {
	    earliest = utime;
	}
	free(sfout);
	if(log->pname != 0)
	  free(log->pname);
	free(log->logitem);
	free(log->log);
	free(log);

	qflag = 0;
	read = 0;
    }
    fout = fopen(fileout, "a");
    fwrite(&earliest, sizeof(time_t), 1, fout);
    fclose(fout);

    free(line_format);

    printf("done\n");

    fclose(fin);
    free(line);

    return 0;
}


log_elem *parseline(char *string, char *format, playr_time * ptime,
		    playr_types type, char *pnamed)
{
    char *stringiter = 0;
    char *formatiter = 0;
    log_elem *log = 0;
    playr_time *stime = 0;
    playr_seconds *duration = 0;
    size_t count = 0;
    int logerror = 0;
    char *result = 0;
    char *logitem = 0;
    char *dtoks = 0;
    int istok = 0;
    size_t toksize = 0;
    size_t pos;
    char *pname = 0;

    if (ptime == 0)
	return 0;

    result = strstr(string, "DEBUG");
    if (result != 0) {
	return 0;
    }

    result = strstr(string, "bind <unnamed>");
    if (result != 0) {
	return 0;
    }


    stringiter = string;
    formatiter = format;
    count = (size_t) 0;
    log = (log_elem *) malloc(sizeof(log_elem));
    if (log == 0)
	return (log_elem *) 0;
    log->log = (playr_blog *) malloc(sizeof(playr_blog));
    if (log->log == 0) {
	free(log);
	return (log_elem *) 0;
    }
    log->log->pid = 0;
    log->log->type = PLAYR_NORMAL_STATEMENT;
    log->log->plength = 0;
    log->log->length = 0;

    logitem = 0;

    while (*formatiter != 0) {
#ifdef DEBUG
	printf("stringiter: '%s'\nformatiter: '%s'\n\n", stringiter,
	       formatiter);
	fflush(0);
#endif				// DEBUG
	if (*formatiter == '%') {
	    formatiter++;
	    if (*formatiter == 'p') {
		log->log->pid = parse_p(stringiter, &count);
		stringiter = stringiter + count;
	    } else if (*formatiter == 'c') {
		result = parse_c(stringiter, &count);
		if (result != 0)
		    free(result);
		stringiter = stringiter + count;
	    } else if (*formatiter == 'm') {
		logerror = parse_m(stringiter, &count, ptime, 1);
		if (logerror == 1) {
		    goto parseline_cleanup;
		}
		stringiter = stringiter + count;
	    } else if (*formatiter == 's') {
		logerror = parse_m(stringiter, &count, 0, 0);
		if (logerror == 1) {
		    printf("%s:%d:Could not parse session start time\n",
			   __FILE__, __LINE__);
		    goto parseline_cleanup;
		}
		stringiter = stringiter + count;
		free(stime);
	    } else if (*formatiter == 'l') {
		result = parse_l(stringiter, &count);
		if (result == 0) {
		    printf("%s:%d:Could not parse session log line\n",
			   __FILE__, __LINE__);
		    logerror = 1;
		    goto parseline_cleanup;
		}
		stringiter = stringiter + count;
		free(result);
	    } else if (*formatiter == 'D') {
		duration = parse_D(stringiter, &count);
		if (result == 0) {
		    logerror = 1;
		    goto parseline_cleanup;
		}
		stringiter = stringiter + count;
	    } else if (*formatiter == 'N') {
	      pname = parse_N(stringiter, &count);
	      printf("Parse name = %s\n", pname);
	      log->log->plength = count;
	      stringiter = stringiter + count;

	    } else if (*formatiter == 'S') {
		if (type == PLAYR_NORMAL_STATEMENT) {
		    count = strlen(stringiter) + 1;
		    logitem = (char *) malloc(sizeof(char) * count);
		    if (logitem == 0) {
			(":%s:%d:Could not instantiate logitem\n",
			 __FILE__, __LINE__);
			logerror = 1;
			goto parseline_cleanup;
		    }
		    strcpy(logitem, stringiter);
		    logitem[count - 1] = (char) 0;
		    log->log->length = count - 1;
		    log->log->type = type;
		    break;
		} else if (type == PLAYR_PREPARED_STATEMENT_PARSE) {
		    count = strlen(stringiter) + 1;
		    logitem = (char *) malloc(sizeof(char) * count);
		    if (logitem == 0) {
			(":%s:%d:Could not instantiate logitem\n",
			 __FILE__, __LINE__);
			logerror = 1;
			goto parseline_cleanup;
		    }
		    strcpy(logitem, stringiter);
		    logitem[count - 1] = (char) 0;
		    log->log->length = count - 1;
		    log->log->type = type;
		    break;
		} else if (type == PLAYR_PREPARED_STATEMENT_DETAIL) {
			/*
			 * get statement detail name from global variable
			 * reset name after this section completes
			 */
		    pos = 0;
#ifdef DEBIG
		    printf("Detail before strtok: %s\n", stringiter);
#endif				//DEBUG
		    count = strlen(stringiter) + 1;
		    dtoks = (char *) strtok(stringiter, "'");
		    logitem =
			(char *) malloc(sizeof(char) * count *
					sizeof(size_t));
		    while (dtoks != 0) {
			if (istok) {
			    toksize = (size_t) strlen(dtoks);
			    toksize += 1;
			    memcpy(logitem + pos, &toksize,
				   sizeof(size_t));
			    toksize -= 1;
			    pos += sizeof(size_t);

			    memcpy(logitem + pos, dtoks, toksize);
			    pos += toksize;
			    logitem[pos] = 0;
			    pos++;
#ifdef DEBUG
			    printf("detail: %d: %s\n", strlen(dtoks),
				   dtoks);
#endif	/* DEBUG */
			}
			dtoks = strtok(0, "'");
			istok = ~istok;
		    }
		    toksize = 0;
		    memcpy(logitem + pos, &toksize, sizeof(size_t));
		    pos += sizeof(size_t);
		    log->log->type = type;
		    log->log->length = pos;
		    break;
		} else if (type == PLAYR_PREPARED_STATEMENT_EXECUTE) {
		    count = strlen(stringiter) + 1;
		    logitem = (char *) malloc(sizeof(char) * count);
		    if (logitem == 0) {
			(":%s:%d:Could not instantiate logitem\n",
			 __FILE__, __LINE__);
			logerror = 1;
			goto parseline_cleanup;
		    }
		    strcpy(logitem, stringiter);
		    logitem[count - 1] = (char) 0;
		    log->log->length = count - 1;
		    log->log->type = type;
		    break;
		} else if (type == PLAYR_NAMED_STATEMENT_BIND) {
		  // placeholder begin
		  count = strlen(stringiter) + 1;
		  logitem = (char *) malloc(sizeof(char) * count);
		  if (logitem == 0) {
		    (":%s:%d:Could not instantiate logitem\n",
		     __FILE__, __LINE__);
		    logerror = 1;
		    goto parseline_cleanup;
		  }
		  strcpy(logitem, stringiter);
		  logitem[count - 1] = (char) 0;
		  log->log->length = count - 1;
		  log->log->type = type;
		  // set global variables specifying the statement name
		  break;
		  // placeholder end
		} else if (type == PLAYR_NAMED_STATEMENT_EXECUTE) {
		  // placeholder begin
		  count = strlen(stringiter) + 1;
		  logitem = (char *) malloc(sizeof(char) * count);
		  if (logitem == 0) {
		    (":%s:%d:Could not instantiate logitem\n",
		     __FILE__, __LINE__);
		    logerror = 1;
		    goto parseline_cleanup;
		  }
		  strcpy(logitem, stringiter);
		  logitem[count - 1] = (char) 0;
		  log->log->length = count - 1;
		  log->log->type = type;
		  break;
		  // placeholder end
		} else {
		    logerror = 1;
		    goto parseline_cleanup;
		}
	    }
	    formatiter++;
	} else if (*formatiter != *stringiter) {
	    logerror = 1;
	    goto parseline_cleanup;
	} else {
	    formatiter++;
	    stringiter++;
	}
    }


  parseline_cleanup:
    if (duration != 0) {
	free(duration);
    }

    if (ptime == 0) {
	logerror = 1;
    }

    if (log == 0)
	return (log_elem *) 0;

    if (logitem == 0) {
	if (log->log != 0) {
	    free(log->log);
	}
	free(log);
	return (log_elem *) 0;
    }

    if (logerror == 1) {
	if (log != 0) {
	    if (log->log != 0) {
		free(log->log);
	    }
	    if (logitem != 0) {
		free(logitem);
	    }
	    free(log);
	}
	return (log_elem *) 0;
    }
    log->logitem = logitem;
    log->pname = pname;
    

    return log;
}

static pid_t parse_p(char *string, size_t * count)
{
    char *result;
    pid_t p;

    *count = 0;

    if (string == 0)
	return 0;

    if (count == 0)
	return 0;


    result = match(string, "^[0-9]{1,}");

    if (result == 0) {
	printf("%s:%d:Could not match pid\n", __FILE__, __LINE__);
	return 0;
    }

    p = (pid_t) atoi(result);
    if (p == 0)
	printf("%s:%d:Could not parse pid\n", __FILE__, __LINE__);
    *count = strlen(result);
    free(result);
    return p;
}

static char *parse_c(char *string, size_t * count)
{
    char *result;
    if (string == 0)
	return 0;

    if (count == 0)
	return 0;

    result = match(string, "^[a-zA-Z0-9.]{1,}");

    if (result == 0) {
	return (char *) 0;
    }

    *count = strlen(result);
    return result;
}

static char *parse_l(char *string, size_t * count)
{
    char *result;
    if (string == 0)
	return 0;
    if (count == 0)
	return 0;

    result = match(string, "^[0-9]{1,}");

    if (result == 0) {
	return (char *) 0;
    }

    *count = strlen(result);
    return result;
}

static playr_seconds *parse_D(char *string, size_t * count)
{
    time_t val;
    char *result;
    char *iter;
    playr_seconds *duration;
    if (string == 0)
	return 0;
    if (count == 0)
	return 0;

    iter = string;

    duration = (playr_seconds *) malloc(sizeof(playr_seconds));
    if (duration == 0)
	return 0;

    result = match(iter, "^[0-9]{1,}");

    if (result == 0) {
	free(duration);
	return (playr_seconds *) 0;
    }

    *count = strlen(result);
    val = (time_t) atoi(result);

    duration->seconds = (time_t) val;
    free(result);

    iter = string + *count;

    result = match(iter, "^[.]");
    if (result == 0) {
	free(duration);
	return (playr_seconds *) 0;
    }
    *count += 1;
    free(result);

    iter = string + *count;
    result = match(iter, "^[0-9]{1,3}");

    *count += strlen(result);
    val = (time_t) atoi(result);
    duration->ms = (int) val;
    free(result);

    return duration;
}

static int parse_m(char *string, size_t * count, playr_time * ptime,
		   int ms)
{
    char *iter = 0;
    char *result = 0;
    int intresult = 0;

    iter = string;
    if (count == 0)
	return 1;

    *count = 0;

    if (string == 0)
	return 1;

    if (ptime != 0) {
	ptime->ptime.tm_sec = 0;
	ptime->ptime.tm_min = 0;
	ptime->ptime.tm_hour = 0;
	ptime->ptime.tm_mday = 1;
	ptime->ptime.tm_mon = 0;
	ptime->ptime.tm_year = 0;
	ptime->ptime.tm_wday = 0;
	ptime->ptime.tm_yday = 0;
	ptime->ptime.tm_isdst = -1;
	ptime->ms = 0;
    }

    result = match(iter, "^[0-9]{4,4}");

    if (result == 0) {
	return 1;
    }
    *count = strlen(result);
    intresult = atoi(result);
    if (intresult == 0) {
	free(result);
	return 1;
    }
    if (ptime != 0) {
	ptime->ptime.tm_year = intresult - 1900;
    }
    iter = string + *count;
    free(result);

    result = match(iter, "^-");
    if (result == 0) {
	return 1;
    }
    *count += 1;
    iter = string + *count;
    free(result);

    result = match(iter, "^[0-9]{2,2}");
    if (result == 0) {
	return 1;
    }
    *count += strlen(result);
    intresult = atoi(result);
    if (intresult < 1 || intresult > 12) {
	free(result);
	return 1;
    }
    if (ptime != 0) {
	ptime->ptime.tm_mon = intresult - 1;
    }
    iter = string + *count;
    free(result);

    result = match(iter, "^-");
    if (result == 0) {
	return 1;
    }
    *count += 1;
    iter = string + *count;
    free(result);

    result = match(iter, "^[0-9]{2,2}");
    if (result == 0) {
	return 1;
    }
    *count += strlen(result);
    intresult = atoi(result);
    if (intresult < 1 || intresult > 31) {
	free(result);
	return 1;
    }
    if (ptime != 0) {
	ptime->ptime.tm_mday = intresult;
    }
    iter = string + *count;
    free(result);

    result = match(iter, "^ ");
    if (result == 0) {
	return 1;
    }
    *count += 1;
    iter = string + *count;
    free(result);

    result = match(iter, "^[0-9]{2,2}");
    if (result == 0) {
	return 1;
    }
    *count += strlen(result);
    intresult = atoi(result);
    if (intresult < 0 || intresult > 23) {
	free(result);
	return 1;
    }
    if (ptime != 0) {
	ptime->ptime.tm_hour = intresult;
    }
    iter = string + *count;
    free(result);

    result = match(iter, "^:");
    if (result == 0) {
	return 1;
    }
    *count += 1;
    iter = string + *count;
    free(result);

    result = match(iter, "^[0-9]{2,2}");
    if (result == 0) {
	return 1;
    }
    *count += strlen(result);
    intresult = atoi(result);
    if (intresult < 0 || intresult > 59) {
	free(result);
	return 1;
    }
    if (ptime != 0) {
	ptime->ptime.tm_min = intresult;
    }
    iter = string + *count;
    free(result);

    result = match(iter, "^:");
    if (result == 0) {
	return 1;
    }
    *count += 1;
    iter = string + *count;
    free(result);

    result = match(iter, "^[0-9]{2,2}");
    if (result == 0) {
	return 1;
    }
    *count += strlen(result);
    intresult = atoi(result);
    if (intresult < 0 || intresult > 59) {
	free(result);
	return 1;
    }
    if (ptime != 0) {
	ptime->ptime.tm_sec = intresult;
    }
    iter = string + *count;
    free(result);

    if (ms == 1) {
	result = match(iter, "^[.]");
	if (result == 0) {
	    return 1;
	}
	*count += 1;
	iter = string + *count;
	free(result);


	result = match(iter, "^[0-9]{3,3}");
	if (result == 0) {
	    return 1;
	}
	*count += strlen(result);
	intresult = atoi(result);
	if (intresult < 0 || intresult > 999) {
	    free(result);
	    return 1;
	}
	if (ptime != 0) {
	    ptime->ms = intresult;
	}
	iter = string + *count;
	free(result);
    }

    result = match(iter, "^ [a-zA-Z]{1,}");
    if (result == 0) {
	return 1;
    }
    *count += strlen(result);
    iter = string + *count;
    free(result);
    return 0;
}

static char* parse_N(char *string, size_t * count)
{
    char *result;
    if (string == 0)
	return 0;
    if (count == 0)
	return 0;

    result = match(string, "^[A-Za-z1-9_]{1,}");

    if (result == 0) {
	return (char *) 0;
    }

    *count = strlen(result);
    return result;
}

////    strcpy(prefixstring, "%m [%p]: [%s %c-%l] LOG:  duration: %D ms  statement: %S");
