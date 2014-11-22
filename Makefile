##
## Copyright (C) 2014 Bert Vermeulen <bert@biot.com>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

FLAGS = -g -O0 -Wall -Wextra -Wno-missing-field-initializers

TOKENLINE_SRC = tokenline.c 
TOKENLINE_DEP = $(TOKENLINE_SRC) tokenline.h 

DEMO_SRC = demo/commands.c demo/demo.c
DEMO_DEP = $(DEMO_SRC)

tokenline-demo: $(TOKENLINE_DEP) $(DEMO_DEP)
	$(CC) $(FLAGS) -I. -Idemo $(TOKENLINE_SRC) $(DEMO_SRC) -o tokenline-demo

clean:
	rm -f tokenline-demo
