#include "parse.h"

Function *find_function(char *name, int namelen) {
  for (Function *func = functions; func; func = func->next) {
    if (strncmp(name, func->name, namelen)==0 && func->namelen == namelen)
      return func;
  }
  return NULL;
}

// f1 is same name and type as f2
bool same_function(Function *f1, Function *f2) {
  // Check name, namelen, type, argc, args's types
  if ( f1->namelen != f2->namelen
    || strncmp(f1->name, f2->name, f1->namelen) != 0
    || !same_type(f1->type, f2->type)
    || f1->argc != f2->argc
  ) {
    return false;
  }

  Var *arg_f1 = f1->args;
  Var *arg_f2 = f2->args;
  for (int i=0; i < f1->argc; i++) {
    if (!arg_f1 || !arg_f2 || !same_type(arg_f1->type, arg_f2->type))
      return false;
    arg_f1 = arg_f1->next;
    arg_f2 = arg_f2->next;
  }
  return true;
}

void add_function(Function *func) {
  if (!functions) {
    functions = func;
    return;
  }

  Function *tail  = functions;
  while (tail->next)
    tail = tail->next;

  tail->next = func;
}

// function_definition = type ident "(" ( ( type ident ( "," type ident ) * ) ?  ")"
Function *function_definition(Token **rest, Token *token, Type *return_type, char *name, int namelen) {
  parse_log("function_definiton()");
  bool definition = false;

  // arguments
  if (!equal(token, "("))
    error_at(token, "expected (");
  token = token->next;

  int argc = 0;

  while (!equal(token, ")")) {
    Type *arg_type = read_type(&token, token, DENY_STATIC, DENY_EXTERN, DENY_CONST);
    if (!arg_type)
      error_at(token, "expected Type");

    argc++;

    if (equal(token, ",")) {
      token = token->next;
      definition = true;
      new_var(arg_type, NULL, 0);
      continue;
    }
    if (equal(token, ")")) {
      definition = true;
      new_var(arg_type, NULL, 0);
      break;
    }

    if(!is_identifer_token(token))
      error_at(token, "expected identifer of argument");

    new_var(arg_type, token->location, token->length);
    token = token->next;

    if (!equal(token, ","))
      break;
    token = token->next;
  }

  if (!equal(token, ")"))
    error_at(token, "expected )");
  token = token->next;


  // create Function struct
  Function *func = calloc(1, sizeof(Function));
  func->name = name;
  func->args = now_scope->vars;
  func->argc = argc;
  func->namelen = namelen;
  func->type = return_type;
  func->definition = definition;

  *rest = token;
  return func;
}
