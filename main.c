#include "chibicc.h" 

int main(void) {
  // Switch to ISO mode
  putchar(0x0f);
  printf("Please enter C string:\n");

  // TODO: Error
  char line[80];
  char *l = line;
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

  printf("\n%s", line);

  //VoidBPtr vtext = bcalloc(1, strlen(argv[1])+1);
  //CharBPtr text = {vtext.bank, vtext.ptr};
  //strcpy(G(text), argv[1]);
  //TokenBPtr tok = tokenize(text);
  //NodeBPtr node = parse(tok);
  //codegen(node);

  return 0;
}
