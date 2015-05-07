##
## Makefile for mallhook
## mallhook is a library that wraps all malloc family functions
##

NAME = mallhook.so
SRC = mallhook.c

all: $(NAME)

$(NAME):
	gcc $(SRC) -shared -fPIC -o $(NAME) -W -Wall -Wextra -ansi -O3 -pipe -ldl

clean:
	rm -f $(NAME)

re:	clean all
