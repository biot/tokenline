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
#include <stdlib.h>
#include <string.h>
#include "tokenline.h"

#define INDENT   "   "
#define NO_HELP  "No help available."NL
#define NL       "\r\n"

static void line_clear(t_tokenline *tl);
static void line_backspace(t_tokenline *tl);
static void set_line(t_tokenline *tl, char *line);


static void unsplit_line(t_tokenline *tl)
{
	int quoted, i;

	quoted = FALSE;
	for (i = 0; i < tl->buf_len; i++) {
		if (tl->buf[i] == '"') {
			quoted = TRUE;
			continue;
		}
		if (!tl->buf[i]) {
			if (quoted) {
				tl->buf[i] = '"';
				quoted = FALSE;
			} else {
				tl->buf[i] = ' ';
			}
		}
	}
	tl->buf[tl->buf_len] = 0;
}

static int split_line(t_tokenline *tl, int *words, int *num_words, int silent)
{
	int state, quoted, i;

	state = 1;
	quoted = FALSE;
	*num_words = 0;
	for (i = 0; i < tl->buf_len && *num_words < TL_MAX_WORDS; i++) {
		switch (state) {
		case 1:
			/* Looking for a new word. */
			if (tl->buf[i] == ' ')
				continue;
			if (tl->buf[i] == '"')
				quoted = TRUE;
			words[(*num_words)++] = i + (quoted ? 1 : 0);
			state = 2;
			break;
		case 2:
			/* In a word. */
			if (quoted && tl->buf[i] == '"') {
				quoted = FALSE;
				tl->buf[i] = 0;
				state = 1;
			} else if (!quoted && tl->buf[i] == ' ') {
				tl->buf[i] = 0;
				state = 1;
			}
			break;
		}
	}
	if (quoted) {
		if (!silent)
			tl->print(tl->user, "Unmatched quote."NL);
		unsplit_line(tl);
		return FALSE;
	}
	if (*num_words == TL_MAX_WORDS) {
		if (!silent)
			tl->print(tl->user, "Too many words."NL);
		unsplit_line(tl);
		return FALSE;
	}

	return TRUE;
}

static int history_previous(t_tokenline *tl, int entry)
{
	int prev;

	if (entry == tl->hist_begin)
		return -1;

	/* Onto the terminating zero of the previous entry. */
	if (--entry < 0)
		entry = TL_MAX_HISTORY_SIZE - 1;

	/* Onto the last byte of the previous entry. */
	prev = entry;
	if (--entry < 0)
		entry = TL_MAX_HISTORY_SIZE - 1;
	while (tl->hist_buf[entry]) {
		prev = entry;
		if (--entry < 0)
			entry = TL_MAX_HISTORY_SIZE - 1;
	}

	return prev;
}

static void history_up(t_tokenline *tl)
{
	int entry;

	if (tl->hist_step == -1)
		entry = history_previous(tl, tl->hist_end);
	else
		entry = history_previous(tl, tl->hist_step);
	if (entry == -1)
		return;
	line_clear(tl);
	set_line(tl, tl->hist_buf + entry);
	tl->hist_step = entry;
}

static void history_down(t_tokenline *tl)
{
	int i;

	if (tl->hist_step == -1)
		return;

	line_clear(tl);
	if (tl->hist_step == tl->hist_end) {
		tl->hist_step = -1;
		return;
	}

	i = tl->hist_step;
	while (tl->hist_buf[i]) {
		if (++i == TL_MAX_HISTORY_SIZE)
			i = 0;
	}
	if (++i == TL_MAX_HISTORY_SIZE)
		i = 0;

	set_line(tl, tl->hist_buf + i);
	tl->hist_step = i;
}

static void history_show(t_tokenline *tl)
{
	int entry, i;

	/* Skip the 'history' command itself. */
	entry = history_previous(tl, tl->hist_end);
	for (entry = history_previous(tl, entry); entry != -1;
			entry = history_previous(tl, entry)) {
		tl->print(tl->user, tl->hist_buf + entry);

		/* Did we drop off the end of the buffer? */
		for (i = entry; i < TL_MAX_HISTORY_SIZE; i++) {
			if (!tl->hist_buf[i])
				break;
		}
		if (i == TL_MAX_HISTORY_SIZE)
			/* Yes we did */
			tl->print(tl->user, tl->hist_buf);

		tl->print(tl->user, NL);
	}

}

