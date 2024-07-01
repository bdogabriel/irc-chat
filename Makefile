all:
	g++ src/server.cpp -o exe/server -pthread -Wall -Werror
	g++ src/client.cpp -o exe/client -pthread -Wall -Werror

clear:
	rm exe/server exe/client