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


#include <unistd.h>	// getopt_long()
#include <stdlib.h>	// exit()
#include <getopt.h>	// getopt_long()
#include <errno.h>	// errno
#include <stdint.h>	// uint16_t
#include <stdio.h>	// snprintf()
#include <sys/types.h>	// waitpid()
#include <sys/wait.h>	// waitpid()
#include <string.h>	// strncmp()

#include "common.h"

#include "malloc.h"
#include "error.h"
#include "popen.h"
#include "lrsync.h"

#define syntax() _syntax(ctx_p)
void _syntax(ctx_t *const ctx_p)
{
	info_short("lrsync possible options:");
	info_short("	--help");
	info_short("	--version");
	info_short("	--clsync-command-only");
	info_short("");
	info_short("");
	info_short("\"rsync  --help\":");
	info_short("");
	exec_rsync (ctx_p, "--help");
	info_short("");
	info_short("");
	info_short("\"clsync  --help\":");
	info_short("");
	exec_clsync(ctx_p, "--help");

	exit(0);
}

#define version() _version(ctx_p)
void _version(ctx_t *const ctx_p)
{
	info_short(PROGRAM" v%i.%i\n\t"AUTHOR"", VERSION_MAJ, VERSION_MIN);
	info_short("\n\n\"clsync --version\":\n");
	exec_clsync(ctx_p, "--version");
	info_short("\n\n\"rsync  --version\":\n");
	exec_rsync (ctx_p, "--version");
	info_short("\n");

	exit(0);
}


void push_rsyncarg(ctx_t *const ctx_p, char *const arg, int isoption) {
	static int nonoption_count = 0;

	if (ctx_p->rsync_argv_count >= MAXARGUMENTS) {
		errno = E2BIG;
		critical("Too many arguments");
	}

	if (!isoption) {
		switch (nonoption_count) {
			case 0:
				ctx_p->dir_from = arg;
				break;
			case 1:
				ctx_p->dir_to   = arg;
				break;
			default:
				errno = EINVAL;
				critical("You cannot use multiple source via lrsync");
		}
		nonoption_count++;
	}

	ctx_p->rsync_argv[ ctx_p->rsync_argv_count++ ] = arg;

	return;
}

void push_clsyncarg(ctx_t *const ctx_p, char *const arg) {
	if (ctx_p->clsync_argv_count >= MAXARGUMENTS) {
		errno = E2BIG;
		critical("Too many arguments");
	}

	ctx_p->clsync_argv[ ctx_p->clsync_argv_count++ ] = arg;

	switch (arg[1]) {
		case '-':
			if (!strcmp(arg, "--verbose")) {
				push_rsyncarg(ctx_p, arg, 1);
				return;
			}
			return;
		case 'v':
			push_rsyncarg(ctx_p, arg, 1);
			return;
	}

	return;
}

