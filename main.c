#include "chibicc.h" 

int main(void) {
  // Open screen to LFN 1
  cbm_k_setnam("");
  cbm_k_setlfs(1, 3, 2);
  cbm_k_open();
  cbm_k_chkout(1);
  putchar(0x0f); // Enable ISO mode

  // Open input file to LFN 2
  cbm_k_setnam("a.in,S,R");
  cbm_k_setlfs(2, 8, 2);
  cbm_k_open();
  cbm_k_chkin(2);

  char text[80];

  char *c = text;
  while (!(cbm_k_readst() & 0x40))
    *c++ = cbm_k_chrin();
  *c++ = '\0';

  // Open output file to LFN 2
  cbm_k_close(2);
  cbm_k_setnam("@:a.out,S,W");
  cbm_k_setlfs(2, 8, 3);
  cbm_k_open();
  cbm_k_chkout(2);

  Token* tok = tokenize(text);
  Function* prog = parse(tok);

  // Traverse the AST to emit assembly.
  codegen(prog);

  cbm_k_close(2);
  cbm_k_close(1);
  cbm_k_clrch();

  return 0;
}
