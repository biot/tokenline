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

enum {
	T_SHOW = 1,
	T_HARDWARE,
	T_VERSION,
	T_DEVICE,
	T_DIRECTORY,
	T_HELP,
	T_CPU,
	T_MEMORY,
};
t_token_dict dict[] = {
	{ /* Dummy entry */ },
	{ T_SHOW, "show" },
	{ T_HARDWARE, "hardware" },
	{ T_VERSION, "version" },
	{ T_DEVICE, "device" },
	{ T_DIRECTORY, "directory" },
	{ T_HELP, "help" },
	{ T_CPU, "cpu" },
	{ T_MEMORY, "memory" },
	{ }
};
t_token tokens_hardware[] = {
	{ T_CPU, 0, NULL, "CPU" },
	{ T_MEMORY, 0, NULL, "Memory" },
	{ }
};
t_token tokens_show[] = {
	{ T_HARDWARE, 0, tokens_hardware, "Hardware information" },
	{ T_VERSION, 0, NULL, "Version" },
	{ T_DEVICE, TARG_INT, NULL, "Device" },
	{ T_DIRECTORY, TARG_STRING, NULL, "Directory" },
	{ }
};
t_token tokens[] = {
	{ T_SHOW, 0, tokens_show, "Show information" },
	{ T_HELP, TARG_HELP, NULL, "Available commands" },
	{ }
};

