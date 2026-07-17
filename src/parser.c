#include "parser.h"
#include "vec.h"
#include "vmem_arena.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/*
 This file "Reads" the code from animals.mm file
*/

#define MAX_ID_LEN 128

#define DISPATCH_TABLE(X)                                                                          \
  X("impl", parse_impl)                                                                            \
  X("interface", parse_interface)                                                                  \
  X("export", parse_export)

ostr dup(Parser* p, bstr str) {
  size_t len = strlen(str);
  ostr dup = vmarena_alloc(p->arena, len);
  memcpy(dup, str, len);
  return dup;
}

void fetch_tok(Parser* p, bstr buf, int start_ch) {
  int i = 0;
  int ch = start_ch;
  while (!isspace(ch)) {
    buf[i++] = ch;
    ch = fgetc(p->file);
    if (ch == EOF) {
      p->err = PE_ERR;
      return;
    }
  }

  buf[i++] = '\0';
  return;
}

int skip_comment(Parser* p, int ch) {
  if (ch == '#') {
    while (ch != '\n' && ch != EOF) {
      ch = fgetc(p->file);
    }
  }
  return ch;
}

int skip_space(Parser* p, int ch) {
  while (isspace(ch) && ch != EOF) {
    ch = fgetc(p->file);
  }
  return ch;
}

int skip_unwanted(Parser* p) {
  int ch = skip_space(p, fgetc(p->file));
  while (ch != EOF) {
    if (isspace(ch))
      ch = skip_space(p, ch);
    else if (ch == '#')
      ch = skip_comment(p, ch);
    else
      break;
  }
  return ch;
}

ostr get_tok(Parser* p, char ch) {
  char buf[MAX_ID_LEN];
  fetch_tok(p, buf, ch);
  return dup(p, buf);
}

void expect(Parser* p, bstr str, char ch) {
  char buf[MAX_ID_LEN];
  fetch_tok(p, buf, ch);
  if (strncmp(buf, str, 2) != 0) {
    p->err = PE_ERR;
    fprintf(stderr, "Error: Expected `%s` found %s\n", str, buf);
    return;
  }
}

void print_interface(Interface* iface) {
  printf("%s Interface %s: ", iface->is_dynamic ? "Dynamic" : "Static", iface->name);
  for (size_t i = 0; i < iface->functions.n; i++) {
    printf("%s", iface->functions.get[i]);
    if (i == iface->functions.n - 1)
      printf("\n");
    else
      printf(", ");
  }
}

void parse_interface(Parser* p) {
  int ch = skip_unwanted(p);
  ostr name = get_tok(p, ch);

  ch = skip_unwanted(p);
  expect(p, "as", ch);
  ch = skip_unwanted(p);

  char type[MAX_ID_LEN];
  fetch_tok(p, type, ch);

  bool is_dynamic;
  if (strcmp(type, "Dynamic") == 0)
    is_dynamic = true;
  else if (strcmp(type, "Static") == 0)
    is_dynamic = false;
  else {
    p->err = PE_ERR;
    fprintf(stderr, "Error: Expected `Dynamic` or `Static` found %s\n", type);
    return;
  }

  ch = skip_unwanted(p);
  expect(p, "{", ch);
  ch = skip_unwanted(p);

  Interface iface = {};
  iface.is_dynamic = is_dynamic;
  iface.name = name;

  while (ch != '}') {
    vec_push(iface.functions, get_tok(p, ch));
    ch = skip_unwanted(p);
  }

  vec_push(p->interfaces, iface);
  return;
}

void parse_pair(Parser* p, ImplKVPair* pair, int* ch) {
  ostr key = get_tok(p, *ch);
  *ch = skip_unwanted(p);

  expect(p, "=", *ch);
  *ch = skip_unwanted(p);

  ostr val = get_tok(p, *ch);

  pair->key = key;
  pair->val = val;
}

Impl* find_impl(Parser* p, Interface* iface, bstr name) {
  for (size_t i = 0; i < iface->impls.n; i++) {
    Impl* impl = &iface->impls.get[i];
    if (strcmp(impl->name, name) == 0)
      return impl;
  }
  return NULL;
}

Interface* find_iface(Parser* p, bstr name) {
  for (size_t i = 0; i < p->interfaces.n; i++) {
    Interface* iface = &p->interfaces.get[i];
    if (strcmp(iface->name, name) == 0)
      return iface;
  }
  return NULL;
}

// Position in relative order in which a function is defined in the interface
int find_fn_id(Interface* iface, bstr name) {
  for (int i = 0; i < iface->functions.n; i++) {
    if (strcmp(iface->functions.get[i], name) == 0)
      return i;
  }
  return -1;
}

void print_impl(Impl* impl) {
  printf("Impl: %s with header %s and pairs: ", impl->name, impl->header);
  for (size_t i = 0; i < impl->pairs.n; i++) {
    printf("%s:%s", impl->pairs.get[i].key, impl->pairs.get[i].val);
    if (i != impl->pairs.n - 1)
      printf(", ");
    else
      printf("\n");
  }
}

