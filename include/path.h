#ifndef CSTDLIB_PATH_H
#define CSTDLIB_PATH_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define attr(a) __attribute__((a))
#define INLINE attr(always_inline) inline

typedef uint8_t u8;
typedef size_t uZ;

typedef struct slice {
	void* data;
	uZ len;
} Slice;

typedef struct str {
	u8* chars;
	uZ len;
} String;

#define RANGE_END ((uZ)-1)
#define SEP ('/')
#define ETOO_LONG -1

static const Slice DOT = {".", 1};
static const Slice DOTDOT = {"..", 2};
static const String SEPSTR = {(u8*)"/", 1};

INLINE Slice Sliced(Slice s, uZ start, uZ end)
{
	Slice sn;
	sn.data = (void*)((uZ)s.data + start);
	sn.len = end == RANGE_END ?
	         end = s.len - start :
	               end - start;
	return sn;
}

INLINE Slice Sliceds(void* data, uZ start, uZ end)
{
	return (Slice) {
		(void*)((uZ)data + start), end - start,
	};
}

#define StringS(s) (int)(s).len, (s).chars

INLINE String String_FromC(const char* cstr)
{
	return (String) {
		(u8*)cstr,
		strlen(cstr)
	};
}

INLINE String String_FromSlice(Slice s)
{
	return (String) {
		(u8*)(s).data, (s).len
	};
}

INLINE bool String_Equal(String s0, String s1)
{
	assert(s0.chars && s1.chars);
	if (s0.len != s1.len) return false;
	return memcmp(s0.chars, s1.chars, s0.len) == 0;
}

INLINE bool Bytes_Equal(Slice a, Slice b)
{
	if (a.len == b.len) {
		return memcmp(a.data, b.data, a.len) == 0;
	}
	return false;
}

INLINE int Bytes_IndexOf(Slice bytes, u8 byte)
{
	// TODO: use memchr
	u8* chars = (u8*)bytes.data;
	for (int i = 0; i < (int)bytes.len; i++) {
		if (chars[i] == byte) return i;
	}
	return -1;
}

INLINE int Bytes_RindexOf(Slice bytes, u8 byte)
{
	// TODO: use memchr
	u8* chars = (u8*)bytes.data;
	for (int i = bytes.len - 1; i >= 0; i--) {
		if (chars[i] == byte) return i;
	}
	return -1;
}

INLINE Slice Bytes_Ltrim(Slice bytes, u8 trim)
{
	uZ i = 0;
	u8* data = (u8*)bytes.data;
	for (; i < bytes.len && data[i] == trim; i++) {}
	return Sliced(bytes, i, RANGE_END);
}

INLINE Slice Bytes_Rtrim(Slice bytes, u8 trim)
{
	uZ i = bytes.len - 1;
	u8* data = (u8*)bytes.data;
	for (; i < bytes.len && data[i] == trim; i--) {}
	return Sliced(bytes, 0, i + 1);
}

typedef struct pathbuffer {
	u8 buf[PATH_MAX];
	uZ end;
} PathBuffer;

#define PathBufS(pathbuf) (int)(pathbuf)->end, (pathbuf)->buf
#define PathBuffer_ToCString(pb) ((pb)->buf[(pb)->end] = 0, (pb)->buf)

INLINE void PathBuffer_Copy(PathBuffer* into, PathBuffer* from)
{
	assert(from->end > 0);
	if (!into || !from) {
		return;
	}
	memcpy((void*)into->buf, (void*)from->buf, from->end);
	into->end = from->end;
}

INLINE bool PathBuffer_IsRoot(PathBuffer* buf)
{
	return buf->end == 1 && buf->buf[0] == SEP;
}

String PathBuffer_ToString(PathBuffer *buf)
{
	String r;
	if (buf->end == 0) {
		r.chars = (u8*)".";
		r.len = 1;
	} else {
		r.chars = buf->buf;
		r.len = buf->end;
	}
	return r;
}

static int AppendLit(PathBuffer* buf, Slice bs)
{
	uZ newend = buf->end;
	if (buf->end == 0 || PathBuffer_IsRoot(buf)) {
		if (PATH_MAX < buf->end + bs.len) return ETOO_LONG;
	} else {
		if (PATH_MAX < buf->end + bs.len + 1) return ETOO_LONG;
		buf->buf[buf->end] = SEP;
		newend += 1;
	}
	memcpy((void* )(buf->buf + newend), bs.data, bs.len);
	return newend + bs.len;
}

