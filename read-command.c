// UCLA CS 111 Lab 1 command reading

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

//#include <error.h>
/* FIXME: You may need to add #include directives, macro definitions,
static function definitions, etc.  */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

typedef struct command_node* command_node_t;
/* FIXME: Define the type 'struct command_stream' here.  This should
complete the incomplete type declaration in command.h.  */
struct command_node
{
	command_t node_command;
	command_node_t node_next;
	command_node_t node_previous;
};
struct command_stream
{
	command_node_t head;
	command_node_t tail;
};
int(*get_next_byte_global) (void *);
void *get_next_byte_argument_global;

int linecount;
char whitespace[] = " \t\n";

const int BUFFBLOCK = 512;
int BUFFMULTIPLIER = 1;
int BOOKMULTIPLIER = 1;

char* buffer;
int buffer_count = 0;

void** book_keeper = NULL;
int book_count = 0;

char* if_word = "if";
char* then_word = "then";
char* else_word = "else";
char* fi_word = "fi";
char* while_word = "while";
char* do_word = "do";
char* done_word = "done";
char* until_word = "until";

void append_command_node(command_stream_t stream, command_node_t node)
{
	node->node_next = NULL;
	node->node_previous = NULL;
	if (stream->head==NULL)
	{
		stream->head = node;
		stream->tail = node;
	}
	else if (stream->head == stream->tail)
	{
		node->node_previous = stream->head;
		stream->tail = node;
		stream->head->node_next = node;
	}
	else
	{
		command_node_t secondLast = stream->tail->node_previous;
		node->node_previous = secondLast;
		secondLast->node_next = node;
		node->node_next = stream->tail;
		stream->tail->node_previous = node;
	}
}
command_stream_t make_command_stream(int(*get_next_byte) (void *),void *get_next_byte_argument)
{
	get_next_byte_global = get_next_byte;
	get_next_byte_argument_global = get_next_byte_argument;
	
	linecount = 1;
	buffer = try_malloc(BUFFBLOCK*BUFFMULTIPLIER); 
	command_stream_t ret = try_malloc(sizeof(struct command_stream));
	ret->head = NULL;
	ret->tail = NULL;
	command_t com = get_next_command();
	command_node_t node = try_malloc(sizeof(struct command_node));
	while (com!=NULL)
	{
		com = get_next_command();
		node->node_command = com;
		append_command_node(ret, node);
	}
	/* FIXME: Replace this with your implementation.  You may need to
	add auxiliary functions and otherwise modify the source code.
	You can also use external functions defined in the GNU C Library.  */
	return ret;
}

