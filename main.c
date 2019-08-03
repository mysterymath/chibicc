#include <assert.h>
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

//
// Tokenizer
//

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

//
// Parser
//

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NEG, // unary -
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_GE,  // >=
  ND_NUM, // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
typedef struct {
  char bank;
  Node *ptr;
} NodeBPtr;
struct Node {
  NodeKind kind; // Node kind
  NodeBPtr lhs;  // Left-hand side
  NodeBPtr rhs;  // Right-hand side
  int val;       // Used if kind == ND_NUM
};

static NodeBPtr new_node(NodeKind kind) {
  VoidBPtr vnode = bcalloc(1, sizeof(Node));
  NodeBPtr node = {vnode.bank, vnode.ptr};
  G(node)->kind = kind;
  return node;
}

static NodeBPtr new_binary(NodeKind kind, NodeBPtr lhs, NodeBPtr rhs) {
  NodeBPtr node = new_node(kind);
  G(node)->lhs = lhs;
  G(node)->rhs = rhs;
  return node;
}

static NodeBPtr new_unary(NodeKind kind, NodeBPtr expr) {
  NodeBPtr node = new_node(kind);
  G(node)->lhs = expr;
  return node;
}

static NodeBPtr new_num(int val) {
  NodeBPtr node = new_node(ND_NUM);
  G(node)->val = val;
  return node;
}

