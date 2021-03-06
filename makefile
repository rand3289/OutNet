# -ansi -pedantic -Wall -Wextra -Werror -Wno-unused-parameter
all:
	g++ -g --std=c++17 -Wall -Wextra -Wno-unused-parameter main.cpp sock.cpp data.cpp crawler.cpp config.cpp http.cpp -lpthread
