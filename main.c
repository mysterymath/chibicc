#include "chibicc.h" 

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  VoidBPtr vtext = bcalloc(1, strlen(argv[1])+1);
  CharBPtr text = {vtext.bank, vtext.ptr};
  strcpy(G(text), argv[1]);
  TokenBPtr tok = tokenize(text);
  NodeBPtr node = parse(tok);
  codegen(node);

  return 0;
}
