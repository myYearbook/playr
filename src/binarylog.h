//
// C Interface: binaryfile
//
// Description:
//
//
// Author: Frederick F. Kautz IV <fkautz@myyearbook.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef BINARYFILE_H_
#define BINARYFILE_H_

#include <sys/types.h>

typedef enum
{
	PLAYR_NORMAL_STATEMENT,
	PLAYR_PREPARED_STATEMENT_PARSE,
	PLAYR_PREPARED_STATEMENT_DETAIL,
	PLAYR_PREPARED_STATEMENT_EXECUTE,
	PLAYR_NAMED_STATEMENT_BIND,
	PLAYR_NAMED_STATEMENT_EXECUTE
} playr_types;

typedef struct
{
  pid_t pid;
  playr_types type;
  size_t plength; // prepared statement name length
  size_t length;
} playr_blog;

#endif				/* BINARYFILE_H_ */
