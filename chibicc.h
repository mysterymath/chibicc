#define _POSIX_C_SOURCE 200809L
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

int bstrncmp(CharBPtr l, CharBPtr r, size_t size);
CharBPtr bstrndup(CharBPtr str, size_t size);

typedef struct Node Node;
typedef struct {
  char bank;
  Node *ptr;
} NodeBPtr;

//
// tokenize.c
//

// Token
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

// Local variable
typedef struct Obj Obj;
typedef struct {
  char bank;
  Obj *ptr;
} ObjBPtr;
struct Obj {
  ObjBPtr next;
  CharBPtr name; // Variable name
  int offset; // Offset from RBP
};

// Function
typedef struct Function Function;
typedef struct {
  char bank;
  Function *ptr;
} FunctionBPtr;
struct Function {
  NodeBPtr body;
  ObjBPtr locals;
  int stack_size;
};

// AST node
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
struct Node {
  NodeKind kind; // Node kind
  NodeBPtr next; // Next node
  NodeBPtr lhs;  // Left-hand side
  NodeBPtr rhs;  // Right-hand side
  ObjBPtr var;   // Used if kind == ND_VAR
  int val;       // Used if kind == ND_NUM
};

FunctionBPtr parse(TokenBPtr tok);

//
// codegen.c
//

void codegen(FunctionBPtr prog);
