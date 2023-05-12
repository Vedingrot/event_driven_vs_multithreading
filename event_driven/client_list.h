#ifndef CLIENT_LIST_SENTRY_
#define CLIENT_LIST_SENTRY_

#include "client_sess.h"

#include <sys/select.h>

struct client_list {
  struct client_sess *s;
  struct client_list *next;
};

void insert_to_client_list(struct client_list **list,
                           struct client_sess *sess);

void delete_certain_sess_from_list(struct client_list **list);

void clean_client_list(struct client_list **list);

int max_fd_from_client_list(const struct client_list *list);

void from_client_list_to_rfds(const struct client_list *l, fd_set *s);

void from_client_list_to_wfds(const struct client_list *l, fd_set *s);

void accept_new_client(struct client_list **l, int listening_sd);

#endif /* CLIENT_LIST_SENTRY_ */
