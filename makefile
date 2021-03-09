# -ansi -pedantic -Wall -Wextra -Werror -Wno-unused-parameter
# need C++20 for erase_if(unordered_multimap&, predicate).  Otherwise C++17 will do.
CC = g++
CFLAGS = -g --std=c++20 -Wall -Wextra -Wno-unused-parameter
DEPS = config.h crawler.h data.h http.h protocol.h sign.h sock.h upnp.h utils.h
OBJ = main.o sock.o data.o crawler.o config.o http.o utils.o sign.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

a.out: $(OBJ)
	$(CC) -o $@ $^ -lpthread

clean:
	/bin/rm -f a.out *.o