/* Free at least size, but also fully zero out any deleted entry. */
static int history_delete(t_tokenline *tl, int entry, int size)
{
	int freed, i;

	freed = 0;
	i = entry;
	while (freed < size || tl->hist_buf[i]) {
		tl->hist_buf[i] = 0;
		freed++;
		if (++i == TL_MAX_HISTORY_SIZE)
			i = 0;
	}
	tl->hist_buf[i] = 0;
	/*
	 * Skip at least over the terminating zero, and any after that,
	 * to the next entry.
	 */
	do {
		if (++i == TL_MAX_HISTORY_SIZE)
			i = 0;
	} while (!tl->hist_buf[i] && i < entry);

	return i;
}

static void history_add(t_tokenline *tl)
{
	int size, entry, part1;

	size = strlen(tl->buf) + 1;
	if (tl->hist_begin == tl->hist_end) {
		/* First entry, both are 0. */
		memcpy(tl->hist_buf, tl->buf, size);
	} else if (tl->hist_begin < tl->hist_end) {
		if (TL_MAX_HISTORY_SIZE - tl->hist_end >= size) {
			memcpy(tl->hist_buf + tl->hist_end, tl->buf, size);
		} else {
			part1 = TL_MAX_HISTORY_SIZE - tl->hist_end;
			entry = tl->hist_begin;
			tl->hist_begin = history_delete(tl, entry, size - part1);
			memcpy(tl->hist_buf + tl->hist_end, tl->buf, part1);
			memcpy(tl->hist_buf, tl->buf + part1, size - part1);
		}
	} else {
		part1 = tl->hist_begin - tl->hist_end;
		if (part1 <= size) {
			/* Not enough room between end and begin. */
			tl->hist_begin = history_delete(tl, tl->hist_begin, size - part1);
			part1 = TL_MAX_HISTORY_SIZE - tl->hist_end;
			if (part1 < size) {
				memcpy(tl->hist_buf + tl->hist_end, tl->buf, part1);
				memcpy(tl->hist_buf, tl->buf + part1, size - part1);
			} else {
				memcpy(tl->hist_buf + tl->hist_end, tl->buf, size);
			}
		} else {
			/* Enough room between end and begin. */
			memcpy(tl->hist_buf + tl->hist_end, tl->buf, size);
		}
	}
	tl->hist_end += size;
	if (tl->hist_end >= TL_MAX_HISTORY_SIZE)
		tl->hist_end -= TL_MAX_HISTORY_SIZE;
	if (tl->hist_begin == tl->hist_end)
		tl->hist_begin = history_delete(tl, tl->hist_begin,
				strlen(tl->hist_buf + tl->hist_begin));
}

/*
 * Converts string to uint32_t. Takes decimal, hex prefixed with 0x,
 * binary prefixed with 0b and octal prefixed with 0. Returns FALSE
 * if conversion in any of these fails.
 */
static int str_to_uint(char *s, uint32_t *out)
{
	int i;
	char *suffix;

	if (strncmp(s, "0b", 2)) {
		*out = strtoul(s, &suffix, 0);
		if (*suffix != '\0')
			return FALSE;
	} else {
		*out = 0;
		for (i = 2; s[i]; i++) {
			if (s[i] != '0' && s[i] != '1')
				return FALSE;
			*out <<= 1;
			*out |= s[i] - 0x30;
		}
	}

	return TRUE;
}

static char *arg_type_to_string(int arg_type)
{
	if (arg_type == T_ARG_INT)
		return "<integer>";
	else if (arg_type == T_ARG_FLOAT)
		return "<float>";
	else if (arg_type == T_ARG_FREQ)
		return "<frequency>";
	else if (arg_type == T_ARG_STRING)
		return "<string>";

	return NULL;
}

static int find_token(t_token *tokens, t_token_dict *token_dict, char *word)
{
	uint32_t arg_uint;
	int token, partial, i;

	/* Find exact match. */
	for (i = 0; tokens[i].token; i++) {
		token = tokens[i].token;
		if (token == T_ARG_INT) {
			if (str_to_uint(word, &arg_uint))
				return i;
		} else {
			if (!strcmp(word, token_dict[token].tokenstr))
				return i;
		}
	}

	/* Find partial match. */
	partial = -1;
	for (i = 0; tokens[i].token; i++) {
		token = tokens[i].token;
		if (token >= T_ARG_INT)
			continue;
		if (strlen(word) >= strlen(token_dict[token].tokenstr))
			continue;
		if (!strncmp(word, token_dict[token].tokenstr, strlen(word))) {
			if (partial != -1)
				/* Not unique. */
				return -1;
			else
				partial = i;
		}
	}

	return partial;
}

