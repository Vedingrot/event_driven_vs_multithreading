#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <ctype.h>

enum { bufsize = 4096 };

struct thread_data {
  int cld;
  int *counter;
  sem_t *cld_sem;
  pthread_mutex_t *cnt_mut;
};

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
trim_whitespaces(char *buf)
{
  int i, s;
  for(s = 0; isspace(buf[s]); s++) {}
  for(i = 0; buf[s+i] && !isspace(buf[s+i]); i++) {
    buf[i] = buf[s+i];
  }
  buf[i] = '\0';
}

static void
write_ok(int cld)
{
  const char msg[] = "Ok!";
  write(cld, msg, sizeof(msg)-1);
}

static void
up_case(int cld, struct thread_data *data)
{
  pthread_mutex_lock(data->cnt_mut);
  (*data->counter)++;
  pthread_mutex_unlock(data->cnt_mut);
  write_ok(cld);
}

static void
down_case(int cld, struct thread_data *data)
{
  pthread_mutex_lock(data->cnt_mut);
  (*data->counter)--;
  pthread_mutex_unlock(data->cnt_mut);
  write_ok(cld);
}

static void
show_case(int cld, struct thread_data *data)
{
  char buf[bufsize];
  int cnt;
  pthread_mutex_lock(data->cnt_mut);
  cnt = *data->counter;
  pthread_mutex_unlock(data->cnt_mut);
  sprintf(buf, "%d", cnt);
  write(cld, buf, strlen(buf));
}

static void
unknown_case(int cld, const char *buf)
{
  const char msg[] = "Unknown command: ";
  write(cld, msg, sizeof(msg) - 1);
  write(cld, buf, strlen(buf));
}

static void
analyze_request_and_response(int cld,
                             const char *buf,
                             struct thread_data *data)
{
  if(strcmp(buf, "up") == 0) {
    up_case(cld, data);
  } else if(strcmp(buf, "down") == 0) {
    down_case(cld, data);
  } else if(strcmp(buf, "show") == 0) {
    show_case(cld, data);
  } else {
    unknown_case(cld, buf);
  }
}

static void*
client_thread_main(void *v_data)
{
  struct thread_data *data = v_data;
  char buf[bufsize];
  int rc, cld = data->cld;
  sem_post(data->cld_sem);
  while((rc = read(cld, buf, bufsize)) != 0) {
    buf[rc] = '\0';
    trim_whitespaces(buf);
    analyze_request_and_response(cld, buf, data);
  }
  close(cld);
  return NULL;
}

int
server_main_loop(int ls)
{
  struct thread_data trd;
  pthread_t thr;
  int res, counter = 0;
  pthread_mutex_t cnt_mut = PTHREAD_MUTEX_INITIALIZER;
  sem_t cld_sem;
  trd.counter = &counter;
  trd.cnt_mut = &cnt_mut;
  trd.cld_sem = &cld_sem;
  sem_init(&cld_sem, 0, 0);
  for(;;) {
    trd.cld = accept(ls, NULL, NULL);
    if(-1 == trd.cld) {
      perror("accept");
      continue;
    }
    pthread_create(&thr, NULL, client_thread_main, &trd);
    pthread_detach(thr);
    sem_wait(&cld_sem);
  }
  sem_destroy(&cld_sem);
  res = pthread_mutex_destroy(&cnt_mut);
  if(res != 0) {
    perror("cnt_mut");
    return 4;
  }
  return 0;
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
