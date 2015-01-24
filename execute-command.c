// UCLA CS 111 Lab 1 command execution

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"

#include <fcntl.h>
#include <error.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <unistd.h>  
#include <stdlib.h>  
#include <stdio.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (0, 0, "warning: profiling not yet implemented");
  return -1;
}
void prep_io(command_t c);
void execute_simple_cmd(command_t c);
void execute_if_cmd(command_t c);
void execute_sequence_cmd(command_t c);
void execute_command_only(command_t c);
void execute_pipe_cmd(command_t c);

void prep_io(command_t c)
{
	if (c->input != NULL)
	{
		int in = open(c->input, O_RDONLY);
		if (in < 0)
			error(1, 0, "input: cannot read %s", c->input);
		//move to 0th (stdin) file descriptor
		if (dup2(in, 0)<0)
			error(1, 0, "input: dup2 error for %s", c->input);
		close(in);
	}
	if (c->output != NULL)
	{
		int out = open(c->output, O_WRONLY | O_CREAT | O_TRUNC,0664);
		if (out < 0)
			error(1, 0, "output: cannot read %s", c->output);
		//move to 1st (stdout) file descriptor
		if (dup2(out, 1)<0)
			error(1, 0, "output: dup2 error for %s", c->output);
		close(out);
	}
}
void execute_simple_cmd(command_t c)
{
	int exit_code;
	pid_t pid = fork();

	//parent
	if (pid > 0)
	{
		waitpid(pid, &exit_code, 0);
		c->status = WEXITSTATUS(exit_code);
	}
	//child
	else
	{	prep_io(c);
		execvp(c->u.word[0], c->u.word);
		//fail
		error(1, 0, "execution failed for %s",c->u.word[0]);
	}
}
void execute_if_cmd(command_t c)
{
	execute_command_only(c->u.command[0]);
	int exit_code;
	//if true then execute 2nd cmd immediately
	if (!(c->u.command[0]->status))
	{
		execute_command_only(c->u.command[1]);
		exit_code = c->u.command[1]->status;
	}
	else
	{
		//else exists
		if (c->u.command[2] != NULL)
		{
			execute_command_only(c->u.command[2]);
			exit_code = c->u.command[2]->status;
		}
		else
		{
			exit_code = 0;
		}
	}
	c->status = exit_code;
}
void execute_sequence_cmd(command_t c)
{
	int exit_code;
	pid_t pid = fork();
	if (pid > 0)
	{
		waitpid(pid, &exit_code, 0);
		c->status = WEXITSTATUS(exit_code);
	}
	else
	{
		pid = fork();
		if (pid == 0)
		{
			execute_command_only(c->u.command[0]);
			_exit(c->u.command[0]->status);
		}
		else
		{
			waitpid(pid, &exit_code, 0);
			execute_command_only(c->u.command[1]);
			_exit(c->u.command[1]->status);
		}
	}
}
void execute_command_only(command_t c)
{
	switch (c->type)
	{
	case SIMPLE_COMMAND:
		execute_simple_cmd(c);
		break;
	case IF_COMMAND:
		execute_if_cmd(c);
		break;
	case PIPE_COMMAND:
		execute_pipe_cmd(c);
		break;
	case SEQUENCE_COMMAND:
		execute_sequence_cmd(c);
		break;
	}
}
void execute_pipe_cmd(command_t c)
{
	int exit_code;
	int file_des_buf[2];
	pid_t pid1;
	pid_t pid2;
	pid_t pid3;

	if (pipe(file_des_buf) == -1)
		error(1, 0, "pipe failed");

	pid1 = fork();
	if (pid1 > 0)
	{
		pid2 = fork();
		if (pid2 > 0)
		{
			close(file_des_buf[0]);
			close(file_des_buf[1]);
			pid3 = waitpid(-1, &exit_code, 0);
			if (pid3 == pid1)
			{
				c->status = WEXITSTATUS(exit_code);
				waitpid(pid2, &exit_code, 0);
			}
			else
			{
				waitpid(pid1, &exit_code, 0);
				c->status = WEXITSTATUS(exit_code);
			}
			return;
		}
		else
		{
			close(file_des_buf[0]);
			if (dup2(file_des_buf[1], 1)<0)
				error(1, 0, "dup2 failed");
			execute_command_only(c->u.command[0]);
			_exit(c->u.command[0]->status);
		}
	}
	else
	{
		close(file_des_buf[1]);
		if (dup2(file_des_buf[0], 0)<0)
			error(1, 0, "dup2 failed");
		execute_command_only(c->u.command[1]);
		_exit(c->u.command[1]->status);
	}
}

int
command_status (command_t c)
{
  return c->status;
}
void
execute_command (command_t c, int profiling)
{
execute_command_only(c);
}