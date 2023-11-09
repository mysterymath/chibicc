#include <assert.h>
#include <ctype.h>
#include <cbm.h>
#include <cx16.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned long strtoul(const char *s, char **p, int base);

//
// bank.c
//

typedef struct {
  char bank;
  void *ptr;
} VoidBPtr;

VoidBPtr bcalloc(size_t count, size_t size);
void set_ram_bank(char bank);

#define G(BPtr) (set_ram_bank(BPtr.bank), BPtr.ptr)

typedef struct {
  char bank;
  char *ptr;
} CharBPtr;

//
// tokenize.c
//

typedef enum {
  TK_IDENT, // Identifiers
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

void error(char *fmt, ...);
void verror_at(CharBPtr loc, char *fmt, va_list ap);
void error_at(CharBPtr loc, char *fmt, ...);
void error_tok(TokenBPtr tok, char *fmt, ...);
bool equal(TokenBPtr tok, char *op);
TokenBPtr skip(TokenBPtr tok, char *s);
TokenBPtr tokenize(CharBPtr p);

//
// parse.c
//

typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_NEG,       // unary -
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_GE,        // >=
  ND_ASSIGN,    // =
  ND_EXPR_STMT, // Expression statement
  ND_VAR,       // Variable
  ND_NUM,       // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
typedef struct {
  char bank;
  Node *ptr;
} NodeBPtr;
struct Node {
  NodeKind kind; // Node kind
  NodeBPtr next; // Next node
  NodeBPtr lhs;  // Left-hand side
  NodeBPtr rhs;  // Right-hand side
  char name;     // Used if kind == ND_VAR
  int val;       // Used if kind == ND_NUM
};

NodeBPtr parse(TokenBPtr tok);

//
// codegen.c
//

void codegen(NodeBPtr node);
