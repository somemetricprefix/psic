#include <sys/socket.h>
#include <unistd.h>

#include <netdb.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <ev.h>

#include "util.h"

static char *host = "irc.dieselpowered.me";
//static char *host = "irc.rizon.net";
static char *port = "6669";

static char *nick = "lolol";

struct ev_io srv_read;
struct ev_io srv_write;
struct ev_io cli_read;
struct ev_io cli_write;

static int srv;
static char bufi[4096];
static char bufo[4096];

static void cli_read_cb(EV_P_ ev_io *w, int revents)
{
  read(STDIN_FILENO, bufo, sizeof(bufo));
  ev_io_start(EV_A_ &srv_write);
}

static void cli_write_cb(EV_P_ ev_io *w, int revents)
{
  write(STDOUT_FILENO, bufi, sizeof(bufi));
  ev_io_stop(EV_A_ w);
}

static void srv_read_cb(EV_P_ ev_io *w, int revents)
{
  read(srv, bufi, sizeof(bufi));
  ev_io_start(EV_A_ &cli_write);
}

static void srv_write_cb(EV_P_ ev_io *w, int revents)
{
  write(srv, bufo, sizeof(bufo));
  ev_io_stop(EV_A_ w);
}

int main(int argc, const char *argv[])
{
  struct ev_loop *loop;

  loop = ev_default_loop(EVFLAG_AUTO);
  if (!loop)
    die("cannot initialize libev, bad $LIBEV_FLAGS in environment?");

  srv = dial(host, port);
  snprintf(bufo, sizeof(bufo), "NICK %s\r\nUSER %s 0 * %s\r\n", nick, nick, nick);

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

