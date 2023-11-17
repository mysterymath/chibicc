#include "chibicc.h" 

int main(void) {
  // Open input file to fd 0
  cbm_k_setnam("a.in,s,r");
  cbm_k_setlfs(0, 3, 255);
  cbm_k_open();
  cbm_k_chkin(1);

  // Open output file to fd 1
  cbm_k_setnam("a.out,s,w");
  cbm_k_setlfs(1, 8, 255);
  cbm_k_open();
  cbm_k_chkout(1);

  // Open screen to fd 2
  cbm_k_setnam("");
  cbm_k_setlfs(2, 3, 255);
  cbm_k_open();

  char text[80];

  char *c = text;
  while (!(cbm_k_readst() & 0x40))
    *c++ = cbm_k_chrin();
  *c++ = '\0';

  Token* tok = tokenize(text);
  Function* prog = parse(tok);

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}