/*
 * Tokenize the current set of NULL-terminated words, allowing for
 * one token sublevel starting from the current token level.
 */
static int tokenize(t_tokenline *tl, int *words, int num_words,
		t_token **complete_tokens, int *complete_arg)
{
	t_token *token_stack[8], *arg_tokens;
	t_tokenline_parsed *p;
	float arg_float;
	uint32_t arg_uint, suffix_uint;
	int done, arg_needed, arg_int, w, t, t_idx, size;
	int cur_tsp, cur_tp, cur_bufsize;
	char *word, *suffix;

	done = FALSE;
	p = &tl->parsed;
	token_stack[0] = tl->token_levels[tl->token_level];
	cur_tsp = 0;
	cur_tp = 0;
	cur_bufsize = 0;
	arg_needed = 0;
	arg_tokens = NULL;
	for (w = 0; w < num_words; w++) {
		word = tl->buf + words[w];
		if (done) {
			if (!complete_tokens)
				tl->print(tl->user, "Too many arguments."NL);
			return FALSE;
		} else if (!arg_needed) {
			/* Token needed. */
			if ((suffix = strchr(word, TL_TOKEN_DELIMITER))) {
				*suffix++ = 0;
				if (!str_to_uint(suffix, &suffix_uint)) {
					tl->print(tl->user, "Invalid number."NL);
					return FALSE;
				}
			} else {
				suffix_uint = 0;
			}

			if ((t_idx = find_token(token_stack[cur_tsp], tl->token_dict, word)) > -1) {
				t = token_stack[cur_tsp][t_idx].token;
				p->tokens[cur_tp++] = t;
				if (t == T_ARG_INT) {
					/* Integer token. */
					str_to_uint(word, &arg_uint);
					p->tokens[cur_tp++] = cur_bufsize;
					memcpy(p->buf + cur_bufsize, &arg_uint, sizeof(uint32_t));
					cur_bufsize += sizeof(uint32_t);
				}
				if (suffix) {
					if (!(token_stack[cur_tsp][t_idx].flags &
							T_FLAG_SUFFIX_TOKEN_DELIM_INT)) {
						if (!complete_tokens)
							tl->print(tl->user, "Token suffix not allowed."NL);
						return FALSE;
					}
					if (suffix_uint > 1) {
						p->tokens[cur_tp++] = T_ARG_TOKEN_SUFFIX_INT;
						p->tokens[cur_tp++] = cur_bufsize;
						memcpy(p->buf + cur_bufsize, &suffix_uint, sizeof(uint32_t));
						cur_bufsize += sizeof(uint32_t);
					}
				}
				p->last_token_entry = &token_stack[cur_tsp][t_idx];

				if (token_stack[cur_tsp][t_idx].arg_type == T_ARG_HELP) {
					/* Nothing to do, just keep cur_tsp from increasing. */
				} else if (token_stack[cur_tsp][t_idx].arg_type) {
					/* Token needs an argument */
					arg_needed = token_stack[cur_tsp][t_idx].arg_type;
					if (arg_needed == T_ARG_TOKEN)
						/* Argument is one of these subtokens. */
						arg_tokens = token_stack[cur_tsp][t_idx].subtokens;
				} else if (token_stack[cur_tsp][t_idx].subtokens) {
					/* Switch to a new token set. */
					token_stack[cur_tsp + 1] = token_stack[cur_tsp][t_idx].subtokens;
					cur_tsp++;
				} else {
					/* Not expecting any more arguments or tokens. */
					done = tl->one_command_per_line;
				}
			} else {
				if (!complete_tokens)
					tl->print(tl->user, "Invalid command."NL);
				return FALSE;
			}
		} else {
			/* Parse word as the type in arg_needed */
			switch (arg_needed) {
			case T_ARG_INT:
				arg_int = strtol(word, &suffix, 0);
				if (*suffix) {
					if (!complete_tokens)
						tl->print(tl->user, "Invalid value."NL);
					return FALSE;
				}
				p->tokens[cur_tp++] = T_ARG_INT;
				p->tokens[cur_tp++] = cur_bufsize;
				memcpy(p->buf + cur_bufsize, &arg_int, sizeof(int));
				cur_bufsize += sizeof(int);
				break;
			case T_ARG_FLOAT:
				arg_float = strtof(word, &suffix);
				if (*suffix) {
					if (!complete_tokens)
						tl->print(tl->user, "Invalid value."NL);
					return FALSE;
				}
				p->tokens[cur_tp++] = T_ARG_FLOAT;
				p->tokens[cur_tp++] = cur_bufsize;
				memcpy(p->buf + cur_bufsize, &arg_float, sizeof(float));
				cur_bufsize += sizeof(float);
				break;
			case T_ARG_FREQ:
				arg_float = strtof(word, &suffix);
				if (*suffix) {
					if (!strcmp(suffix, "khz"))
						arg_float *= 1000;
					else if (!strcmp(suffix, "mhz"))
						arg_float *= 1000000;
					else if (!strcmp(suffix, "ghz"))
						arg_float *= 1000000000L;
					else {
						if (!complete_tokens)
							tl->print(tl->user, "Invalid value."NL);
						return FALSE;
					}
				}
				p->tokens[cur_tp++] = T_ARG_FREQ;
				p->tokens[cur_tp++] = cur_bufsize;
				memcpy(p->buf + cur_bufsize, &arg_float, sizeof(float));
				cur_bufsize += sizeof(float);
				break;
			case T_ARG_STRING:
				p->tokens[cur_tp++] = T_ARG_STRING;
				p->tokens[cur_tp++] = cur_bufsize;
				size = strlen(word) + 1;
				memcpy(p->buf + cur_bufsize, word, size);
				cur_bufsize += size;
				p->buf[cur_bufsize] = 0;
				break;
			case T_ARG_TOKEN:
				if ((t_idx = find_token(arg_tokens, tl->token_dict, word)) > -1) {
					p->tokens[cur_tp++] = arg_tokens[t_idx].token;
					p->last_token_entry = &arg_tokens[t_idx];
				} else {
					if (!complete_tokens)
						tl->print(tl->user, "Invalid value."NL);
					return FALSE;
				}
				break;
			}
			arg_needed = 0;
			done = tl->one_command_per_line;
		}
	}
	if (arg_needed && !complete_tokens) {
		tl->print(tl->user, "Missing argument."NL);
		return FALSE;
	}

	p->tokens[cur_tp] = 0;

	if (complete_tokens) {
		if (done) {
			/* Nothing to add. */
			*complete_tokens = NULL;
		} else {
			/* Fill in the completion token list. */
			if (arg_needed == T_ARG_TOKEN)
				*complete_tokens = arg_tokens;
			else
				*complete_tokens = token_stack[cur_tsp];
		}
	}
	if (complete_arg)
		*complete_arg = arg_needed;

	return TRUE;
}

