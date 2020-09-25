#include "chibicc.h"

static int depth;

static void push(void) {
  printf("  pha\n");
  printf("  txa\n");
  printf("  pha\n");
  depth++;
}

static void pop(char reg) {
  printf("  tay\n");
  printf("  pla\n");
  printf("  sta __rc%d\n", reg+1);
  printf("  pla\n");
  printf("  sta __rc%d\n", reg);
  printf("  tya\n");
  depth--;
}

static void gen_expr(NodeBPtr node) {
  switch (G(node)->kind) {
  case ND_NUM: {
    unsigned val = G(node)->val;
    printf("  lda #%d\n", val & 0xff);
    printf("  ldx #%d\n", val >> 8);
    return;
  }
  case ND_NEG:
    gen_expr(G(node)->lhs);
    printf("  clc\n");
    printf("  eor #$ff\n");
    printf("  adc #1\n");
    printf("  tay\n");
    printf("  txa\n");
    printf("  eor #$ff\n");
    printf("  adc #0\n");
    printf("  tax\n");
    printf("  tya\n");
    return;
  }

  gen_expr(G(node)->rhs);
  push();
  gen_expr(G(node)->lhs);
  pop(2);

  switch (G(node)->kind) {
  case ND_ADD:
    printf("  clc\n");
    printf("  adc __rc2\n");
    printf("  tay\n");
    printf("  txa\n");
    printf("  adc __rc3\n");
    printf("  tax\n");
    printf("  tya\n");
    return;
  case ND_SUB:
    printf("  sec\n");
    printf("  sbc __rc2\n");
    printf("  tay\n");
    printf("  txa\n");
    printf("  sbc __rc3\n");
    printf("  tax\n");
    printf("  tya\n");
    return;
  case ND_MUL:
    printf("  jsr __mulqi3\n");
    return;
  case ND_DIV:
    printf("  jsr __divqi3\n");
    return;
  case ND_EQ:
  case ND_NE:
    printf("  cpx __rc3\n");
    printf("  bne 1f\n");
    printf("  cmp __rc2\n");
    printf("  bne 1f\n");
    if (G(node)->kind == ND_EQ) {
      printf("  lda #1\n");
      printf("  bne 2f\n");
      printf("1:\n");
      printf("  lda #0\n");
    } else {
      printf("  lda #0\n");
      printf("  bne 2f\n");
      printf("1:\n");
      printf("  lda #1\n");
    }
    printf("2:\n");
    printf("  ldx #0\n");

    return;

  case ND_LT:
  case ND_GE:
    printf("  cmp __rc2\n");
    printf("  tay\n");
    printf("  txa\n");
    printf("  sbc __rc2\n");
    printf("  bvc 1f\n");
    printf("  eor #$80\n");
    printf("1:\n");

    if (G(node)->kind == ND_LT)
      printf("  bcs 1f\n");
    else
      printf("  bcc 1f\n");
    printf("  lda #1\n");
    printf("  bne 2f\n");
    printf("1:\n");
    printf("  lda #0\n");
    printf("2:\n");
    printf("  ldx #0\n");

    return;
  }

  error("invalid expression");
}

static void gen_stmt(NodeBPtr node) {
  if (G(node)->kind == ND_EXPR_STMT) {
    gen_expr(G(node)->lhs);
    return;
  }

  error("invalid statement");
}
void codegen(NodeBPtr node) {
  printf("  .zeropage __rc2\n");
  printf("\n");

  printf("  .globl main\n");
  printf("main:\n");

  for (NodeBPtr n = node; n.bank; n = G(n)->next) {
    gen_stmt(n);
    assert(depth == 0);
  }

  printf("  rts\n");
}
