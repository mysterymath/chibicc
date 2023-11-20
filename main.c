#include "chibicc.h" 

int main(void) {
  putchar(0x0f); // Enable ISO mode

  freopen("c.c", "r", stdin);

  char text[4096];

  char *c = text;
  while (!(cbm_k_readst() & 0x40))
    *c++ = cbm_k_chrin();
  *c++ = '\0';

  // Open output file to LFN 2
  freopen("c.s", "w", stdout);

  Token* tok = tokenize(text);
  Function* prog = parse(tok);

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}
