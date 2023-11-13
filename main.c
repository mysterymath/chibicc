#include "chibicc.h" 

int main(void) {
  // Switch to ISO mode
  putchar(0x0f);
  printf("Please enter C string:\n");

  char text[80];

  // TODO: Error
  char* l = text;
  while (true) {
    *l = cbm_k_chrin();
    if (*l == '\r') {
      *l = '\n';
      l++;
      break;
    }
    l++;
  }
  *l = '\0';
  putchar('\n');

  Token* tok = tokenize(text);
  Function* prog = parse(tok);

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}
