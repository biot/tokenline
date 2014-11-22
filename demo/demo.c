/*
 * Copyright (C) 2014 Bert Vermeulen <bert@biot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "tokenline.h"

extern t_token tokens[];
extern t_token_dict dict[];

void print(void *user, const char *str)
{
	(void)user;

	printf("%s", str);
}

static void dump_parsed(void *user, t_tokenline_parsed *p)
{
	float arg_float;
	int arg_int, i;

	(void)user;

	for (i = 0; p->tokens[i]; i++) {
		printf("%d: ", i);
		switch (p->tokens[i]) {
		case T_ARG_INT:
			memcpy(&arg_int, p->buf + p->tokens[++i], sizeof(int));
			printf("integer %d\n", arg_int);
			break;
		case T_ARG_FLOAT:
			memcpy(&arg_float, p->buf + p->tokens[++i], sizeof(float));
			printf("float %f\n", arg_float);
			break;
		case T_ARG_FREQ:
			memcpy(&arg_float, p->buf + p->tokens[++i], sizeof(float));
			printf("frequency %f\n", arg_float);
			break;
		case T_ARG_STRING:
			printf("string '%s'\n", p->buf + p->tokens[++i]);
			break;
		default:
			printf("token %d (%s)\n", p->tokens[i],
					dict[p->tokens[i]].tokenstr);
		}
	}
}

int main(int argc, char **argv)
{
	t_tokenline tl;
	struct termios old_termios, new_termios;
	int c;

	(void)argc;
	(void)argv;

	tcgetattr(0, &old_termios);
	tcgetattr(0, &new_termios);
	new_termios.c_lflag = ~(ICANON|ECHO);
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 0;
	tcsetattr(0, TCSAFLUSH, &new_termios);
	tl_init(&tl, tokens, dict, print, NULL);
	tl_set_prompt(&tl, "> ");
	tl_set_callback(&tl, dump_parsed);
	while (TRUE) {
		c = getchar();
		if (!tl_input(&tl, c))
			break;
	}
	tcsetattr(0, TCSANOW, &old_termios);
	printf("\n");

	return 0;
}
