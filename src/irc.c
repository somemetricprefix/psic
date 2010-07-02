#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>

#include <stdio.h> /* snprintf */
#include <stdlib.h>
#include <string.h>

#include <ev.h>

#include "irc.h"
#include "queue.h"

static void parse(struct irc_client *irc)
{
  struct irc_message *msg;
  char *p, *tmp;

  /* we don't need to parse the message if the user doesn't handle it */
  if (!irc->on_message)
    return;

  msg = (struct irc_message *)calloc(1, sizeof(struct irc_message));
  if (!msg)
    return;

  msg->irc = irc;

  msg->raw = strdup(irc->buf);
  if (!msg->raw)
    return;

  p = tmp = msg->raw;

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

  irc->on_message(msg);
}

static void read_cb(EV_P_ ev_io *w, int revents)
{
  struct irc_client *irc = w->data;
  char ch;
  char buf[4096];
  size_t len, i;

  len = recv(w->fd, buf, sizeof(buf), 0);
  if (len == 0)
    return; /* TODO: handle this */

  /* Convert every line to a cstring and parse the line if complete. If not
   * complete store the received data in a buffer and wait for more data. */
  for (i = 0; i < len && irc->buflen < sizeof(irc->buf); i++) {
    ch = buf[i];
    if (ch == '\r')
      irc->buf[(irc->buflen)++] = '\0';
    else if (ch == '\n') {
      parse(irc);
      irc->buflen = 0;
    } else
      irc->buf[(irc->buflen)++] = ch;
  }
}

static void write_cb(EV_P_ ev_io *w, int revents)
{
  ev_io_stop(EV_A_ w);
}

void irc_init(struct irc_client *irc, struct ev_loop *loop)
{
  irc->fd = -1;
  irc->loop = loop;
  irc->read_w.data = irc;
  ev_init(&irc->read_w, read_cb);
  irc->write_w.data = irc;
  ev_init(&irc->write_w, write_cb);
  memset(irc->buf, 0, sizeof(irc->buf));
  irc->buflen = 0;

  irc->nick = NULL;
  irc->on_message = NULL;
}

int irc_connect(struct irc_client *irc, const char *host, const char *port)
{
  struct addrinfo hints, *res, *r;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if(getaddrinfo(host, port, &hints, &res))
    return -1;
  for(r = res; r; r = r->ai_next) {
    irc->fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if(irc->fd == -1)
      continue;
    if(connect(irc->fd, r->ai_addr, r->ai_addrlen) == 0)
      break; /* connected */
    close(irc->fd);
  }
  freeaddrinfo(res);
  if(!r)
    return -2;

  ev_io_set(&irc->read_w, irc->fd, EV_READ);
  ev_io_start(irc->loop, &irc->read_w);
  ev_io_set(&irc->write_w, irc->fd, EV_WRITE);
  ev_io_start(irc->loop, &irc->write_w);

  return irc->fd;
}

void irc_disconnect(struct irc_client *irc)
{
  ev_io_stop(irc->loop, &irc->read_w);
  ev_io_stop(irc->loop, &irc->write_w);
  close(irc->fd);
}

void irc_message_free(struct irc_message *msg)
{
  free(msg->raw);
  free(msg);
}

