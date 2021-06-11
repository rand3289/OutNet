# Need C++20 for erase_if(unordered_multimap&, predicate).  Otherwise C++17 would do.
# -Werror -ansi -pedantic -Wall -Wextra -Wno-unused-parameter
CC = g++  # notice CFLAGS contains -g which will compile everything in debug mode!
CFLAGS = -g --std=c++20 -Wall -Wextra -Wno-unused-parameter -Iupnp -Isign -pthread
DEPS = config.h crawler.h data.h http.h sign.h sock.h utils.h upnp/upnpnat.h upnp/xmlParser.h sign/tweetnacl.h
OBJ = main.o sock.o data.o crawler.o config.o http.o utils.o sign.o log.o upnp/upnpnat.o upnp/xmlParser.o sign/tweetnacl.o svclin.o svcwin.o

# Linux generates a warning when using -static
# Using 'gethostbyname' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking
ifdef OS # windows defines this environment variable
	LDFLAGS = -lwsock32 -static
endif

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

outnet: $(OBJ)
	$(CC) -o $@ $^ -lpthread $(LDFLAGS)

clean:
	/bin/rm -f $(OBJ)