static int AppendNorm(PathBuffer* buf, Slice seg)
{
	if (seg.len == 0 || Bytes_Equal(DOT, seg)) return buf->end;
	if (Bytes_Equal(DOTDOT, seg)) {
		if (PathBuffer_IsRoot(buf)) return buf->end;
		int isep = Bytes_RindexOf(Sliceds(buf->buf, 0, buf->end), SEP);
		isep = isep == -1 ? 0 : isep + 1;
		if (buf->end == 0 || Bytes_Equal(Sliceds(buf->buf, isep, buf->end), DOTDOT)) {
			return AppendLit(buf, DOTDOT);
		} else {
			return isep <= 1 ? isep : isep - 1;
		}
	} else {
		return AppendLit(buf, seg);
	}
}

static uZ Split(PathBuffer* buf, String* s)
{
	if (buf->end == 0 || PathBuffer_IsRoot(buf)) {
		s->chars = NULL;
		s->len = 0;
		return buf->end;
	}
	int rindex = Bytes_RindexOf(Sliceds(buf->buf, 0, buf->end), SEP);
	if (rindex == -1) {
		s->chars = buf->buf;
		s->len = buf->end;
		return 0;
	} else {
		*s = String_FromSlice(Sliceds(buf->buf, rindex + 1, buf->end));
		assert(s->len == buf->end - (rindex + 1));
		return rindex == 0 ? 1 : rindex;
	}
}

String PathBuffer_Pop(PathBuffer* buf)
{
	String s;
	uZ end = Split(buf, &s);
	buf->end = end;
	return s;
}

// `items` must end with a NULL
bool PathBuffer_Push(PathBuffer* buf, String path)
{
	int r;
	Slice p = (Slice) {
		path.chars, path.len
	};
	for (;;) {
		int index = Bytes_IndexOf(p, SEP);
		if (index == -1) {
			r = AppendNorm(buf, p);
			if (r == -1) return false;
			buf->end = (uZ)r;
			break;
		} else {
			if (index == 0 && buf->end == 0) {
				buf->buf[0] = SEP;
				buf->end = 1;
			} else {
				r = AppendNorm(buf, Sliced(p, 0, index));
				if (r == -1) return false;
				buf->end = (uZ)r;
			}
			p = Sliced(p, index + 1, RANGE_END);
		}

	}
	return true;
}

INLINE bool PathBuffer_Set(PathBuffer *buf, String path)
{

	buf->end = 0;
	return PathBuffer_Push(buf, path);
}

bool PathBuffer_Parent(PathBuffer* buf, String* parent)
{
	int newend = AppendNorm(buf, DOTDOT);
	if (newend == -1) abort();

	*parent = newend == 0 ?
	          String_FromC(".") :
	          String_FromSlice(Sliceds(buf->buf, 0, newend));

	return true;

}

String PathBuffer_Basename(PathBuffer* buf)
{
	Slice path = {.data = buf->buf, .len = buf->end};
	if (path.len == 0) return String_FromC(".");

	path = Bytes_Rtrim(path, SEP);
	if (path.len == 0) return SEPSTR;

	int rindex = Bytes_RindexOf(path, SEP);
	assert(rindex > -2);
	if (rindex > -1) path = Sliced(path, (uZ)rindex + 1, RANGE_END);

	return String_FromSlice(path);
}

String PathBuffer_Dirname(PathBuffer* buf)
{
	Slice path = {.data = buf->buf, .len = buf->end};
	if (path.len == 0) return String_FromC(".");

	path = Bytes_Rtrim(path, SEP);
	if (path.len == 0) return SEPSTR;

	int rindex = Bytes_RindexOf(path, SEP);
	if (rindex == -1) return String_FromC(".");
	else path = Sliced(path, 0, (uZ)rindex);

	path = Bytes_Rtrim(path, SEP);
	if (path.len == 0) return SEPSTR;

	return (String) {
		.chars = (u8*)path.data, .len = path.len
	};
}

#endif /* CSTDLIB_PATH_H */
