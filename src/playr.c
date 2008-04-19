#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <postgresql/libpq-fe.h>
#include <dirent.h>

#include "binarylog.h"

#define DEBUG

struct timespec *get_offset(time_t basetime, time_t offsettime,
			    int offsetms);
struct timespec *set_offset(struct timespec *baseoffset);
struct timespec *get_sleep_time(struct timespec *baseoffset,
				struct timeval *currentoffset);

time_t basetime = 0;
time_t starttime = 0;

int main(int argc, char **argv)
{
    char *file = 0;
    char *substr = 0;
    size_t filesize = 100;
    pid_t pid = 0;
    int ret;
    DIR *dir = 0;
    FILE *configfile = 0;
    struct dirent *nextfile = 0;
    
    if (argc < 2 || strcmp(argv[1],"--help")==0) {
        print_help();
	return -1;
    }

    filesize = strlen(argv[argc-1]);
    file = (char *) malloc(sizeof(char) * (filesize + 2));
    if (file == 0)
	return -1;
    strcpy(file, argv[argc-1]);
    file[filesize] = '.';
    file[filesize + 1] = '\0';

    dir = opendir(".");

    configfile = fopen(argv[argc-1], "r");
    if(configfile == 0) {
      printf("Could not find file: `%s`\n", argv[argc-1]);
      return -1;
    }
    fread(&basetime, sizeof(time_t), 1, configfile);
    fclose(configfile);
    basetime = basetime - 5;
    starttime = time(0);

#ifdef DEBUG
    printf("substring matcher: %s\n", file);
#endif				// DEBUG

    while (nextfile = readdir(dir)) {
	substr = strstr((char *) &(nextfile->d_name), file);
#ifdef DEBUG
	printf("substr: %s\n", substr);
#endif				// DEBUG
	if (substr == (char *) &(nextfile->d_name)) {
	    pid = fork();
	    if(pid == -1) {
	      printf("fork failed for %s\n", pid);
	      continue;
	    }
	    if (pid != 0) {
		printf("Playing %s\n", substr);
		// TODO: Allow configuration string to be set by user.
		ret =
		    replay_log(substr,
			       "");
		return ret;
	    }
	}
	substr = 0;
    }
    closedir(dir);
    return 0;
}

int replay_log(const char *file, const char *connstring)
{
    PGconn *conn = 0;
    PGresult *result = 0;
    FILE *fin = 0;
    char *line = 0;
    size_t linesize = 2048;
    struct timespec *baseoffset;
    struct timespec *currentoffset;
    playr_blog log;
    time_t seconds = 0;
    int ms = 0;
    struct timeval *before;
    struct timeval *after;
    char *filename = 0;
    size_t filenamelength = 0;
    size_t itemsread = 0;

    line = (char *) malloc(sizeof(char) * linesize);
    if (line == 0) {
	printf("%s:%d: could not instantiate memory for line\n");
	return -1;
    }

    fin = fopen(file, "r");
    if (fin == 0) {
	printf("%s:%d: Could not open %s\n", __FILE__, __LINE__, filename);
	free(line);
	return -1;
    }

    before = (struct timeval *) malloc(sizeof(struct timeval));
    after = (struct timeval *) malloc(sizeof(struct timeval));



    conn = PQconnectdb(connstring);

    if (PQstatus(conn) != CONNECTION_OK) {
	fprintf(stderr, "Connection to database failed: %s",
		PQerrorMessage(conn));
	return -1;
    }


    if (conn == 0) {
	printf("%s:%d: Could not initiate connection for %d", __FILE__,
	       __LINE__, file);
    }

    while (!feof(fin)) {
        itemsread = fread(&(log.pid), sizeof(pid_t), 1, fin);
	if(itemsread != 1) {
	  break;
	}
	fread(&(log.type), sizeof(playr_types), 1, fin);
	fread(&(log.length), sizeof(size_t), 1, fin);
#ifdef DEBUG
	printf("log: pid: %d type: %d length: %d\n", log.pid, log.type,
	       log.length);
#endif				// DEBUG
	fread(&seconds, sizeof(time_t), 1, fin);
	fread(&ms, sizeof(int), 1, fin);
	if (linesize <= log.length) {
	    line = realloc(line, sizeof(char) * (log.length + 1));
	    linesize = log.length + 1;
	}
	fread(line, sizeof(char), log.length, fin);
	line[log.length] = '\0';
#ifdef DEBUG
	printf("%s\n", line);
#endif				//DEBUG

	// base offset = logtime - basetime
	baseoffset = get_offset(basetime, seconds, ms);
	gettimeofday(before, 0);

	currentoffset = set_offset(baseoffset);
	if (currentoffset != 0) {
#ifdef DEBUG
	    printf("%d: sleeping %d seconds, %d nanoseconds\n", time(0),
		   currentoffset->tv_sec, currentoffset->tv_nsec);
#endif				// DEBUG
	    nanosleep(currentoffset, 0);
	    gettimeofday(after, 0);
	} else {
#ifdef DEBUG
	    printf("%d: not sleeping\n", time(0));
#endif				// DEBUG
	}
	free(baseoffset);
	free(currentoffset);

	result = PQexec(conn, line);

	if (!
	    (PQresultStatus(result) == PGRES_TUPLES_OK
	     || PQresultStatus(result) == PGRES_COMMAND_OK)) {
#ifdef DEBUG
	    printf("command failed: %s\n", PQerrorMessage(conn));
#endif				//DEBUG
	}
	PQclear(result);
    }


    free(before);
    free(after);
    PQfinish(conn);
    fclose(fin);
    free(line);
    return 0;
}

