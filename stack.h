#ifndef FW_STACK_H
#define FW_STACK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

INLINE func
bool IsEmpty(DirStack *s)
{
	return s->top == 0;
}

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

	memcpy(s->elems + s->top, list, sizeof(Dirp));
	s->top += 1;
}

INLINE
func Dirp* PopTop(DirStack *s)
{
	return s->top == 0 ? NULL : &s->elems[--s->top];
}

INLINE
func Dirp* Last(DirStack *s)
{
	return s->top == 0 ? NULL : &s->elems[s->top - 1];
}

INLINE
func size_t Length(DirStack *s)
{
	return s->top;
}

#endif /* FW_STACK_H */
