#include "chibicc.h"

static int depth;
static Obj *current_fn;

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
  depth--;
}

static void pop_imag16(char reg) {
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
  case ND_VAR:
    if (node->var->is_local) {
      // Local variable
      unsigned offset = node->var->offset;
      printf("  clc\n");
      printf("  lda __rc30\n");
      printf("  adc #%d\n", offset & 0xff);
      printf("  tay\n");
      printf("  lda __rc31\n");
      printf("  adc #%d\n", offset >> 8 & 0xff);
      printf("  tax\n");
      printf("  tya\n");
    } else {
      // Global variable
      printf("  lda #<%s\n", node->var->name);
      printf("  ldx #>%s\n", node->var->name);
    }
    return;
  case ND_DEREF:
    gen_expr(node->lhs);
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

// Load a value from where AX is pointing to.
static void load(Type *ty) {
  if (ty->kind == TY_ARRAY) {
    // If it is an array, do not attempt to load a value to the
    // register because in general we can't load an entire array to a
    // register. As a result, the result of an evaluation of an array
    // becomes not the array itself but the address of the array.
    // This is where "array is automatically converted to a pointer to
    // the first element of the array in C" occurs.
    return;
  }

  printf("  sta __rc2\n");
  printf("  stx __rc3\n");
  if (ty->size == 1) {
    printf("  ldx #0\n");
    printf("  ldy #0\n");
    printf("  lda (__rc2),y\n");
  } else {
    printf("  ldy #1\n");
    printf("  lda (__rc2),y\n");
    printf("  tax\n");
    printf("  dey\n");
    printf("  lda (__rc2),y\n");
  }
}

// Store AX to an address that the stack top is pointing to.
static void store(Type *ty) {
  pop_imag16(2);

  printf("  ldy #0\n");
  if (ty->size == 1) {
    printf("  sta (__rc2),y\n");
  } else {
    printf("  sta (__rc2),y\n");
    printf("  pha\n");
    printf("  txa\n");
    printf("  iny\n");
    printf("  sta (__rc2),y\n");
    printf("  pla\n");
  }
}

#define NUM_ARG_REGS 16

static int assign_ptr_reg(bool *reg_used) {
  int reg;
  for (reg = 2; reg < NUM_ARG_REGS; reg += 2)
    if (!reg_used[reg] && !reg_used[reg + 1])
      break;
  reg_used[reg] = reg_used[reg + 1] = true;
  return reg;
}

static int assign_byte_reg(bool *reg_used) {
  int reg;
  for (reg = 0; reg < NUM_ARG_REGS; reg++)
    if (!reg_used[reg])
      break;
  reg_used[reg] = true;
  return reg;
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
    load(node->ty);
    return;
  case ND_DEREF:
    gen_expr(node->lhs);
    load(node->ty);
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    push();
    gen_expr(node->rhs);
    store(node->ty);
    return;
  case ND_FUNCALL: {
    Obj *fn = find_fn(node->funcname);

    int nargs = 0;
    bool reg_used[NUM_ARG_REGS] = {0};
    // Pushed registers in push order.
    int saved_regs[4];
    int num_saved = 0;
    Obj *param = fn ? fn->params : NULL;
    for (Node *arg = node->args; arg;
         arg = arg->next, param = param ? param->next : NULL) {
      if (fn && !param)
        error_tok(arg->tok, "too many arguments");

      Type *ty;
      if (param)
        ty = param->ty;
      else if (is_integer(arg->ty) && arg->ty->size < 2)
        ty = ty_int;
      else
        ty = arg->ty;

      gen_expr(arg);

      // Pointers are assigned to imaginary pointer registers
      if (ty->kind == TY_PTR) {
        int reg = assign_ptr_reg(reg_used);

        // RS1 is used for generating expressions, so push it.
        if (reg == 2) {
          printf("  pha\n");
          saved_regs[num_saved++] = reg;
          printf("  txa\n");
          printf("  pha\n");
          saved_regs[num_saved++] = reg + 1;
          continue;
        }

        printf("  sta __rc%d\n", reg);
        printf("  stx __rc%d\n", reg + 1);
        continue;
      }

      // Assign each byte separately.
      for (int i = 0; i < ty->size; i++) {
        int reg = assign_byte_reg(reg_used);

        if (reg < 4) {
          if (i == 1) {
            printf("  tay\n");
            printf("  txa\n");
          }
          printf("  pha\n");
          if (i == 1)
            printf("  tya\n");
          saved_regs[num_saved++] = reg;
          continue;
        }

        if (i == 0)
          printf("  sta __rc%d\n", reg);
        else
          printf("  stx __rc%d\n", reg);
      }
    }

    // Restore any saved regs.
    bool a_free = true;
    bool a_in_y = false;
    for (; num_saved > 0; --num_saved) {
      int reg = saved_regs[num_saved - 1];

      if (reg != 0 && !a_free) {
        printf("  tay\n");
        a_free = true;
        a_in_y = true;
      }

      switch (reg) {
      case 0:
        printf("  pla\n");
        a_free = false;
        break;
      case 1:
        printf("  pla\n");
        printf("  tax\n");
        break;
      default:
        printf("  pla\n");
        printf("  sta __rc%d\n", reg);
        break;
      }
    }
    if (a_in_y)
      printf("  tya\n");

    printf("  jsr %s\n", node->funcname);
    return;
  }
  }

  gen_expr(node->rhs);
  push();
  gen_expr(node->lhs);
  pop_imag16(2);

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
static void assign_lvar_offsets(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function)
      continue;

    int offset = 0;
    for (Obj *var = fn->locals; var; var = var->next) {
      offset += var->ty->size;
      var->offset = -offset;
    }
    fn->stack_size = offset;
  }
}