static void show_help(t_tokenline *tl, int *words, int num_words)
{
	t_token *tokens;
	int i;
	char *s, space[] = "               ";

	(void)words;

	if (tl->parsed.last_token_entry->help) {
		tl->print(tl->user, tl->parsed.last_token_entry->help);
		tl->print(tl->user, NL);
	}

	if (num_words == 1)
		/* Just "help" -- global command overview. */
		tokens = tl->token_levels[tl->token_level];
	else
		tokens = tl->parsed.last_token_entry->subtokens;
	if (tokens) {
		for (i = 0; tokens[i].token; i++) {
			tl->print(tl->user, INDENT);
			if (tokens[i].token < T_ARG_INT)
				s = tl->token_dict[tokens[i].token].tokenstr;
			else
				s = arg_type_to_string(tokens[i].token);
			tl->print(tl->user, s);
			if (tokens[i].help) {
				tl->print(tl->user, space + strlen(s));
				tl->print(tl->user, tokens[i].help);
			}
			tl->print(tl->user, NL);
		}
	}
	if (!tl->parsed.last_token_entry->help && !tokens)
		tl->print(tl->user, NO_HELP);
}

static void process_line(t_tokenline *tl)
{
	t_token *tokens;
	int words[TL_MAX_WORDS], num_words;

	tl->print(tl->user, NL);
	do {
		if (!tl->buf_len)
			break;
		history_add(tl);
		if (!split_line(tl, words, &num_words, FALSE))
			break;
		if (!num_words)
			break;
		if (!strcmp(tl->buf + words[0], "help")) {
			/* Tokenize with errors turned off. */
			tokenize(tl, words + 1, num_words - 1, &tokens, NULL);
			if (tl->parsed.last_token_entry) {
				show_help(tl, words, num_words);
			}
		} else if (!strcmp(tl->buf + words[0], "history")) {
			history_show(tl);
		} else {
			if (!tokenize(tl, words, num_words, NULL, NULL))
				break;
			if (tl->callback)
				tl->callback(tl->user, &tl->parsed);
		}
	} while (FALSE);

	tl->buf[0] = 0;
	tl->buf_len = 0;
	tl->escape_len = 0;
	tl->pos = 0;
	tl->hist_step = -1;
	tl->print(tl->user, tl->prompt);
}

