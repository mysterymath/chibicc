#ifndef _ASSERT_H
#define _ASSERT_H

#ifdef NDEBUG
#  define assert(condition) ((void)0)
#else
#  define assert(condition) __assert(condition, #condition)
#endif

void __assert(char condition, const char *str);

#endif // not _ASSERT_H
