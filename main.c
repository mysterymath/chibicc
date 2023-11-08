#include <ctype.h>

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

typedef struct Chunk Chunk;
typedef struct {
  char bank;
  Chunk *ptr;
} ChunkBPtr;
struct Chunk {
  ChunkBPtr next;
  char text[];
};

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

// Reports an error and exit.
static void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Consumes the current token if it matches `s`.
static bool equal(TokenBPtr tok, char *op) {
  return memcmp(G(G(tok)->loc), op, G(tok)->len) == 0 &&
         op[G(tok)->len] == '\0';
}

// Ensure that the current token is `s`.
static TokenBPtr skip(TokenBPtr tok, char *s) {
  if (!equal(tok, s))
    error("expected '%s'", s);
  return G(tok)->next;
}

// Ensure that the current token is TK_NUM.
static int get_number(TokenBPtr tok) {
  if (G(tok)->kind != TK_NUM)
    error("expected a number");
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

// Tokenize `p` and returns new tokens.
static TokenBPtr tokenize(ChunkBPtr c) {
  VoidBPtr vhead = bcalloc(1, sizeof(Token));
  TokenBPtr head = {vhead.bank, vhead.ptr};
  TokenBPtr cur = head;
  CharBPtr p;

  while (c.bank) {
    p.bank = c.bank;
    p.ptr = c.ptr->text;
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

      error("invalid token");
    }
    c = G(c)->next;
  }

  cur = G(cur)->next = new_token(TK_EOF, p, p.ptr);
  return G(head)->next;
}

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  VoidBPtr vchunk = bcalloc(1, strlen(argv[1])+1);
  ChunkBPtr chunk = {vchunk.bank, vchunk.ptr};
  strcpy(G(chunk)->text, argv[1]);

  TokenBPtr tok = tokenize(chunk);

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
