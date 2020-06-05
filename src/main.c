#include "willani.h"

char *user_input;

Function *functions;

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments.\n", argv[0]);
  }

  user_input = argv[1];
  Token *token = tokenize(user_input);

  // parse
  program(token);

  code_generate();

  return 0;
}
