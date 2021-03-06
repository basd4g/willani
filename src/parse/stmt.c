#include "parse.h"

// block_stmt = "{" stmt* "}"
Node *block_stmt(Token **rest, Token *token) {
  parse_log("block_stmt()");
  Token *bracket_token = token;
  if (!equal(bracket_token, "{"))
    error_at(bracket_token, "expected {");
  token = bracket_token->next;

  Node head = {};
  Node *tail = &head;
  while (!equal(token, "}")) {
    tail->next = stmt(&token, token);
    while (tail->next)
      tail = tail->next;
  }

  *rest = token->next;
  return new_node_block(head.next, bracket_token);
}


// stmt       = block_stmt
//            | if_stmt
//            | while_stmt
//            | for_stmt
//            | return_stmt
//            | expr_stmt
//            | "continue" ";"
//            | "break" ";"
//            | switch_stmt
//            | "case" expr ":"
//            | "default" ":"

//            | declare_lvar_stmt

Node *stmt(Token **rest, Token *token) {
  parse_log("stmt()");
  Node *node;

  if (equal(token, "typedef")) {
    typedef_stmt(&token, token);
    *rest = token;
    return NULL;
  }

  if (is_type_tokens(token, ALLOW_STATIC, DENY_EXTERN, ALLOW_CONST))
    node = declare_lvar_stmt(&token, token);
  else {
    scope_in();
    node = stmt_without_declaration(&token, token);
    scope_out();
  }
  *rest = token;
  return node;
}

Node *stmt_without_declaration(Token **rest, Token *token) {
  parse_log("stmt_without_declaration()");
  Node *node;

  if (equal(token, "if"))
    node = if_stmt(&token, token);
  else if (equal(token, "while"))
    node = while_stmt(&token, token);
  else if (equal(token, "for")) {
    scope_in();
    node = for_stmt(&token, token);
    scope_out();
  }
  else if (equal(token, "switch"))
    node = switch_stmt(&token, token);
  else if (equal(token, "{"))
    node = block_stmt(&token, token);
  else if (equal(token, "return"))
    node = return_stmt(&token, token);
  else if (equal(token, "continue")) {
    if (!equal(token->next, ";"))
      error_at(token->next, "expected ';' of continue statement");
    node = new_node_continue(token);
    token = token->next->next;
  } else if (equal(token, "break")) {
    if (!equal(token->next, ";"))
      error_at(token->next, "expected ';' of break statement");
    node = new_node_break(token);
    token = token->next->next;
  } else
    node = expr_stmt(&token, token);

  *rest = token;
  return node;
}


// if_stmt = "if" "(" expr ")" stmt ( "else" stmt ) ?
Node *if_stmt(Token **rest, Token *token) {
  parse_log("if_stmt()");
  Token *if_token = token;
  if (!equal(if_token, "if" ))
    error_at(if_token, "expected if");
  token = if_token->next;

  if (!equal(token, "("))
    error_at(token, "expected (");
  token = token->next;

  Node *cond = expr(&token, token);

  if (!equal(token, ")"))
    error_at(token, "expected )");
  token = token->next;

  scope_in();
  Node *then = stmt_without_declaration(&token, token);
  scope_out();

  Node *els = NULL;
  if (equal(token, "else")) {
    token = token->next;
    scope_in();
    els = stmt_without_declaration(&token, token);
    scope_out();
  }
  Node *node = new_node_if(cond, then, els, if_token);

  *rest = token;
  return node;
}

// while_stmt = "while" "(" expr ")" stmt
Node *while_stmt(Token **rest, Token *token) {
  parse_log("while_stmt()");
  Token *while_token = token;
  if (!equal(while_token, "while" ))
    error_at(while_token, "expected while");
  token = while_token->next;

  if (!equal(token, "("))
    error_at(token, "expected (");
  token = token->next;

  Node *cond = expr(&token, token);

  if (!equal(token, ")"))
    error_at(token, "expected )");
  token = token->next;

  scope_in();
  Node *then = stmt_without_declaration(&token, token);
  scope_out();

  Node *node = new_node_while(cond, then, while_token);

  *rest = token;
  return node;
}

// declare_lvar_stmt | expr ";"
static Node *for_init(Token **rest, Token *token) {
  Node *node;
  if (equal(token, ";")) {
    *rest = token->next;
    return NULL;
  }
  // TODO correspond static local variable
  if (is_type_tokens(token, DENY_STATIC, DENY_EXTERN, ALLOW_CONST)) {
    node = declare_lvar_stmt(&token, token);
    *rest = token;
    return node;
  }
  node = new_node_expr_stmt(expr(&token, token), token);
  if (!equal(token, ";"))
    error_at(token, "expected ; to initialize for statement");
  *rest = token->next;
  return node;
}

Node *for_stmt(Token **rest, Token *token) {
  parse_log("for_stmt()");
  // "for"
  Token *for_token = token;
  if (!equal(for_token, "for"))
    error_at(for_token, "expected for");
  token = for_token->next;

  // "("
  if (!equal(token, "("))
    error_at(token, "expected (");
  token = token->next;

  // expr? ";"
  Node *init = for_init(&token, token);

  // expr? ";"
  Node *cond = equal(token, ";") ? new_node_num(1, token) : expr(&token, token);
  if (!equal(token, ";"))
    error_at(token, "expected ; of conditional expression tail of for statement");
  token = token->next;

  // expr? ")"
  Node *increment = NULL;
  if (!equal(token, ")"))
    increment = new_node_expr_stmt(expr(&token, token), token);
  if (!equal(token, ")"))
    error_at(token, "expected )");
  token = token->next;

  Node *then = stmt_without_declaration(&token, token);
  Node *node = new_node_for(init, cond, increment, then, for_token);

  *rest = token;
  return node;
}