static NodeBPtr expr(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr equality(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr relational(TokenBPtr  *rest, TokenBPtr  tok);
static NodeBPtr add(TokenBPtr  *rest, TokenBPtr  tok);
static NodeBPtr mul(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr unary(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr primary(TokenBPtr *rest, TokenBPtr tok);

// expr = equality
static NodeBPtr expr(TokenBPtr *rest, TokenBPtr tok) {
  return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)*
static NodeBPtr equality(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = relational(&tok, tok);

  for (;;) {
    if (equal(tok, "==")) {
      node = new_binary(ND_EQ, node, relational(&tok, G(tok)->next));
      continue;
    }

    if (equal(tok, "!=")) {
      node = new_binary(ND_NE, node, relational(&tok, G(tok)->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static NodeBPtr relational(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = add(&tok, tok);

  for (;;) {
    if (equal(tok, "<")) {
      node = new_binary(ND_LT, node, add(&tok, G(tok)->next));
      continue;
    }

    if (equal(tok, "<=")) {
      node = new_binary(ND_GE, add(&tok, G(tok)->next), node);
      continue;
    }

    if (equal(tok, ">")) {
      node = new_binary(ND_LT, add(&tok, G(tok)->next), node);
      continue;
    }

    if (equal(tok, ">=")) {
      node = new_binary(ND_GE, node, add(&tok, G(tok)->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static NodeBPtr add(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = mul(&tok, tok);

  for (;;) {
    if (equal(tok, "+")) {
      node = new_binary(ND_ADD, node, mul(&tok, G(tok)->next));
      continue;
    }

    if (equal(tok, "-")) {
      node = new_binary(ND_SUB, node, mul(&tok, G(tok)->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static NodeBPtr mul(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = unary(&tok, tok);

  for (;;) {
    if (equal(tok, "*")) {
      node = new_binary(ND_MUL, node, unary(&tok, G(tok)->next));
      continue;
    }

    if (equal(tok, "/")) {
      node = new_binary(ND_DIV, node, unary(&tok, G(tok)->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// unary = ("+" | "-") unary
//       | primary
static NodeBPtr unary(TokenBPtr *rest, TokenBPtr tok) {
  if (equal(tok, "+"))
    return unary(rest, G(tok)->next);

  if (equal(tok, "-"))
    return new_unary(ND_NEG, unary(rest, G(tok)->next));

  return primary(rest, tok);
}

// primary = "(" expr ")" | num
static NodeBPtr primary(TokenBPtr *rest, TokenBPtr tok) {
  if (equal(tok, "(")) {
    NodeBPtr node = expr(&tok, G(tok)->next);
    *rest = skip(tok, ")");
    return node;
  }

  if (G(tok)->kind == TK_NUM) {
    NodeBPtr node = new_num(G(tok)->val);
    *rest = G(tok)->next;
    return node;
  }

  error_tok(tok, "expected an expression");
}

//
// Code generator
//

static int depth;

static void push(void) {
  printf("  pha\n");
  printf("  txa\n");
  printf("  pha\n");
  depth++;
}

static void pop(char reg) {
  printf("  tay\n");
  printf("  pla\n");
  printf("  sta __rc%d\n", reg+1);
  printf("  pla\n");
  printf("  sta __rc%d\n", reg);
  printf("  tya\n");
  depth--;
}

static void gen_expr(NodeBPtr node) {
  switch (G(node)->kind) {
  case ND_NUM: {
    unsigned val = G(node)->val;
    printf("  lda #%d\n", val & 0xff);
    printf("  ldx #%d\n", val >> 8);
    return;
  }
  case ND_NEG:
    gen_expr(G(node)->lhs);
    printf("  clc\n");
    printf("  eor #$ff\n");
    printf("  adc #1\n");
    printf("  tay\n");
    printf("  txa\n");
    printf("  eor #$ff\n");
    printf("  adc #0\n");
    printf("  tax\n");
    printf("  tya\n");
    return;
  }

  gen_expr(G(node)->rhs);
  push();
  gen_expr(G(node)->lhs);
  pop(2);

  switch (G(node)->kind) {
  case ND_ADD:
    printf("  clc\n");
    printf("  adc __rc2\n");
    printf("  tay\n");
    printf("  txa\n");
    printf("  adc __rc3\n");
    printf("  tax\n");
    printf("  tya\n");
    return;
  case ND_SUB:
    printf("  sec\n");
    printf("  sbc __rc2\n");
    printf("  tay\n");
    printf("  txa\n");
    printf("  sbc __rc3\n");
    printf("  tax\n");
    printf("  tya\n");
    return;
  case ND_MUL:
    printf("  jsr __mulqi3\n");
    return;
  case ND_DIV:
    printf("  jsr __divqi3\n");
    return;
  case ND_EQ:
  case ND_NE:
    printf("  cpx __rc3\n");
    printf("  bne 1f\n");
    printf("  cmp __rc2\n");
    printf("  bne 1f\n");
    if (G(node)->kind == ND_EQ) {
      printf("  lda #1\n");
      printf("  bne 2f\n");
      printf("1:\n");
      printf("  lda #0\n");
    } else {
      printf("  lda #0\n");
      printf("  bne 2f\n");
      printf("1:\n");
      printf("  lda #1\n");
    }
    printf("2:\n");
    printf("  ldx #0\n");

    return;

  case ND_LT:
  case ND_GE:
    printf("  cmp __rc2\n");
    printf("  tay\n");
    printf("  txa\n");
    printf("  sbc __rc2\n");
    printf("  bvc 1f\n");
    printf("  eor #$80\n");
    printf("1:\n");

    if (G(node)->kind == ND_LT)
      printf("  bcs 1f\n");
    else
      printf("  bcc 1f\n");
    printf("  lda #1\n");
    printf("  bne 2f\n");
    printf("1:\n");
    printf("  lda #0\n");
    printf("2:\n");
    printf("  ldx #0\n");

    return;
  }

  error("invalid expression");
}

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  VoidBPtr vtext = bcalloc(1, strlen(argv[1])+1);
  current_input.bank = vtext.bank;
  current_input.ptr = vtext.ptr;
  strcpy(G(current_input), argv[1]);

  TokenBPtr tok = tokenize();
  NodeBPtr node = expr(&tok, tok);

  if (G(tok)->kind != TK_EOF)
    error_tok(tok, "extra token");

  printf("  .zeropage __rc2\n");
  printf("\n");
  printf("  .globl main\n");
  printf("main:\n");

  // Traverse the AST to emit assembly.
  gen_expr(node);
  printf("  rts\n");

  assert(depth == 0);
  return 0;
}
