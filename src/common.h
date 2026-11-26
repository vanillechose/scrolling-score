#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define LEN(a) (sizeof (a) / sizeof (a)[0])

__attribute__((noreturn)) extern void die(const char *fmt, ...);

extern void *xmalloc(size_t n);

extern char *concat(const char *s0, ...);

extern char *slurp(const char *pathname);

/*
 * Voice loading
 */
struct note {
    float note;
    float start;
    float end;
};

extern size_t parse_voice(struct note **dst, const char *text,
                            double tick_per_bar);

#endif /* COMMON_H */
