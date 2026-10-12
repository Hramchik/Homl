#include "homl/homl.h"

#include <cstddef>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace {

constexpr uint32_t kDefaultMaxDepth = 256u;

struct Diag {
  std::string message;
  std::string hint;
  std::string snippet;
  size_t offset = 0;
  size_t length = 0;
  uint32_t line = 0;
  uint32_t column = 0;
};

homl_str slice(const std::string &s) noexcept {
  homl_str out;
  out.ptr = s.data();
  out.len = s.size();
  return out;
}

template <typename Fn> homl_status guard(Fn &&fn) noexcept {
  try {
    return fn();
  } catch (const std::bad_alloc &) {
    return HOML_ERR_OOM;
  } catch (...) {
    return HOML_ERR_INTERNAL;
  }
}

} // namespace

struct homl_node {
  homl_kind kind = HOML_KIND_NULL;
  int optional = 0;
};

struct homl_document {
  std::string text;
  std::string filename;
  std::vector<Diag> diags;
  homl_node root;
};

uint32_t homl_abi_version(void) noexcept { return HOML_ABI_VERSION; }

const char *homl_status_message(homl_status status) noexcept {
  switch (status) {
  case HOML_OK:
    return "ok";
  case HOML_ERR_PARSE:
    return "parse diagnostics present";
  case HOML_ERR_NOT_FOUND:
    return "path not found";
  case HOML_ERR_NULL_VALUE:
    return "value is null";
  case HOML_ERR_TYPE:
    return "wrong value type";
  case HOML_ERR_RANGE:
    return "index or value out of range";
  case HOML_ERR_BUFFER:
    return "buffer too small";
  case HOML_ERR_DEPTH:
    return "nesting depth limit exceeded";
  case HOML_ERR_INVALID:
    return "invalid argument";
  case HOML_ERR_OOM:
    return "out of memory";
  case HOML_ERR_INTERNAL:
    return "internal error";
  }
  return "unknown status";
}

namespace {

homl_status parse_impl(const char *text, size_t len, const char *filename,
                       uint32_t max_depth, homl_document **out_doc) {
  static_cast<void>(max_depth);

  auto doc = std::make_unique<homl_document>();
  doc->text.assign(text ? text : "", len);
  doc->filename = filename ? filename : "<memory>";
  doc->root.kind = HOML_KIND_OBJECT;

  const size_t eol = doc->text.find('\n');
  Diag d;
  d.message = "разбор не реализован: этапы 04-05 ТЗ";
  d.hint = "граница ABI уже на месте, тело parse_impl ещё нет";
  d.snippet =
      doc->text.substr(0, eol == std::string::npos ? doc->text.size() : eol);
  d.offset = 0;
  d.length = 0;
  d.line = 1;
  d.column = 1;
  doc->diags.push_back(std::move(d));

  *out_doc = doc.release();
  return HOML_ERR_PARSE;
}

} // namespace

homl_status homl_parse(const char *text, size_t len, const char *filename,
                       homl_document **out_doc) noexcept {
  return homl_parse_ex(text, len, filename, nullptr, out_doc);
}

homl_status homl_parse_ex(const char *text, size_t len, const char *filename,
                          const homl_parse_options *options,
                          homl_document **out_doc) noexcept {
  if (out_doc == nullptr) {
    return HOML_ERR_INVALID;
  }
  *out_doc = nullptr;
  if (text == nullptr && len != 0) {
    return HOML_ERR_INVALID;
  }

  uint32_t max_depth = kDefaultMaxDepth;
  if (options != nullptr) {
    constexpr size_t need =
        offsetof(homl_parse_options, max_depth) + sizeof(uint32_t);
    if (options->struct_size <
        offsetof(homl_parse_options, struct_size) + sizeof(size_t)) {
      return HOML_ERR_INVALID;
    }
    if (options->struct_size >= need && options->max_depth != 0) {
      max_depth = options->max_depth;
    }
  }

  return guard(
      [&] { return parse_impl(text, len, filename, max_depth, out_doc); });
}

void homl_document_free(homl_document *doc) noexcept { delete doc; }

homl_str homl_document_filename(const homl_document *doc) noexcept {
  static const std::string empty;
  return slice(doc ? doc->filename : empty);
}

size_t homl_diag_count(const homl_document *doc) noexcept {
  return doc ? doc->diags.size() : 0u;
}

homl_status homl_diag_at(const homl_document *doc, size_t index,
                         homl_diag *out_diag) noexcept {
  if (doc == nullptr || out_diag == nullptr) {
    return HOML_ERR_INVALID;
  }
  if (index >= doc->diags.size()) {
    return HOML_ERR_RANGE;
  }
  const Diag &d = doc->diags[index];
  out_diag->message = slice(d.message);
  out_diag->hint = slice(d.hint);
  out_diag->snippet = slice(d.snippet);
  out_diag->offset = d.offset;
  out_diag->length = d.length;
  out_diag->line = d.line;
  out_diag->column = d.column;
  return HOML_OK;
}

