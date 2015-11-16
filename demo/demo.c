/*
 * Copyright (C) 2014 Bert Vermeulen <bert@biot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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
#include "commands.h"

#include "tokenline.h"

extern t_token tokens[];
extern t_token_dict dict[];
extern t_token tokens_mode_device[];
extern t_token tokens_mode_calc[];

enum {
	MODE_TOP,
	MODE_DEVICE,
	MODE_CALC,
};

struct demo_context {
	t_tokenline *tl;
	int mode;
} ctx = { };

void print(void *user, const char *str)
{
	(void)user;

	printf("%s", str);
}

static void dump_parsed(void *user, t_tokenline_parsed *p)
{
	struct demo_context *ctx;
	float arg_float;
	uint32_t arg_uint;
	int arg_int, i;

	ctx = user;
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
		case T_ARG_STRING:
			printf("string '%s'\n", p->buf + p->tokens[++i]);
			break;
		case T_ARG_TOKEN_SUFFIX_INT:
			memcpy(&arg_uint, p->buf + p->tokens[++i], sizeof(uint32_t));
			printf("token-suffixed integer %d\n", arg_uint);
			break;
		default:
			printf("token %d (%s)\n", p->tokens[i],
					dict[p->tokens[i]].tokenstr);
		}
	}

	switch (p->tokens[0]) {
	case T_DEVICE:
		ctx->mode = MODE_DEVICE;
		tl_set_prompt(ctx->tl, "device> ");
		tl_mode_push(ctx->tl, tokens_mode_device);
		break;
	case T_CALC:
		ctx->mode = MODE_CALC;
		tl_set_prompt(ctx->tl, "calc> ");
		tl_mode_push(ctx->tl, tokens_mode_calc);
		break;
	case T_EXIT:
		ctx->mode = MODE_TOP;
		tl_set_prompt(ctx->tl, "> ");
		tl_mode_pop(ctx->tl);
		break;
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
	ctx.tl = &tl;
	tl_init(&tl, tokens, dict, print, &ctx);
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
