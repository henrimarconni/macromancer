#include "diagnostics.h"
#include "parser.h"
#include "vec.h"
#include "vmem_arena.h"
#include <assert.h>
#include <ctype.h>
#include <setjmp.h>
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

int nextch(Parser* p) {
  if (p->source[p->pos.id] == '\0')
    return EOF;
  char ch = p->source[p->pos.id++];
  if (ch == '\n') {
    p->pos.row++;
    p->pos.col = 0;
  } else
    p->pos.col++;
  return ch;
}

int peekch(Parser* p) {
  if (p->source[p->pos.id] == '\0')
    return EOF;
  return p->source[p->pos.id];
}

void skip_comment(Parser* p) {
  int ch = peekch(p);
  if (ch == '#') {
    ch = peekch(p);
    while (ch != '\n' && ch != EOF) {
      ch = nextch(p);
      ch = peekch(p);
    }
  }
}

void skip_space(Parser* p) {
  int ch = peekch(p);
  while (isspace(ch) && ch != EOF) {
    ch = nextch(p);
    ch = peekch(p);
  }
}

void skip_unwanted(Parser* p) {
  int ch = peekch(p);
  while (ch != EOF && (isspace(ch) || ch == '#')) {
    skip_comment(p);
    skip_space(p);
    ch = peekch(p);
  }
}

bstr get_tok(Parser* p) {
  skip_unwanted(p);
  bstr str = &p->source[p->pos.id];
  char ch = peekch(p);
  int len = 0;
  while (!isspace(ch) && ch != EOF) {
    nextch(p);
    ch = peekch(p);
    len++;
  }
  assert(len != 0);
  skip_unwanted(p);
  ostr cpy = vmarena_alloc(p->arena, len + 1);
  cpy[len] = '\0';
  memcpy(cpy, str, len);
  return cpy;
}

void expect(Parser* p, bstr str) {
  bstr buf = get_tok(p);
  if (strcmp(buf, str) != 0)
    throw_error(p, ERR_UNEXPECTED_TOK, str, buf);
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
  ostr name = get_tok(p);
  expect(p, "as");
  bstr type = get_tok(p);

  bool is_dynamic;
  if (strcmp(type, "Dynamic") == 0)
    is_dynamic = true;
  else if (strcmp(type, "Static") == 0)
    is_dynamic = false;
  else
    throw_error(p, ERR_UNEXPECTED_TOK, "`Dynamic` or `Static`", type);

  expect(p, "{");

  Interface iface = {};
  iface.is_dynamic = is_dynamic;
  iface.name = name;

  while (true) {
    vec_push(iface.functions, get_tok(p));
    skip_unwanted(p);
    int ch = peekch(p);
    if (ch == EOF)
      throw_error(p, ERR_UNEXPECTED_TOK, "}", "End of File");
    else if (ch == '}')
      break;
  }
  nextch(p); // skip }

  vec_push(p->interfaces, iface);
  return;
}

void parse_pair(Parser* p, ImplKVPair* pair) {
  ostr key = get_tok(p);
  expect(p, "=");
  ostr val = get_tok(p);
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
  for (size_t i = 0; i < iface->functions.n; i++) {
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
  ostr name = get_tok(p);
  expect(p, "as");

  bstr iface_name = get_tok(p);

  Interface* iface = find_iface(p, iface_name);
  if (iface == NULL)
    throw_error(p, ERR_INTERFACE_DOESNT_EXIST, iface_name);
  expect(p, "{");

  ImplKVPair header_pair;
  parse_pair(p, &header_pair);

  if (strcmp(header_pair.key, "$header") != 0)
    throw_error(p, ERR_UNEXPECTED_TOK, "`$header = none` or `$header = \"xxxxx.h\"`",
                header_pair.key);
  if (strcmp(header_pair.val, "none") == 0)
    header_pair.val = NULL;
  else if (header_pair.val[0] != '"' || header_pair.val[strlen(header_pair.val) - 1] != '"')
    throw_error(p, ERR_HEADER_FILE_NOT_IN_DOUBLE_QUOTES, header_pair.val);

  Impl impl = {};
  impl.header = header_pair.val;
  impl.name = name;
  vec_resize(impl.pairs, iface->functions.n);

  while (true) {
    if (peekch(p) == '}') {
      nextch(p);
      break;
    }
    ImplKVPair pair;
    parse_pair(p, &pair);

    int id = find_fn_id(iface, pair.key);
    if (id < 0)
      throw_error(p, ERR_FN_NOT_DEFINED_BUT_REFERENCED, pair.key, iface->name, impl.name);
    impl.pairs.get[id] = pair;
    impl.pairs.n++;
  }

  vec_push(iface->impls, impl);
}

void print_export(ExportCmd* cmd) {
  printf("Export: %s as %s by default\n", cmd->iface->name, cmd->impl->name);
}

void parse_export(Parser* p) {
  bstr iface_name = get_tok(p);
  Interface* iface = find_iface(p, iface_name);
  if (iface == NULL)
    throw_error(p, ERR_INTERFACE_DOESNT_EXIST, iface_name);

  expect(p, "as");

  bstr impl_name = get_tok(p);
  Impl* impl = find_impl(p, iface, impl_name);
  if (impl == NULL)
    throw_error(p, ERR_IMPL_NOT_DEFINED, impl_name);

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
  throw_error(p, ERR_INVALID_KEYWORD, keyw);
}

void parse_conf(Parser* p) {
  while (true) {
    if (peekch(p) == EOF)
      break;
    bstr keyw = get_tok(p);
    if (keyw[0] != '$')
      throw_error(p, ERR_UNEXPECTED_TOK, "`$interface`, `$export`, `$impl`", keyw);
    parse_keyw(p, ++keyw);
  }
  return;
}

void read_into(Parser* p, bstr confpath) {
  FILE* file = fopen(confpath, "r");
  if (!file)
    throw_error(p, ERR_CANT_OPEN_FILE, confpath);

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  p->source = vmarena_alloc(p->arena, file_size + 1);
  size_t n = fread(p->source, 1, file_size, file);
  assert(n == file_size);
  p->source[file_size] = '\0';

  fclose(file);
}

void read_conf(Parser* p, bstr confpath, VMEMArena* arena, jmp_buf* onerror) {
  *p = (Parser){0};
  p->arena = arena;
  p->onerror = onerror;
  read_into(p, confpath);

  parse_conf(p);
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
