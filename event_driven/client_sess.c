#include "client_sess.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

enum { cl_bsize = 256 };
enum { cnt_buf_size = 32 };

struct client_sess*
make_new_sess(int ls)
{
  struct client_sess *s = malloc(sizeof(*s));
  s->sd = accept(ls, NULL, NULL);
  if(-1 == s->sd) {
    perror("accept");
    free(s);
  }
  s->buf = malloc(cl_bsize);
  s->state = clst_start;
  return s;
}

void
close_session(struct client_sess **s)
{
  close((*s)->sd);
  free((*s)->buf);
  free(*s);
  *s = NULL;
}

static void
trim_whitespaces(char *cl_buf)
{
  int i, s;
  for(s = 0; isspace(cl_buf[s]); s++) {}
  for(i = 0; cl_buf[s+i] && !isspace(cl_buf[s+i]); i++) {
    cl_buf[i] = cl_buf[s+i];
  }
  cl_buf[i] = '\0';
}

int
recieve_data(struct client_sess *s)
{
  int rc, buf_lost = cl_bsize - sizeof('\0');
  rc = read(s->sd, s->buf, buf_lost);
  if(rc > 0) {
    s->buf[rc] = '\0';
    trim_whitespaces(s->buf);
    analyze_data(s);
  } else if(-1 == rc) {
    s->state = clst_error;
    perror("socket read");
  } else {
    s->state = clst_finish;
  }
  return rc;
}

void
analyze_data(struct client_sess *s)
{
  if(strcmp(s->buf, "up") == 0) {
    s->state = clst_up;
  } else if(strcmp(s->buf, "down") == 0) {
    s->state = clst_down;
  } else if(strcmp(s->buf, "show") == 0) {
    s->state = clst_show;
  } else {
    s->state = clst_unknown;
  }
}

static int
send_ok(struct client_sess *s)
{
  static const char msg[] = "Ok!\n";
  int wc;
  wc = write(s->sd, msg, sizeof(msg)-1);
  if(-1 == wc) {
    perror("socket write");
  }
  return wc;
}

static int
send_counter(struct client_sess *s, int counter)
{
  int wc;
  char buf[cnt_buf_size];
  sprintf(buf, "%d\n", counter);
  wc = write(s->sd, buf, strlen(buf));
  return wc;
}

static int
send_unknown_command(struct client_sess *s)
{
  int wc;
  const char msg[] = "Unknown command: ";
  wc = write(s->sd, msg, sizeof(msg)-1);
  wc = write(s->sd, s->buf, strlen(s->buf));
  wc = write(s->sd, "\n", 1);
  return wc;
}

int
send_message(struct client_sess *s, int counter)
{
  int wc;
  switch(s->state) {
    case clst_wait:
    case clst_up:
    case clst_down:
    case clst_error:
    case clst_finish:
      fprintf(stderr, "bad case!\n");
      s->state = clst_error;
      return -1;
    case clst_print_ok:
      wc = send_ok(s);
      break;
    case clst_show:
      wc = send_counter(s, counter);
      break;
    case clst_unknown:
      wc = send_unknown_command(s);
      break;
  }
  s->state = clst_wait;
  return wc;
}
