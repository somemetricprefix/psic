#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* exit */
#include <string.h> /* strlen */

#include <ev.h>

#include "irc.h"

void die(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  /* check for errno if string has ':' at the end */
  if(fmt[0] && fmt[strlen(fmt) - 1] == ':') {
    fprintf(stderr, " %s\n", strerror(errno));
  }

  exit(EXIT_FAILURE);
}

static void on_message(struct irc_message *msg)
{
  puts(msg->prefix);
  puts(msg->command);
  puts(msg->params);
  irc_message_free(msg);
  //irc_disconnect(msg->irc);
}

int main(int argc, const char *argv[])
{
  struct ev_loop *loop;
  struct irc_client irc;

  loop = ev_default_loop(EVFLAG_AUTO);
  if (!loop)
    die("cannot initialize libev, bad $LIBEV_FLAGS in environment?");

  irc_init(&irc, loop);
  irc.on_message = on_message;
  if (irc_connect(&irc, "irc.rizon.net", "6669") == -1)
    die("error: cannot to connect:");

  ev_loop(loop, 0);

  return 0;
}

