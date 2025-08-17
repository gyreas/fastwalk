#ifndef FW_WALKER_H
#define FW_WALKER_H

#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "include/path.h"
#include "stack.h"

enum {
	T_DIRECTORY = DT_DIR,
	T_FILE = DT_REG,
	T_UNKNOWN = DT_UNKNOWN,
	T_DONE = -1,
};

typedef struct DirEntry {
	PathBuffer path;
	char type;
	size_t depth;
	uint64_t ino;
} DirEntry;

func DirEntry DirEntry_FromDirent(size_t depth, struct dirent *dent)
{
	DirEntry de = {0};

	de.depth = depth;
	de.ino = dent->d_ino;
	de.type = dent->d_type;
	PathBuffer_Set(&de.path, String_FromC(dent->d_name));

	return de;
}

func DirEntry DirEntry_FromPath(size_t depth, const char* path)
{
	static struct stat statbuf = {0};
	if (stat(path, &statbuf) == -1) {
		abort();
	};

	DirEntry de = {0};
	de.depth = depth;
	de.ino = statbuf.st_ino;
	PathBuffer_Set(&de.path, String_FromC(path));

	switch (statbuf.st_mode & S_IFMT) {
	case S_IFREG:
		de.type = T_FILE;
		break;
	case S_IFDIR:
		de.type = T_DIRECTORY;
		break;
	default:
		de.type = T_UNKNOWN;
		break;
	}

	return de;
}

static INLINE
func bool IsDirectory(DirEntry *de)
{
	return de->type == T_DIRECTORY;
}

func void DirEntryPrint(DirEntry *de)
{
	printf("entry:'%.*s'\n", PathBufS(&de->path));
}

typedef struct WalkOptions {
	bool followLinks;
} WalkOptions;

typedef struct Walker {
	WalkOptions opts;
	PathBuffer root;

	// Iterator
	PathBuffer* start; // start=&root
	DirStack stack;
	size_t depth;
} Walker;

func void WalkerInit(Walker *walker, const char* root)
{
	PathBuffer_Set(&walker->root, String_FromC(root));
	walker->start = &walker->root;
}

static INLINE
func void Pop(Walker *walker)
{
	Dirp *popped = PopTop(&walker->stack);
	if (!popped) {
		puts("empty stack");
		abort();
	};
	closedir(popped->dirp);
}

static INLINE
func void Push(Walker *walker, DirEntry *dent)
{
	DIR *dirp = opendir((char*)PathBuffer_ToCString(&dent->path));
	if (!dirp) {
		errorfln("opendir: '%.*s': %s", PathBufS(&dent->path), strerror(errno));
		abort();
	}

	Dirp list;
	list.depth = walker->depth;
	list.dirp = dirp;
	memcpy(&list.path, &dent->path, sizeof(PathBuffer));
	PushTop(&walker->stack, &list);
}

static INLINE
func bool HandleEntry(Walker *walker, DirEntry *dent)
{
	if (IsDirectory(dent)) {
		// printf(">>> ");
		// DirEntryPrint(dent);
		Push(walker, dent);
	}
	return true;
}

func bool WalkerNext(Walker *walker, DirEntry *result)
{
	static PathBuffer path = {0};

	if (walker->start != NULL) {
		memcpy(&path, walker->start, sizeof(path));

		DirEntry dent =
		        DirEntry_FromPath(0, (char*)PathBuffer_ToCString(walker->start));
		walker->start = NULL;
		if (!HandleEntry(walker, &dent)) {
			abort();
		};
	}

	assert(path.end > 0 && "path is not set");

	while (!IsEmpty(&walker->stack)) {
		walker->depth = Length(&walker->stack);
		if (walker->depth > 64) {
			puts("max depth exceeded");
			abort();
		}

		Dirp *last = Last(&walker->stack);
		errno = 0;
		struct dirent *next = readdir(last->dirp);
		if (!next && errno) {
			printf("error: '%.*s': %s\n", PathBufS(&last->path), strerror(errno));
			abort();
		}
		if (next) {
			if (streq(next->d_name, ".") || streq(next->d_name, "..")) {
				continue;
			}

			DirEntry dent = DirEntry_FromDirent(last->depth + 1, next);
			PathBuffer_Push(&path, PathBuffer_ToString(&dent.path));
			memcpy(&dent.path, &path, sizeof(path));

			// HandleEntry(walker, &dent);
			if (IsDirectory(&dent)) {
				Push(walker, &dent);
			} else {
				PathBuffer_Pop(&path);
			}

			*result = dent;
			return true;
		} else {
			Pop(walker);
			PathBuffer_Pop(&path);
		}
	}

	result = NULL;
	return false;
}

#endif /* FW_WALKER_H */
