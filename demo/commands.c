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

#include "tokenline.h"
#include "commands.h"

t_token_dict dict[] = {
	{ /* Dummy entry */ },
	{ T_SHOW, "show" },
	{ T_SET, "set" },
	{ T_HARDWARE, "hardware" },
	{ T_VERSION, "version" },
	{ T_DEVICE, "device" },
	{ T_DIRECTORY, "directory" },
	{ T_HELP, "help" },
	{ T_CPU, "cpu" },
	{ T_MEMORY, "memory" },
	{ T_FREQUENCY, "frequency" },
	{ T_NUMBER, "number" },
	{ T_MKDIR, "mkdir" },
	{ T_LS, "ls" },
	{ T_TAP, "tap" },
	{ T_CALC, "calc" },
	{ T_PLUS, "+" },
	{ T_EXIT, "exit" },
	{ }
};

t_token tokens_mode_calc[] = {
	{ T_ARG_INT,
		.help = "Operand" },
	{ T_PLUS },
	{ T_EXIT,
		.help = "Exit calc mode" },
};

t_token tokens_mode_device[] = {
	{ T_SHOW,
		.help = "Show device information" },
	{ T_MKDIR,
		.help = "Create directory" },
	{ T_LS,
		.help = "List files and directories" },
	{ T_EXIT,
		.help = "Exit device mode" },
	{ }
};

t_token tokens_set[] = {
	{ T_FREQUENCY,
		.arg_type = T_ARG_FREQ,
		.help = "Frequency" },
	{ T_NUMBER,
		.arg_type = T_ARG_INT,
		.help = "Number" },
	{ }
};

t_token tokens_hardware[] = {
	{ T_CPU,
		.help = "CPU" },
	{ T_MEMORY,
		.help = "Memory" },
	{ }
};
t_token tokens_show[] = {
	{ T_HARDWARE,
		.subtokens = tokens_hardware,
		.help = "Hardware information" },
	{ T_VERSION,
		.help = "Version" },
	{ T_DEVICE,
		.arg_type = T_ARG_INT,
		.help = "Device" },
	{ T_DIRECTORY,
		.arg_type = T_ARG_STRING,
		.help = "Directory" },
	{ }
};
t_token tokens[] = {
	{ T_SHOW,
		.subtokens = tokens_show,
		.help = "Show information" },
	{ T_SET,
		.subtokens = tokens_set,
		.help = "Set things" },
	{ T_DEVICE,
		.help = "Device mode" },
	{ T_TAP,
		.flags = T_FLAG_SUFFIX_TOKEN_DELIM_INT,
		.help = "Tap" },
	{ T_CALC,
		.subtokens = tokens_mode_calc,
		.help = "Calculator" },
	{ T_HELP,
		.arg_type = T_ARG_HELP,
		.help = "Available commands" },
	{ }
};

