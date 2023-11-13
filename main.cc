#include "chibicc.h" 

int main(void) {
  // Switch to ISO mode
  putchar(0x0f);
  printf("Please enter C string:\n");

  VoidBPtr vtext = bcalloc(1, 80);
  CharBPtr text = {vtext.bank, vtext.ptr};

  // TODO: Error
  char line[80];
  char *l = G(text);
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

  TokenBPtr tok = tokenize(text);
  FunctionBPtr prog = parse(tok);

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}
