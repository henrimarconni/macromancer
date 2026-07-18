#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "parser.h"
#include <stdio.h> // IWYU pragma: keep

#define ERRORS(X) \
X(ERR_UNEXPECTED_TOK, "Expected `%s` found `%s`")\
X(ERR_CANT_OPEN_FILE, "Cannot open file: %s")\
X(ERR_INTERFACE_DOESNT_EXIST, "Interface %s doesn't exist")\
X(ERR_HEADER_FILE_NOT_IN_DOUBLE_QUOTES, "Header file must be in double quotes and shouldn't have spaces: found %s")\
X(ERR_FN_NOT_DEFINED_BUT_REFERENCED, "Function %s is not defined in interface %s but is referenced in implementation %s")\
X(ERR_IMPL_NOT_DEFINED, "Implementation %s is not defined")\
X(ERR_INVALID_KEYWORD, "Invalid Keyword: `%s`")

#define FORMAT_SPECS(X) \
X("%s", print_str, bstr)\
X("%c", putchar, int)

typedef enum {
#define X(a, _) a,
  ERRORS(X)
#undef X
__error_type_len
} ErrorType;

#define throw_error(eng, type, ...)\
do {\
  printf("In %s:%d %s():\n", __FILE__, __LINE__, __func__);\
  _throw_error((eng), (type), __VA_ARGS__);\
} while (0)

void _throw_error(Parser* p, ErrorType type, ...);

#endif
