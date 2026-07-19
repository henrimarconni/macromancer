#include "diagnostics.h"
#include "parser.h"
#include "span.h"
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

Span get_tok(Parser* p) {
  skip_unwanted(p);
  Span span = {p->pos.row, p->pos.col, 0, &p->source[p->pos.id]};
  bstr str = &p->source[p->pos.id];
  char ch = peekch(p);
  int len = 0;
  while (!isspace(ch) && ch != EOF) {
    nextch(p);
    ch = peekch(p);
    len++;
  }
  assert(len != 0);
  span.len = len;
  skip_unwanted(p);
  return span;
}

void expect(Parser* p, bstr str) {
  Span span = get_tok(p);
  if (!span_str_cmp(span, str))
    throw_error(p, ERR_UNEXPECTED_TOK, str, span);
}

void parse_interface(Parser* p) {
  Span name = get_tok(p);
  expect(p, "as");
  Span type = get_tok(p);

  bool is_dynamic;
  if (span_str_cmp(type, "Dynamic"))
    is_dynamic = true;
  else if (span_str_cmp(type, "Static"))
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
  Span key = get_tok(p);
  expect(p, "=");
  Span val = get_tok(p);
  pair->key = key;
  pair->val = val;
}

Impl* find_impl(Parser* p, Interface* iface, Span name) {
  for (size_t i = 0; i < iface->impls.n; i++) {
    Impl* impl = &iface->impls.get[i];
    if (span_cmp(impl->name, name))
      return impl;
  }
  return NULL;
}

Interface* find_iface(Parser* p, Span name) {
  for (size_t i = 0; i < p->interfaces.n; i++) {
    Interface* iface = &p->interfaces.get[i];
    if (span_cmp(iface->name, name))
      return iface;
  }
  return NULL;
}

// Position in relative order in which a function is defined in the interface
int find_fn_id(Parser* p, Interface* iface, Span name) {
  for (size_t i = 0; i < iface->functions.n; i++) {
    if (span_cmp(iface->functions.get[i], name))
      return i;
  }
  return -1;
}

void parse_impl(Parser* p) {
  Span name = get_tok(p);
  expect(p, "as");

  Span iface_name = get_tok(p);

  Interface* iface = find_iface(p, iface_name);
  if (iface == NULL)
    throw_error(p, ERR_INTERFACE_DOESNT_EXIST, iface_name);
  expect(p, "{");

  ImplKVPair header_pair;
  parse_pair(p, &header_pair);

  if (!span_str_cmp(header_pair.key, "$header"))
    throw_error(p, ERR_UNEXPECTED_TOK, "`$header = none` or `$header = \"xxxxx.h\"`",
                header_pair.key);
  if (span_str_cmp(header_pair.val, "none"))
    header_pair.val.len = 0;
  else if (header_pair.val.str[0] != '"' || header_pair.val.str[header_pair.val.len - 1] != '"')
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

    int id = find_fn_id(p, iface, pair.key);
    if (id < 0)
      throw_error(p, ERR_FN_NOT_DEFINED_BUT_REFERENCED, pair.key, iface->name, impl.name);
    impl.pairs.get[id] = pair;
    impl.pairs.n++;
  }

  vec_push(iface->impls, impl);
}

void add_export_cli(Parser* p, bstr iface_name, bstr impl_name) {
  int dup_id = -1;
  Span iface_span, impl_span;
  for (int i = 0; i < p->exports.n; i++) {
    ExportCmd cmd = p->exports.get[i];
    if (span_str_cmp(cmd.iface->name, iface_name)) {
      add_note(p, NOTE_OVERRIDING_EXPORT_CLI, cmd.iface->name, cmd.impl->name, iface_name,
               impl_name);
      dup_id = i;
    }
  }

  Interface* iface = find_iface(p, str_to_span(iface_name));
  if (iface == NULL)
    throw_error(p, ERR_INTERFACE_DOESNT_EXIST, iface_name);

  Impl* impl = find_impl(p, iface, str_to_span(impl_name));
  if (impl == NULL)
    throw_error(p, ERR_IMPL_NOT_DEFINED, impl_name, iface_name);

  ExportCmd export;
  export.iface = iface;
  export.impl = impl;
  if (dup_id < 0)
    vec_push(p->exports, export);
  else
    p->exports.get[dup_id] = export;
}

void add_export_conf(Parser* p, Span iface_name, Span impl_name) {
  int dup_id = -1;
  Span iface_span, impl_span;
  for (int i = 0; i < p->exports.n; i++) {
    ExportCmd cmd = p->exports.get[i];
    if (span_cmp(cmd.iface->name, iface_name)) {
      add_note(p, NOTE_OVERRIDING_EXPORT_CONF, cmd.iface->name, cmd.impl->name, iface_name,
               impl_name);
      dup_id = i;
    }
  }

  Interface* iface = find_iface(p, iface_name);
  if (iface == NULL)
    throw_error(p, ERR_INTERFACE_DOESNT_EXIST, iface_name);

  Impl* impl = find_impl(p, iface, impl_name);
  if (impl == NULL)
    throw_error(p, ERR_IMPL_NOT_DEFINED, impl_name, iface_name);

  ExportCmd export;
  export.iface = iface;
  export.impl = impl;
  if (dup_id < 0)
    vec_push(p->exports, export);
  else
    p->exports.get[dup_id] = export;
}

void parse_export(Parser* p) {
  Span iface_name = get_tok(p);
  expect(p, "as");
  Span impl_name = get_tok(p);
  add_export_conf(p, iface_name, impl_name);
}

void parse_keyw(Parser* p, Span keyw) {
#define X(str, fn)                                                                                 \
  if (span_str_cmp(keyw, str)) {                                                                   \
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
    Span keyw = get_tok(p);
    if (keyw.str[0] != '$')
      throw_error(p, ERR_UNEXPECTED_TOK, "`$interface`, `$export`, `$impl`", keyw);
    parse_keyw(p, advance_span(keyw));
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

void read_conf(Parser* p, bstr confpath, ExportOverrideVec* export_override, VMEMArena* arena,
               jmp_buf* onerror) {
  *p = (Parser){0};
  p->arena = arena;
  p->onerror = onerror;
  read_into(p, confpath);
  parse_conf(p);

  for (size_t i = 0; i < export_override->n; i++) {
    ExportOverride ov = export_override->get[i];
    add_export_cli(p, ov.iface, ov.impl);
  }
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
