#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
    return 1;
  }

  char *p = argv[1];

  printf("  .globl main\n");
  printf("main:\n");
  printf("  lda #%d\n", strtol(p, &p, 10));

  while (*p) {
    if (*p == '+') {
      p++;
      printf("  clc\n");
      printf("  adc #%ld\n", strtol(p, &p, 10));
      continue;
    }

    if (*p == '-') {
      p++;
      printf("  sec\n");
      printf("  sbc #%ld\n", strtol(p, &p, 10));
      continue;
    }

    fprintf(stderr, "unexpected character: '%c'\n", *p);
    return 1;
  }

  printf("  rts\n");
  return 0;
}
