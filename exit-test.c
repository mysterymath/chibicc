#include <cbm.h>
#include <stdio.h>

asm(".global __after_main\n"
    ".section .after_main,\"axR\",@progbits\n"
    "__after_main:\n"
    "  jmp __exit_return\n");

__attribute__((leaf)) void _fini(void);

// Runs finalizers, then returns to the caller. Jumping to here after main has
// the effect of returning from _start with the given status. When applicable,
// this can be more efficient than using non-local jump machinery to perform a
// general _exit.
char __exit_return(int status) {
  _fini();

  cbm_k_setnam("@:result,S,W");
  cbm_k_setlfs(1, 8, 3);
  cbm_k_open();
  cbm_k_chkout(1);
  printf("%d\n", status);
  cbm_k_close(1);
  cbm_k_clrch();

  return (char)status;
}

void __putchar(char c) { cbm_k_chrout(c); }