int parse_arguments(const int argc, char *const argv[], ctx_t *const ctx_p)
{
	char **argv_p;
	int    reqarg     = 0;
	int    nonoptions = 0;

	argv_p = (char **)&argv[1];

	while (*argv_p != NULL) {
		char   *arg     = *(argv_p++);
		size_t  arg_len = strlen(arg);

		// If "--" was been already met
		if (nonoptions) {
			push_rsyncarg(ctx_p, arg, 0);
			continue;
		}

		// This argument is just an argument to previously pushed clsync option
		if (reqarg) {
			push_clsyncarg(ctx_p, arg);
			continue;
		}

		// Empty argument. It's not for clsync :)
		if (!arg_len) {
			push_rsyncarg(ctx_p, arg, 0);
			continue;
		}

		// Not an option. Not for clsync.
		if (arg[0] != '-') {
			push_rsyncarg(ctx_p, arg, 0);
			continue;
		}

		// Short option
		if (arg[1] != '-') {
#ifdef CLSYNC_SHORTOPTS
			unsigned char shortopt = (unsigned char)arg[1];

			// Not a clsync option.
			if (!ctx_p->clsync_shortopt[shortopt]) {
				push_rsyncarg(ctx_p, arg, 1);
				continue;
			}

			// clsync option
			push_clsyncarg(ctx_p, arg);

			// Have we already passed the argument if it's required? (like "-d99")
			if ((ctx_p->clsync_shortopt[shortopt] == 2) && (arg_len == 2))
				reqarg = 1;
#else
			push_rsyncarg(ctx_p, arg, 1);
#endif
			continue;
		}

		// Is an "--"
		if (arg_len == 2) {
			nonoptions = 1;
			continue;
		}

		// Long option
		{
			char *longopt = strdup(&arg[2]), *ptr, **lo_ptr, found;
			int lo_i;

			// If option with an argument already then temporary removing the argument
			ptr = strchr(longopt, '=');
			if (ptr != NULL)
				*ptr = 0;

			if (!strcmp(longopt, "help"))
				syntax();
			if (!strcmp(longopt, "version"))
				version();
			if (!strcmp(longopt, "clsync-command-only")) {
				ctx_p->flags[FL_CLSYNCCOMMANDONLY]=1;
				continue;
			}

			// Is it clsync option?
			found = 0;
			lo_i  = 0;
			lo_ptr = ctx_p->clsync_longopt;
			while (lo_ptr[lo_i] != NULL) {
				char *lo = lo_ptr[lo_i];
				if (!strcmp(lo, longopt)) {
					found = 1;
					break;
				}
				lo_i++;
			}
			free(longopt);

			// If not => rsync
			if (!found) {
				push_rsyncarg(ctx_p, arg, 1);
				continue;
			}

			// It's a clsync option. Pushing.
			push_clsyncarg(ctx_p, arg);

			// Have we already passed the argument if it's required? (like "--debug=99")
			if (ctx_p->clsync_longopt_reqarg[lo_i] && (ptr == NULL))
				reqarg = 1;
			continue;
		}
	}

	return 0;
}

int parse_clsyncarguments(ctx_t *const ctx_p)
{
	FILE       *clsync_stderr;
	char       *line = NULL;
	size_t      len = 0;
	ssize_t     read;
	const char *clsync_path = path_clsync();
	char       *argv[] = { "clsync", "--help", NULL };

	int lcount = 0;

	pid_t clsync_pid = my_popen(clsync_path, argv, NULL, NULL, &clsync_stderr);

	if (clsync_stderr == NULL) {
		error("Cannot my_popen(\"%s\", { \"--help\", NULL }, NULL, NULL, &clsynchelp)", clsync_path);
		return -1;
	}

	// From clsync source: "info("\t--%-24s%c%c%s", ...)"
	while ((read = getline(&line, &len, clsync_stderr)) != -1) {
		static char *magic = "Info: \t--";

		if (!strncmp(line, magic, sizeof(magic)-1)) {
			char *ptr, *longopt, shorthand, reqarg;
			// Parsing an option

			// longopt

			longopt = &line[sizeof(magic)+1];
			ptr = longopt;
			while (*(++ptr) != ' ');
			*ptr = 0;

			// shorthand

			while (*(++ptr) == ' ');
			shorthand = 0;
			if (*ptr == '-') {
				shorthand = *(++ptr);
				while (*(++ptr) == ' ');
			}

			// reqarg

			reqarg = (*ptr != '\n');

			if (lcount > CLSYNC_MAXLONGOPTS)
				critical("Too many clsync long options");

			ctx_p->clsync_longopt[lcount]        = strdup(longopt);
			ctx_p->clsync_longopt_reqarg[lcount] = reqarg;
			lcount++;

			if (shorthand)
				ctx_p->clsync_shortopt[(unsigned char)shorthand] = reqarg+1;

//			printf(">>> \"%s\" \"%c\" %i (%u)\n", longopt, shorthand, reqarg, lcount);
		}
	}

	free(line);

	int status;
	while (waitpid(clsync_pid, &status, 0) < 0)
		if (errno != EINTR) {
			error("Got error while waitpid(%u, &status, 0)", clsync_pid);
			return -1;
		}
	fclose(clsync_stderr);

	if (status == -1)
		error("Got error from pclose()");

	return 0;
}

int main(int argc, char *argv[])
{
	ctx_t ctx = {{0}};
	int ret;

	ret = parse_clsyncarguments(&ctx);
	if (ret)
		error("Cannot parse supported clsync arguments");

	if (!ret) {
		ret = parse_arguments(argc, argv, &ctx);
		if (ret)
			error("Cannot parse arguments");
	}

	if (!ret)
		ret = lrsync(&ctx);

	return ret;
}

