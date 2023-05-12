#include "client_list.h"

#include <stdlib.h>
#include <sys/select.h>

void
insert_to_client_list(struct client_list **l, struct client_sess *s)
{
  struct client_list *tmp = malloc(sizeof(*tmp));
  tmp->s = s;
  tmp->next = *l;
  *l = tmp;
}

void
delete_certain_sess_from_list(struct client_list **l)
{
  struct client_list *tmp = *l;
  close_session(&(*l)->s);
  *l = (*l)->next;
  free(tmp);
}

void
clean_client_list(struct client_list **l)
{
  while(*l) {
    delete_certain_sess_from_list(l);
  }
}

int
max_fd_from_client_list(const struct client_list *l)
{
  int max = -1;
  for(; l; l = l->next) {
    if(l->s->sd > max) {
      max = l->s->sd;
    }
  }
  return max;
}

void
from_client_list_to_rfds(const struct client_list *l, fd_set *s)
{
  for(; l; l = l->next) {
    FD_SET(l->s->sd, s);
  }
}

void
from_client_list_to_wfds(const struct client_list *l, fd_set *s)
{
  for(; l; l = l->next) {
    switch(l->s->state) {
      case clst_wait:
      case clst_up:
      case clst_down:
      case clst_error:
      case clst_finish:
        FD_CLR(l->s->sd, s);
        break;
      case clst_print_ok:
      case clst_show:
      case clst_unknown:
        FD_SET(l->s->sd, s);
        break;
    }
  }
}

void
accept_new_client(struct client_list **l, int ls)
{
  struct client_sess *s = make_new_sess(ls);
  if(s) {
    insert_to_client_list(l, s);
  }
}
