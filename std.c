#include "chibicc.h"

#define FILE_NUM 10
FILE _files[FILE_NUM] = {
    {/*is_open=*/true, /*is_screen=*/true},
    {/*is_open=*/true, /*is_screen=*/true},
    {/*is_open=*/true, /*is_screen=*/true},
};

static char get_lfn(FILE *f) { return f - _files + 1; }

__attribute__((constructor)) static void init() {
  cbm_k_setnam("");

  cbm_k_setlfs(get_lfn(stdin), 0, 2);
  cbm_k_open();
  cbm_k_chkin(get_lfn(stdin));

  cbm_k_setlfs(get_lfn(stdout), 3, 2);
  cbm_k_open();
  cbm_k_chkout(get_lfn(stdout));

  cbm_k_setlfs(get_lfn(stderr), 3, 2);
  cbm_k_open();
}

__attribute__((destructor)) static void fini() {
  cbm_k_close(get_lfn(stdin));
  cbm_k_close(get_lfn(stdout));
  cbm_k_close(get_lfn(stderr));
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

static FILE *get_free_file() {
  for (int i = 0; i < FILE_NUM; i++)
    if (!_files[i].is_open)
      return &_files[i];
  return NULL;
}

static void open_file(const char *restrict pathname, const char *restrict mode,
                      FILE *f) {
  f->is_open = true;

  size_t len = strlen(pathname);
  char cbm_name[len + 7];

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
  cbm_k_setlfs(get_lfn(f), 8, 2);
  cbm_k_open();
}

FILE *fopen(const char *restrict pathname, const char *restrict mode) {
  FILE *stream = get_free_file();
  open_file(pathname, mode, stream);
  return stream;
}

int fclose(FILE *restrict stream) {
  cbm_k_close(get_lfn(stream));
  memset(stream, 0, sizeof(FILE));
  return 0;
}

FILE *freopen(const char *restrict pathname, const char *restrict mode,
              FILE *restrict stream) {
  fclose(stream);
  open_file(pathname, mode, stream);
  // stdin and stdout must be checked out again once reopened
  if (stream == stdin)
    cbm_k_chkin(get_lfn(stream));
  else if (stream == stdout)
    cbm_k_chkout(get_lfn(stream));
  return stream;
}

int feof(FILE *stream) { return stream->is_eof; }

int fprintf(FILE *restrict stream, const char *restrict format, ...) {
  va_list va;
  va_start(va, format);
  const int ret = vfprintf(stream, format, va);
  va_end(va);
  return ret;
}

int vfprintf(FILE *restrict stream, const char *restrict format,
             va_list vlist) {
  cbm_k_chkout(get_lfn(stream));
  const int ret = vprintf(format, vlist);
  cbm_k_chkout(get_lfn(stdout));
  return ret;
}

static char get_char(FILE *stream) {
  char c = cbm_k_chrin();
  stream->is_eof = cbm_k_readst() & 0x40;
  return c;
}

size_t fread(void *restrict ptr, size_t size, size_t nitems,
             FILE *restrict stream) {
  if (feof(stream))
    return 0;
  cbm_k_chkin(get_lfn(stream));
  size_t nread = 0;
  char *cur = ptr;
  while (nitems--) {
    size_t s = size;
    while (s--) {
      if (feof(stream))
        goto done;
      *cur++ = get_char(stream);
    }
    ++nread;
  }
done:
  cbm_k_chkin(get_lfn(stdin));
  return nread;
}

int fgetc(FILE *stream) {
  if (feof(stream))
    return EOF;
  cbm_k_chkin(get_lfn(stream));
  char c = get_char(stream);
  cbm_k_chkin(get_lfn(stdin));
  return c;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nitems,
              FILE *restrict stream) {
  cbm_k_chkout(get_lfn(stream));
  const char *c = (const char *)ptr;
  while (nitems--) {
    size_t s = size;
    while (s--)
      putchar(*c++);
  }
  cbm_k_chkout(get_lfn(stdout));
  return nitems;
}

void __putchar(char c) {
  if (stdout->is_screen && c == '\n')
    cbm_k_chrout('\r');
  else
    cbm_k_chrout(c);
}
