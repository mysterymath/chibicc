#include "chibicc.h" 

int main(void) {
  putchar(0x0f); // Enable ISO mode

  FILE *input = fopen("c.c", "r");

  char text[256];
  size_t size = fread(text, 1, sizeof(text), input);
  text[size] = '\0';

  fclose(input);

  // Tokenize and parse.
  Token* tok = tokenize(text);
  Obj* prog = parse(tok);

  // Traverse the AST to emit assembly.
  freopen("a.s", "w", stdout);
  codegen(prog);

  return 0;
}
