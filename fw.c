#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include "walker.h"

static const char indent[] =
        "                                                                                "
        "                                                                                "
        "                                                                                "
        "                                                                                ";

#define Indent(i) (indent + ((sizeof(indent) - 1)-(i)*4))

int main(int argc, char** argv)
{
	if (argc != 2) {
		errorfln("error: arguments\n");
		exit(1);
	}

	const char* start = argv[1];
	if (!start) {
		errorfln("error: '%s': %s\n", argv[1], strerror(errno));
		exit(1);
	}

	Walker w = {0};
	DirEntry entry = {0};

	WalkerInit(&w, start);
	while (WalkerNext(&w, &entry)) {
		printf("%s%.*s\n", Indent(entry.depth), PathBufS(&entry.path));
	}
	assert(IsEmpty(&w.stack));

	return 0;
}
