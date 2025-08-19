#ifndef FW_STACK_H
#define FW_STACK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/path.h"
#include "util.h"

typedef struct Dirp {
	DIR* dirp;
	size_t depth;
	PathBuffer path;
} Dirp;

typedef struct DirStack {
	Dirp* elems;
	size_t top; // this is the length as well
	size_t capacity;
} DirStack;

INLINE
func bool IsEmpty(DirStack *s)
{
	return s->top == 0;
}

INLINE
func void PushTop(DirStack *s, Dirp *list)
{
	if (!s || !list) {
		return;
	}

	if (s->top + 1 >= s->capacity) {
		if (s->capacity == 0) {
			s->capacity = 64;
		} else {
			s->capacity *= 2;
		}

		s->elems = realloc(s->elems, sizeof(Dirp) * s->capacity);

		if (!s->elems) {
			abort();
		}
	}
	s->elems[s->top++] = *list;
	// PathBuffer_Copy(&top->path, &list->path);
}

INLINE
func Dirp* PopTop(DirStack *s)
{
	return s->top == 0 ? NULL : &s->elems[--s->top];
}

INLINE
func Dirp* Last(DirStack *s)
{
	Dirp* last = s->top == 0 ? NULL : &s->elems[s->top - 1];
	assert(last->path.end > 0);
	return last;
}

INLINE
func size_t Length(DirStack *s)
{
	return s->top;
}

#endif /* FW_STACK_H */