static void add_char(t_tokenline *tl, int c)
{
	int i;

	if (tl->pos == tl->buf_len) {
		tl->buf[tl->buf_len++] = c;
		tl->buf[tl->buf_len] = 0;
		tl->print(tl->user, tl->buf + tl->buf_len - 1);
		tl->pos++;
	} else {
		memmove(tl->buf + tl->pos + 1, tl->buf + tl->pos,
				tl->buf_len - tl->pos + 1);
		tl->buf[tl->pos] = c;
		tl->print(tl->user, tl->buf + tl->pos);
		for (i = 0; i < tl->buf_len - tl->pos; i++)
			tl->print(tl->user, "\x1b\x5b\x44");
		tl->buf_len++;
		tl->pos++;
	}
}

static void set_line(t_tokenline *tl, char *line)
{
	int size;

	size = strlen(line);
	if (size > TL_MAX_LINE_LEN - 1) {
		add_char(tl, '!');
		return;
	}

	if (tl->pos == tl->buf_len) {
		tl->print(tl->user, line);
		tl->pos += size;
		memcpy(tl->buf + tl->buf_len, line, size);
		tl->buf_len += size;
		tl->buf[tl->buf_len] = 0;
	}
}

static void complete(t_tokenline *tl)
{
	t_token *tokens;
	unsigned int i;
	int words[TL_MAX_WORDS], num_words, arg_needed, partial, multiple;
	int reprompt, t;
	char *word;

	reprompt = FALSE;
	if (!tl->pos) {
		/* Tab on an empty line: show all top-level commmands. */
		tl->print(tl->user, NL);
		tokens = tl->token_levels[tl->token_level];
		for (i = 0; tokens[i].token; i++) {
			tl->print(tl->user, INDENT);
			if (tokens[i].token < T_ARG_INT)
				tl->print(tl->user, tl->token_dict[tokens[i].token].tokenstr);
			else
				tl->print(tl->user, arg_type_to_string(tokens[i].token));
			tl->print(tl->user, NL);
		}
		reprompt = TRUE;
	} else if (tl->buf[tl->pos - 1] != ' ') {
		/* Try to complete the current word. */
		if (!split_line(tl, words, &num_words, TRUE) || !num_words)
			return;
		if (tokenize(tl, words, num_words - 1, &tokens, NULL)) {
			if (tokens) {
				word = tl->buf + words[num_words - 1];
				partial = 0;
				multiple = FALSE;
				for (t = 0; tokens[t].token; t++) {
					if (tokens[t].token >= T_ARG_INT)
						continue;
					if (!strncmp(word, tl->token_dict[tokens[t].token].tokenstr, strlen(word))) {
						if (partial) {
							/* Not the first match, print previous one. */
							multiple = TRUE;
							tl->print(tl->user, NL);
							tl->print(tl->user, INDENT);
							tl->print(tl->user, tl->token_dict[partial].tokenstr);
						}
						partial = tokens[t].token;
					}
				}
				if (partial) {
					if (multiple) {
						/* Last partial match. */
						tl->print(tl->user, NL);
						tl->print(tl->user, INDENT);
						tl->print(tl->user, tl->token_dict[partial].tokenstr);
						tl->print(tl->user, NL);
						reprompt = TRUE;
					} else {
						for (i = strlen(word); i < strlen(tl->token_dict[partial].tokenstr); i++)
							add_char(tl, tl->token_dict[partial].tokenstr[i]);
						add_char(tl, ' ');
					}
				}
			}
		}
	} else {
		/* List all possible tokens from this point. */
		if (!split_line(tl, words, &num_words, TRUE) || !num_words)
			return;
		if (tokenize(tl, words, num_words, &tokens, &arg_needed)) {
			if (arg_needed && arg_needed != T_ARG_TOKEN) {
				tl->print(tl->user, INDENT NL);
				tl->print(tl->user, arg_type_to_string(arg_needed));
				tl->print(tl->user, NL);
				reprompt = TRUE;
			} else if (tokens) {
				tl->print(tl->user, NL);
				for (t = 0; tokens[t].token; t++) {
					if (tokens[t].token >= T_ARG_INT)
						continue;
					tl->print(tl->user, INDENT);
					tl->print(tl->user, tl->token_dict[tokens[t].token].tokenstr);
					tl->print(tl->user, NL);
					reprompt = TRUE;
				}
			}
		}
	}
	unsplit_line(tl);
	if (reprompt) {
		tl->print(tl->user, tl->prompt);
		tl->print(tl->user, tl->buf);
	}
}

