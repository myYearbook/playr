/*
*  C Implementation: match
*
* Description: 
*
*
* Author: Frederick F. Kautz IV <fkautz@myyearbook.com>, (C) 2008
*
* Copyright: See COPYING file that comes with this distribution
*
*/



#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "match.h"


/*
 * Match string against the extended regular expression in
 * pattern, treating errors as no match.
 *
 * Return new string if matches
 */

/*@null@*/ char *match(char *string, char *regex)
{
    regex_t preg;
    size_t nmatch = (size_t) 1;
    regmatch_t *pmatch;
    char *result;
    unsigned long long int resultlength;
    int ret = 0;

    result = 0;

    if (string == 0) {
	return 0;
    }

    if (regex == 0) {
	return 0;
    }

    pmatch = (regmatch_t *) malloc(sizeof(regmatch_t));
    if (pmatch == 0) {
	printf("%s:%d: pmatch memory allocation failed", __FILE__,
	       __LINE__);
	return 0;
    }

    ret = regcomp(&preg, regex, REG_EXTENDED);

    ret = regexec(&preg, string, nmatch, pmatch, 0);

    if (ret != 0) {
	regfree(&preg);
	free(pmatch);
	return 0;
    }

    if (pmatch[0].rm_eo > strlen(string)) {
	regfree(&preg);
	free(pmatch);
	return 0;
    }

    resultlength = (size_t) pmatch[0].rm_eo - (size_t) pmatch[0].rm_so;

    result = (char *) malloc(sizeof(char) * (resultlength + 1));

    if (result == 0) {
	printf("%s:%d: pmatch memory allocation failed", __FILE__,
	       __LINE__);
	free(pmatch);
	return 0;
    }
    strncpy(result, string + pmatch[0].rm_so, resultlength);
    result[resultlength] = (char) 0;

    regfree(&preg);
    free(pmatch);

    return result;
}
