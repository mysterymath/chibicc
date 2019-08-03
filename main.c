#include <ctype.h>

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char bank;
  void *ptr;
} VoidBPtr;

VoidBPtr bcalloc(size_t count, size_t size) {
  VoidBPtr ret = {1, calloc(count, size)};
  return ret;
}

typedef struct {
  char bank;
  char *ptr;
} CharBPtr;

typedef enum {
  TK_PUNCT, // Punctuators
  TK_NUM,   // Numeric literals
  TK_EOF,   // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
typedef struct {
  char bank;
  Token *ptr;
} TokenBPtr;
struct Token {
  TokenKind kind; // Token kind
  TokenBPtr next; // Next token
  int val;        // If kind is TK_NUM, its value
  CharBPtr loc;      // Token location
  unsigned len;        // Token length
};

void set_ram_bank(char bank) {}

#define G(BPtr) (set_ram_bank(BPtr.bank), BPtr.ptr)

// Input string
static CharBPtr current_input;

// Reports an error and exit.
static void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
static void verror_at(CharBPtr loc, char *fmt, va_list ap) {
  assert(loc.bank == current_input.bank && "Error location out of range");
  int pos = loc.ptr - current_input.ptr;
  fprintf(stderr, "%s\n", G(current_input));
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static void error_at(CharBPtr loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

static void error_tok(TokenBPtr tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(G(tok)->loc, fmt, ap);
}

// Consumes the current token if it matches `s`.
static bool equal(TokenBPtr tok, char *op) {
  return memcmp(G(G(tok)->loc), op, G(tok)->len) == 0 &&
         op[G(tok)->len] == '\0';
}

// Ensure that the current token is `s`.
static TokenBPtr skip(TokenBPtr tok, char *s) {
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

// Tokenize `current_input` and returns new tokens.
static TokenBPtr tokenize(void) {
  CharBPtr p = current_input;
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

    // Punctuator
    if (*G(p) == '+' || *G(p) == '-') {
      cur = G(cur)->next = new_token(TK_PUNCT, p, p.ptr + 1);
      p.ptr++;
      continue;
    }

    error_at(p, "invalid token");
  }

  cur = G(cur)->next = new_token(TK_EOF, p, p.ptr);
  return G(head)->next;
}

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  VoidBPtr vtext = bcalloc(1, strlen(argv[1])+1);
  current_input.bank = vtext.bank;
  current_input.ptr = vtext.ptr;
  strcpy(G(current_input), argv[1]);

  TokenBPtr tok = tokenize();

  printf("  .globl main\n");
  printf("main:\n");

  // The first token must be a number
  printf("  lda #%d\n", get_number(tok));
  tok = G(tok)->next;

  // ... followed by either `+ <number>` or `- <number>`.
  while (G(tok)->kind != TK_EOF) {
    if (equal(tok, "+")) {
      printf("  clc\n");
      printf("  adc #%d\n", get_number(G(tok)->next));
      tok = G(G(tok)->next)->next;
      continue;
    }

    tok = skip(tok, "-");
    printf("  sec\n");
    printf("  sub #%d\n", get_number(tok));
    tok = G(tok)->next;
  }

  printf("  rts\n");
  return 0;
}
