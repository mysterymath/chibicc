#ifndef AS_TOKENIZE_H
#define AS_TOKENIZE_H

#include <stdlib.h>

typedef enum {
  TK_PUNCT,
  TK_SYMBOL,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  long val;
  char *loc;
  size_t len;
};

Token *tokenize(char *stmt);

#endif // not AS_TOKENIZE_H
