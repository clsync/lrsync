/*
    lrsync - rsync-like clsync wrapper

    Copyright (C) 2014  Dmitry Yu Okunev <dyokunev@ut.mephi.ru> 0x8E30679C

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CLSYNCMGR_COMMON_H
#define __CLSYNCMGR_COMMON_H

#define PROGRAM "lrsync"
#define AUTHOR "Dmitry Yu Okunev <dyokunev@ut.mephi.ru> 0x8E30679C"
#define VERSION_MAJ 0
#define VERSION_MIN 1

#include <unistd.h>	/* gid_t */
#include <sys/types.h>	/* gid_t */

#include "configuration.h"

#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifdef _GNU_SOURCE
#	ifndef likely
#		define likely(x)    __builtin_expect(!!(x), 1)
#	endif
#	ifndef unlikely
#		define unlikely(x)  __builtin_expect(!!(x), 0)
#	endif
#else
#	ifndef likely
#		define likely(x)   (x)
#	endif
#	ifndef unlikely
#		define unlikely(x) (x)
#	endif
#endif

#define _TEMPLATE(X, Y) X ## _ ## Y
#define TEMPLATE(X, Y) _TEMPLATE(X, Y)


#define FLM_LONGOPTONLY (1<<8)

enum flag {
	FL_CLSYNCCOMMANDONLY = 0,
	FL_RSYNCCOMMANDS     = 1,

	FL_MAX

};
typedef enum flag flag_t;

struct ctx {
	flag_t flags[FL_MAX];

	char  clsync_shortopt[(1<<8)];
	char *clsync_longopt       [CLSYNC_MAXLONGOPTS+1];
	char  clsync_longopt_reqarg[CLSYNC_MAXLONGOPTS+1];

	char *rsync_argv [MAXARGUMENTS+1];
	char *clsync_argv[MAXARGUMENTS+1];
	int   rsync_argv_count;
	int   clsync_argv_count;

	char *dir_from;
	char *dir_to;
};
typedef struct ctx ctx_t;

#endif

