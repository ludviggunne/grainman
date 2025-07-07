#ifndef PATH_STACK_H
#define PATH_STACK_H

struct path_stack {
  char *path;
  struct path_stack *next;
};

const char *path_stack_push(struct path_stack **stack, const char *filename);
void path_stack_pop(struct path_stack **stack);

/* Returned string should be freed by caller */
char *get_path(struct path_stack **stack, const char *filename);

void path_stack_destroy(struct path_stack **stack);

#endif
