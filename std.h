#ifndef _STD_H
#define _STD_H

#include <stdarg.h>
#include <stdbool.h>

#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define assert(condition) __assert(condition, #condition)
#endif

void __assert(char condition, const char *str);

#define EOF (-1)

typedef struct {
  bool is_open;
  bool is_screen;
  bool is_eof;
} FILE;
extern FILE _files[];
#define stdin (&_files[0])
#define stdout (&_files[1])
#define stderr (&_files[2])

FILE *fopen(const char *restrict pathname, const char *restrict mode);
int fclose(FILE *restrict stream);
FILE *freopen(const char *restrict pathname, const char *restrict mode,
              FILE *restrict stream);

int fgetc(FILE *stream);

int feof(FILE *stream);

int fprintf(FILE *restrict stream, const char *restrict format, ...);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list vlist);

size_t fread(void *restrict ptr, size_t size, size_t nitems,
              FILE *restrict stream);
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems,
              FILE *restrict stream);

char *strndup(const char *str, size_t size);
unsigned long strtoul(const char *restrict s, char **restrict p, int base);

typedef struct Type Type;
typedef struct Node Node;

#endif // not _STD_H
