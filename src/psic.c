#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <stdio.h> /* snprintf */
#include <stdlib.h>
#include <string.h>

#include <ev.h>

#include "util.h"
#include "queue.h"

int a, b;

struct irc_msg {
  STAILQ_ENTRY(irc_msg) link;
  char *raw;
  char *prefix;
  char *command;
  char *params;
};
STAILQ_HEAD(irc_msg_queue, irc_msg);

static char *host = "irc.rizon.net";
static char *port = "6669";
static char *nick = "lolol";

/* according to rfc2812 no irc message should be longer then that */
static char srv_buf[512];
static size_t srv_buf_len;
static char cli_buf[512];

static struct ev_io cli_write;

static struct irc_msg_queue msg_queue = STAILQ_HEAD_INITIALIZER(msg_queue);

static void parse(char *str)
{
  struct irc_msg *msg;
  char *p, *tmp;

  p = tmp = str;

  msg = (struct irc_msg *)calloc(1, sizeof(struct irc_msg));
  if (!msg)
    die("error: calloc():");

  msg->raw = strdup(tmp);
  if (!msg->raw)
    die("error: strdup():");

  while (*p) {
    switch (*p) {
    case ':': /* we have a prefix but only one */
      if (!msg->prefix)
        msg->prefix = tmp;
      break;
    case ' ':
      tmp = p + 1;
      if (!msg->command)
        msg->command = tmp;
      else if (!msg->params)
        msg->params = tmp;
      else
        break;
      *p = '\0';
      break;
    }
    p++;
  }

  printf("a%d: %s\n", a++, msg->params);
  STAILQ_INSERT_TAIL(&msg_queue, msg, link);
}

static void srv_read_cb(EV_P_ ev_io *w, int revents)
{
  char ch;
  char buf[4096];
  size_t len, i;

  len = recv(w->fd, buf, sizeof(buf), 0);
  if (len == 0)
    die("error: remote host closed connection:");

  /* Convert every line to a cstring and parse the line if complete. If not
   * complete store the received data in a buffer and wait for more data. */
  for (i = 0; i < len && srv_buf_len < sizeof(srv_buf); i++) {
    ch = buf[i];
    if (ch == '\r')
      srv_buf[srv_buf_len++] = '\0';
    else if (ch == '\n') {
      parse(srv_buf);
      srv_buf_len = 0;
    } else
      srv_buf[srv_buf_len++] = ch;
  }

  ev_io_start(EV_A_ &cli_write);
}

static void srv_write_cb(EV_P_ ev_io *w, int revents)
{
  send(w->fd, cli_buf, sizeof(cli_buf), 0);
  ev_io_stop(EV_A_ w);
}

static void cli_write_cb(EV_P_ ev_io *w, int revents)
{
  struct irc_msg *msg, *tmp;

  STAILQ_FOREACH_SAFE(msg, &msg_queue, link, tmp) {
    printf("b%d: %s\n", b++, msg->params);
    STAILQ_REMOVE(&msg_queue, msg, irc_msg, link);
    free(msg->raw);
    free(msg);
  }

  ev_io_stop(EV_A_ w);
}

int main(int argc, const char *argv[])
{
  int srv;
  struct ev_loop *loop;
  struct ev_io srv_read;
  struct ev_io srv_write;

  loop = ev_default_loop(EVFLAG_AUTO);
  if (!loop)
    die("cannot initialize libev, bad $LIBEV_FLAGS in environment?");

  srv = dial(host, port);
  snprintf(cli_buf, sizeof(cli_buf),
           "NICK %s\r\nUSER %s 0 * %s\r\n", nick, nick, nick);

  ev_io_init(&srv_read, srv_read_cb, srv, EV_READ);
  ev_io_start(loop, &srv_read);

  ev_io_init(&srv_write, srv_write_cb, srv, EV_WRITE);
  ev_io_start(loop, &srv_write);

  ev_io_init(&cli_write, cli_write_cb, STDOUT_FILENO, EV_WRITE);
  //ev_io_start(loop, &cli_write);

  ev_loop(loop, 0);
  return 0;
}

