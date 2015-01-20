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

#include <error.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

enum token_type
{
	WORD,

	LEFTBRACKET,
	RIGHTBRACKET,

	LEFTREDIRECTION,
	RIGHTREDIRECTION,

	PIPE,
	SEMICONLON,

	UNTIL_KEYWORD,
	WHILE_KEYWORD,
	DO_KEYWORD,
	DONE_KEYWORD,

	IF_KEYWORD,
	THEN_KEYWORD,
	ELSE_KEYWORD,
	FI_KEYWORD,

	NEWLINE,
	UNDETERMINED
};
typedef struct token* token_t;
struct token
{
	enum token_type type;
	char* text;
	size_t line;

	token_t next;
	token_t previous;
};
struct token_chain
{
	token_t head;
	token_t tail;
};
typedef struct token_chain* token_chain_t;

void** book_keeper = NULL;
size_t book_count = 0;
const size_t BLOCK = 1024;
size_t BOOKMULTIPLIER = 1;

void clean_up()
{
	size_t i;
	for (i = 0; i < book_count; i++)
	{
		free(book_keeper[i]);
	}
	free(book_keeper);
}
void* try_malloc(size_t size)
{
	if (book_keeper == NULL)
	{
		book_keeper = malloc(BOOKMULTIPLIER*BLOCK*sizeof(void**));
		if (!book_keeper)
		{
			fprintf(stderr, "malloc failed");
			clean_up();
			exit(1);
		}
	}
	void * ret = malloc(size);
	if (!ret)
	{
		fprintf(stderr, "malloc failed");
		clean_up();
		exit(1);
	}
	if (book_count >= BOOKMULTIPLIER*BLOCK)
	{
		BOOKMULTIPLIER++;
		book_keeper = realloc(book_keeper, BOOKMULTIPLIER*BLOCK);
		if (!book_keeper)
		{
			fprintf(stderr, "malloc failed");
			clean_up();
			exit(1);
		}
	}
	book_keeper[book_count] = ret;
	book_count++;
	return ret;
}
void* try_realloc(void* ptr, size_t size)
{
	void* ret;
	void* old = ptr;
	ret = realloc(ptr, size);
	if (!ret)
	{
		fprintf(stderr, "malloc failed");
		clean_up();
		exit(1);
	}
	size_t i;
	for (i = 0; i < book_count; i++)
	{
		if (book_keeper[i] == old)
		{
			book_keeper[i] = ret;
			break;
		}
	}
	return ret;
}
char* buffer_stream(int(*get_next_byte) (void *), void *get_next_byte_argument)
{
	size_t BLOCKMULTIPLIER = 1;
	char* buffer = try_malloc(BLOCK*BLOCKMULTIPLIER);
	size_t iter = 0;
	char c = get_next_byte(get_next_byte_argument);
	size_t len = BLOCK*BLOCKMULTIPLIER;
	while (c != EOF)
	{
		if (iter >= len)
		{
			BLOCKMULTIPLIER++;
			len = BLOCK*BLOCKMULTIPLIER;
			buffer = try_realloc(buffer, len);
		}
		buffer[iter] = c;
		iter++;
		c = get_next_byte(get_next_byte_argument);
	}
	if (iter >= len)
	{
		BLOCKMULTIPLIER++;
		len = BLOCK*BLOCKMULTIPLIER;
		buffer = try_realloc(buffer, len);
	}
	buffer[iter] = '\0';
	return buffer;
}

bool is_word_token(char c)
{
	return isalnum(c) || (strchr("!%+,-./:@^_", c) != NULL);
}

void append_token(token_chain_t chain, token_t ctoken)
{
	ctoken->next = NULL;
	ctoken->previous = NULL;
	if (chain->head == NULL)
	{
		chain->head = ctoken;
		chain->tail = ctoken;
	}
	else if (chain->head == chain->tail)
	{
		chain->tail = ctoken;
		ctoken->previous = chain->head;
		chain->head->next = ctoken;
	}
	else
	{
		ctoken->previous = chain->tail;
		chain->tail->next = ctoken;
		chain->tail = ctoken;
	}
}
token_t make_token(enum token_type type, char* text, size_t line)
{
	token_t ret = try_malloc(sizeof(struct token));
	ret->next = NULL;
	ret->previous = NULL;
	ret->type = type;
	ret->text = text;
	ret->line = line;
	return ret;
}
token_chain_t make_token_chain()
{
	token_chain_t chain = try_malloc(sizeof(struct token_chain));
	chain->head = NULL;
	chain->tail = NULL;
	return chain;
}

