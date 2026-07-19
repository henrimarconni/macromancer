#include "span.h"
#include <stdbool.h>
#include <string.h>

bool span_cmp(Span span1, Span span2) {
  return span1.len == span2.len &&
         (span1.str == span2.str || strncmp(span1.str, span2.str, span1.len));
}

Span str_to_span(bstr str) { return (Span){1, 1, strlen(str), str}; }

bool span_str_cmp(Span span, bstr str) { return strncmp(span.str, str, span.len) == 0; }

// Gives null terminated duplicate
bstr dup_span_buf(Span span, bstr buf) {
  mempcpy(buf, span.str, span.len);
  buf[span.len] = '\0';
  return buf;
}

Span advance_span(Span span) {
  span.str++;
  span.len--;
  return span;
}
