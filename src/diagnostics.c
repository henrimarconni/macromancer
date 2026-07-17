#include "diagnostics.h"
#include "parser.h"
#include "stringdef.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>

bstr type_to_msg[__error_type_len] = {
#define X(_, str) str,
    ERRORS(X)
#undef X
};

void print_str(bstr str) {
  while (*str)
    putchar(*str++);
}

void vformat(bstr str, va_list args) {
  while (*str) {
    if (0)
      ;
#define X(fstr, fn, T)                                                                             \
  else if (strncmp(fstr, str, strlen(fstr)) == 0) {                                                \
    str += strlen(fstr);                                                                           \
    fn(va_arg(args, T));                                                                           \
  }
    FORMAT_SPECS(X)
#undef X
    else putchar(*str++);
  }
}

[[noreturn]]
void _throw_error(Parser* p, ErrorType type, ...) {
  va_list args;
  va_start(args, type);
  vformat(type_to_msg[type], args);
  va_end(args);
  putchar('\n');
  printf("In line: ");
  size_t start_id = p->pos.id;
  while (p->source[start_id - 1] != '\n')
    start_id--;
  char ch = p->source[start_id++];
  while (ch != '\0' && ch != '\n') {
    putchar(ch);
    ch = p->source[start_id++];
  }
  putchar('\n');
  longjmp(*p->onerror, -1);
}