void syntax_error(size_t line)
{
	clean_up();
	fprintf(stderr, "syntax error at: %u", line);
	exit(1);
}
///
token_t evaluate_text(char* input, size_t size, size_t line)
{
	enum token_type tokentype;
	char* con = NULL;
	if (size >= 2 && !memcmp(input, "if", 2)) { tokentype = IF_KEYWORD; con = "if"; }
	else if (size >= 4 && !memcmp(input, "then", 4)){ tokentype = THEN_KEYWORD; con = "then"; }
	else if (size >= 4 && !memcmp(input, "else", 4)){ tokentype = ELSE_KEYWORD; con = "else"; }
	else if (size >= 2 && !memcmp(input, "fi", 2)){ tokentype = FI_KEYWORD; con = "fi"; }

	else if (size >= 5 && !memcmp(input, "while", 5)){ tokentype = WHILE_KEYWORD; con = "while"; }
	else if (size >= 5 && !memcmp(input, "until", 5)){ tokentype = UNTIL_KEYWORD; con = "until"; }
	else if (size >= 4 && !memcmp(input, "done", 4)){ tokentype = DONE_KEYWORD; con = "done"; }
	else if (size >= 2 && !memcmp(input, "do", 2)) { tokentype = DO_KEYWORD; con = "do"; }
	else
	{
		tokentype = WORD;
		con = try_malloc(strlen(input) + 1);
		memcpy(con, input, strlen(input) + 1);
	}
	return make_token(tokentype, con, line);
}
token_chain_t tokenize(char* input)
{
	token_chain_t ret = make_token_chain();
	size_t iter = 0;
	size_t tmp_iter = 0;
	size_t line = 1;
	size_t MULTIPLIER = 1;
	char c = input[iter];
	enum token_type tokentype;
	char* tmp_buffer = try_malloc(BLOCK*MULTIPLIER);
	while (c != '\0')
	{
		switch (c)
		{
		case '|':
			tokentype = PIPE;
			break;
		case ';':
			tokentype = SEMICONLON;
			break;
		case '<':
			tokentype = LEFTREDIRECTION;
			break;
		case '>':
			tokentype = RIGHTREDIRECTION;
			break;
		case '(':
			tokentype = LEFTBRACKET;
			break;
		case ')':
			tokentype = RIGHTBRACKET;
			break;
		case '#':
			if (iter != 0 && is_word_token(input[iter - 1]))syntax_error(line);
			iter++;
			while ((input[iter] != '\n') && (input[iter] != '\0'))
				iter++;
			if (input[iter] == '\n')
			{
				line++;
				iter++;
			}
			c = input[iter];
			continue;
		case '\n':
			line++;
			char* tmp = try_malloc(2);
			tmp[0] = c;
			tmp[1] = '\0';
			append_token(ret, make_token(NEWLINE, tmp, line));
		case '\t':
		case ' ':
			iter++;
			c = input[iter];
			continue;
		default:
			tokentype = UNDETERMINED;
		}

		if (tokentype != UNDETERMINED)
		{
			char* tmp = try_malloc(2);
			tmp[0] = c;
			tmp[1] = '\0';
			append_token(ret, make_token(tokentype, tmp, line));
			iter++;
			c = input[iter];
			continue;
		}
		while (is_word_token(input[iter]) && (input[iter] != '\0'))
		{
			if (tmp_iter >= BLOCK*MULTIPLIER)
			{
				MULTIPLIER++;
				tmp_buffer = try_realloc(tmp_buffer, BLOCK*MULTIPLIER);
			}
			tmp_buffer[tmp_iter] = input[iter];
			tmp_iter++;
			iter++;
		}
		memcpy(tmp_buffer + tmp_iter, "\0", 1);
		append_token(ret, evaluate_text(tmp_buffer, tmp_iter, line));
		tmp_iter = 0;
		c = input[iter];
	}
	return ret;
}
//debugging method
void print_token_chain(token_chain_t chain)
{
	if (chain->head == NULL)return;
	token_t iter = chain->head;
	while (iter->next != NULL)
	{
		printf("%s\n", iter->text);
		iter = iter->next;
	}
	printf("%s\n", iter->text);

}

