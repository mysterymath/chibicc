#include "chibicc.h"

// Input string
static CharBPtr current_input;

// Reports an error and exit.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
void verror_at(CharBPtr loc, char *fmt, va_list ap) {
  assert(loc.bank == current_input.bank && "Error location out of range");
  int pos = loc.ptr - current_input.ptr;
  fprintf(stderr, "%s\n", G(current_input));
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(CharBPtr loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

void error_tok(TokenBPtr tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(G(tok)->loc, fmt, ap);
}

// Consumes the current token if it matches `s`.
bool equal(TokenBPtr tok, char *op) {
  return memcmp(G(G(tok)->loc), op, G(tok)->len) == 0 &&
         op[G(tok)->len] == '\0';
}

// Ensure that the current token is `s`.
TokenBPtr skip(TokenBPtr tok, char *s) {
  if (!equal(tok, s))
    error_tok(tok, "expected '%s'", s);
  return G(tok)->next;
}

// Ensure that the current token is TK_NUM.
static int get_number(TokenBPtr tok) {
  if (G(tok)->kind != TK_NUM)
    error_tok(tok, "expected a number");
  return G(tok)->val;
}

// Create a new token.
static TokenBPtr new_token(TokenKind kind, CharBPtr start, char *end) {
  VoidBPtr vtok = bcalloc(1, sizeof(Token));
  TokenBPtr tok = {vtok.bank, vtok.ptr};
  G(tok)->kind = kind;
  G(tok)->loc = start;
  G(tok)->len = end - start.ptr;
  return tok;
}

static bool startswith(char *p, char *q) {
  return strncmp(p, q, strlen(q)) == 0;
}

// Read a punctuator token from p and returns its length.
static int read_punct(char *p) {
  if (startswith(p, "==") || startswith(p, "!=") ||
      startswith(p, "<=") || startswith(p, ">="))
    return 2;

  return ispunct(*p) ? 1 : 0;
}

// Tokenize `current_input` and returns new tokens.
TokenBPtr tokenize(CharBPtr p) {
  current_input = p;
  VoidBPtr vhead = bcalloc(1, sizeof(Token));
  TokenBPtr head = {vhead.bank, vhead.ptr};
  TokenBPtr cur = head;

  while (*G(p)) {
    // Skip whitespace characters.
    if (isspace(*G(p))) {
      ++p.ptr;
      continue;
    }

    // Numeric literal
    if (isdigit(*G(p))) {
      cur = G(cur)->next = new_token(TK_NUM, p, p.ptr);
      char *end;
      G(cur)->val = strtoul(G(p), &end, 10);
      G(cur)->len = end - G(p);
      p.ptr = end;
      continue;
    }

    // Punctuators
    int punct_len = read_punct(G(p));
    if (punct_len) {
      cur = G(cur)->next = new_token(TK_PUNCT, p, p.ptr + punct_len);
      p.ptr += G(cur)->len;
      continue;
    }

    error_at(p, "invalid token");
  }

  cur = G(cur)->next = new_token(TK_EOF, p, p.ptr);
  return G(head)->next;
}

