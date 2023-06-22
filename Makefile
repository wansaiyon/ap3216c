PHONY := all clean
all:
	@gcc -o ap3216c ap3216c.c

clean:
	@rm -rf ap3216c