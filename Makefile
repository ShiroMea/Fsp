CC = gcc
CP = cp
RM = rm
MKDIR = mkdir

CFLAGS = -Wall -fPIE -pie -DFSP_VERSION='"$(VERSION)"'
CPFLAGS =
RMFLAGS = -r
MKDIRFLAGS = -p

VERSION = 1.0.8(Beta)

prefix = /usr/local
BIN = $(prefix)/bin

NAME = fsp
OBJ = main.o base.o data.o
LIB = -lz -lshiro

$(NAME):main.o base.o data.o
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LIB)

main.o:base.h main.c
base.o:base.h base.c
data.o:base.h data.c

.PHONY: install uninstall clean

install:
	$(CP) $(CPFLAGS) $(NAME) $(BIN)/$(NAME)
uninstall:
	$(RM) $(RMFLAGS) $(BIN)/$(NAME)
clean:
	-$(RM) $(RMFLAGS) $(NAME) $(OBJ)
