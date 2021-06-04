# Need C++20 for erase_if(unordered_multimap&, predicate).  Otherwise C++17 would do.
# -Werror -ansi -pedantic -Wall -Wextra -Wno-unused-parameter
CC = g++  # notice CFLAGS contains -g which will compile everything in debug mode!
CFLAGS = -g --std=c++20 -Wall -Wextra -Wno-unused-parameter -I.. -I../sign
DEPS = client.h ../sign.h ../sock.h ../sign/tweetnacl.h
OBJ =  onlist.o client.o ../sock.o ../sign.o ../log.o ../sign/tweetnacl.o

ifdef OS # windows defines this environment variable
	LDFLAGS = -lwsock32 -static
endif

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

onlist: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	/bin/rm -f $(OBJ) onlist onlist.exe
