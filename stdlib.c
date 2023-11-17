#include "chibicc.h"

void __assert(char condition, const char *str) {
  if (condition)
    return;
  printf("assertion failed: %s\n", str);
  abort();
}

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

int fprintf(FILE *restrict stream, const char *restrict format, ...) {
  va_list va;
  va_start(va, format);
  const int ret = vfprintf(stream, format, va);
  va_end(va);
  return ret;
}

static char lfn = 2;

int vfprintf(FILE *restrict stream, const char *restrict format, va_list vlist) {
  char old_lfn = lfn;
  // Assume stderr
  cbm_k_chkout((lfn = 1));
  const int ret = vprintf(format, vlist);
  cbm_k_chkout(old_lfn);
  return ret;
}

void __putchar(char c) {
  if (lfn == 1 && c == '\n')
    cbm_k_chrout('\r');
  else
    cbm_k_chrout(c);
}