command_t parse_command(char* beginning,size_t bytes)
{
	if (bytes <= 0||beginning==NULL) return NULL;

	command_t ret = try_malloc(sizeof(struct command));
	command_t tmp1;
	command_t tmp2;
	command_t tmp3;

	int first_keyword;
	int second_keyword;
	int third_keyword;
	int fourth_keyword;
    
	if (bytes >= 2 && *(beginning) == '('&& *(beginning + bytes - 1) == ')')
	{
		tmp1 = parse_command(beginning + 1, bytes - 2);
		if (tmp1 != NULL)
		{
			ret = make_command(SUBSHELL_COMMAND);
			ret->u.command[0] = tmp1;
			return ret;
		}
		else raise_syntax_error();
	}
	
		first_keyword = index_of(beginning, bytes, if_word, strlen(if_word));
		second_keyword = index_of(beginning, bytes, then_word, strlen(then_word));
		third_keyword = index_of(beginning, bytes, else_word, strlen(else_word));
		fourth_keyword =  index_of(beginning, bytes, fi_word, strlen(fi_word));
	
	if (!(first_keyword == 0 && fourth_keyword == bytes - 2 ));
	else if (!(is_whitespace(*(beginning + 2) && is_whitespace(*(beginning + fourth_keyword - 1)))));
	else if (second_keyword < 0 || bytes < 10) raise_syntax_error();
	else if (!(is_whitespace(*(beginning + second_keyword - 1)) && is_whitespace(*(beginning + second_keyword + 4 + 1))))raise_syntax_error();
	else
	{
		ret = make_command(IF_COMMAND);
		char* a = get_first_non_whitespace(beginning + 2, second_keyword-2);
		char* b = get_last_non_whitespace(beginning + 2, second_keyword - 2);
		if (a == NULL)
			raise_syntax_error();
		tmp1 = parse_command(a,b-a);
		if (tmp1==NULL)
			raise_syntax_error();
		ret->u.command[0] = tmp1;

		int tag;
		if (third_keyword > 0)
		{
			tag = third_keyword;
			a = get_first_non_whitespace(third_keyword + 4, fourth_keyword - third_keyword - 4);
			b = get_last_non_whitespace(third_keyword + 4, fourth_keyword - third_keyword - 4);
			if (a == NULL)
				raise_syntax_error();
			tmp3 = parse_command(a, b - a);
			if (tmp3 == NULL)
				raise_syntax_error();
			ret->u.command[2] = tmp3;
		}
		else
		{
			tag = fourth_keyword;
		}
		a = get_first_non_whitespace(second_keyword + 4, tag - second_keyword - 4);
		b = get_last_non_whitespace(second_keyword + 4, tag - second_keyword - 4);
		if (a == NULL)
			raise_syntax_error();
		tmp2 = parse_command(a, b - a);
		if (tmp2 == NULL)
			raise_syntax_error();
		ret->u.command[1] = tmp2;
		return ret;
	}
	
	first_keyword = index_of(beginning, bytes, while_word, strlen(while_word));
	second_keyword = index_of(beginning, bytes, until_word, strlen(until_word));
	third_keyword = index_of(beginning, bytes, do_word, strlen(do_word));
	fourth_keyword = index_of(beginning, bytes, done_word, strlen(done_word));
	
	if (!((first_keyword == 0 || second_keyword == 0)&& fourth_keyword == bytes - 4));
	else if (!(is_whitespace(*(beginning + 5) && is_whitespace(*(beginning + fourth_keyword - 1)) )));
	else if (third_keyword < 0 || bytes < 13) raise_syntax_error();
	else if (!(is_whitespace(*(beginning + third_keyword - 1)) && is_whitespace(*(beginning + third_keyword + 2 + 1))))raise_syntax_error();
	else
	{
		if (first_keyword == 0) ret = make_command(WHILE_COMMAND);
		else ret = make_command(UNTIL_COMMAND);
		char* a = get_first_non_whitespace(beginning + 5, third_keyword - 5);
		char* b = get_first_non_whitespace(beginning + 5, third_keyword - 5);
		if (a == NULL)
			raise_syntax_error();
		tmp1 = parse_command(a, b - a);
		if (tmp1 == NULL)
			raise_syntax_error();
		ret->u.command[0] = tmp1;

		a = get_first_non_whitespace(third_keyword + 2, fourth_keyword - third_keyword - 2);
		b = get_last_non_whitespace(third_keyword + 2, fourth_keyword - third_keyword - 2);
		if (a == NULL)
			raise_syntax_error();
		tmp2 = parse_command(a, b - a);
		if (tmp2 == NULL)
			raise_syntax_error();
		ret->u.command[1] = tmp2;
		return ret;
	}

	if (is_word(beginning, bytes))
	{
		char* tmp = try_malloc(bytes);
		memcpy(beginning, tmp, buffer_count);
		char** word = tmp;
		ret = make_command(SIMPLE_COMMAND);
		ret->u.word = word;
		return ret;
	}
	return NULL;
}
command_t get_next_command()
{
	bool left_bracket = false;

	bool right_bracket = false;
	bool if_flag = false;
	bool fi_flag = false;
	bool while_flag = false;
	bool until_flag = false;
	bool done_flag = false;
	
	strip_front();
	while (!feof(get_next_byte_argument_global))
	{
		int c = getc(get_next_byte_argument_global);
		switch (c)
		{
		case '#':
			if (buffer_count != 0 && is_word_token(buffer[buffer_count - 1])) raise_syntax_error();
			remove_line();
			return get_next_command();
		case '(':
			if (buffer_count != 0) raise_syntax_error();
			left_bracket = true;
			break;
		case ')':
			if (buffer_count == 0 || buffer[0] != '(') raise_syntax_error();
			right_bracket = true;
			break;
		case '\n':
			linecount++;
			if (!(if_flag || while_flag || until_flag)) return parse_command(buffer, buffer_count);
			break;
		case '|':
		case ';':
			command_t ret;
			if (c == '|') ret = make_command(PIPE_COMMAND);
			else ret = make_command(SEQUENCE_COMMAND);
			if (buffer_count == 0)raise_syntax_error();
			command_t tmp1 = parse_command(buffer, buffer_count);
			clean_buffer();
			command_t tmp2 = get_next_command();
			ret->u.command[0] = tmp1;
			ret->u.command[1] = tmp2;
			return ret;
			break;
		}

		if (buffer_count >= BUFFBLOCK*BUFFMULTIPLIER)
		{
			BUFFMULTIPLIER++;
			buffer = try_realloc(buffer, BUFFBLOCK*BUFFMULTIPLIER);
		}
		buffer[buffer_count] = c;
		buffer_count++;
		if (left_bracket&&right_bracket)
		{
			command_t tmp = parse_command(buffer, buffer_count);
			if(tmp==NULL) raise_syntax_error();
			return tmp;
		}
		if (!if_flag && buffer_count >= 3 && memcmp(buffer, "if", 2 == 0) && is_whitespace(buffer[2])) if_flag = true;
		else if (!fi_flag && buffer_count >= 3 && memcmp(buffer + buffer_count - 2, "fi", 2 == 0) && is_whitespace(buffer[buffer_count-3])) fi_flag = true;
		else if (!until_flag && buffer_count >= 6 && memcmp(buffer, "until", 5 == 0) && is_whitespace(buffer[5])) until_flag = true;
		else if (!while_flag && buffer_count >= 6 && memcmp(buffer, "while", 5 == 0) && is_whitespace(buffer[5])) while_flag = true;
		else if (!done_flag && buffer_count >= 5 && memcmp(buffer + buffer_count - 4, "done", 4 == 0) && is_whitespace(buffer[buffer_count - 5])) done_flag = true;

		if ((if_flag&& fi_flag) || ((while_flag || until_flag) && done_flag))	return parse_command(buffer, buffer_count);
	}
	if (buffer_count != 0)return parse_command(buffer, buffer_count);
}
command_t make_command(enum command_type ctype)
{
	command_t ret = try_malloc(sizeof(struct command));
	ret->input = NULL;
	ret->output = NULL;
	ret->status = -1;
	ret->type = ctype;
}
command_t read_command_stream(command_stream_t s)
{
	command_node_t iter = s->head;
	if(iter != NULL)
	{
	s->head = iter->node_next;
	return iter->node_command;
	}
	return NULL;
}
void* try_malloc(size_t size)
{
	if (book_keeper == NULL)
	{
		book_keeper = malloc(BOOKMULTIPLIER*BUFFBLOCK);
		if (!book_keeper)
			error(1, 0, "malloc failed");
	}
	void * ret;
	ret = malloc(sizeof(size));
	if (!ret)
		error(1, 0, "malloc failed");
	
	if (book_count >= BOOKMULTIPLIER*BUFFBLOCK)
	{
		BOOKMULTIPLIER++;
		book_keeper = realloc(book_keeper, BOOKMULTIPLIER*BUFFBLOCK);
		if (!book_keeper)
			error(1, 0, "realloc failed");
	}
	else
	{
		book_keeper[book_count] = ret;
		book_count++;
	}
	return ret;
}
void clean_up()
{
	int i;
	for (i = 0; i < book_count; i++)
	{
		free(book_keeper[i]);
	}
	free(book_keeper);
}
void* try_realloc(void* ptr,size_t size)
{
	void* ret;
	void* old = ptr;
	ret = realloc(ptr,sizeof(size));
	if (!ret)
		error(1, 0, "realloc failed");
	int i;
	for (i = 0; i < book_count; i++)
	{
		if (book_keeper[i] == ptr)
		{
			book_keeper[i] = ret;
			break;
		}
	}
	return ret;
}
bool is_word(char* beginning, size_t bytes)
{
	int i;
	for (i = 0; i < bytes; i++)
	{
		if (!is_word_token(*(beginning + i)))return false;
	}
	return true;
}
bool is_word_token(char c)
{
	return isalnum(c) || (strchr("!%+,-./:@^_\t ", c) != NULL);
}
bool is_whitespace(char c)
{
	if ((strchr(whitespace, c) == NULL))return false;
	return true;
}
void strip_front()
{
	int c = get_next_byte_global(get_next_byte_argument_global);
	while (c != EOF)
	{
		if (is_whitespace(c))
		{
			ungetc(c, get_next_byte_argument_global);
			return;
		}
		if (c == '\n')
			linecount++;
		c = get_next_byte_global(get_next_byte_argument_global);
	}
}
void remove_line()
{
	int c = get_next_byte_global(get_next_byte_argument_global);
	while (c != EOF && c != '\n')
		c = get_next_byte_global(get_next_byte_argument_global);
	if (c == '\n')
		linecount++;
}
void raise_syntax_error()
{
	error(1, 0, "syntax error; line %d", linecount);
}
void clean_buffer()
{
	buffer_count = 0;
}
int index_of(char* c, size_t c_size, char* pattern, size_t pattern_size)
{
	int i;
	for (i = 0; i<c_size; i++)
	{
		if (i + pattern_size <= c_size)
		{
			if (!memcmp((c + i), pattern, pattern_size)) return i;
		}
		else return -1;
	}
	return -1;
}
char* get_first_non_whitespace(char* c, size_t c_size)
{
	int i;
	for (i = 0; i < c_size; i++)
	{
		if (!is_whitespace(*(c + i)))
			return (c + i);
	}
	return NULL;
}
char* get_last_non_whitespace(char* c, size_t c_size)
{
	int i;
	for (i = c_size-1; i>=0; i--)
	{
		if (!is_whitespace(*(c + i)))
			return (c + i);
	}
	return NULL;
}