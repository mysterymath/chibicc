#include "as-tokenize.h"

#include <ctype.h>

#include "std.h"

static char *current_stmt;

// Reports an error location and exit.
static void verror_at(char *loc, char *fmt, va_list ap) {
  int pos = loc - current_stmt;
  fprintf(stderr, "%s\n", current_stmt);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

static Token *new_token(TokenKind kind, char *start, char *end) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = start;
  tok->len = end - start;
  return tok;
}

// Returns true if c is valid as the first character of a symbol.
static bool is_symbol1(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' ||
         c == '.' || c == '$';
}

// Returns true if c is valid as a non-first character of a symbol.
static bool is_symbol2(char c) {
  return is_symbol1(c) || ('0' <= c && c <= '9');
}

// Read a punctuator token from p and returns its length.
static int read_punct(char *p) {
  return ispunct(*p) ? 1 : 0;
}

Token *tokenize(char *p) {
  current_stmt = p;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    // Skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }

    // Numeric literal
    if (isdigit(*p)) {
      cur = cur->next = new_token(TK_NUM, p, p);
      char *q = p;
      cur->val = strtoul(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    // Symbol
    if (is_symbol1(*p)) {
      char *start = p;
      do {
        p++;
      } while (is_symbol2(*p));
      cur = cur->next = new_token(TK_SYMBOL, start, p);
      continue;
    }

    // Punctuators
    int punct_len = read_punct(p);
    if (punct_len) {
      cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
      p += cur->len;
      continue;
    }

    error_at(p, "invalid token");
  }
}