static void emit_data(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function)
      continue;

    printf("  .data\n");
    printf("  .globl %s\n", var->name);
    printf("%s:\n", var->name);

    if (var->init_data) {
      for (int i = 0; i < var->ty->size; i++)
        printf("  .byte %d\n", var->init_data[i]);
    } else {
      printf("  .zero %d\n", var->ty->size);
    }
  }
}

static void emit_text(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function)
      continue;

    printf("  .globl %s\n", fn->name);
    printf("  .text\n");
    printf("%s:\n", fn->name);
    current_fn = fn;

    // Prologue
    unsigned stack_size = fn->stack_size;
    printf("  tay\n");
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
    printf("  tya\n");

    // Save passed-by-register arguments to the stack
    bool reg_used[16] = {0};
    unsigned reg_offsets[16];
    for (Obj *var = fn->params; var; var = var->next) {
      if (var->ty->kind == TY_PTR) {
        int reg = assign_ptr_reg(reg_used);
        reg_offsets[reg] = var->offset;
        reg_offsets[reg + 1] = var->offset + 1;
      } else {
        for (int i = 0; i < var->ty->size; i++) {
          int reg = assign_byte_reg(reg_used);
          reg_offsets[reg] = var->offset + i;
        }
      }
    }

    for (int reg = 0; reg < sizeof(reg_used); reg++) {
      if (!reg_used[reg])
        continue;
      if (reg == 0)
        printf("  pha\n");

      unsigned offset = reg_offsets[reg];
      printf("  clc\n");
      printf("  lda __rc30\n");
      printf("  adc #%d\n", offset & 0xff);
      printf("  sta __rc16\n");
      printf("  lda __rc31\n");
      printf("  adc #%d\n", offset >> 8 & 0xff);
      printf("  sta __rc17\n");

      switch (reg) {
      case 0:
        printf("  pla\n");
        break;
      case 1:
        printf("  txa\n");
        break;
      default:
        printf("  lda __rc%d\n", reg);
        break;
      }
      printf("  ldy #0\n");
      printf("  sta (__rc16),y\n");
    }

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

void codegen(Obj *prog) {
  assign_lvar_offsets(prog);
  emit_data(prog);
  emit_text(prog);
}
