#include <sys/socket.h>
#include <sys/queue.h>
#include <unistd.h>

#include <netdb.h>

#include <stdio.h> /* snprintf */
#include <stdlib.h>
#include <string.h>

#include <ev.h>

#include "util.h"

struct irc_message {
  /* STAILQ would be enough but linux doesn't have it */
  TAILQ_ENTRY(irc_message) link;
  char *usr;
  char *cmd;
  char *args;
};
TAILQ_HEAD(irc_msg_queue, irc_message);

static char *host = "irc.rizon.net";
//static char *host = "irc.dieselpowered.me";
static char *port = "6669";
static char *nick = "lolol";

struct ev_io srv_read;
struct ev_io srv_write;
struct ev_io cli_read;
struct ev_io cli_write;

static int srv;
static char srv_buf[4096];
static char cli_buf[4096];

static struct irc_msg_queue srv_msg_queue = TAILQ_HEAD_INITIALIZER(srv_msg_queue);
static struct irc_msg_queue cli_msg_queue = TAILQ_HEAD_INITIALIZER(cli_msg_queue);

static char *extract(char **str, char ch, size_t len)
{
  char *p = *str;
  while (**str != ch && **str && *str < p + len)
    (*str)++;
  if (**str && *str < p + len)
    *(*str)++ = '\0';
  return p;
}

static char *msg_parse(char *str)
{
  char *usr, *cmd, *args;
  struct irc_message *msg;

  usr  = *str == ':' ? extract(&str, ' ', sizeof(srv_buf)) : host;
  cmd  = extract(&str, ' ', sizeof(srv_buf));
  args = extract(&str, '\r', sizeof(srv_buf));

  msg = (struct irc_message *)malloc(sizeof(struct irc_message));
  if (!msg)
    die("error: malloc():");

  msg->usr  = strdup(usr);
  msg->cmd  = strdup(cmd);
  msg->args = strdup(args);

  TAILQ_INSERT_TAIL(&srv_msg_queue, msg, link);

  return ++str; /* skip also '\n' */
}

static void srv_read_cb(EV_P_ ev_io *w, int revents)
{
  size_t len;
  char *ptr;

  len = read(srv, srv_buf, sizeof(srv_buf));

  ptr = srv_buf;
  while(ptr < srv_buf + len)
    ptr = msg_parse(ptr);

  ev_io_start(EV_A_ &cli_write);
}

static void srv_write_cb(EV_P_ ev_io *w, int revents)
{
  write(srv, cli_buf, sizeof(cli_buf));
  ev_io_stop(EV_A_ w);
}

static void cli_read_cb(EV_P_ ev_io *w, int revents)
{
  read(STDIN_FILENO, cli_buf, sizeof(cli_buf));
  ev_io_start(EV_A_ &srv_write);
}

static void cli_write_cb(EV_P_ ev_io *w, int revents)
{
  struct irc_message *msg;
  TAILQ_FOREACH(msg, &srv_msg_queue, link) {
    puts(msg->usr);
    puts(msg->cmd);
    puts(msg->args);
    TAILQ_REMOVE(&srv_msg_queue, msg, link);
  }
  ev_io_stop(EV_A_ w);
}

int main(int argc, const char *argv[])
{
  struct ev_loop *loop;

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

  ev_io_init(&cli_read, cli_read_cb, STDIN_FILENO, EV_READ);
  ev_io_start(loop, &cli_read);

  ev_io_init(&cli_write, cli_write_cb, STDOUT_FILENO, EV_WRITE);
  //ev_io_start(loop, &cli_write);

  ev_loop(loop, 0);
  return 0;
}

