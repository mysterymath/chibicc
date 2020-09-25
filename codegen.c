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

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(NodeBPtr node) {
  if (G(node)->kind == ND_VAR) {
    unsigned offset = (G(node)->name - 'a') * -2;
    printf("  clc\n");
    printf("  lda __rc30\n");
    printf("  adc #%d\n", offset & 0xff);
    printf("  tay\n");
    printf("  lda __rc31\n");
    printf("  adc #%d\n", offset >> 8 & 0xff);
    printf("  tax\n");
    printf("  tya\n");
    return;
  }

  error("not an lvalue");
}

// Generate code for a given node.
static void gen_expr(NodeBPtr node) {
  switch (G(node)->kind) {
  case ND_NUM: {
    unsigned val = G(node)->val;
    printf("  lda #%d\n", val & 0xff);
    printf("  ldx #%d\n", val >> 8 & 0xff);
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
  case ND_VAR:
    gen_addr(node);
    printf("  sta __rc2\n");
    printf("  stx __rc3\n");
    printf("  ldy #1\n");
    printf("  lda (__rc2),y\n");
    printf("  tax\n");
    printf("  dey\n");
    printf("  lda (__rc2),y\n");
    return;
  case ND_ASSIGN:
    gen_addr(G(node)->lhs);
    push();
    gen_expr(G(node)->rhs);
    pop(2);
    printf("  ldy #0\n");
    printf("  sta (__rc2),y\n");
    printf("  pha\n");
    printf("  txa\n");
    printf("  iny\n");
    printf("  sta (__rc2),y\n");
    printf("  pla\n");
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

  // Prologue
  printf("  lda __rc30\n");
  printf("  pha\n");
  printf("  lda __rc31\n");
  printf("  pha\n");
  printf("  lda __rc0\n");
  printf("  sta __rc30\n");
  printf("  lda __rc1\n");
  printf("  sta __rc31\n");
  printf("  sec\n");
  printf("  lda __rc0\n");
  printf("  sbc #52\n");
  printf("  sta __rc0\n");
  printf("  lda __rc1\n");
  printf("  sbc #0\n");
  printf("  sta __rc1\n");

  for (NodeBPtr n = node; n.bank; n = G(n)->next) {
    gen_stmt(n);
    assert(depth == 0);
  }

  printf("  lda __rc30\n");
  printf("  sta __rc0\n");
  printf("  lda __rc31\n");
  printf("  sta __rc1\n");
  printf("  pla\n");
  printf("  sta __rc31\n");
  printf("  pla\n");
  printf("  sta __rc30\n");
  printf("  rts\n");
}