void parse_impl(Parser* p) {
  int ch = skip_unwanted(p);
  ostr name = get_tok(p, ch);

  ch = skip_unwanted(p);
  expect(p, "as", ch);
  ch = skip_unwanted(p);

  char iface_name[MAX_ID_LEN];
  fetch_tok(p, iface_name, ch);

  Interface* iface = find_iface(p, iface_name);
  if (iface == NULL) {
    fprintf(stderr, "Error: Interface %s doesn't exist\n", iface_name);
    p->err = PE_ERR;
    return;
  }

  ch = skip_unwanted(p);
  expect(p, "{", ch);
  ch = skip_unwanted(p);

  ImplKVPair header_pair;
  parse_pair(p, &header_pair, &ch);

  if (strcmp(header_pair.key, "$header") != 0) {
    fprintf(stderr, "Error: Expected `$header = none` or `$header = \"xxxx.h\", found %s\n`",
            header_pair.key);
    p->err = PE_ERR;
    return;
  }
  if (strcmp(header_pair.val, "none") == 0)
    header_pair.val = NULL;
  else if (ch != '"' && header_pair.val[strlen(header_pair.val) - 1] != '"') {
    fprintf(stderr, "Header file should be in double-quotes \"xxx.h\", not %s\n", header_pair.val);
    p->err = PE_ERR;
    return;
  }

  Impl impl = {};
  impl.header = header_pair.val;
  impl.name = name;
  vec_resize(impl.pairs, iface->functions.n);

  while (true) {
    ch = skip_unwanted(p);
    if (ch == '}')
      break;

    ImplKVPair pair;
    parse_pair(p, &pair, &ch);

    int id = find_fn_id(iface, pair.key);
    if (id < 0) {
      fprintf(
          stderr,
          "Error: function %s not defined in interface %s but referenced in Implementation %s\n",
          pair.key, iface->name, impl.name);
      p->err = PE_ERR;
      return;
    }
    impl.pairs.get[id] = pair;
    impl.pairs.n++;
  }

  vec_push(iface->impls, impl);
}

void print_export(ExportCmd* cmd) {
  printf("Export: %s as %s by default\n", cmd->iface->name, cmd->impl->name);
}

void parse_export(Parser* p) {
  int ch = skip_unwanted(p);

  char iface_name[MAX_ID_LEN];
  fetch_tok(p, iface_name, ch);

  Interface* iface = find_iface(p, iface_name);
  if (iface == NULL) {
    fprintf(stderr, "Error: Interface %s doesn't exist\n", iface_name);
    p->err = PE_ERR;
    return;
  }

  ch = skip_unwanted(p);
  expect(p, "as", ch);
  ch = skip_unwanted(p);

  char impl_name[MAX_ID_LEN];
  fetch_tok(p, impl_name, ch);
  Impl* impl = find_impl(p, iface, impl_name);
  if (impl == NULL) {
    p->err = PE_ERR;
    fprintf(stderr, "Error: Implemetation %s doesn't exist\n", impl_name);
    return;
  }

  ExportCmd export;
  export.iface = iface;
  export.impl = impl;
  vec_push(p->exports, export);
}

void parse_keyw(Parser* p, bstr keyw) {
#define X(str, fn)                                                                                 \
  if (strcmp(str, keyw) == 0) {                                                                    \
    return fn(p);                                                                                  \
  }
  DISPATCH_TABLE(X)
#undef X
  fprintf(stderr, "Invalid keyword: %s\n", keyw);
  p->err = PE_ERR;
}

void parse_conf(Parser* p) {
  int ch = skip_unwanted(p);
  while (true) {
    if (ch == EOF)
      break;
    if (ch != '$') {
      fprintf(stderr, "Invalid character found in config: %c\n", ch);
      p->err = PE_ERR;
      return;
    }

    char buf[MAX_ID_LEN];
    fetch_tok(p, buf, fgetc(p->file));
    parse_keyw(p, buf);
    ch = skip_unwanted(p);
  }
  return;
}

void read_conf(Parser* p, bstr confpath, VMEMArena* arena) {
  *p = (Parser){0};
  p->arena = arena;
  p->file = fopen(confpath, "r");
  if (!p->file) {
    fprintf(stderr, "Couldnt open file: %s\n", confpath);
    p->err = PE_ERR;
    return;
  }
  parse_conf(p);
  fclose(p->file);
}

void parser_destroy(Parser* p) {
  vec_destroy(p->exports);
  for (size_t i = 0; i < p->interfaces.n; i++) {
    Interface* iface = &p->interfaces.get[i];
    vec_destroy(iface->functions);
    for (size_t i = 0; i < iface->impls.n; i++)
      vec_destroy(iface->impls.get[i].pairs);
    vec_destroy(iface->impls);
  }
  vec_destroy(p->interfaces);
}
