#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

__attribute__((noreturn)) extern void die(const char *fmt, ...) {
    fprintf(stderr, "fatal: ");

    va_list ap;
    va_start(ap, fmt);

    vfprintf(stderr, fmt, ap);

    va_end(ap);

    fprintf(stderr, ".\n");

    exit(1);
}

extern void *xmalloc(size_t n) {
    return malloc(n) ?: (die("out of memory"), NULL);
}

extern char *concat(const char *s0, ...) {
    size_t needed = 0;

    va_list ap;

    va_start(ap, s0);
    for(const char *s = s0; s; s = va_arg(ap, const char *))
        needed += strlen(s);
    va_end(ap);

    char *ret = xmalloc(needed + 1);
    ret[0] = '\0';

    va_start(ap, s0);
    for(const char *s = s0; s; s = va_arg(ap, const char *))
        strcat(ret, s);
    va_end(ap);

    return ret;
}

extern char *slurp(const char *pathname) {
    FILE *stream = fopen(pathname, "r");

    if(!stream)
        die("could not open %s: %s", pathname, strerror(errno));

    struct stat st;
    if(fstat(fileno(stream), &st) != 0)
        die("could not read stats for %s: %s", pathname, strerror(errno));

    size_t len = st.st_size;
    char *buf = xmalloc(len + 1);

    size_t nread = fread(buf, 1, len, stream);
    if(nread != len) {
        if(ferror(stream)) {
            die("could not read file %s: %s", pathname, strerror(errno));
        } else {
            die("could not read file %s: expected %lu bytes, got %lu",
                    pathname, len, nread);
        }
    }

    buf[len] = '\0';

    fclose(stream);

    return buf;
}
