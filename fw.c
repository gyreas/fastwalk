#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "walker.h"

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "error: arguments\n");
		exit(1);
	}

	const char* root = argv[1];
	Walker w = {0};
	const char* start = realpath(root, NULL);
	if (!start) {
		fprintf(stderr, "error: '%s': %s\n", root, strerror(errno));
		exit(1);
	}
	WalkerInit(&w, start);

	DirEntry entry = {0};
	while (WalkerNext(&w, &entry)) {
		DirEntryPrint(&entry);
	}

	printf("w.dir = '%.*s'\n", PathBufS(&w.root));

	return 0;
}