struct Stack
{
	enum token_type* data;
	size_t count;
	size_t capacity;
};

typedef struct Stack* Stack_t;
Stack_t stack_construct()
{
	Stack_t ret = try_malloc(sizeof(struct Stack));
	ret->capacity = BLOCK;
	ret->count = 0;
	ret->data = try_malloc(ret->capacity*sizeof(enum token_type*));
	return ret;
}
bool stack_push(Stack_t stack, enum token_type in)
{
	if (stack == NULL)return false;
	if (stack->count >= stack->capacity)
	{
		stack->capacity *= 2;
		stack->data = try_realloc(stack->data, stack->capacity);
	}
	stack->data[stack->count] = in;
	stack->count++;
	return true;
}
enum token_type stack_top(Stack_t stack)
{
	if (stack == NULL || stack->count == 0)return UNDETERMINED;
	return stack->data[stack->count - 1];
}
enum token_type stack_pop(Stack_t stack)
{
	if (stack == NULL || stack->count == 0)return UNDETERMINED;
	stack->count--;
	return stack->data[stack->count];
}

typedef struct command_node* command_node_t;
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

command_t make_command(enum command_type ctype)
{
	command_t ret = try_malloc(sizeof(struct command));
	ret->input = NULL;
	ret->output = NULL;
	ret->status = -1;
	ret->type = ctype;
	return ret;
}
command_node_t make_command_node(command_t cmd)
{
	command_node_t ret = try_malloc(sizeof(struct command_node));
	ret->node_next = NULL;
	ret->node_previous = NULL;
	ret->node_command = cmd;
	return ret;
}
int where_is_last_token(token_t* tokens, enum command_type type, size_t size)
{
	Stack_t stack = stack_construct();
	int i;
	switch (type)
	{
	case THEN_KEYWORD:
		for (i = 0; i < size; i++)
		{
			if (tokens[i]->type == IF_KEYWORD)stack_push(stack, IF_KEYWORD);
			else if (tokens[i]->type == THEN_KEYWORD)
			{
				if (stack_top(stack) != IF_KEYWORD)
					syntax_error(tokens[i]->line);
				if (stack->count == 1) return i;
				else stack_push(stack, THEN_KEYWORD);
			}
			else if (tokens[i]->type == ELSE_KEYWORD)
			{
				if (stack_top(stack) != THEN_KEYWORD)
					syntax_error(tokens[i]->line);
				stack_push(stack, THEN_KEYWORD);
			}
			else if (tokens[i]->type == FI_KEYWORD)
			{
				if (stack == NULL) syntax_error(tokens[i]->line);
				while (stack_pop(stack) != IF_KEYWORD);
			}
		}
		break;
	case ELSE_KEYWORD:
		for (i = 0; i < size; i++)
		{
			if (tokens[i]->type == IF_KEYWORD)stack_push(stack, IF_KEYWORD);
			else if (tokens[i]->type == THEN_KEYWORD)
			{
				if (stack_top(stack) != IF_KEYWORD)
					syntax_error(tokens[i]->line);
				stack_push(stack, THEN_KEYWORD);
			}
			else if (tokens[i]->type == ELSE_KEYWORD)
			{
				if (stack_top(stack) != THEN_KEYWORD)
					syntax_error(tokens[i]->line);
				if (stack->count == 2) return i;
				else stack_push(stack, THEN_KEYWORD);
			}
			else if (tokens[i]->type == FI_KEYWORD)
			{
				if (stack == NULL) syntax_error(tokens[i]->line);
				while (stack_pop(stack) != IF_KEYWORD);
			}
		}
		break;
	case DO_KEYWORD:
		for (i = 0; i < size; i++)
		{
			if (tokens[i]->type == WHILE_KEYWORD || tokens[i]->type == UNTIL_KEYWORD)stack_push(stack, tokens[i]->type);
			else if (tokens[i]->type == DO_KEYWORD)
			{
				if (!(stack_top(stack) == WHILE_KEYWORD || stack_top(stack) == UNTIL_KEYWORD))
					syntax_error(tokens[i]->line);
				if (stack->count == 1) return i;
				else stack_push(stack, DO_KEYWORD);
			}
			else if (tokens[i]->type == DONE_KEYWORD)
			{
				if (stack == NULL) syntax_error(tokens[i]->line);
				enum token_type a = stack_pop(stack);
				while (!(a == WHILE_KEYWORD || a == UNTIL_KEYWORD))a = stack_pop(stack);
			}
		}
		break;
	default:
		for (i = size - 1; i >= 0; i--)
		{
			if (tokens[i]->type == type)
				return i;
		}
	}
	return -1;
}

