#include <sys/socket.h>
#include <unistd.h>

#include <netdb.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* exit */
#include <string.h> /* strlen */

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

int dial(const char *host, const char *port) {
  int srv;
  struct addrinfo hints;
  struct addrinfo *res, *r;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if(getaddrinfo(host, port, &hints, &res)) {
    die("error: cannot resolve hostname '%s':", host);
  }
  for(r = res; r; r = r->ai_next) {
    srv = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if(srv == -1) {
      continue;
    }
    if(!connect(srv, r->ai_addr, r->ai_addrlen)) {
      break; /* connected */
    }
    close(srv);
  }
  freeaddrinfo(res);
  if(!r) {
    die("error: cannot connect to host '%s'\n", host);
  }

  return srv;
}

