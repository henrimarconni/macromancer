#include "codegen.h"
#include "parser.h"
#include "stringdef.h"
#include "vmem_arena.h"
#include <stdio.h>

int parse_arg(bstr arg, bstr* input_path, bstr* output_path) {
  *input_path = arg;
  while (*arg && *arg != ':') {
    arg++;
  }
  if (!*arg) {
    return -1;
  }
  *arg++ = '\0'; // skip :
  *output_path = arg;
  return 0;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Error: expected a single inputfile.mm:outputlocation, found either no "
                    "arguments or extra arguments\n");
    return -1;
  }
  VMEMArena* arena = vmarena_new(1024 * 128); // max size is 128 kb
  int res = 0;
  bstr confpath;
  bstr output_path;

  if (parse_arg(argv[1], &confpath, &output_path) < 0) {
    fprintf(stderr, "Error: unexpected argument, expected inputfile.mm:outputlocation\n");
    res = -1;
    goto end;
  }

  Parser p;
  read_conf(&p, confpath, arena);
  Codegen c;
  generate_code(&c, &p);
  parser_destroy(&p);
  codegen_destroy(&c);

end:
  vmarena_free(arena);
  return res;
}
