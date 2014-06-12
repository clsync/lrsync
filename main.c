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

static const struct option long_options[] =
{
	{"help",		optional_argument,	NULL,	FL_HELP},
	{"version",		optional_argument,	NULL,	FL_SHOW_VERSION},

	{NULL,			0,			NULL,	0}
};

#define syntax() _syntax(ctx_p)
void _syntax(ctx_t *ctx_p)
{
	info("lrsync possible options:");

	int i=0;
	while(long_options[i].name != NULL) {
		info("\t--%-24s%c%c%s", long_options[i].name, 
			long_options[i].val & FLM_LONGOPTONLY ? ' ' : '-', 
			long_options[i].val & FLM_LONGOPTONLY ? ' ' : long_options[i].val, 
			(long_options[i].has_arg == required_argument ? " argument" : ""));
		i++;
	}
	info_short("\n\n\"rsync  --help\":\n");
	exec_rsync (ctx_p, "--help");
	info_short("\n\n\"clsync  --help\":\n");
	exec_clsync(ctx_p, "--help");

	exit(0);
}

#define version() _version(ctx_p)
void _version(ctx_t *ctx_p)
{
	info_short(PROGRAM" v%i.%i\n\t"AUTHOR"", VERSION_MAJ, VERSION_MIN);
	info_short("\n\n\"clsync --version\":\n");
	exec_clsync(ctx_p, "--version");
	info_short("\n\n\"rsync  --version\":\n");
	exec_rsync (ctx_p, "--version");
	info_short("\n");

	exit(0);
}

int parse_parameter(ctx_t *ctx_p, uint16_t param_id, char *arg)
{
	switch (param_id) {
		case FL_HELP:
			syntax();
			break;
		case FL_SHOW_VERSION:
			version();
			break;
	}
	return 0;
}

int parse_arguments(int argc, char *argv[], ctx_t *ctx_p)
{
	int c;
	int option_index = 0;

	// Generating "optstring" (man 3 getopt_long) with using information from struct array "long_options"
	char *optstring     = alloca((('z'-'a'+1)*3 + '9'-'0'+1)*3 + 1);
	char *optstring_ptr = optstring;

	const struct option *lo_ptr = long_options;
	while(lo_ptr->name != NULL) {
		if(!(lo_ptr->val & (FLM_LONGOPTONLY))) {
			*(optstring_ptr++) = lo_ptr->val & 0xff;

			if(lo_ptr->has_arg == required_argument)
				*(optstring_ptr++) = ':';

			if(lo_ptr->has_arg == optional_argument) {
				*(optstring_ptr++) = ':';
				*(optstring_ptr++) = ':';
			}
		}
		lo_ptr++;
	}
	*optstring_ptr = 0;

	// Parsing arguments
	while(1) {
		c = getopt_long(argc, argv, optstring, long_options, &option_index);
	
		if (c == -1) break;
		int ret = parse_parameter(ctx_p, c, optarg);
		if (ret) return ret;
	}

	printf("%i\n", option_index);

	return 0;
}

int parse_clsyncarguments(ctx_t *ctx_p)
{
	FILE       *clsync_stderr;
	char       *line = NULL;
	size_t      len = 0;
	ssize_t     read;
	const char *clsync_path = path_clsync();
	char       *argv[] = { "clsync", "--help", NULL };

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

			printf(">>> \"%s\" \"%c\" %i\n", longopt, shorthand, reqarg);
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

	return status;
}

int main(int argc, char *argv[])
{
	ctx_t ctx;
	int ret;

	ret = parse_clsyncarguments(&ctx);
	if (!ret)
		error("Cannot parse supported clsync arguments");

	if (!ret) {
		ret = parse_arguments(argc, argv, &ctx);
		if (ret)
			error("Cannot parse arguments");
	}

	if (!ret)
		ret = lrsync(&ctx, argc, argv);

	return ret;
}