command_t parse_command(token_t* tokens, size_t count)
{
	int else_k;
	int then_k;
	int fi_k;
	int do_k;
	int done_k;
	if (count <= 0)return NULL;
	enum command_type type = SIMPLE_COMMAND;
	Stack_t special_stack = stack_construct();
	if (tokens[0]->type == IF_KEYWORD)type = IF_COMMAND;
	else if (tokens[0]->type == UNTIL_KEYWORD && ((tokens[count - 1]->type == DONE_KEYWORD) || ((tokens[count - 2]->type == LEFTREDIRECTION || tokens[count - 2]->type == RIGHTREDIRECTION) && tokens[count - 1]->type == WORD && (tokens[count - 3]->type == DONE_KEYWORD))))type = UNTIL_COMMAND;
	else if (tokens[0]->type == WHILE_KEYWORD && ((tokens[count - 1]->type == DONE_KEYWORD) || ((tokens[count - 2]->type == LEFTREDIRECTION || tokens[count - 2]->type == RIGHTREDIRECTION) && tokens[count - 1]->type == WORD && (tokens[count - 3]->type == DONE_KEYWORD))))type = WHILE_COMMAND;
	else if (tokens[0]->type == LEFTBRACKET)type = SUBSHELL_COMMAND;
	else
	{
		size_t i;
		for (i = 0; i < count; i++)
		{
			if (tokens[i]->type == PIPE){ type = PIPE_COMMAND; }
			else if (tokens[i]->type == SEMICONLON  && i != count - 1 && tokens[i + 1]->type == WORD)
			{
				tokens[i]->text = ";";
				tokens[i]->type = SEMICONLON;
				type = SEQUENCE_COMMAND;
			}
		}
	}

	command_t cmd = make_command(type);

	size_t capacity = 1024;
	size_t tmp_count = 0;
	char** tmp = try_malloc(capacity);

	size_t buffer_capacity = 100;
	size_t buffer_count = 0;
	token_t* buffer = try_malloc(sizeof(token_t)*capacity);

	bool in = false;
	bool out = false;
	size_t i;
	switch (type)
	{
	case SEQUENCE_COMMAND:
	case PIPE_COMMAND:
		for (i = 0; i < count; i++)
		{
			if ((tokens[i]->type == PIPE || tokens[i]->type == SEMICONLON) && i != count - 1 && tokens[i + 1]->type == WORD)
			{
				if (i == 0 || i == count - 1)
					syntax_error(tokens[i]->line);
				cmd->u.command[0] = parse_command(tokens, i);
				cmd->u.command[1] = parse_command(tokens + i + 1, count - i - 1);
				break;
			}
		}
		break;
	case SUBSHELL_COMMAND:
		if (tokens[count - 1]->type != RIGHTBRACKET)syntax_error(tokens[count - 1]->line);
		cmd->u.command[0] = parse_command(tokens + 1, count - 2);
		break;
	case SIMPLE_COMMAND:
		for (i = 0; i < count; i++)
		{
			if (i == 0 && tokens[i]->type != WORD)
				syntax_error(tokens[i]->line);
			if (tokens[i]->type == WORD)
			{
				if (tmp_count >= capacity)
				{
					capacity *= 2;
					tmp = try_realloc(tmp, capacity);
				}
				tmp[tmp_count] = tokens[i]->text;
				tmp_count++;
			}
			else if (tokens[i]->type == LEFTREDIRECTION)
			{
				if (in || i == count - 1 || tokens[i + 1]->type != WORD)syntax_error(tokens[i]->line);
				cmd->input = tokens[i + 1]->text;
				in = true;
				i++;
			}
			else if (tokens[i]->type == RIGHTREDIRECTION)
			{
				if (i == count - 1 || tokens[i + 1]->type != WORD)syntax_error(tokens[i]->line);
				cmd->output = tokens[i + 1]->text;
				out = true;
				i++;
			}
			else if (tokens[i]->type == SEMICONLON&& i == count - 1);
			else syntax_error(tokens[i]->line);
		}
		tmp[tmp_count] = "\0";
		cmd->u.word = tmp;
		break;
	case IF_COMMAND:
		else_k = where_is_last_token(tokens, ELSE_KEYWORD, count);
		then_k = where_is_last_token(tokens, THEN_KEYWORD, count);
		fi_k = where_is_last_token(tokens, FI_KEYWORD, count);
		if (fi_k != count - 1)syntax_error(tokens[count - 1]->line);

		if (then_k == -1)
		{
			syntax_error(tokens[else_k]->line);
		}
		else if (then_k>else_k)
		{
			if (else_k != -1 && where_is_last_token(tokens + else_k + 1, FI_KEYWORD, then_k - else_k - 1) == -1)
				syntax_error(tokens[else_k]->line);
			cmd->u.command[0] = parse_command(tokens + 1, then_k - 1);
			cmd->u.command[1] = parse_command(tokens + then_k + 1, fi_k - then_k - 1);
		}
		else
		{
			cmd->u.command[0] = parse_command(tokens + 1, then_k - 1);
			cmd->u.command[1] = parse_command(tokens + then_k + 1, else_k - then_k - 1);
			cmd->u.command[2] = parse_command(tokens + else_k + 1, fi_k - else_k - 1);
			if (cmd->u.command[0] == NULL || cmd->u.command[1] == NULL || cmd->u.command[2] == NULL)syntax_error(tokens[count - 1]->line);
		}
		break;
	case UNTIL_COMMAND:
	case WHILE_COMMAND:
		do_k = where_is_last_token(tokens, DO_KEYWORD, count);
		done_k = where_is_last_token(tokens, DONE_KEYWORD, count);
		enum token_type cc = tokens[count - 2]->type;
		if (tokens[count - 1]->type == WORD && (cc == LEFTREDIRECTION || cc == RIGHTREDIRECTION))
		{
			if (cc == LEFTREDIRECTION)cmd->input = tokens[count - 1]->text;
			else cmd->output = tokens[count - 1]->text;
			count -= 2;
		}
		else if (done_k != count - 1)syntax_error(tokens[count - 1]->line);

		if ((do_k == -1 || done_k == -1) || do_k > done_k)
		{
			syntax_error(tokens[0]->line);
		}
		else
		{
			cmd->u.command[0] = parse_command(tokens + 1, do_k - 1);
			cmd->u.command[1] = parse_command(tokens + do_k + 1, done_k - do_k - 1);
			if (cmd->u.command[0] == NULL || cmd->u.command[1] == NULL)syntax_error(tokens[count - 1]->line);
		}
		break;
	}
	return cmd;
}
void append_command_node(command_stream_t stream, command_node_t node)
{
	node->node_next = NULL;
	node->node_previous = NULL;
	if (stream->head == NULL)
	{
		stream->head = node;
		stream->tail = node;
	}
	else if (stream->head == stream->tail)
	{
		stream->tail = node;
		node->node_previous = stream->head;
		stream->head->node_next = node;
	}
	else
	{
		node->node_previous = stream->tail;
		stream->tail->node_next = node;
		stream->tail = node;
	}
}
command_stream_t make_command_stream(int(*get_next_byte) (void *), void *get_next_byte_argument)
{
	char* txt = buffer_stream(get_next_byte,get_next_byte_argument);
	token_chain_t chain = tokenize(txt);

	size_t capacity = 1024;
	size_t count = 0;
	token_t* buffer = try_malloc(sizeof(token_t)*capacity);

	command_stream_t ret = try_malloc(sizeof(struct command_stream));
	ret->head = NULL;
	ret->tail = NULL;

	Stack_t special_stack = stack_construct();

	token_t iter = chain->head;
	while (iter != NULL)
	{
		switch (iter->type)
		{
		case IF_KEYWORD:
		case THEN_KEYWORD:
		case ELSE_KEYWORD:

		case WHILE_KEYWORD:
		case UNTIL_KEYWORD:
		case DO_KEYWORD:
		case LEFTBRACKET:
			stack_push(special_stack, iter->type);

		case PIPE:
			//do here
		case SEMICONLON:
			//do here
		case WORD:
		case LEFTREDIRECTION:
		case RIGHTREDIRECTION:
			if (count >= capacity)
			{
				capacity *= 2;
				buffer = try_realloc(buffer, capacity);
			}
			buffer[count] = iter;
			count++;
			break;
		case FI_KEYWORD:
			if (count >= capacity)
			{
				capacity *= 2;
				buffer = try_realloc(buffer, capacity);
			}
			buffer[count] = iter;
			count++;
			enum token_type aa = stack_pop(special_stack);
			if (aa == ELSE_KEYWORD)
			{
				if (stack_pop(special_stack) != THEN_KEYWORD)syntax_error(iter->line);
				if (stack_pop(special_stack) != IF_KEYWORD)syntax_error(iter->line);
			}
			else if (aa == THEN_KEYWORD)
			{
				if (stack_pop(special_stack) != IF_KEYWORD)syntax_error(iter->line);
			}
			if (special_stack->count == 0)
			{
				append_command_node(ret, make_command_node(parse_command(buffer, count)));
				count = 0;
			}
			break;
		case DONE_KEYWORD:
			if (count >= capacity)
			{
				capacity *= 2;
				buffer = try_realloc(buffer, capacity);
			}
			buffer[count] = iter;
			count++;
			if (stack_pop(special_stack) != DO_KEYWORD)syntax_error(iter->line);
			enum token_type bb = stack_pop(special_stack);
			if (!(bb == WHILE_KEYWORD || bb == UNTIL_KEYWORD))syntax_error(iter->line);
			if (special_stack->count == 0)
			{
				append_command_node(ret, make_command_node(parse_command(buffer, count)));
				count = 0;
			}
			break;
		case RIGHTBRACKET:
			if (count >= capacity)
			{
				capacity *= 2;
				buffer = try_realloc(buffer, capacity);
			}
			buffer[count] = iter;
			count++;
			if (stack_pop(special_stack) != LEFTBRACKET)syntax_error(iter->line);
			if (special_stack->count == 0)
			{
				append_command_node(ret, make_command_node(parse_command(buffer, count)));
				count = 0;
			}
			break;
		case NEWLINE:
			if (special_stack->count != 0 && iter->next != NULL&&iter->next->type == WORD && (iter->previous->type == DONE_KEYWORD || iter->previous->type == FI_KEYWORD || iter->previous->type == RIGHTBRACKET))
			{
				iter->text = ";";
				iter->type = SEMICONLON;
				buffer[count] = iter;
				count++;
			}
			else if (count != 0 && special_stack->count == 0)
			{
				append_command_node(ret, make_command_node(parse_command(buffer, count)));
				count = 0;
			}

			break;
		default:
			break;
		}
		iter = iter->next;
	}
	if (count != 0)
	{
		append_command_node(ret, make_command_node(parse_command(buffer, count)));
		count = 0;
	}
	return ret;
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