static void line_clear(t_tokenline *tl)
{
	while (tl->pos < tl->buf_len) {
		tl->pos++;
		tl->print(tl->user, "\x1b\x5b\x43");
	}
	while (tl->pos)
		line_backspace(tl);
}

static void line_backspace(t_tokenline *tl)
{
	int i;

	if (tl->pos == tl->buf_len) {
		tl->buf[tl->buf_len - 1] = 0;
		tl->print(tl->user, "\x1b\x5b\x44 \x1b\x5b\x44");
	} else {
		memmove(tl->buf + tl->pos - 1, tl->buf + tl->pos,
				tl->buf_len - tl->pos + 1);
		tl->print(tl->user, "\x1b\x5b\x44");
		tl->print(tl->user, tl->buf + tl->pos - 1);
		tl->print(tl->user, " ");
		for (i = 0; i < tl->buf_len - tl->pos + 1; i++)
			tl->print(tl->user, "\x1b\x5b\x44");
	}
	tl->buf_len--;
	tl->pos--;
}

static void line_home(t_tokenline *tl)
{
	while (tl->pos) {
		tl->pos--;
		tl->print(tl->user, "\x1b\x5b\x44");
	}
}

static void line_end(t_tokenline *tl)
{
	while (tl->pos < tl->buf_len) {
		tl->pos++;
		tl->print(tl->user, "\x1b\x5b\x43");
	}
}

static void line_delete_char(t_tokenline *tl)
{
	int i;

	if (tl->pos < tl->buf_len) {
		memmove(tl->buf + tl->pos, tl->buf + tl->pos + 1,
				tl->buf_len - tl->pos);
		tl->print(tl->user, tl->buf + tl->pos);
		tl->print(tl->user, " ");
		for (i = 0; i < tl->buf_len - tl->pos; i++)
			tl->print(tl->user, "\x1b\x5b\x44");
		tl->buf_len--;
	}
}

static int process_escape(t_tokenline *tl)
{
	if (tl->escape_len == 4) {
		if (!strncmp(tl->escape, "\x1b\x5b\x33\x7e", 4)) {
			/* Delete */
			line_delete_char(tl);
		} else if (!strncmp(tl->escape, "\x1b\x5b\x31\x7e", 4)) {
			/* Home */
			line_home(tl);
		} else if (!strncmp(tl->escape, "\x1b\x5b\x34\x7e", 4)) {
			/* End */
			line_end(tl);
		}
	} else if (tl->escape_len == 3) {
		if (!strncmp(tl->escape, "\x1b\x5b\x41", 3)) {
			/* Up arrow */
			history_up(tl);
		} else if (!strncmp(tl->escape, "\x1b\x5b\x42", 3)) {
			/* Down arrow */
			history_down(tl);
		} else if (!strncmp(tl->escape, "\x1b\x5b\x44", 3)) {
			/* Left arrow */
			if (tl->pos > 0) {
				tl->pos--;
				tl->print(tl->user, "\x1b\x5b\x31\x44");
			}
		} else if (!strncmp(tl->escape, "\x1b\x5b\x43", 3)) {
			/* Right arrow */
			if (tl->pos < tl->buf_len) {
				tl->pos++;
				tl->print(tl->user, "\x1b\x5b\x31\x43");
			}
		} else if (!strncmp(tl->escape, "\x1b\x4f\x48", 3)) {
			/* Home */
			line_home(tl);
		} else if (!strncmp(tl->escape, "\x1b\x4f\x46", 3)) {
			/* End */
			line_end(tl);
		} else
			return FALSE;
	} else
		return FALSE;

	return TRUE;
}

