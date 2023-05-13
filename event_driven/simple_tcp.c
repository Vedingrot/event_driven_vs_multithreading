#include "client_sess.h"
#include "client_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

static int
set_reuseaddr_opt_to_socket(int sd)
{
  int res;
  int oval = 1;
  res = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &oval, sizeof(oval));
  if(-1 == res) {
    perror("reuse addr");
    return -1;
  }
  return res;
}

static int
bind_tcp_socket(int sd, int port)
{
  struct sockaddr_in addr;
  int res;
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  res = bind(sd, (struct sockaddr*)&addr, sizeof(addr));
  if(-1 == res) {
    perror("bind");
  }
  return res;
}

static int
create_and_bind_tcp_socket(int port)
{
  int sd, res;
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if(-1 == sd) {
    perror("socket");
    return -1;
  }
  res = set_reuseaddr_opt_to_socket(sd);
  if(-1 == res) {
    close(sd);
    return -1;
  }
  res = bind_tcp_socket(sd, port);
  if(-1 == res) {
    close(sd);
    return -1;
  }
  return sd;
}

int
open_listening_tcp_socket(int port)
{
  enum { backlog = 5 };
  int res, sd;
  sd = create_and_bind_tcp_socket(port);
  if(-1 == sd) {
    return -1;
  }
  res = listen(sd, backlog);
  if(-1 == res) {
    perror("listen");
    close(sd);
    return -1;
  }
  return sd;
}

static void
set_select_arguments(struct client_list *clist,
                                        int ls,
                                   int *max_fd,
                                   fd_set *rfds,
                                   fd_set *wfds)
{
  *max_fd = max_fd_from_client_list(clist);
  *max_fd = *max_fd > ls ? *max_fd : ls;
  FD_ZERO(rfds);
  FD_ZERO(wfds);
  from_client_list_to_rfds(clist, rfds);
  FD_SET(ls, rfds);
  from_client_list_to_wfds(clist, wfds);
}

static int
need_to_close_session(const struct client_sess *s)
{
  return s->state == clst_error || s->state == clst_finish;
}

static void
up_and_down_proccessing(struct client_sess *s, int *counter)
{
  if(s->state == clst_up) {
    (*counter)++;
    s->state = clst_print_ok;
  }
  if(s->state == clst_down) {
    (*counter)--;
    s->state = clst_print_ok;
  }
}

static void
events_processing(struct client_list **clist,
                                      int ls,
                          const fd_set *rfds,
                          const fd_set *wfds,
                                int *counter)
{
  if(FD_ISSET(ls, rfds)) {
    accept_new_client(clist, ls);
  }
  while(*clist) {
    if(FD_ISSET((*clist)->s->sd, wfds)) {
      send_message((*clist)->s, *counter);
    }
    if(FD_ISSET((*clist)->s->sd, rfds)) {
      recieve_data((*clist)->s);
      up_and_down_proccessing((*clist)->s, counter);
    }
    if(need_to_close_session((*clist)->s)) {
      delete_certain_sess_from_list(clist);
    } else {
      clist = &(*clist)->next;
    }
  }
}

int
server_main_loop(int ls)
{
  struct client_list *clist = NULL;
  fd_set rfds, wfds;
  int max_fd, res, counter = 0, retval = 0;
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  for(;;) {
    set_select_arguments(clist, ls, &max_fd, &rfds, &wfds);
    res = select(max_fd+1, &rfds, &wfds, NULL, NULL);
    if(-1 == res) {
      perror("select");
      retval = 3;
      break;
    }
    events_processing(&clist, ls, &rfds, &wfds, &counter);
  }
  clean_client_list(&clist);
  return retval;
}

int
server(int port)
{
  int retval;
  int ls = open_listening_tcp_socket(port);
  if(-1 == ls) {
    return 2;
  }
  retval = server_main_loop(ls);
  close(ls);
  return retval;
}

int
convert_string_to_port(const char *s)
{
  int p;
  char *endp;
  p = strtol(s, &endp, 10);
  if(*endp != '\0') {
    p = -1;
  }
  return p;
}

int
main(int argc, char **argv)
{
  int port;
  if(argc != 2) {
    printf("Usage: simple_tcp.out PORT\n");
    return 1;
  }
  port = convert_string_to_port(argv[1]);
  if(-1 == port) {
    fprintf(stderr, "Incorrect port: %s\n", argv[1]);
    return 1;
  }
  return server(port);
}
