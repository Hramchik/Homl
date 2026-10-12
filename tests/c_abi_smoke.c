/* c_abi_smoke.c — проверка границы ABI со стороны чистого C.
 *
 * TODO — они включаются по мере прохождения этапов 04-07 ТЗ.
 */
#include "homl/homl.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(int ok, const char *expr, int line) {
  if (!ok) {
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, line, expr);
    ++failures;
  }
}

static void check_status(homl_status actual, homl_status expected,
                         const char *expr, int line) {
  if (actual != expected) {
    fprintf(stderr, "FAIL %s:%d: %s дал \"%s\", ожидалось \"%s\"\n", __FILE__,
            line, expr, homl_status_message(actual),
            homl_status_message(expected));
    ++failures;
  }
}

#define CHECK(cond) check((cond), #cond, __LINE__)
#define CHECK_ST(call, exp) check_status((call), (exp), #call, __LINE__)

static int str_eq(homl_str s, const char *literal) {
  const size_t n = strlen(literal);
  return s.len == n && (n == 0 || memcmp(s.ptr, literal, n) == 0);
}

static void test_abi_version(void) {
  CHECK(homl_abi_version() == HOML_ABI_VERSION);
  CHECK(homl_status_message(HOML_OK) != NULL);
  CHECK(homl_status_message((homl_status)9999) != NULL);
}

static void test_null_contract(void) {
  homl_document *doc = NULL;
  homl_diag diag;
  const homl_node *node = NULL;
  int64_t i64 = 0;

  CHECK_ST(homl_parse("x", 1, NULL, NULL), HOML_ERR_INVALID);
  CHECK_ST(homl_parse(NULL, 5, NULL, &doc), HOML_ERR_INVALID);
  CHECK(doc == NULL);

  homl_document_free(NULL);

  CHECK(homl_root(NULL) == NULL);
  CHECK(homl_kind_of(NULL) == HOML_KIND_NULL);
  CHECK(homl_is_optional(NULL) == 0);
  CHECK(homl_count(NULL) == 0);
  CHECK(homl_diag_count(NULL) == 0);
  CHECK(str_eq(homl_document_filename(NULL), ""));

  CHECK_ST(homl_diag_at(NULL, 0, &diag), HOML_ERR_INVALID);
  CHECK_ST(homl_path(NULL, "a", 1, &node), HOML_ERR_INVALID);
  CHECK_ST(homl_as_i64(NULL, &i64), HOML_ERR_INVALID);
  CHECK_ST(homl_format(NULL, NULL, 0, NULL), HOML_ERR_INVALID);
}

static void test_document_ownership(void) {
  static const char source[] = "host: \"localhost\"\nport: 8080\n";

  homl_document *doc = NULL;
  homl_diag diag;

  CHECK_ST(homl_parse(source, strlen(source), "smoke.homl", &doc),
           HOML_ERR_PARSE);
  CHECK(doc != NULL);
  if (doc == NULL) {
    return;
  }

  CHECK(str_eq(homl_document_filename(doc), "smoke.homl"));
  CHECK(homl_diag_count(doc) == 1);

  CHECK_ST(homl_diag_at(doc, 0, &diag), HOML_OK);
  CHECK(diag.message.len > 0);
  CHECK(diag.line == 1);
  CHECK(diag.column == 1);
  CHECK(str_eq(diag.snippet, "host: \"localhost\""));

  CHECK_ST(homl_diag_at(doc, 1, &diag), HOML_ERR_RANGE);
  CHECK_ST(homl_diag_at(doc, 0, NULL), HOML_ERR_INVALID);

  homl_document_free(doc);
}

static void test_tree_access(void) {
  static const char source[] = "";

  homl_document *doc = NULL;
  const homl_node *root = NULL;
  const homl_node *node = NULL;
  homl_str key;
  int64_t i64 = 0;

  CHECK_ST(homl_parse(source, 0, "empty.homl", &doc), HOML_ERR_PARSE);
  CHECK(doc != NULL);
  if (doc == NULL) {
    return;
  }

  root = homl_root(doc);
  CHECK(root != NULL);
  CHECK(homl_kind_of(root) == HOML_KIND_OBJECT);
  CHECK(homl_is_optional(root) == 0);
  CHECK(homl_count(root) == 0);

  CHECK_ST(homl_path(root, "host", 4, &node), HOML_ERR_NOT_FOUND);
  CHECK(node == NULL);
  CHECK_ST(homl_child(root, "host", 4, &node), HOML_ERR_NOT_FOUND);

  CHECK_ST(homl_path(root, "", 0, &node), HOML_ERR_INVALID);
  CHECK_ST(homl_path(root, "a..b", 4, &node), HOML_ERR_INVALID);
  CHECK_ST(homl_path(root, ".host", 5, &node), HOML_ERR_INVALID);

  CHECK_ST(homl_as_i64(root, &i64), HOML_ERR_TYPE);

  CHECK_ST(homl_at(root, 0, &node), HOML_ERR_RANGE);
  CHECK_ST(homl_key_at(root, 0, &key), HOML_ERR_RANGE);

  homl_document_free(doc);
}

static void test_format_buffer_protocol(void) {
  homl_document *doc = NULL;
  char buffer[256];
  size_t needed = (size_t)-1;

  CHECK_ST(homl_parse("", 0, "empty.homl", &doc), HOML_ERR_PARSE);
  CHECK(doc != NULL);
  if (doc == NULL) {
    return;
  }

  CHECK_ST(homl_format(doc, NULL, 0, &needed), HOML_ERR_BUFFER);
  CHECK(needed != (size_t)-1);
  CHECK(needed + 1 <= sizeof buffer);

  if (needed + 1 <= sizeof buffer) {
    CHECK_ST(homl_format(doc, buffer, needed, &needed), HOML_ERR_BUFFER);
    CHECK_ST(homl_format(doc, buffer, needed + 1, &needed), HOML_OK);
    CHECK(strlen(buffer) == needed);
  }

  CHECK_ST(homl_format(doc, NULL, 1, &needed), HOML_ERR_INVALID);

  homl_document_free(doc);
}

int main(void) {
  test_abi_version();
  test_null_contract();
  test_document_ownership();
  test_tree_access();
  test_format_buffer_protocol();

  if (failures != 0) {
    fprintf(stderr, "\nc_abi_smoke: провалено проверок: %d\n", failures);
    return 1;
  }
  printf("c_abi_smoke: граница ABI в порядке (ABI v%" PRIu32 ")\n",
         homl_abi_version());
  return 0;
}