struct timespec *get_offset(time_t basetime, time_t offsettime,
			    int offsetms)
{
    struct timespec *offset = 0;

    if (offsetms < 0 || offsetms >= 1000) {
	printf("%s:%d: offsetms out of range", __FILE__, __LINE__);
	return 0;
    }

    offset = (struct timespec *) malloc(sizeof(struct timespec));
    if (offset == 0) {
	printf("%s:%d: Could not instantiate memory for timespec\n",
	       __FILE__, __LINE__);
	return 0;
    }

    offset->tv_sec = offsettime - basetime;
    offset->tv_nsec = offsetms * 1000000;

    return offset;
}

/*
 * takes an offset, and returns with a timespec that is equal
 * to the amount of time needed to reach that offset from the start of the application
 */
struct timespec *set_offset(struct timespec *baseoffset)
{
    struct timespec *offset;
    struct timeval currenttime;

    offset = (struct timespec *) malloc(sizeof(struct timespec));
    if (offset == 0)
	return (struct timespec *) 0;
    offset->tv_sec = 0;
    offset->tv_nsec = 0;

    gettimeofday(&currenttime, 0);
#ifdef DEBUG
    printf("%d - %d = %d > %d\n", currenttime.tv_sec, starttime,
	   currenttime.tv_sec - starttime, baseoffset->tv_sec);
#endif				//DEBUG

    if (currenttime.tv_sec - starttime > baseoffset->tv_sec) {
	free(offset);
	return 0;
    }

    if (currenttime.tv_sec - starttime == baseoffset->tv_sec) {
	offset->tv_sec = 0;
	if (currenttime.tv_usec * 1000 >= baseoffset->tv_nsec) {
	    free(offset);
	    return 0;
	} else {
	    offset->tv_nsec =
		baseoffset->tv_nsec - currenttime.tv_usec * 1000;
	}
    } else {
	offset->tv_sec =
	    baseoffset->tv_sec - (currenttime.tv_sec - starttime);
	if (currenttime.tv_usec * 1000 >= baseoffset->tv_nsec) {
	    offset->tv_nsec =
		(1000000000 - currenttime.tv_usec * 1000) +
		baseoffset->tv_nsec;
	    offset->tv_sec = offset->tv_sec - 1;
	} else {
	    offset->tv_nsec =
		baseoffset->tv_nsec - (currenttime.tv_usec * 1000);
	}
    }

    return offset;
}

print_help(void) {
  printf("Usage: playr [FILE]\n"
	 "Replays a binary log FILE\n"
	 );
}
