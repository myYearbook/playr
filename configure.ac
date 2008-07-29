# The libpq tests are taken from libpqxx

AC_INIT([playr], [0.2], [fkautz@myyearbook.com])
AM_INIT_AUTOMAKE([-Wall])

AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

AC_PATH_PROGS(PG_CONFIG, pg_config)
if test -z "$PG_CONFIG" || test ! -r "$PG_CONFIG"; then
	AC_MSG_ERROR([
PostgreSQL configuration script pg_config not found.  Make sure this is in your
command path before configuring.  Without it, the configure script has no way to
find the right location for the libpq library and its headers.
])
fi
with_postgres_include=`$PG_CONFIG --includedir`
AC_MSG_NOTICE([using PostgreSQL headers at $with_postgres_include])
with_postgres_lib=`$PG_CONFIG --libdir`
AC_MSG_NOTICE([using PostgreSQL libraries at $with_postgres_lib])

AC_SUBST(with_postgres_lib)
AC_SUBST(with_postgres_include)
#POSTGRES_LIB="-R${with_postgres_lib}"
AC_SUBST(POSTGRES_LIB)
POSTGRES_INCLUDE="-I${with_postgres_include}"
AC_SUBST(POSTGRES_INCLUDE)

AC_CHECK_HEADER([${with_postgres_include}/libpq-fe.h],[],[AC_MSG_ERROR([
Can't find libpq-fe.h in ${with_postgres_include}.  Are you sure the libpq
headers are installed correctly?  They should be in the directory returned by
"pg_config --includedir".

If you do have libpq (the C-language client library for PostgreSQL) installed,
make sure you have the related development materials--mainly its header files--
as well as the library binary.  Some system distributions keep the two in
seperate packages with names like "alibrary" and "alibrary-dev", respectively.
In that case, make sure you have the latter installed as well.
])])

AC_MSG_CHECKING([for ability to compile source files using libpq])
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>],[PQexec(0,"")],
[],
[AC_MSG_ERROR([
Could not compile a call to a basic libpq function.  There must be something
seriously wrong with the headers that "pg_config --includedir" pointed to; the
contents of config.log may give you a clue about the nature of the failure.
Source including the libpq header libpq-fe.h can be compiled, but a call to the
most basic libpq function PQexec() failed to compile successfully.  This is the
litmus test for a working libpq.
])])
AC_MSG_RESULT(yes)

# Perform this check in C; autoconf's AC_HAVE_LIBRARY breaks in C++ because it
# tries to call main()
AC_LANG_PUSH(C)

AC_HAVE_LIBRARY(pq, [], [AC_MSG_ERROR([
Could not link to libpq.  Make sure you have the PostgreSQL client library
installed, and that the library binary can be found in the location returned by
"pg_config --libdir".
])], -L${with_postgres_lib})

AC_CHECK_LIB([pq], [PQexec],[], [AC_MSG_ERROR([
Did not find the PQexec() function in libpq.  This is the litmus test for a
working libpq installation.

A source file using the PQexec() function did compile without problems, and the
libpq library is available for linking, but for some reason a call to PQexec()
failed to link properly to the libpq library.  This may be because the libpq
library file is damaged, or in some incorrect format. or if your libpq is much
more recent than libpqxx version @PQXXVERSION@, perhaps libpq has undergone a
radical ABI change.

The last parts of config.log may give you a clue as to what really went wrong,
but be warned that this is no easy reading.  Look for the last error message
occurring in the file.
])], -L${with_postgres_lib})

# Remove redundant occurrances of -lpq
LIBS="`echo "$LIBS" | sed -e 's/-lpq[[:space:]]*[[:space:]]-lpq\>/-lpq/g'`"

AC_LANG_POP(C)

LDFLAGS="$LDFLAGS -L${with_postgres_lib}"


AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
