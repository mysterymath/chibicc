#ifndef _STD_H
#define _STD_H

#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define assert(condition) __assert(condition, #condition)
#endif

void __assert(char condition, const char *str);

struct _FILE;
typedef struct _FILE FILE;
extern struct _FILE _stdin;
extern struct _FILE _stdout;
extern struct _FILE _stderr;
#define stdin (&_stdin)
#define stdout (&_stdout)
#define stderr (&_stderr)

FILE *freopen(const char *restrict pathname, const char *restrict mode,
              FILE *restrict stream);

int fprintf(FILE *restrict stream, const char *restrict format, ...);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list vlist);

char *strndup(const char *str, size_t size);
unsigned long strtoul(const char *restrict s, char **restrict p, int base);

typedef struct Type Type;
typedef struct Node Node;

#endif // not _STD_H
