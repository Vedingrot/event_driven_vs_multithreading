#ifndef CLIENT_SESS_SENTRY_
#define CLIENT_SESS_SENTRY_

enum client_state {
  clst_start,
  clst_wait = clst_start,
  clst_up,
  clst_down,
  clst_print_ok,
  clst_show,
  clst_unknown,
  clst_error,
  clst_finish
};

struct client_sess {
  int sd;
  char *buf;
  enum client_state state;
};

struct client_sess *make_new_sess(int listening_sd);
void close_session(struct client_sess **sess);
int recieve_data(struct client_sess *sess);
void analyze_data(struct client_sess *sess);
int send_message(struct client_sess *sess, int counter);

#endif /* CLIENT_SESS_SENTRY_ */
