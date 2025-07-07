#include <string.h>
#include <assert.h>
#include <libgen.h>

#include "../xmalloc.h"
#include "../error.h"
#include "path-stack.h"

const char *path_stack_push(struct path_stack **stack, const char *filename)
{
  struct path_stack *head = xcalloc(1, sizeof(*head));
  char *tmp = strdup(filename);

  if (filename[0] == '/') {
    head->path = strdup(dirname(tmp));
  } else if (*stack) {
    head->path = printf_alloc("%s/%s", (*stack)->path, dirname(tmp));
  } else {
    head->path = printf_alloc("./%s", dirname(tmp));
  }

  head->next = *stack;
  *stack = head;
  xfree(tmp);

  return head->path;
}

void path_stack_pop(struct path_stack **stack)
{
  assert(*stack && "path_stack_pop called on empty stack");

  struct path_stack *tmp = *stack;
  *stack = (*stack)->next;
  xfree(tmp->path);
  xfree(tmp);
}

char *get_path(struct path_stack **stack, const char *filename)
{
  if (*stack == NULL) {
    if (filename[0] == '/') {
      return strdup(filename);
    } else {
      return printf_alloc("./%s", filename);
    }
  }

  return printf_alloc("%s/%s", (*stack)->path, filename);
}

void path_stack_destroy(struct path_stack **stack)
{
  while (*stack) {
    path_stack_pop(stack);
  }
}
