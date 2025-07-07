#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/poll.h>

#include <pthread.h>

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#include "error.h"
#include "xmalloc.h"
#include "event.h"

enum {
  POLLFD_INDEX_EVENTFD,
  POLLFD_INDEX_FD_START,
};

struct event_queue {
  struct event        event;
  struct event_queue *next;
  struct event_queue *prev;
};

struct event_loop {
  size_t              numpollfds;
  struct pollfd      *pollfds;
  struct event_queue *front;
  struct event_queue *back;
  pthread_mutex_t     lock;
};

struct event_loop *event_loop_create(int *fds, size_t nfds)
{
  struct event_loop *event_loop = xmalloc(sizeof(*event_loop));

  event_loop->numpollfds = POLLFD_INDEX_FD_START + nfds;
  event_loop->pollfds = xmalloc(event_loop->numpollfds * sizeof(*event_loop->pollfds));
  event_loop->pollfds[POLLFD_INDEX_EVENTFD].fd = eventfd(0, EFD_SEMAPHORE);
  event_loop->pollfds[POLLFD_INDEX_EVENTFD].events = POLLIN;

  for (size_t i = 0; i < nfds; ++i) {
    event_loop->pollfds[i + POLLFD_INDEX_FD_START].fd = fds[i];
    event_loop->pollfds[i + POLLFD_INDEX_FD_START].events = POLLIN;
  }

  event_loop->front = NULL;
  event_loop->back = NULL;
  event_loop->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

  return event_loop;
}

void event_loop_destroy(struct event_loop *event_loop)
{
  close(event_loop->pollfds[POLLFD_INDEX_EVENTFD].fd);
}

static struct event event_loop_dequeue_event(struct event_loop *event_loop);

struct event event_loop_poll(struct event_loop *event_loop)
{
  struct event event;

  if (poll(event_loop->pollfds, event_loop->numpollfds, -1) < 0) {
    event.type = EVENT_ERROR;
    event.error = printf_alloc("poll() failed: %s", strerror(errno));
    return event;
  }

  if (event_loop->pollfds[POLLFD_INDEX_EVENTFD].revents & POLLIN) {
    event_loop->pollfds[POLLFD_INDEX_EVENTFD].revents &= ~POLLIN;
    return event_loop_dequeue_event(event_loop);
  }

  for (size_t i = POLLFD_INDEX_FD_START; i < event_loop->numpollfds; ++i) {
    if (event_loop->pollfds[i].revents & POLLIN) {
      event_loop->pollfds[i].revents &= ~POLLIN;
      event.type = EVENT_FD;
      event.fd = event_loop->pollfds[i].fd;
      return event;
    }
  }

  assert(0 && "reached end of event_loop_poll");
}

void event_loop_enqueue_event(struct event_loop *event_loop, struct event *event)
{
  event_loop_lock(event_loop);

  struct event_queue *entry = xmalloc(sizeof(*entry));
  memcpy(&entry->event, event, sizeof(struct event));
  entry->next = NULL;
  entry->prev = NULL;

  if (event_loop->back) {
    event_loop->back->prev = entry;
    entry->next = event_loop->back;
    event_loop->back = entry;
  } else {
    event_loop->back = entry;
    event_loop->front = entry;
  }

  int64_t eventfd_value = 1;
  ssize_t unused = write(event_loop->pollfds[POLLFD_INDEX_EVENTFD].fd,
                         &eventfd_value, sizeof(eventfd_value));
  (void) unused;

  event_loop_unlock(event_loop);
}

static struct event event_loop_dequeue_event(struct event_loop *event_loop)
{
  struct event event;

  event_loop_lock(event_loop);

  assert(event_loop->front && "event_loop_dequeue_event called with empty event queue");
  memcpy(&event, &event_loop->front->event, sizeof(struct event));

  if (event_loop->front->prev) {
    struct event_queue *tmp = event_loop->front;
    event_loop->front = event_loop->front->prev;
    event_loop->front->next = NULL;
    xfree(tmp);
  } else {
    xfree(event_loop->front);
    event_loop->back = NULL;
    event_loop->front = NULL;
  }

  int64_t eventfd_value;
  ssize_t unused = read(event_loop->pollfds[POLLFD_INDEX_EVENTFD].fd, &eventfd_value, sizeof(eventfd_value));
  (void) unused;

  event_loop_unlock(event_loop);

  return event;
}

void event_loop_lock(struct event_loop *event_loop)
{
  pthread_mutex_lock(&event_loop->lock);
}

void event_loop_unlock(struct event_loop *event_loop)
{
  pthread_mutex_unlock(&event_loop->lock);
}

