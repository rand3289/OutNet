# Need C++20 for erase_if(unordered_multimap&, predicate).  Otherwise C++17 would do.
# -Werror -ansi -pedantic -Wall -Wextra -Wno-unused-parameter
CC = g++  # notice CFLAGS contains -g which will compile everything in debug mode!
CFLAGS = -g --std=c++20 -Wall -Wextra -Wno-unused-parameter -I.. -I../sign -pthread
DEPS = client.h ../sign.h ../sock.h ../sign/tweetnacl.h
OBJ =  tests.o client.o ../sock.o ../sign.o ../sign/tweetnacl.o
ifdef OS # windows defines this environment variable
	FORWINDOWS = -lwsock32 -static
endif

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

tests: $(OBJ)
	$(CC) -o $@ $^ -lpthread $(FORWINDOWS)

clean:
	/bin/rm -f *.o ../sign/*.o tests tests.exe
