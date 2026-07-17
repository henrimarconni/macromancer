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
  if (argc == 1) {
    fprintf(stderr, "Error: expected inputfile.mm:outputlocation, found nothing\n");
    return -1;
  }
  VMEMArena* arena = vmarena_new(1024 * 128); // max size is 128 kb
  int res = 0;
  int i = 1;
  char* arg;
  while ((arg = argv[i++]) != NULL) {
    bstr confpath;
    bstr output_path;
    if (parse_arg(arg, &confpath, &output_path) < 0) {
      fprintf(stderr, "Error: unexpected argument, expected inputfile.mm:outputlocation\n");
      res = -1;
      break;
    }
    printf("Proc(%d) doing %s -> %s\n", i - 1, confpath, output_path);
    Parser p;
    read_conf(&p, confpath, arena);
    vmarena_reset(arena);
  }

  vmarena_free(arena);
  return res;
}
