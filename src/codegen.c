#include "codegen.h"
#include "parser.h"
#include "vec.h"

void append_str(StringBuilder* b, bstr str) {
  while (*str) {
    vec_push(*b, *(str++));
  }
}

int export_static(Codegen* c, ExportCmd* cmd) { return 0; }

int export_dynamic(Codegen* c, ExportCmd* cmd) { return 0; }

int export(Codegen* c, ExportCmd* cmd) {
  bstr header = cmd->impl->header;
  if (header != NULL) {
    append_str(&c->output, "#include \"");
    append_str(&c->output, header);
    append_str(&c->output, "\"\n");
  }

  if (cmd->iface->is_dynamic)
    return export_dynamic(c, cmd);
  else
    return export_static(c, cmd);
}

int generate_code(Codegen* c, Parser* p) {
  *c = (Codegen){0};
  c->parser = p;

  for (size_t i = 0; i < p->exports.n; i++) {
    if (export(c, &p->exports.get[i]) < 0)
      return -1;
  }
  return 0;
}
