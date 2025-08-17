#ifndef FW_UTIL_H
#define FW_UTIL_H

#define attr(a) __attribute__((a))
#define INLINE attr(always_inline) inline
#define streq(s0, s1) (strcmp((s0), (s1)) == 0)
#define func

func void errorfln(const char* fmt, ...) attr(format(printf, 1, 2));
func void errorfln(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");
}

#endif /* FW_UTIL_H */
