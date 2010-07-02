#ifndef IRC_H
#define IRC_H

#include "queue.h"

/* according to rfc2812 no irc message should be longer then 512 bytes */
#define IRC_BUFSIZE 512

struct irc_message {
  /* readonly */
  char *raw;
  char *prefix;
  char *command;
  char *params;
  struct irc_client *irc;
};

struct irc_client {
  /* readonly */
  int fd;

  /* private */
  struct ev_loop *loop;
  struct ev_io read_w;
  struct ev_io write_w;
  char buf[IRC_BUFSIZE];
  size_t buflen;

  /* public */
  char *nick;
  /* msg needs to be freed by the user with 'irc_msg_free()' */
  void (*on_message)(struct irc_message *msg);
};

void irc_init(struct irc_client *irc, struct ev_loop *loop);
int  irc_connect(struct irc_client *irc, const char *host, const char *port);
void irc_disconnect(struct irc_client *irc);

void irc_message_free(struct irc_message *msg);

#endif /* IRC_H */

