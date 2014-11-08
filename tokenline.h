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

#include <stdint.h>

#define TL_MAX_LINE_LEN         128
#define TL_MAX_ESCAPE_LEN       8
#define TL_MAX_WORDS            64
#define TL_MAX_TOKEN_LEVELS     8
#define TL_MAX_HISTORY_SIZE     512

typedef struct token_dict {
	int token;
	char *tokenstr;
} t_token_dict;

typedef struct token {
	int token;
	int arg_type;
	struct token *subtokens;
	char *help;
} t_token;

typedef struct tokenline_parsed {
	int tokens[TL_MAX_WORDS];
	char buf[TL_MAX_LINE_LEN];
	t_token *last_token_entry;
} t_tokenline_parsed;

typedef void (*tl_printfunc)(void *user, const char *str);
typedef void (*tl_callback)(void *user, t_tokenline_parsed p);
typedef struct tokenline {
	t_token *token_levels[TL_MAX_TOKEN_LEVELS];
	int token_level;
	t_token_dict *token_dict;
	tl_printfunc print;
	void *user;
	char buf[TL_MAX_LINE_LEN];
	int buf_len;
	char escape[TL_MAX_ESCAPE_LEN];
	int escape_len;
	char *prompt;
	tl_callback callback;
	int pos;
	t_tokenline_parsed parsed;
	int hist_step;
	int hist_begin;
	int hist_end;
	/* Need a terminating zero to print last chunk in ring buffer. */
	char hist_buf[TL_MAX_HISTORY_SIZE + 1];
} t_tokenline;

/* These share a number space with the tokens. */
enum token_argtypes {
	TARG_INT = 10000,
	TARG_FLOAT,
	TARG_STRING,
	/* Argument is one of the tokens in subtokens. */
	TARG_TOKEN,
	TARG_HELP,
};

void tl_init(t_tokenline *tl, t_token *tokens_top, t_token_dict *token_dict,
		tl_printfunc printfunc, void *user);
void tl_set_prompt(t_tokenline *tl, char *prompt);
void tl_set_callback(t_tokenline *tl, tl_callback callback);
int tl_input(t_tokenline *tl, uint8_t c);

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

