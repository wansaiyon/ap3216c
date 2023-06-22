CC :=gcc
PHONY := all clean
all:
	@$(CC) -o ap3216c ap3216c.c

clean:
	@rm -rf ap3216c
