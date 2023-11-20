#include "chibicc.h"

struct _FILE {
  char lfn;
  bool is_screen;
};

struct _FILE _stdin = {/*lfn=*/0, /*is_screen=*/true};
struct _FILE _stdout = {/*lfn=*/1, /*is_screen=*/true};
struct _FILE _stderr = {/*lfn=*/2, /*is_screen=*/true};

__attribute__((constructor)) static void init() {
  cbm_k_setnam("");

  cbm_k_setlfs(stdin->lfn, 0, 2);
  cbm_k_open();
  cbm_k_chkin(stdin->lfn);

  cbm_k_setlfs(stdout->lfn, 3, 2);
  cbm_k_open();
  cbm_k_chkout(stdout->lfn);

  cbm_k_setlfs(stderr->lfn, 3, 2);
  cbm_k_open();
}

__attribute__((destructor)) static void fini() {
  cbm_k_close(stdin->lfn);
  cbm_k_close(stdout->lfn);
  cbm_k_close(stderr->lfn);
  cbm_k_clrch();
}

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

FILE *freopen(const char *restrict pathname, const char *restrict mode,
              FILE *restrict stream) {
  cbm_k_close(stream->lfn);
  stream->is_screen = false;

  size_t len = strlen(pathname);
  char cbm_name[len+7];
  switch (mode[0]) {
  case 'r':
    strcpy(cbm_name, pathname);
    strcat(cbm_name, ",S,R");
    break;
  case 'w':
    strcpy(cbm_name, "@:");
    strcat(cbm_name, pathname);
    strcat(cbm_name, ",S,W");
    break;
  }
  cbm_k_setnam(cbm_name);
  cbm_k_setlfs(stream->lfn, 8, 2);
  cbm_k_open();
  // stdin and stdout must be checked out again once reopened
  if (stream == stdin)
    cbm_k_chkin(stream->lfn);
  else if (stream == stdout)
    cbm_k_chkout(stream->lfn);

  return stream;
}

int fprintf(FILE *restrict stream, const char *restrict format, ...) {
  va_list va;
  va_start(va, format);
  const int ret = vfprintf(stream, format, va);
  va_end(va);
  return ret;
}

int vfprintf(FILE *restrict stream, const char *restrict format,
             va_list vlist) {
  // Assume stderr
  cbm_k_chkout((char)stream);
  const int ret = vprintf(format, vlist);
  cbm_k_chkout((char)stdout);
  return ret;
}

void __putchar(char c) {
  if (stdout->is_screen && c == '\n')
    cbm_k_chrout('\r');
  else
    cbm_k_chrout(c);
}
