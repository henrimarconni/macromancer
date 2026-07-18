#include "ce_getopt.h"
#include "codegen.h"
#include "parser.h"
#include "stringdef.h"
#include "vmem_arena.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

void parse_arg(int argc, char** argv, bstr* output_path, bstr* confpath) {
  ce_initopt(argc, argv);
  ce_addopt("output", 'o', 's', "Output file.h location");
  ce_addopt("help", 'h', 0, "Print help message");
  char ch;
  ParsedOpt popt;
  while (ce_getopt(&ch, &popt)) {
    switch (ch) {
    case 'o': {
      if (*output_path) {
        printf("Error: --output flag used 2+ times\n");
        exit(-1);
      }
      *output_path = popt.s;
      break;
    }
    case 'h': {
      ce_printhelp();
      exit(0);
    }
    case CE_PLAIN_VALUE: {
      if (*confpath) {
        printf("Error: input file already specified: %s, cannot overwrite it with: %s\n", *confpath,
               popt.s);
        exit(-1);
      }
      *confpath = popt.s;
      break;
    }

    default:
      assert(false && "Unreachable");
    }
  }
  if (!*confpath) {
    printf("Error: input file not specified\n");
    exit(-1);
  }
}

void write_out(bstr output_path, Codegen* c) {
  FILE* out = output_path ? fopen(output_path, "w") : stdout;
  if (!out) {
    printf("Error: Could not open output file %s for writing\n", output_path);
    exit(-1);
  }

  fprintf(out, "%s\n", c->output.get);

  if (output_path) {
    fclose(out);
  }
}

int main(int argc, char** argv) {
  bstr confpath = NULL;
  bstr output_path = NULL;

  parse_arg(argc, argv, &output_path, &confpath);

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
  write_out(output_path, &c);

  codegen_destroy(&c);
  parser_destroy(&p);
  vmarena_free(arena);
  return 0;
}