void tl_init(t_tokenline *tl, t_token *tokens_top, t_token_dict *token_dict,
		tl_printfunc printfunc, void *user)
{
	memset(tl, 0, sizeof(t_tokenline));
	tl->token_levels[0] = tokens_top;
	tl->token_dict = token_dict;
	tl->print = printfunc;
	tl->user = user;
	tl->one_command_per_line = TL_ONE_COMMAND_PER_LINE;
	tl->hist_step = -1;
}

void tl_set_prompt(t_tokenline *tl, char *prompt)
{
	if (!tl->prompt)
		tl->print(tl->user, prompt);
	tl->prompt = prompt;
}

void tl_set_callback(t_tokenline *tl, tl_callback callback)
{
	tl->callback = callback;
}

int tl_mode_push(t_tokenline *tl, t_token *tokens)
{
	if (tl->token_level == TL_MAX_TOKEN_LEVELS - 1)
		return FALSE;

	tl->token_levels[++tl->token_level] = tokens;

	return TRUE;
}

int tl_mode_pop(t_tokenline *tl)
{
	if (!tl->token_level)
		return FALSE;

	tl->token_level--;

	return TRUE;
}

int tl_input(t_tokenline *tl, uint8_t c)
{
	int ret, i;

	if (tl->escape_len) {
		tl->escape[tl->escape_len++] = c;
		if (process_escape(tl))
			tl->escape_len = 0;
		else if (tl->escape_len == TL_MAX_ESCAPE_LEN)
			/* Not a valid escape sequence, and buffer full. */
			tl->escape_len = 0;
		return TRUE;
	}

	ret = TRUE;
	switch (c) {
	case 0x1b:
		/* Start of escape sequence. */
		tl->escape[tl->escape_len++] = c;
		break;
	case '\r':
	case '\n':
		process_line(tl);
		break;
	case '\t':
		if (tl->buf_len == tl->pos)
			complete(tl);
		break;
	case 0x7f:
		/* Backspace */
		if (tl->pos)
			line_backspace(tl);
		break;
	case 0x01:
		/* Ctrl-a */
		while (tl->pos) {
			tl->pos--;
			tl->print(tl->user, "\x1b\x5b\x44");
		}
		break;
	case 0x03:
		/* Ctrl-c */
		tl->print(tl->user, "^C");
		tl->buf_len = 0;
		process_line(tl);
		break;
	case 0x04:
		/* Ctrl-d */
		if (tl->buf_len)
			line_delete_char(tl);
		else
			/* Ctrl-d on empty line exits. */
			ret = FALSE;
		break;
	case 0x05:
		/* Ctrl-e */
		while (tl->pos < tl->buf_len) {
			tl->pos++;
			tl->print(tl->user, "\x1b\x5b\x43");
		}
		break;
	case 0x0b:
		/* Ctrl-k */
		if (tl->buf_len > tl->pos) {
			for (i = 0; i < tl->buf_len - tl->pos; i++)
				tl->print(tl->user, " ");
			for (i = 0; i < tl->buf_len - tl->pos; i++)
				tl->print(tl->user, "\x1b\x5b\x44");
			tl->buf_len = tl->pos;
			tl->buf[tl->buf_len] = 0;
		}
		break;
	case 0x0c:
		/* Ctrl-l */
		tl->print(tl->user, "\x1b\x5b\x32\x4a\x1b\x5b\x48");
		tl->print(tl->user, tl->prompt);
		tl->print(tl->user, tl->buf);
		break;
	case 0x10:
		/* Ctrl-p */
		history_up(tl);
		break;
	case 0x0e:
		/* Ctrl-n */
		history_down(tl);
		break;
	case 0x17:
		/* Ctrl-w */
		while (tl->pos && tl->buf[tl->pos - 1] == ' ')
			line_backspace(tl);
		while (tl->pos && tl->buf[tl->pos - 1] != ' ')
			line_backspace(tl);
		break;
	default:
		if (c >= 0x20 && c <= 0x7e) {
			if (tl->buf_len < TL_MAX_LINE_LEN - 1)
				add_char(tl, c);
			tl->hist_step = -1;
		}
		break;
	}

	return ret;
}

