#include "chibicc.h"

// All local variable instances created during parsing are
// accumulated to this list.
ObjBPtr locals;

static NodeBPtr expr(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr expr_stmt(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr assign(TokenBPtr  *rest, TokenBPtr tok);
static NodeBPtr equality(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr relational(TokenBPtr  *rest, TokenBPtr  tok);
static NodeBPtr add(TokenBPtr  *rest, TokenBPtr  tok);
static NodeBPtr mul(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr unary(TokenBPtr *rest, TokenBPtr tok);
static NodeBPtr primary(TokenBPtr *rest, TokenBPtr tok);

// Find a local variable by name.
static ObjBPtr find_var(TokenBPtr tok) {
  for (ObjBPtr var = locals; var.ptr; var = G(var)->next)
    if (strlen(G(G(var)->name)) == G(tok)->len && !bstrncmp(G(tok)->loc, G(var)->name, G(tok)->len))
      return var;
  ObjBPtr null = {0, NULL};
  return null;
}

static NodeBPtr new_node(NodeKind kind) {
  VoidBPtr vnode = bcalloc(1, sizeof(Node));
  NodeBPtr node = {vnode.bank, vnode.ptr};
  G(node)->kind = kind;
  return node;
}

static NodeBPtr new_binary(NodeKind kind, NodeBPtr lhs, NodeBPtr rhs) {
  NodeBPtr node = new_node(kind);
  G(node)->lhs = lhs;
  G(node)->rhs = rhs;
  return node;
}

static NodeBPtr new_unary(NodeKind kind, NodeBPtr expr) {
  NodeBPtr node = new_node(kind);
  G(node)->lhs = expr;
  return node;
}

static NodeBPtr new_num(int val) {
  NodeBPtr node = new_node(ND_NUM);
  G(node)->val = val;
  return node;
}

static NodeBPtr new_var_node(ObjBPtr var) {
  NodeBPtr node = new_node(ND_VAR);
  G(node)->var = var;
  return node;
}

static ObjBPtr new_lvar(CharBPtr name) {
  VoidBPtr vvar = bcalloc(1, sizeof(Obj));
  ObjBPtr var = {vvar.bank, vvar.ptr};
  G(var)->name = name;
  G(var)->next = locals;
  locals = var;
  return var;
}

// stmt = "return" expr ";"
//      | expr-stmt
static NodeBPtr stmt(TokenBPtr *rest, TokenBPtr tok) {
  if (equal(tok, "return")) {
    NodeBPtr node = new_unary(ND_RETURN, expr(&tok, G(tok)->next));
    *rest = skip(tok, ";");
    return node;
  }

  return expr_stmt(rest, tok);
}

// expr-stmt = expr ";"
static NodeBPtr expr_stmt(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = new_unary(ND_EXPR_STMT, expr(&tok, tok));
  *rest = skip(tok, ";");
  return node;
}

// expr = assign
static NodeBPtr expr(TokenBPtr *rest, TokenBPtr tok) {
  return assign(rest, tok);
}

// assign = equality ("=" assign)?
static NodeBPtr assign(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = equality(&tok, tok);
  if (equal(tok, "="))
    node = new_binary(ND_ASSIGN, node, assign(&tok, G(tok)->next));
  *rest = tok;
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static NodeBPtr equality(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = relational(&tok, tok);

  for (;;) {
    if (equal(tok, "==")) {
      node = new_binary(ND_EQ, node, relational(&tok, G(tok)->next));
      continue;
    }

    if (equal(tok, "!=")) {
      node = new_binary(ND_NE, node, relational(&tok, G(tok)->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static NodeBPtr relational(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = add(&tok, tok);

  for (;;) {
    if (equal(tok, "<")) {
      node = new_binary(ND_LT, node, add(&tok, G(tok)->next));
      continue;
    }

    if (equal(tok, "<=")) {
      node = new_binary(ND_GE, add(&tok, G(tok)->next), node);
      continue;
    }

    if (equal(tok, ">")) {
      node = new_binary(ND_LT, add(&tok, G(tok)->next), node);
      continue;
    }

    if (equal(tok, ">=")) {
      node = new_binary(ND_GE, node, add(&tok, G(tok)->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static NodeBPtr add(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = mul(&tok, tok);

  for (;;) {
    if (equal(tok, "+")) {
      node = new_binary(ND_ADD, node, mul(&tok, G(tok)->next));
      continue;
    }

    if (equal(tok, "-")) {
      node = new_binary(ND_SUB, node, mul(&tok, G(tok)->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static NodeBPtr mul(TokenBPtr *rest, TokenBPtr tok) {
  NodeBPtr node = unary(&tok, tok);

  for (;;) {
    if (equal(tok, "*")) {
      node = new_binary(ND_MUL, node, unary(&tok, G(tok)->next));
      continue;
    }

    if (equal(tok, "/")) {
      node = new_binary(ND_DIV, node, unary(&tok, G(tok)->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// unary = ("+" | "-") unary
//       | primary
static NodeBPtr unary(TokenBPtr *rest, TokenBPtr tok) {
  if (equal(tok, "+"))
    return unary(rest, G(tok)->next);

  if (equal(tok, "-"))
    return new_unary(ND_NEG, unary(rest, G(tok)->next));

  return primary(rest, tok);
}

// primary = "(" expr ")" | ident | num
static NodeBPtr primary(TokenBPtr *rest, TokenBPtr tok) {
  if (equal(tok, "(")) {
    NodeBPtr node = expr(&tok, G(tok)->next);
    *rest = skip(tok, ")");
    return node;
  }

  if (G(tok)->kind == TK_IDENT) {
    ObjBPtr var = find_var(tok);
    if (!var.ptr)
      var = new_lvar(bstrndup(G(tok)->loc, G(tok)->len));
    *rest = G(tok)->next;
    return new_var_node(var);
  }

  if (G(tok)->kind == TK_NUM) {
    NodeBPtr node = new_num(G(tok)->val);
    *rest = G(tok)->next;
    return node;
  }

  error_tok(tok, "expected an expression");
}

// program = stmt*
FunctionBPtr parse(TokenBPtr tok) {
  VoidBPtr vhead = bcalloc(1, sizeof(Node));
  NodeBPtr head = {vhead.bank, vhead.ptr};
  NodeBPtr cur = head;

  while (G(tok)->kind != TK_EOF)
    cur = G(cur)->next = stmt(&tok, tok);

  VoidBPtr vprog = bcalloc(1, sizeof(Function));
  FunctionBPtr prog = {vprog.bank, vprog.ptr};
  G(prog)->body = G(head)->next;
  G(prog)->locals = locals;
  return prog;
}
