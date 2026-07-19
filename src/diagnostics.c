#include "diagnostics.h"
#include "parser.h"
#include "span.h"
#include "stringdef.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <threads.h>

#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
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

void print_str(bstr str) {
  while (*str)
    putchar(*str++);
}

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

void print_n_spaces(int n) {
  while (n--)
    putchar(' ');
}

void print_line_start(int offset1) {
  print_n_spaces(offset1);
  printf("| ");
}

[[noreturn]]
void _throw_error(Parser* p, Span span, ErrorType type, ...) {
  printf(ANSI_RED "Error: " ANSI_RESET);
  va_list args;
  va_start(args, type);
  vformat(errtype_to_msg[type], args);
  va_end(args);
  putchar('\n');

  if (span.str) {
    char line_no[32];
    int offset1 = snprintf(line_no, sizeof(line_no), "%zu ", span.row);

    print_line_start(offset1);
    putchar('\n');

    printf("%s| ", line_no);
    ptrdiff_t id = span.str - p->source;
    size_t i = span.col;
    char ch = p->source[id - i];
    while ((ch = p->source[id - i]), i--)
      putchar(ch);
    printf(ANSI_YELLOW "%.*s" ANSI_RESET, (int)span.len, span.str);
    putchar('\n');

    print_line_start(offset1);
    print_n_spaces(span.col);

    if (span.len > 0) {
      putchar('^');
      size_t tildes = span.len - 1;
      while (tildes--) {
        putchar('~');
      }
    }
    putchar('\n');
  }
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