const homl_node *homl_root(const homl_document *doc) noexcept {
  return doc ? &doc->root : nullptr;
}

homl_kind homl_kind_of(const homl_node *node) noexcept {
  return node ? node->kind : HOML_KIND_NULL;
}

int homl_is_optional(const homl_node *node) noexcept {
  return node ? node->optional : 0;
}

homl_status homl_child(const homl_node *node, const char *key, size_t key_len,
                       const homl_node **out_node) noexcept {
  if (node == nullptr || key == nullptr || key_len == 0 ||
      out_node == nullptr) {
    return HOML_ERR_INVALID;
  }
  *out_node = nullptr;
  if (node->kind != HOML_KIND_OBJECT) {
    return HOML_ERR_TYPE;
  }
  return HOML_ERR_NOT_FOUND;
}

homl_status homl_path(const homl_node *node, const char *path, size_t path_len,
                      const homl_node **out_node) noexcept {
  if (node == nullptr || path == nullptr || path_len == 0 ||
      out_node == nullptr) {
    return HOML_ERR_INVALID;
  }
  *out_node = nullptr;

  size_t begin = 0;
  for (size_t i = 0; i <= path_len; ++i) {
    if (i == path_len || path[i] == '.') {
      if (i == begin) {
        return HOML_ERR_INVALID; /* ".a", "a..b", "a." */
      }
      begin = i + 1;
    }
  }

  const homl_node *cur = node;
  begin = 0;
  for (size_t i = 0; i <= path_len; ++i) {
    if (i != path_len && path[i] != '.') {
      continue;
    }
    const homl_node *next = nullptr;
    const homl_status s = homl_child(cur, path + begin, i - begin, &next);
    if (s != HOML_OK) {
      return s;
    }
    cur = next;
    begin = i + 1;
  }

  *out_node = cur;
  return HOML_OK;
}

size_t homl_count(const homl_node *node) noexcept {
  if (node == nullptr) {
    return 0u;
  }
  return 0u;
}

homl_status homl_at(const homl_node *node, size_t index,
                    const homl_node **out_node) noexcept {
  if (node == nullptr || out_node == nullptr) {
    return HOML_ERR_INVALID;
  }
  *out_node = nullptr;
  if (node->kind != HOML_KIND_ARRAY && node->kind != HOML_KIND_OBJECT) {
    return HOML_ERR_TYPE;
  }
  static_cast<void>(index);
  return HOML_ERR_RANGE;
}

homl_status homl_key_at(const homl_node *node, size_t index,
                        homl_str *out_key) noexcept {
  if (node == nullptr || out_key == nullptr) {
    return HOML_ERR_INVALID;
  }
  if (node->kind != HOML_KIND_OBJECT) {
    return HOML_ERR_TYPE;
  }
  static_cast<void>(index);
  return HOML_ERR_RANGE;
}

namespace {

homl_status check_kind(const homl_node *node, homl_kind expected) noexcept {
  if (node == nullptr) {
    return HOML_ERR_INVALID;
  }
  if (node->kind == HOML_KIND_NULL) {
    return HOML_ERR_NULL_VALUE;
  }
  if (node->kind != expected) {
    return HOML_ERR_TYPE;
  }
  return HOML_OK;
}

} // namespace

homl_status homl_as_bool(const homl_node *node, int *out_value) noexcept {
  if (out_value == nullptr) {
    return HOML_ERR_INVALID;
  }
  return check_kind(node, HOML_KIND_BOOL);
}

homl_status homl_as_i64(const homl_node *node, int64_t *out_value) noexcept {
  if (out_value == nullptr) {
    return HOML_ERR_INVALID;
  }
  return check_kind(node, HOML_KIND_INT);
}

homl_status homl_as_double(const homl_node *node, double *out_value) noexcept {
  if (out_value == nullptr) {
    return HOML_ERR_INVALID;
  }
  return check_kind(node, HOML_KIND_DOUBLE);
}

homl_status homl_as_str(const homl_node *node, homl_str *out_value) noexcept {
  if (out_value == nullptr) {
    return HOML_ERR_INVALID;
  }
  return check_kind(node, HOML_KIND_STRING);
}

homl_status homl_format(const homl_document *doc, char *buffer, size_t capacity,
                        size_t *out_needed) noexcept {
  if (doc == nullptr || out_needed == nullptr) {
    return HOML_ERR_INVALID;
  }
  if (buffer == nullptr && capacity != 0) {
    return HOML_ERR_INVALID;
  }

  return guard([&]() -> homl_status {
    /* Канонический вывод пустого документа — пустой текст.
     * Форматтер появится на этапе 08. */
    const std::string out;

    *out_needed = out.size();
    if (buffer == nullptr || capacity < out.size() + 1) {
      return HOML_ERR_BUFFER;
    }
    if (!out.empty()) {
      out.copy(buffer, out.size());
    }
    buffer[out.size()] = '\0';
    return HOML_OK;
  });
}
