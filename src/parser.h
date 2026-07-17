#ifndef PARSER_H
#define PARSER_H

#include "stringdef.h"
#include "vec.h"
#include "vmem_arena.h"
#include <stdbool.h>
#include <stdio.h>


typedef struct {
  ostr key, val;
} ImplKVPair;

typedef struct {
  bstr name;
  bstr header;
  vec(ImplKVPair) pairs;
} Impl;

typedef struct {
  bstr name;
  vec(bstr) functions;
  vec(Impl) impls;
  bool is_dynamic;
} Interface;

typedef enum {
  PE_OK,
  PE_ERR
} ParserError;

typedef struct {
  Interface* iface;
  Impl* impl;
} ExportCmd;

typedef struct {
  FILE* file;
  vec(Interface) interfaces;
  vec(ExportCmd) exports;
  ParserError err;
  VMEMArena* arena;
} Parser;

void read_conf(Parser* p, bstr confpath, VMEMArena* arena);
void parser_destroy(Parser* p);


#endif