Node *switch_stmt(Token **rest, Token *token) {
  parse_log("switch_stmt()");
  Token *switch_token = token;
  if (!equal(token, "switch"))
    error_at(token, "expected switch");
  token = token->next;

  if (!equal(token, "("))
    error_at(token, "expected ( of switch statement");
  token = token->next;

  Node *cond = expr(&token, token);

  if (!equal(token, ")"))
    error_at(token, "expected ) of switch statement");
  token = token->next;

  if (!equal(token, "{"))
    error_at(token, "expected { of switch statement");
  token = token->next;

  Node stmt_head = {};
  Node *stmt_tail = &stmt_head;
  Node case_head = {};
  Node *case_tail = &case_head;
  int case_num = 1;
  bool have_default = false;
  while (!equal(token, "}")) {
    if (equal(token, "case")) {
      Token *case_token = token;
      case_tail = case_tail->next = expr(&token, token->next);

      if (!equal(token, ":"))
        error_at(token, "expected : of case label in switch statement");
      token = token->next;

      stmt_tail = stmt_tail->next = new_node_case(case_token, case_num++);
      continue;
    }
    if (equal(token, "default")) {
      if (!equal(token->next, ":"))
        error_at(token->next, "expected : of default label in switch statement");
      if (have_default)
        error_at(token, "duplicated default label in switch statement");

      have_default = true;
      stmt_tail = stmt_tail->next = new_node_default(token);
      token = token->next->next;
      continue;
    }

    stmt_tail->next = stmt_without_declaration(&token, token);
    while (stmt_tail->next)
      stmt_tail = stmt_tail->next;
  }
  *rest = token->next;
  return new_node_switch(cond, case_head.next, stmt_head.next, have_default, switch_token);
}

// return_stmt = return expr? ";"
Node *return_stmt(Token **rest, Token *token) {
  parse_log("return_stmt()");
  Token *return_token = token;
  if (!equal(return_token, "return"))
    error_at(return_token, "expected return");
  token = return_token->next;

  Node *value;
  if (equal(token, ";"))
    value = new_node_num(0, token);
  else
    value = expr(&token, token);

  Node *node = new_node_return(value, return_token);

  if (!equal(token, ";"))
    error_at(token, "expected ; of return statement");
  token = token->next;

  *rest = token;
  return node;
}

// expr_stmt  =  expr ";"
Node *expr_stmt(Token **rest, Token *token) {
  parse_log("expr_stmt()");
  Token *expr_token = token;
  Node *node = new_node_expr_stmt(expr(&token, token), expr_token);

  if (!equal(token, ";"))
    error_at(token, "expected ; of expression statement");
  token = token->next;

  *rest = token;
  return node;
}

// declare_lvar_stmt =  type identifer type_suffix ("=" expr)? ";"
// declare node is skipped by codegen

Node *declare_lvar_stmt(Token **rest, Token *token) {
  parse_log("declare_lvar_stmt()");
  char *name;
  int namelen;
  Type *type;
  if (!is_type_tokens(token, ALLOW_STATIC, DENY_EXTERN, ALLOW_CONST))
    error_at(token, "expected type to declare local variable");

  type = read_type(&token, token, ALLOW_STATIC, DENY_EXTERN, ALLOW_CONST);
  if (equal(token,  ";")  && (type->kind == TYPE_STRUCT || type->kind == TYPE_ENUM)) {
    // struct or enum declaration only
    *rest = token->next;
    return NULL;
  }
  type = declarator(&token, token, type, &name, &namelen, ALLOW_OMIT_LENGTH);
  type = type_suffix(&token, token, type, DENY_OMIT_LENGTH);

  if (find_in_vars(name, namelen, now_scope->vars) || find_in_typedefs(name, namelen, now_scope->tdfs)
    || find_in_enum_tags(name, namelen, now_scope->etags))
    error_at(token, "duplicate scope declarations of variable/typedef/enum");
  new_var(type, name, namelen);

  if (equal(token, ";")) {
    token = token->next;
    *rest = token;
    return NULL;
  }

  if (!equal(token, "="))
    error_at(token, "expected ; or = to declare local variable statement");
  token = token->next;

  Node *node = read_var_init(&token, token, now_scope->vars);

  if (!equal(token, ";"))
    error_at(token, "expected ; to declare local variable statement");
  token = token->next;

  *rest = token;
  return node;
}

void typedef_stmt(Token **rest, Token *token) {
  parse_log("typedef_stmt()");
  if (!equal(token, "typedef"))
     error_at(token, "expected 'typedef'");
  token = token->next;

  char *name;
  int namelen;
  Type *type = read_type(&token, token, ALLOW_STATIC, DENY_EXTERN, ALLOW_CONST);
  type = declarator(&token, token, type, &name, &namelen, DENY_OMIT_LENGTH);
  type = type_suffix(&token, token, type, DENY_OMIT_LENGTH);

  if ( find_in_typedefs(name, namelen, now_scope->tdfs)
    || find_in_vars(name, namelen, now_scope->vars)
    || find_in_enum_tags(name, namelen, now_scope->etags)
    || !(now_scope->parent) && find_function(name, namelen)
  )
    error_at(token, "duplicate scope declarations of variable/typedef/function/enum");

  new_typedef(type, name, namelen);

  if (!equal(token, ";"))
    error_at(token, "expected ; of the end of the typedef statement");
  token = token->next;

  *rest = token;
}
