# Need C++20 for erase_if(unordered_multimap&, predicate).  Otherwise C++17 would do.
# -ansi -pedantic -Wall -Wextra -Werror -Wno-unused-parameter
CC = g++
CFLAGS = -g --std=c++20 -Wall -Wextra -Wno-unused-parameter
DEPS = config.h crawler.h data.h http.h protocol.h sign.h sock.h upnp.h utils.h
OBJ = main.o sock.o data.o crawler.o config.o http.o utils.o sign.o
TESTOBJ =  tests.o sock.o crawler.o data.o sign.o utils.o # for building tests

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

outnet: $(OBJ)
	$(CC) -o $@ $^ -lpthread
# -static
# warning: Using 'gethostbyname' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking

tests: $(TESTOBJ)
	$(CC) -o $@ $^

clean:
	/bin/rm -f *.o
