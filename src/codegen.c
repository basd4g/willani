#include "willani.h"

static void gen(Node *node);

static void print_token_comment(Token *token) {
  // printf("  # %.*s\n", token->length, token->location);
}

static void gen_num(Node *node) {
  // printf("  # gen_num\n");
  print_token_comment(node->token);
  printf("  push %ld\n", node->value); // push constant
}

// change the stack top from addr to value
static void load(void) {
  // printf("  # load() : change the stack top from addr to value\n");
  printf("  pop rax\n");          // load the stack top to rax
  printf("  mov rax, [rax]\n");   // load the actual value of rax to rax
  printf("  push rax\n");         // store rax to the stack top
}

// store value to the variable.
static void store(void) {
  // stack
  // before : (top) value, (variable's address), ...
  // after  : (top) value, ...
  // printf("  # store(): store value to the variable.\n");
  printf("  pop rdi\n");          // load the stack top to rdi
  printf("  pop rax\n");          // load the stack top to rax
  printf("  mov [rax], rdi\n");   // copy rdi's value to the address pointed by rax
  printf("  push rdi\n");         // store rdi to the stack top
}

// load the address of node's variable to the stack top
static void gen_addr(Node *node) {
  if (node->kind != ND_VAR) {
    error("Left side is expected a variable.");
  }
  // printf("  # gen_addr(): load the address of node's variable to the stack top\n");
  print_token_comment(node->token);
  printf("  lea rax, [rbp-%d]\n", node->lvar->offset); // load the address of the actual value of (rbp - offset)
  printf("  push rax\n");         // push rbp - offset
}

static void gen_binary_operator(Node *node) {
  print_token_comment(node->token);
  // printf("  # gen_op2 left\n");
  gen(node->left);
  // printf("  # gen_op2 right\n");
  gen(node->right);
  // printf("  # gen_op2 operator\n");

  printf("  pop rdi\n");          // load the stack top to rdi to calculate
  printf("  pop rax\n");          // load the stack top to rax to calculate

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");   // rax += rdi
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");   // rax -= rdi
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");  // rax *= rdi
    break;
  case ND_DIV:
    printf("  cqo\n");            // [rdx rax](128bit) = rax (64bit)
    printf("  idiv rdi\n");       // rax = [rdx rax] / rdi
                                  // rdx = [rdx rax] % rdi
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");   // set flag register with comparing rax and rdi
    printf("  sete al\n");        // al = ( flag register means rax == rdi ) ? 1 : 0
    printf("  movzb rax, al\n");  // rax(64bit) = al(8bit)
                                  // al refer to the lower 8 bits of the rax
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");       // al = ( flag register means rax != rdi ) ? 1 : 0
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");        // al = ( flag register means rax < rdi ) ? 1 : 0
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");       // al = ( flag register means rax <= rdi ) ? 1 : 0
    printf("  movzb rax, al\n");
    break;
  }
  printf("  push rax\n");         // store result to stack top
}

static void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    gen_num(node);
    return;
  case ND_VAR:
    gen_addr(node);
    load();
    return;
  case ND_ASSIGN:
    gen_addr(node->left);
    gen(node->right);
    store();
    return;
  }

  // expect binary operator node
  gen_binary_operator(node);
}

static void prologue(int offset) {
  printf("  # prologue\n");
  printf("  push rbp\n");         // record caller's rbp
  printf("  mov rbp, rsp\n");     // set current stack top to rbp
  printf("  sub rsp, %d\n", offset);     // allocate memory for a-z variables
}

static void epilogue(void) {
  printf("  # epilogue\n");       // rax is already set, and will be exit code
  printf("  mov rsp, rbp\n");   // ignore the remanig data in the stack
  printf("  pop rbp\n");        // set caller's rbp to rsp
  printf("  ret\n");
}

void code_generate(Function *func) {

  // assembly code header
  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  prologue(func->stack_size);

  for(Node *n = func->node; n; n = n->next) {
    gen(n);
    printf("  pop rax\n"); // load result(stack top) to rax
  }

  epilogue();
}