# psic - pretty simple irc client

psic is a very simple and fast irc client which uses libev. It is loosely based
on [sic](http://tools.suckless.org/sic)

Currently it only prints all server output to standart output and sends
everything from standart input to the server.

## build instructions

	./waf configure
	./waf

### release/debug variants

default variant is called default

	./waf --debug
	./waf --release

