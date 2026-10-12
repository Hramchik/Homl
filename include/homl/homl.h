#ifndef HOML_H_INCLUDED
#define HOML_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#if defined(HOML_STATIC)
#define HOML_API
#elif defined(_WIN32)
#if defined(HOML_BUILDING)
#define HOML_API __declspec(dllexport)
#else
#define HOML_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define HOML_API __attribute__((visibility("default")))
#else
#define HOML_API
#endif

/* Ни одна из функций ниже не выпускает исключение: для C++ вызывающих это
 * выражено в типе, для C макрос пуст. */
#ifdef __cplusplus
#define HOML_NOEXCEPT noexcept
#else
#define HOML_NOEXCEPT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define HOML_ABI_VERSION 0u

HOML_API uint32_t homl_abi_version(void) HOML_NOEXCEPT;

typedef enum homl_status {
  HOML_OK = 0,

  HOML_ERR_PARSE = 1,

  HOML_ERR_NOT_FOUND = 2,

  HOML_ERR_NULL_VALUE = 3,

  HOML_ERR_TYPE = 4,    /* запрошен не тот тип значения            */
  HOML_ERR_RANGE = 5,   /* индекс вне диапазона  */
  HOML_ERR_BUFFER = 6,  /* буфер вызывающего мал */
  HOML_ERR_DEPTH = 7,   /* превышен лимит вложенности              */
  HOML_ERR_INVALID = 8, /* нарушен контракт вызова  */
  HOML_ERR_OOM = 9,
  HOML_ERR_INTERNAL = 10
} homl_status;

HOML_API const char *homl_status_message(homl_status status) HOML_NOEXCEPT;

typedef enum homl_kind {
  HOML_KIND_NULL = 0,
  HOML_KIND_BOOL,
  HOML_KIND_INT, /* int64*/
  HOML_KIND_DOUBLE,
  HOML_KIND_STRING,
  HOML_KIND_ARRAY,
  HOML_KIND_OBJECT
} homl_kind;

typedef struct homl_str {
  const char *ptr;
  size_t len;
} homl_str;

typedef struct homl_document homl_document; /* владеющий указатель       */
typedef struct homl_node homl_node;         /* указатель внутрь документа */

typedef struct homl_parse_options {
  size_t struct_size;
  uint32_t max_depth;
} homl_parse_options;

HOML_API homl_status homl_parse(const char *text, size_t len,
                                const char *filename,
                                homl_document **out_doc) HOML_NOEXCEPT;

HOML_API homl_status homl_parse_ex(const char *text, size_t len,
                                   const char *filename,
                                   const homl_parse_options *options,
                                   homl_document **out_doc) HOML_NOEXCEPT;

HOML_API void homl_document_free(homl_document *doc) HOML_NOEXCEPT;

typedef struct homl_diag {
  homl_str message;
  homl_str hint;
  homl_str snippet;
  size_t offset;
  size_t length;
  uint32_t line;
  uint32_t column;
} homl_diag;

HOML_API homl_str homl_document_filename(const homl_document *doc)
    HOML_NOEXCEPT;
HOML_API size_t homl_diag_count(const homl_document *doc) HOML_NOEXCEPT;
HOML_API homl_status homl_diag_at(const homl_document *doc, size_t index,
                                  homl_diag *out_diag) HOML_NOEXCEPT;

HOML_API const homl_node *homl_root(const homl_document *doc) HOML_NOEXCEPT;

HOML_API homl_kind homl_kind_of(const homl_node *node) HOML_NOEXCEPT;

HOML_API int homl_is_optional(const homl_node *node) HOML_NOEXCEPT;

HOML_API homl_status homl_path(const homl_node *node, const char *path,
                               size_t path_len,
                               const homl_node **out_node) HOML_NOEXCEPT;

HOML_API homl_status homl_child(const homl_node *node, const char *key,
                                size_t key_len,
                                const homl_node **out_node) HOML_NOEXCEPT;

HOML_API size_t homl_count(const homl_node *node) HOML_NOEXCEPT;

HOML_API homl_status homl_at(const homl_node *node, size_t index,
                             const homl_node **out_node) HOML_NOEXCEPT;

HOML_API homl_status homl_key_at(const homl_node *node, size_t index,
                                 homl_str *out_key) HOML_NOEXCEPT;

HOML_API homl_status homl_as_bool(const homl_node *node,
                                  int *out_value) HOML_NOEXCEPT;
HOML_API homl_status homl_as_i64(const homl_node *node,
                                 int64_t *out_value) HOML_NOEXCEPT;
HOML_API homl_status homl_as_double(const homl_node *node,
                                    double *out_value) HOML_NOEXCEPT;
HOML_API homl_status homl_as_str(const homl_node *node,
                                 homl_str *out_value) HOML_NOEXCEPT;

HOML_API homl_status homl_format(const homl_document *doc, char *buffer,
                                 size_t capacity,
                                 size_t *out_needed) HOML_NOEXCEPT;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HOML_H_INCLUDED */
