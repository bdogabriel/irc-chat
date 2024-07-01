all:
	g++ server.cpp -o server -pthread -Wall -Werror
	g++ client.cpp -o client -pthread -Wall -Werror
