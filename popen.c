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
#include <stdlib.h>	// exit()
#include <stdio.h>	// fdopen()
#include <errno.h>	// EINVAL

#include "popen.h"

pid_t my_popen(const char *bin_path, char *const *argv, FILE **stdinf, FILE **stdoutf, FILE **stderrf)
{
	int	i_pfd[2];
	int	o_pfd[2];
	int	e_pfd[2];
	pid_t	pid;

	if (stdinf == NULL && stdoutf == NULL && stderrf == NULL)
		return EINVAL;

	if (stdinf  != NULL)
		if (pipe(i_pfd) < 0)
			return -1;
	if (stdoutf != NULL)
		if (pipe(o_pfd) < 0)
			return -1;
	if (stderrf != NULL)
		if (pipe(e_pfd) < 0)
			return -1;

	pid = fork();

	if (pid == -1)
		return -1;

	if (pid == 0) {
		// Child:

		if (stdinf  != NULL) {
			close(i_pfd[1]);
			dup2 (i_pfd[0], STDIN_FILENO );
			close(i_pfd[0]);
		}

		if (stdoutf != NULL) {
			close(o_pfd[0]);
			dup2 (o_pfd[1], STDOUT_FILENO);
			close(o_pfd[1]);
		}

		if (stderrf != NULL) {
			close(e_pfd[0]);
			dup2 (e_pfd[1], STDERR_FILENO);
			close(e_pfd[1]);
		}

		exit(execvp(bin_path, argv));
	}

	// Parent:

	if (stdinf  != NULL) {
		close(i_pfd[0]);
		*stdinf  = fdopen(i_pfd[1], "w");
	}

	if (stdoutf != NULL) {
		close(o_pfd[1]);
		*stdoutf = fdopen(o_pfd[0], "r");
	}

	if (stderrf != NULL) {
		close(e_pfd[1]);
		*stderrf = fdopen(e_pfd[0], "r");
	}

	return pid;
}

