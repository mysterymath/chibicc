#include "chibicc.h"

static int depth;
static Function *current_fn;

static void gen_expr(Node *node);

static int count(void) {
  static int i = 1;
  return i++;
}

static void push(void) {
  printf("  pha\n");
  printf("  txa\n");
  printf("  pha\n");
  depth++;
}

static void popax() {
  printf("  pla\n");
  printf("  tax\n");
  printf("  pla\n");
}

static void pop(char reg) {
  printf("  tay\n");
  printf("  pla\n");
  printf("  sta __rc%d\n", reg + 1);
  printf("  pla\n");
  printf("  sta __rc%d\n", reg);
  printf("  tya\n");
  depth--;
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
static int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR: {
    unsigned offset = node->var->offset;
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
  case ND_DEREF:
    gen_expr(node->lhs);
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

// Generate code for a given node.
static void gen_expr(Node *node) {
  switch (node->kind) {
  case ND_NUM: {
    unsigned val = node->val;
    printf("  lda #%d\n", val & 0xff);
    printf("  ldx #%d\n", val >> 8 & 0xff);
    return;
  }
  case ND_NEG:
    gen_expr(node->lhs);
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
  case ND_DEREF:
    gen_expr(node->lhs);
    printf("  sta __rc2\n");
    printf("  stx __rc3\n");
    printf("  ldy #1\n");
    printf("  lda (__rc2),y\n");
    printf("  tax\n");
    printf("  dey\n");
    printf("  lda (__rc2),y\n");
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    push();
    gen_expr(node->rhs);
    pop(2);
    printf("  ldy #0\n");
    printf("  sta (__rc2),y\n");
    printf("  pha\n");
    printf("  txa\n");
    printf("  iny\n");
    printf("  sta (__rc2),y\n");
    printf("  pla\n");
    return;
  case ND_FUNCALL: {
    int nargs = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      gen_expr(arg);
      push();
      nargs++;
    }

    for (int i = nargs - 1; i > 0; i--)
      pop(2*i);
    if (nargs)
      popax();

    printf("  jsr %s\n", node->funcname);
    return;
  }
  }

  gen_expr(node->rhs);
  push();
  gen_expr(node->lhs);
  pop(2);

  switch (node->kind) {
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
    printf("  jsr __mulhi3\n");
    return;
  case ND_DIV:
    printf("  jsr __divhi3\n");
    return;
  case ND_EQ:
  case ND_NE:
    printf("  cpx __rc3\n");
    printf("  bne 1f\n");
    printf("  cmp __rc2\n");
    printf("  bne 1f\n");
    if (node->kind == ND_EQ) {
      printf("  lda #1\n");
      printf("  bne 2f\n");
      printf("1:\n");
      printf("  lda #0\n");
    } else {
      printf("  lda #0\n");
      printf("  beq 2f\n");
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
    printf("  sbc __rc3\n");
    printf("  bvc 1f\n");
    printf("  eor #$80\n");
    printf("1:\n");

    if (node->kind == ND_LT)
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

  error_tok(node->tok, "invalid expression");
}

static void gen_stmt(Node *node) {
  switch (node->kind) {
  case ND_IF: {
    int c = count();
    gen_expr(node->cond);
    printf("  cpx #0\n");
    printf("  bne .L.then.%d\n", c);
    printf("  cmp #0\n");
    printf("  bne .L.then.%d\n", c);
    if (node->els)
      gen_stmt(node->els);
    printf("  jmp .L.end.%d\n", c);
    printf(".L.then.%d:\n", c);
    gen_stmt(node->then);
    printf(".L.end.%d:\n", c);
    return;
  }
  case ND_FOR: {
    int c = count();
    if (node->init)
      gen_stmt(node->init);
    printf(".L.begin.%d:\n", c);
    if (node->cond) {
      gen_expr(node->cond);
      printf("  cpx #0\n");
      printf("  bne .L.cont.%d\n", c);
      printf("  cmp #0\n");
      printf("  bne .L.cont.%d\n", c);
      printf("  jmp .L.end.%d\n", c);
    }
    printf(".L.cont.%d:\n", c);
    gen_stmt(node->then);
    if (node->inc)
      gen_expr(node->inc);
    printf("  jmp .L.begin.%d\n", c);
    printf(".L.end.%d:\n", c);
    return;
  }
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return;
  case ND_RETURN:
    gen_expr(node->lhs);
    printf("  jmp .L.return.%s\n", current_fn->name);
    return;
  case ND_EXPR_STMT:
    gen_expr(node->lhs);
    return;
  }

  error_tok(node->tok, "invalid statement");
}

// Assign offsets to local variables.
static void assign_lvar_offsets(Function *prog) {
  for (Function *fn = prog; fn; fn = fn->next) {
    int offset = 0;
    for (Obj *var = fn->locals; var; var = var->next) {
      offset += 2;
      var->offset = -offset;
    }
    fn->stack_size = offset;
  }
}

void codegen(Function *prog) {
  assign_lvar_offsets(prog);

  for (Function *fn = prog; fn; fn = fn->next) {
    printf("  .globl %s\n", fn->name);
    printf("%s:\n", fn->name);
    current_fn = fn;

    // Prologue
    unsigned stack_size = fn->stack_size;
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
    printf("  sbc #%d\n", stack_size & 0xff);
    printf("  sta __rc0\n");
    printf("  lda __rc1\n");
    printf("  sbc #%d\n", stack_size >> 8 & 0xff);
    printf("  sta __rc1\n");

    // Emit code
    gen_stmt(fn->body);
    assert(depth == 0);

    // Epilogue
    printf(".L.return.%s:\n", fn->name);
    printf("  sta __rc2\n");
    printf("  stx __rc3\n");
    printf("  lda __rc30\n");
    printf("  sta __rc0\n");
    printf("  lda __rc31\n");
    printf("  sta __rc1\n");
    printf("  pla\n");
    printf("  sta __rc31\n");
    printf("  pla\n");
    printf("  sta __rc30\n");
    printf("  lda __rc2\n");
    printf("  ldx __rc3\n");
    printf("  rts\n");
  }
}
