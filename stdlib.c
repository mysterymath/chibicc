#include "chibicc.h"

char *strndup(const char *str, size_t size) {
  char *dup = malloc(size + 1);
  char *cur = dup;
  while (size--)
    if (!(*cur++ = *str++))
      return dup;
  *cur = '\0';
  return dup;
}

unsigned long strtoul(const char *restrict s, char **restrict p, int base) {
  assert(base == 10 && "Other bases not yet implemented.");
  while (s && isspace(*s))
    ++s;
  if (!s) {
    if (p)
      *p = s;
    return 0;
  }

  // TODO: Error checking; errno, etc.
  char sign = 0;
  if (*s == '+') {
    ++s;
  } else if (*s == '-') {
    sign = 1;
    ++s;
  }
  unsigned long val = 0;
  while (s && isdigit(*s)) {
    val = val * 10 + (*s - '0');
    ++s;
  }
  if (p)
    *p = s;
  return val;
}
