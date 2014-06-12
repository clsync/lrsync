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

#include <unistd.h>	// fork()
#include <stdlib.h>	// getenv()
#include <stdarg.h>	// va_start()
#include <string.h>	// memset()
#include <errno.h>	// errno
#include <sys/types.h>	// waitpid()
#include <sys/wait.h>	// waitpid()

#include "common.h"
#include "error.h"
#include "malloc.h"
#include "lrsync.h"

int forkexecwaitvp(const char *const bin_path, const char **const argv)
{
	pid_t pid;
	int status;

	// Forking
	pid = fork();
	switch(pid) {
		case -1: 
			error("Cannot fork().");
			return errno;
		case  0:
			argv[0] = bin_path;
			execvp(bin_path, (char *const *)argv);
			return errno;
	}

	if(waitpid(pid, &status, 0) != pid) {
		error("Cannot waitid().");
		return errno;
	}

	// Return
	int exitcode = WEXITSTATUS(status);

	return exitcode;
}

#define arglist2argv(argv, firstarg, COPYARG) {\
	va_list arglist;\
	va_start(arglist, firstarg);\
\
	int i = 1;\
	do {\
		char *arg;\
		if(i >= MAXARGUMENTS) {\
			error("Too many arguments (%i >= %i).", i, MAXARGUMENTS);\
			return ENOMEM;\
		}\
		arg = (char *)va_arg(arglist, const char *const);\
		argv[i] = arg!=NULL ? COPYARG : NULL;\
	} while(argv[i++] != NULL);\
	va_end(arglist);\
}

const char *path_rsync() {
	char *rsync_path;
	rsync_path = getenv("RSYNC_PATH");
	if (rsync_path == NULL)
		rsync_path = DEFAULT_RSYNC_PATH;

	return rsync_path;
}

const char *path_clsync() {
	char *clsync_path;
	clsync_path = getenv("CLSYNC_PATH");
	if (clsync_path == NULL)
		clsync_path = DEFAULT_CLSYNC_PATH;

	return clsync_path;
}

const char *path_listsdir() {
	char *listsdir_path;
	listsdir_path = getenv("CLSYNC_LISTS_PATH");
	if (listsdir_path == NULL)
		listsdir_path = DEFAULT_LISTSDIR_PATH;

	return listsdir_path;
}

static inline int exec_rsync_argv(const char **argv)
{
	return forkexecwaitvp(path_rsync(), argv);
}

static inline int exec_clsync_argv(const char **argv)
{
	return forkexecwaitvp(path_clsync(), argv);
}

int exec_rsync(ctx_t *const ctx_p, ...)
{
	char **argv = (char **)xcalloc(sizeof(char *), MAXARGUMENTS);
	memset(argv, 0, sizeof(char *)*MAXARGUMENTS);
	arglist2argv(argv, ctx_p, arg);

	int rc = exec_rsync_argv((const char **)argv);
	free(argv);
	return rc;
}

int exec_clsync(ctx_t *const ctx_p, ...)
{
	char **argv = (char **)xcalloc(sizeof(char *), MAXARGUMENTS);
	memset(argv, 0, sizeof(char *)*MAXARGUMENTS);
	arglist2argv(argv, ctx_p, arg);

	int rc = exec_clsync_argv((const char **)argv);
	free(argv);
	return rc;
}

int print_cmd(const char *const *const argv) {
	const char *const *argv_p;

	argv_p = argv;
	if (*argv_p != NULL)
		printf("%s ", *(argv_p++));

	while (*argv_p != NULL)
		printf("\\\n  %s ", *(argv_p++));

	printf("\n\n");
	return 0;
}

static inline void push_arg(const char **argv, int *count_p, const char *arg) {
	if ((*count_p) >= MAXARGUMENTS) {
		errno = E2BIG;
		critical("Too many arguments");
	}
	argv[(*count_p)++] = arg;
	return;
}

int lrsync(ctx_t *const ctx_p)
{
	int          count = 0;
	const char  *argv[MAXARGUMENTS];
	char       **argv_p;

	if (ctx_p->dir_from == NULL)
		errno=EINVAL, critical("Source is not entered");
	if (ctx_p->dir_to   == NULL)
		errno=EINVAL, critical("Destination is not entered");

	push_arg(argv, &count, path_clsync());
	push_arg(argv, &count, "-Klrsync");
	push_arg(argv, &count, "-Mrsyncdirect");
	push_arg(argv, &count, "-S");
	push_arg(argv, &count, path_rsync());
	push_arg(argv, &count, "-W");
	push_arg(argv, &count, ctx_p->dir_from);
	push_arg(argv, &count, "-D");
	push_arg(argv, &count, ctx_p->dir_to);
	push_arg(argv, &count, "-L");
	push_arg(argv, &count, path_listsdir());

	argv_p = ctx_p->clsync_argv;
	while (*argv_p != NULL)
		push_arg(argv, &count, *(argv_p++));

	push_arg(argv, &count, "--");
	push_arg(argv, &count, "%RSYNC-ARGS%");

	argv_p = ctx_p->rsync_argv;
	while (*argv_p != NULL)
		push_arg(argv, &count, *(argv_p++));

	if (ctx_p->flags[FL_COMMANDONLY])
		return print_cmd(argv);

	return exec_clsync_argv(argv);
}
