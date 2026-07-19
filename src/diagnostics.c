#include "diagnostics.h"
#include "parser.h"
#include "span.h"
#include "stringdef.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>

#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET "\x1b[0m"

bstr errtype_to_msg[__error_type_len] = {
#define X(_, str) str,
    ERRORS(X)
#undef X
};

bstr notetype_to_msg[__note_type_len] = {
#define X(_, str) str,
    NOTES(X)
#undef X
};

void print_span(Span span) {
  while (span.len--)
    putchar(*span.str++);
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
  printf(ANSI_RED "Error: " ANSI_RESET);
  va_list args;
  va_start(args, type);
  vformat(errtype_to_msg[type], args);
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

void _add_note(Parser* p, NoteType type, ...) {
  printf(ANSI_YELLOW "Note: " ANSI_RESET);
  va_list args;
  va_start(args, type);
  vformat(notetype_to_msg[type], args);
  va_end(args);
  putchar('\n');
}
