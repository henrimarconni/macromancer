#include "codegen.h"
#include "parser.h"
#include "stringdef.h"
#include "vmem_arena.h"
#include <setjmp.h>
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
    fprintf(stderr, "Error: Invalid usage\n"
                    "Usage: macromancer inputfile:outputfile\n");
    return -1;
  }

  bstr confpath;
  bstr output_path;

  if (parse_arg(argv[1], &confpath, &output_path) < 0) {
    fprintf(stderr, "Error: unexpected argument, expected inputfile.mm:outputfile\n");
    return -1;
  }

  jmp_buf onerror;
  Parser p;
  VMEMArena* arena = vmarena_new(1024 * 128); // max size is 128 kb

  if (setjmp(onerror) == 0) {
    read_conf(&p, confpath, arena, &onerror);
  } else {
    printf("Parsing failed, exiting\n");
    return -1;
  }

  Codegen c;
  generate_code(&c, &p);
  codegen_destroy(&c);
  parser_destroy(&p);
  vmarena_free(arena);
  return 0;
}
