#ifndef CONCAT
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#endif

#ifndef LIST_IMPLEMENTATION

#include <stddef.h>

struct LIST_TYPE {
  LIST_ELEMENT *data;
  size_t size;
  size_t capacity;
};

struct LIST_TYPE CONCAT(LIST_TYPE, _create)(void);
void CONCAT(LIST_TYPE, _destroy)(struct LIST_TYPE *list);
void CONCAT(LIST_TYPE, _append)(struct LIST_TYPE *list, LIST_ELEMENT element);
int CONCAT(LIST_TYPE, _find)(struct LIST_TYPE *list, const char *key);

#else

#include <string.h>
#include "xmalloc.h"

struct LIST_TYPE CONCAT(LIST_TYPE, _create)(void)
{
  const size_t init_capacity = 16;
  struct LIST_TYPE list = {
    .data = xmalloc(init_capacity * sizeof(LIST_ELEMENT)),
    .size = 0,
    .capacity = init_capacity,
  };
  return list;
}

void CONCAT(LIST_TYPE, _destroy)(struct LIST_TYPE *list)
{
  xfree(list->data);
}

void CONCAT(LIST_TYPE, _append)(struct LIST_TYPE *list, LIST_ELEMENT element)
{
  if (list->size == list->capacity) {
    list->capacity *= 2;
    list->data = xrealloc(list->data, list->capacity * sizeof(*list->data));
  }
  list->data[list->size++] = element;
}

int CONCAT(LIST_TYPE, _find)(struct LIST_TYPE *list, const char *key)
{
  for (size_t i = 0; i < list->size; ++i) {
    if (strcmp(key, LIST_KEY(list->data[i])) == 0) {
      return (int) i;
    }
  }
  return -1;
}

#endif

#undef CONCAT
#undef CONCAT_
#undef LIST_TYPE
#undef LIST_ELEMENT
#undef LIST_KEY
