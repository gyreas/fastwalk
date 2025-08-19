CFLAGS := "-ggdb -Wall -Wextra -Wpedantic"

fw:
	cc {{CFLAGS}} -o fw fw.c
